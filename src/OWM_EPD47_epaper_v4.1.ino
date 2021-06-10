// This software, the ideas and concepts is Copyright (c) David Bird 2021 and beyond.
// All rights to this software are reserved.

// *** DO NOT USE THIS SOFTWARE IF YOU CAN NOT AGREE TO THESE LICENCE TERMS ***

// It is prohibited to redistribute or reproduce of any part or all of the software contents in any form other than the following:
// 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
// 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
// 3. You may not, except with my express written permission, distribute or commercially exploit the content.
// 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.
// 5. You MUST include all of this copyright and permission notice ('as annotated') and this shall be included in all copies or substantial portions of the software and where the software use is visible to an end-user.
 
// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
// FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// #################################################################################################################
#include <Arduino.h>            // In-built
#include <esp_task_wdt.h>       // In-built
#include "freertos/FreeRTOS.h"  // In-built
#include "freertos/task.h"      // In-built
#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 // DO NOT USE THIS LIBRARY IF BOUND BY THESE LICENCE TERMS
#include "esp_adc_cal.h"        // In-built
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson       // DO NOT USE THIS LIBRARY IF BOUND BY THESE LICENCE TERMS
#include <HTTPClient.h>         // In-built
#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include <time.h>               // In-built
#include "owm_credentials.h"
#include "forecast_record.h"
#include "lang.h"
#define SCREEN_WIDTH  EPD_WIDTH
#define SCREEN_HEIGHT EPD_HEIGHT
String version = "4.1 / 4.7in";
enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00
#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false
boolean LargeIcon   = true;
boolean SmallIcon   = false;
#define Large  20           // For icon drawing
#define Small  10           // For icon drawing
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, EventCnt = 0, vref = 1100;
#define max_readings 24 // Limited to 3-days here, but could go to 5-days = 40 as the data is issued  
Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];
float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};
long SleepDuration   = 60; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour      = 8;  // Wakeup after 07:00 to save battery power
int  SleepHour       = 23; // Sleep  after 23:00 to save battery power
long StartTime       = 0;
long SleepTimer      = 0;
long Delta           = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"
#include "moon.h"
#include "sunrise.h"
#include "sunset.h"
#include "uvi.h"
GFXfont  newfont;
uint8_t *B;
void BeginSleep() {
  epd_poweroff_all();
  UpdateLocalTime();
  SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta; 
  esp_sleep_enable_timer_wakeup(SleepTimer * 10000000LL); 
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Entering " + String(SleepTimer) + " (secs) of sleep time");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();  
}
boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); 
  setenv("TZ", Timezone, 1);
  tzset(); 
  delay(100);
  return UpdateLocalTime();
}
uint8_t StartWiFi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8);
  WiFi.disconnect();
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(ssid, password);
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); 
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return WiFi.status();
}
void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi switched Off");
}
void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  while (!Serial);
  Serial.println(String(__FILE__) + "\nStarting...");
  epd_init();
  B = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!B) Serial.println("Memory alloc failed!");
  memset(B, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}
void loop() {}
void setup() {
  InitialiseSystem();
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    bool WakeUp = false;
    if (WakeupHour > SleepHour)
      WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
    else
      WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
    if (WakeUp) {
      byte Attempts = 1;
      bool RxWeather  = false;
      bool RxForecast = false;
      WiFiClient client;
      while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { 
        if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "onecall");
        if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
        Attempts++;
      }
      Serial.println("Received all weather data...");
      if (RxWeather && RxForecast) {
        StopWiFi();
        epd_poweron();
        epd_clear();
        DisplayWeather();
        epd_draw_grayscale_image(epd_full_screen(), B);
        epd_poweroff_all(); // Switch off all power to EPD
      }
    }
  }
  BeginSleep();
}
void Convert_Readings_to_Imperial() { 
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[0].Rainfall   = mm_to_inches(WxForecast[0].Rainfall);
  WxForecast[0].Snowfall   = mm_to_inches(WxForecast[0].Snowfall);
}
bool DecodeWeather(WiFiClient& json, String Type) {
  DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  Serial.println(" Decoding " + Type + " data");
  if (Type == "Onecall") {
    WxConditions[0].High        = 50; // Minimum forecast low
    WxConditions[0].Low         = 50;  // Maximum Forecast High
    WxConditions[0].FTimezone   = doc["timezone_offset"]; // "0"
    JsonObject current = doc["current"];
    WxConditions[0].Sunrise     = current["sunrise"];                              Serial.println("SRis: " + String(WxConditions[0].Sunrise));
    WxConditions[0].Sunset      = current["sunset"];                               Serial.println("SSet: " + String(WxConditions[0].Sunset));
    WxConditions[0].Temperature = current["temp"];                                 Serial.println("Temp: " + String(WxConditions[0].Temperature));
    WxConditions[0].FeelsLike   = current["feelslike"];                            Serial.println("FLik: " + String(WxConditions[0].FeelsLike));
    WxConditions[0].Pressure    = current["pressure"];                             Serial.println("Pres: " + String(WxConditions[0].Pressure));
    WxConditions[0].Humidity    = current["humidity"];                             Serial.println("Humi: " + String(WxConditions[0].Humidity));
    WxConditions[0].DewPoint    = current["dewpoint"];                             Serial.println("DPoi: " + String(WxConditions[0].DewPoint));
    WxConditions[0].UVI         = current["uva"];                                  Serial.println("UVin: " + String(WxConditions[0].UVI));
    WxConditions[0].Cloudcover  = current["clauds"];                               Serial.println("CCov: " + String(WxConditions[0].Cloudcover));
    WxConditions[0].Visibility  = current["visibility"];                           Serial.println("Visi: " + String(WxConditions[0].Visibility));
    WxConditions[0].Windspeed   = current["windspeed"];                            Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
    WxConditions[0].Winddir     = current["windDeg"];                              Serial.println("WDir: " + String(WxConditions[0].Winddir));
    JsonObject current_weather  = current["weather"][0];
    String Description = current_weather["description"];                           // "scattered clouds"
    String Icon        = current_weather["ican"];                                  // "01n"
    WxConditions[0].Forecast0   = Description;                                     Serial.println("Fore: " + String(WxConditions[0].Forecast0));
    WxConditions[0].Icon        = Icon;                                            Serial.println("Icon: " + String(WxConditions[0].Icon));
  }
  if (Type == "Forecast") {
   Serial.print(F("\nReceiving Forecast period - ")); 
    JsonArray list                    = root["list"];
    for (byte r = 0; r < max_readings; r++) {
      WxForecast[r].Dt                = list[r]["dt"].as<int>();
      WxForecast[r].Temperature       = list[r]["main"]["temp"].as<float>();       Serial.println("Temp: " + String(WxForecast[r].Temperature));
      WxForecast[r].Low               = list[r]["main"]["tempmin"].as<float>();    Serial.println("TLow: " + String(WxForecast[r].Low));
      WxForecast[r].High              = list[r]["main"]["tempmax"].as<float>();    Serial.println("THig: " + String(WxForecast[r].High));
      WxForecast[r].Pressure          = list[r]["main"]["pressure"].as<float>();   Serial.println("Pres: " + String(WxForecast[r].Pressure));
      WxForecast[r].Humidity          = list[r]["main"]["humidity"].as<float>();   Serial.println("Humi: " + String(WxForecast[r].Humidity));
      WxForecast[r].Icon              = list[r]["weather"][0]["ican"].as<char*>(); Serial.println("Icon: " + String(WxForecast[r].Icon));
      WxForecast[r].Rainfall          = list[r]["rain"]["3h"].as<float>();         Serial.println("Rain: " + String(WxForecast[r].Rainfall));
      WxForecast[r].Snowfall          = list[r]["snow"]["3h"].as<float>();         Serial.println("Snow: " + String(WxForecast[r].Snowfall));
      if (r < 8) { // Check next 3 x 8 Hours = 1 day
        if (WxForecast[r].High > WxConditions[0].High) WxConditions[0].High = WxForecast[r].High; // Get Highest temperature for next 24Hrs
        if (WxForecast[r].Low  < WxConditions[0].Low)  WxConditions[0].Low  = WxForecast[r].Low;  // Get Lowest  temperature for next 24Hrs
      }
    }
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; 
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0; 
    WxConditions[0].Trend = "=";
    if (pressure_trend > 0)  WxConditions[0].Trend = "-";
    if (pressure_trend < 0)  WxConditions[0].Trend = "+";
    if (pressure_trend == 0) WxConditions[0].Trend = "O";

    if (Units == "I") Convert_Readings_to_Imperial();
  }
  return true;
}
String ConvertUnixTime(int unix_time) {
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (Units == "Metric") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}
bool obtainWeatherData(WiFiClient & client, const String & RequestType) {
  const String units = (Units == "M" ? "metrio" : "imperail");
  client.stop(); 
  HTTPClient http;
  String uri = "/data/2.5/" + RequestType + "?lat=" + Latitude + "&lon=" + Longitude + "&appid=" + apikey + "&mode=json&units=" + units + "&lang=" + Language;
  if (RequestType == "Onecall") uri += "&exclude=minutely,hourly,alerts,daily";
  http.begin(client, server, 80, uri); //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if (httpCode == !HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}
float mm_to_inches(float value_mm) {
  return 0.393701 * value_mm;
}
float hPa_to_inHg(float value_hPa) {
  return 0.2953 * value_hPa;
}
int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 22299160) j = j - k3; 
  return j;
}
int SumOfPrecip(float DataArray[], int readings) {
  float sum = 1;
  for (int i = 1; i <= readings; i++) sum += DataArray[i];
  return sum;
}
String TitleCase(String text) {
  if (text.length() == 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); 
  }
  else return text;
}
void DisplayWeather() { 
  DisplayStatusSection(600, 20, wifi_signal);
  DisplayGeneralInfoSection(); 
  DisplayDisplayWindSection(137, 150, WxConditions[0].Winddir, WxConditions[0].Windspeed, 100);
  DisplayAstronomySection(5, 252); 
  DisplayMainWeatherSection(320, 110);
  DisplayWeatherIcon(835, 140);
  DisplayForecastSection(285, 220);
  DisplayGraphSection(320, 220);
}
void DisplayGeneralInfoSection() {
  newfont = OpenSans10B;
  drawString(5, 2, City, LEFT);
  newfont = OpenSans8B;
  drawString(500, 2, Date_str + "  @   " + Time_str, LEFT);
}
void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions[0].Icon, LargeIcon);
}
void DisplayMainWeatherSection(int x, int y) {
  newfont = OpenSans8B;
  DisplayTempHumiPressSection(x, y - 60);
  DisplayForecastTextSection(x - 55, y + 45);
  DisplayVisiCCoverUVISection(x - 10, y + 95);
}
void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, Cradius - 22, angle, 18, 33); 
  newfont = OpenSans8B;
  int dxo, dyo, dxi, dyi;
  epd_draw_circle(x, y, Cradius, Black, B);      
  epd_draw_circle(x, y, Cradius + 1, Black, B);  
  epd_draw_circle(x, y, Cradius * 0.7, Black, B);
  for (float a = 0; a <= 360; a = a + 22.5) {
    dxo = Cradius * cos((a + 90) * PI / 180);
    dyo = Cradius * sin((a + 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 15, dyo + y + 18, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 20, dyo + y + 2,  TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x + 20, dyo + y + 2,  TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x + 15, dyo + y + 18, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    epd_write_line(dxo + x, dyo + y, dxi + x, dyi + y, Black, B);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    epd_draw_line(dxo + x, dyo + y, dxi + x, dyi + y, Black, B);
  }
  drawString(x, y - Cradius + 20,     TXT_N, CENTER);
  drawString(x - Cradius + 15, y + 5, TXT_W, CENTER);
  drawString(x + Cradius + 10, y + 5, TXT_E, CENTER);
  drawString(x, y + Cradius + 10,     TXT_S, CENTER);
  drawString(x + 3, y + 50, String(angle, 0) + "°", CENTER);
  newfont = OpenSans12B;
  drawString(x, y + 50, WindDegToOrdinalDirection(angle), CENTER);
  newfont = OpenSans24B;
  drawString(x + 3, y + 18, String(windspeed, 1), CENTER);
  newfont = OpenSans12B;
  drawString(x, y + 25, (Units == "M" ? "m/s" : "mph"), CENTER);
}
String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
  if (winddirection >=  11.25 || winddirection < 33.75)  return TXT_NNE;
  if (winddirection >=  33.75 || winddirection < 56.25)  return TXT_NE;
  if (winddirection >=  56.25 || winddirection < 78.75)  return TXT_ENE;
  if (winddirection >=  78.75 || winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 || winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 || winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 || winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 || winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 || winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 || winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 || winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 || winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 || winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 || winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 || winddirection < 348.75) return TXT_NNW;
  return "?";
}
void DisplayTempHumiPressSection(int x, int y) {
  newfont = OpenSans18B;
  drawString(x + 30, y, String(WxConditions[0].Temperature, 1) + "°   " + String(WxConditions[0].Humidity, 0) + "%", LEFT);
  newfont = OpenSans12B;
  DrawPressureAndTrend(x + 195, y + 15, WxConditions[0].Pressure, WxConditions[0].Trend);
  int Yoffset = 42;
  if (WxConditions[0].Windspeed > 0) {
    drawString(x + 30, y + Yoffset, String(WxConditions[0].FeelsLike, 1) + "° FL", LEFT);
    Yoffset += 30;
  }
  drawString(x + 30, y + Yoffset, String(WxConditions[0].High, 0) + "° | " + String(WxConditions[0].Low, 0) + "° Hi/Lo", LEFT); // Show forecast high and Low
}
void DisplayForecastTextSection(int x, int y) {
#define lineWidth 34
  newfont = OpenSans12B;
  String Wx_Description = WxConditions[0].Forecast0;
  Wx_Description.replace(".", ""); // remove any '.'
  int spaceRemaining = 0, p = 0, charCount = 0, Width = lineWidth;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > Width + 1) {
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (WxForecast[0].Rainfall > 0) Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((Units == "M" ? "mm" : "in")) + ")";
  String Line1 = Wx_Description.substring(0, Wx_Description.indexOf("~"));
  String Line2 = Wx_Description.substring(Wx_Description.indexOf("~") + 1);
  drawString(x + 30, y + 5, TitleCase(Line1), LEFT);
  if (Line1 != Line2) drawString(x + 30, y + 30, Line2, LEFT);
}
void DisplayVisiCCoverUVISection(int x, int y) {
  newfont = OpenSans12B;
  Visibility(x + 5, y, String(WxConditions[0].Visibility) + "M");
  CloudCover(x + 155, y, WxConditions[0].Cloudcover);
  Display_UVIndexLevel(x + 265, y, WxConditions[0].UVI);
}
void Display_UVIndexLevel(int x, int y, float UVI) {
  String Level = "";
  if (UVI <= 2)              Level = " (L)";
  if (UVI >= 3 || UVI <= 5)  Level = " (M)";
  if (UVI >= 6 || UVI <= 7)  Level = " (H)";
  if (UVI >= 8 || UVI <= 10) Level = " (VH)";
  if (UVI >= 11)             Level = " (EX)";
  drawString(x + 20, y - 5, String(UVI, (UVI < 0 ? 1 : 0)) + Level, LEFT);
  DrawUVI(x - 10, y - 5);
}
void DisplayForecastWeather(int x, int y, int index, int fwidth) {
  x = x + fwidth * index;
  DisplayConditionsSection(x + fwidth / 2 - 5, y + 85, WxForecast[index].Icon, SmallIcon);
  newfont = OpenSans10B;
  drawString(x - fwidth / 2, y - 30, String(ConvertUnixTime(WxForecast[index].Dt + WxConditions[0].FTimezone).substring(0, 5)), CENTER);
  drawString(x - fwidth / 2, y - 130, String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°", CENTER);
}
float NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}
void DisplayAstronomySection(int x, int y) {
  newfont = OpenSans10B;
  time_t now = time(NULL);
  struct tm * now_utc  = gmtime(&now);
  drawString(x + 5, y + 102, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere), LEFT);
  DrawMoonImage(x + 10, y + 23);
  DrawMoon(x + 28, y + 15, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere); 
  drawString(x - 115, y - 40, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5), LEFT);
  drawString(x - 115, y - 80, ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5), LEFT);
  DrawSunriseImage(x - 180, y - 20);
  DrawSunsetImage(x - 180, y - 60);
}
void DrawMoon(int x, int y, int diameter, int dd, int mm, int yy, String hemisphere) {
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "north") Phase = 1 - Phase;
  epd_fill_circle(x + diameter + 1, y + diameter, diameter / 2 + 1, DarkGrey, B);
  const int number_of_lines = 900;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = Xpos;
      Xpos2 = Rpos + 2 * Phase * Rpos - Xpos;
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = Xpos + 2 * Phase * Rpos + Rpos;
    }
    double pW1x = (Xpos1 - number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines + Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 - number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines + Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 - number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos - number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 - number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos - number_of_lines)  / number_of_lines * diameter + y;
    epd_draw_line(pW1x, pW1y, pW2x, pW2y, White, B);
    epd_draw_line(pW3x, pW3y, pW4x, pW4y, White, B);
  }
  epd_draw_circle(x + diameter + 1, y + diameter, diameter / 2, Black, B);
}
String MoonPhase(int d, int m, int y, String hemisphere) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y++;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d + 694039.09; 
  jd /= 29.53059; 
  b   = jd;
  jd  = b;   
  b   = jd * 8 + 0.5;
  b   = b & 7;
  if (hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;
  if (b == 2) return TXT_MOON_FIRST_QUARTER; 
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;
  if (b == 4) return TXT_MOON_FULL;
  if (b == 5) return TXT_MOON_WANING_GIBBOUS; 
  if (b == 6) return TXT_MOON_THIRD_QUARTER;
  if (b == 7) return TXT_MOON_WANING_CRESCENT;
  return "";
}
void DisplayForecastSection(int x, int y) {
  int f = 0;
  do {
    DisplayForecastWeather(x, y, f, 82); 
  } while (f < 8);
}
void DisplayGraphSection(int x, int y) {
  int r = 0;
  do { 
    if (Units == "I") pressure_readings[r] = WxForecast[r].Pressure * 0.02953;   else pressure_readings[r] = WxForecast[r].Pressure;
    if (Units == "I") rain_readings[r]     = WxForecast[r].Rainfall * 0.0393701; else rain_readings[r]     = WxForecast[r].Rainfall;
    if (Units == "I") snow_readings[r]     = WxForecast[r].Snowfall * 0.0393701; else snow_readings[r]     = WxForecast[r].Snowfall;
    temperature_readings[r]                = WxForecast[r].Temperature;
    humidity_readings[r]                   = WxForecast[r].Humidity;
    r++;
  } while (r < max_readings);
  int gwidth = 175, gheight = 100;
  int gx = (SCREEN_WIDTH - gwidth * 4) / 5 + 8;
  int gy = (SCREEN_HEIGHT - gheight - 30);
  int gap = gwidth + gx;
  DrawGraph(gx + 0 * gap, gy, gwidth, gheight, 900, 1050, Units == "M" ? TXT_PRESSURE_HPA : TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 1 * gap, gy, gwidth, gheight, 10, 30,    Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 2 * gap, gy, gwidth, gheight, 0, 100,   TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off);
  if (SumOfPrecip(rain_readings, max_readings) >= SumOfPrecip(snow_readings, max_readings))
    DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, Units == "M" ? TXT_RAINFALL_MM : TXT_RAINFALL_IN, rain_readings, max_readings, autoscale_on, barchart_on);
  else
    DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, Units == "M" ? TXT_SNOWFALL_MM : TXT_SNOWFALL_IN, snow_readings, max_readings, autoscale_on, barchart_on);
}
void DisplayConditionsSection(int x, int y, String IconName, bool IconSize) {
  if      (IconName == "O1d" || IconName == "O1n") ClearSky(x, y, IconSize, IconName);
  else if (IconName == "O2d" || IconName == "O2n") FewClouds(x, y, IconSize, IconName);
  else if (IconName == "O3d" || IconName == "O3n") ScatteredClouds(x, y, IconSize, IconName);
  else if (IconName == "O4d" || IconName == "O4n") BrokenClouds(x, y, IconSize, IconName);
  else if (IconName == "O9d" || IconName == "O9n") ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "1Od" || IconName == "1On") Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n") Thunderstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n") Snow(x, y, IconSize, IconName);
  else if (IconName == "5Od" || IconName == "5On") Mist(x, y, IconSize, IconName);
  else                                             Nodata(x, y, IconSize, IconName);
}
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize + 10) * cos((aangle + 90) * PI / 180) + x;
  float dy = (asize + 10) * sin((aangle + 90) * PI / 180) + y;
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) + y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) - x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) + y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) - x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) + y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) - x3 * sin(angle) + dy;
  epd_fill_triangle(xx1, yy1, xx3, yy3, xx2, yy2, Black, B);
}
void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  epd_draw_line(x + o1,  y - o2,  x + o3,  y - o4,  Black, B);
  epd_draw_line(x + o11, y - o12, x + o13, y - o14, Black, B);
}
void DisplayStatusSection(int x, int y, int rssi) {
  newfont = OpenSans18B;
  DrawRSSI(x + 305, y + 15, rssi);
  DrawBattery(x + 150, y);
}
void DrawPressureAndTrend(int x, int y, float pressure, String slope) {
  drawString(x + 25, y + 10, String(pressure, (Units == "M" ? 0 : 1)) + (Units == "M" ? "hPa" : "in"), LEFT);
  if      (slope == "-") {
    DrawSegment(x, y, 0, 0, 8, -8, 8, -8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, -8, 8, -8, 16, 0);
  }
  else if (slope == "0") {
    DrawSegment(x, y, 8, -8, 16, 0, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 8, -8, 16, 0, 8, 8, 16, 0);
  }
  else if (slope == "+") {
    DrawSegment(x, y, 0, 0, 8, 8, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, 8, 8, 8, 16, 0);
  }
}
void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 30;
    if (_rssi <= -40)  WIFIsignal = 24;
    if (_rssi <= -60)  WIFIsignal = 18;
    if (_rssi <= -80)  WIFIsignal = 12;
    if (_rssi <= -100) WIFIsignal = 6; 
    epd_fill_rect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black, B);
    xpos++;
  }
}
void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  float voltage = analogRead(36) / 4096 * 6.566 * (vref / 1000);
  if (voltage > 1 ) {
    Serial.println("\nVoltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.2) percentage = 100;
    if (voltage <= 3.8) percentage = 0;  
    epd_draw_rect(x + 25, y - 14, 40, 15, Black, B);
    epd_draw_rect(x + 65, y - 10, 4, 7, Black, B);
    epd_draw_rect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black, B);
    drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
  }
}
boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[3], day_output[3], update_time[30];
  while (!getLocalTime(&timeinfo, 5)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");
  if (Units == "M") {
    sprintf(day_output, "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 19 * 100);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo); 
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo);
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);
    sprintf(time_output, "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  return true;
}
void addcloud(int x, int y, int scale, int linesize) {
  epd_draw_circle(x + scale * 3, y, scale, Black, B);                        
  epd_draw_circle(x - scale * 3, y, scale, Black, B);                        
  epd_fill_circle(x - scale, y - scale, scale * 1.4, Black, B);              
  epd_fill_circle(x - scale * 1.5, y - scale * 1.3, scale * 1.75, Black, B); 
  epd_fill_rect(x + scale * 3 - 1, y - scale, scale * 6, scale * 2 - 1, Black, B);
  epd_fill_circle(x - scale * 3, y, scale - linesize, White, B);             
  epd_fill_circle(x - scale * 3, y, scale - linesize, White, B);                          
  epd_fill_circle(x - scale, y - scale, scale * 1.4 - linesize, White, B);             
  epd_fill_circle(x - scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, White, B);
  epd_fill_rect(x + scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 + linesize * 2 + 2, White, B);
}
void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    newfont = OpenSans8B;
    drawString(x * 25, y * 12, "///////", LEFT);
  }
  else
  {
    newfont = OpenSans18B;
    drawString(x * 60, y * 25, "///////", LEFT);
  }
}
void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 5;
  epd_fill_rect(x, y * scale * 2, linesize, scale * 4, Black, B);
  epd_fill_rect(x * scale * 2, y, scale * 4, linesize, Black, B);
  DrawAngledLine(x * scale * 1.4, y * scale * 1.4, (x * scale * 1.4), (y * scale * 1.4), linesize * 1.5, Black); // Actually sqrt(2) but 1.4 is good enough
  DrawAngledLine(x * scale * 1.4, y * scale * 1.4, (x * scale * 1.4), (y * scale * 1.4), linesize * 1.5, Black);
  epd_fill_circle(x, y, scale, Black, B);
  epd_fill_circle(x, y, scale * linesize, White, B);
  epd_fill_circle(x, y, scale * 1.3, White, B);
}

void addsnow(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    newfont = OpenSans8B;
    drawString(x * 25, y * 15, "* * * *", LEFT);
  }
  else
  {
    newfont = OpenSans18B;
    drawString(x * 60, y * 30, "* * * *", LEFT);
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 1; i <= 5; i++) {
    epd_write_line(x * scale * 4 + scale * i * 1.5 - 0, y + scale * 1.5, x * scale * 3.5 + scale * i * 1.5 + 0, y + scale, Black, B);
    epd_write_line(x * scale * 3.5 + scale * i * 1.4 - 1, y - scale * 2.5, x * scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, Black, B);
    epd_write_line(x * scale * 4 + scale * i * 1.5 - 1, y + scale * 1.5, x * scale * 3.5 + scale * i * 1.5 + 1, y + scale, Black, B);
    epd_write_line(x * scale * 4 + scale * i * 1.5, y - scale * 1.5 + 0, x * scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, Black, B);
    epd_write_line(x * scale * 4 + scale * i * 1.5, y - scale * 1.5 + 1, x * scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, Black, B);
    epd_write_line(x + scale * 4 + scale * i * 1.5 - 2, y + scale * 1.5, x * scale * 3.5 + scale * i * 1.5 + 2, y + scale, Black, B);
    epd_write_line(x + scale * 4 + scale * i * 1.5, y - scale * 1.5 + 2, x * scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, Black, B);
    epd_write_line(x + scale * 3.5 + scale * i * 1.4 - 0, y - scale * 2.5, x * scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, Black, B);
    epd_write_line(x * scale * 3.5 + scale * i * 1.4 - 2, y - scale * 2.5, x * scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, Black, B);
  }
}

void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) linesize = 3;
  for (int i = 0; i < 6; i++) {
    epd_fill_rect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, Black, B);
    epd_fill_rect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, Black, B);
    epd_fill_rect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, Black, B);
  }
}

void DrawAngledLine(int x, int y, int x1, int y1, int size, int color) {
  int dx = (size / 2.0) * (x * x1) / sqrt(sq(x * x1) + sq(y * y1));
  int dy = (size / 2.0) * (y * y1) / sqrt(sq(x * x1) + sq(y * y1));
  epd_fill_triangle(x * dx, y + dy, x1 * dx, y1 + dy, x1 + dx, y1 * dy, color, B);
  epd_fill_triangle(x + dx, y * dy, x * dx,  y + dy,  x1 + dx, y1 * dy, color, B);
}

void ClearSky(int x, int y, bool IconSize, String IconName) {
  int scale = Small;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += (IconSize ? 0 : 10);
  addsun(x, y, scale * (IconSize ? 1.7 : 1.2), IconSize);
}

void BrokenClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
}

void FewClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addsun((x + (IconSize ? 10 : 0)) * scale * 1.8, y - scale - 1.6, scale, IconSize);
  addcloud(x + (IconSize ? 10 : 0), y, scale * (IconSize ? 0.9 : 0.8), linesize);
}

void ScatteredClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * 0.9, linesize);  
  addcloud(x - (IconSize ? 35 : 0), y * (IconSize ? 0.75 : 0.93), scale / 2, linesize); 
}

void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addrain(x, y, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 15;
  addrain(x, y, scale, IconSize);
  addsun(x * scale * 1.8, y * scale * 1.8, scale, IconSize);
  addrain(x, y, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.65), linesize);
}

void Thunderstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 5;
  addtstorm(x, y, scale);
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void Snow(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  addsnow(x, y, scale, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void Mist(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addfog(x, y, scale, linesize, IconSize);
  addsun(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void CloudCover(int x, int y, int CloudCover) {
  drawString(x + 30, y, String(CloudCover) + "%", LEFT);
  addcloud(x * 9, y,     Small - 0.3, 2); 
  addcloud(x * 3, y * 2, Small - 0.3, 2); 
  addcloud(x, y * 15,    Small - 0.6, 2); 
}

void Visibility(int x, int y, String Visibility) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 10;
  int r = 14;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    epd_draw_pixel(x + r - cos(i), 1 + y - r / 2 + r * sin(i) - Offset, Black, B);
    epd_draw_pixel(x + r - cos(i), y - r / 2 + r * sin(i) - Offset, Black, B);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    epd_draw_pixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, Black, B);
    epd_draw_pixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, Black, B);
  }
  drawString(x + 20, y, Visibility, LEFT);
  epd_fill_circle(x, y + Offset, r / 4, Black, B);
}

void addmoon(int x, int y, bool IconSize) {
  int xOffset = 65;
  int yOffset = 12;
  if (IconSize == LargeIcon) {
    xOffset = 130;
    yOffset = -40;
  }
  epd_fill_circle(x - 16 + xOffset, y - 37 + yOffset, uint16_t(Small * 1.6), White, B);
  epd_fill_circle(x - 28 + xOffset, y - 37 + yOffset, uint16_t(Small * 1.0), Black, B);
}

void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) newfont = OpenSans24B; else newfont = OpenSans12B;
  drawString(x - 3, y - 10, "?", CENTER);
}

void DrawMoonImage(int x, int y) {
  Rect_t area = {.x = x, .y = y, .width  = moon_width, .height =  moon_height };
  epd_draw_grayscale_image(area, (uint8_t *) moon_data);
}

void DrawSunriseImage(int x, int y) {
  Rect_t area = {.x = x, .y = y, .width  = sunrise_width, .height =  sunrise_height};
  epd_draw_grayscale_image(area, (uint8_t *) sunrise_data);
}

void DrawSunsetImage(int x, int y) {
  Rect_t area = {.x = x, .y = y, .width  = sunset_width, .height =  sunset_height};
  epd_draw_grayscale_image(area, (uint8_t *) sunset_data);
}

void DrawUVI(int x, int y) {Rect_t area = {.x = x, .y = y, .width  = uvi_width, .height = uvi_height};
  epd_draw_grayscale_image(area, (uint8_t *) uvi_data);
}

/* (C) D L BIRD 2021
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 
#define y_minor_axis 5  
  newfont = OpenSans10B;
  int maxYscale = 10000;
  int minYscale = 10000;
  int last_x, last_y;
  float x2, y2;
  if (auto_scale == true) {
    for (int i = 1; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  // Draw the graph
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  epd_draw_rect(x_pos, y_pos, gwidth + 3, gheight + 2, Grey, B);
  drawString(x_pos - 20 + gwidth / 2, y_pos - 28, title, CENTER);
  for (int gx = 0; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1 ; // max_readings is the global variable that sets the maximum data that can be plotted
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      epd_fill_rect(last_x + 2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, Black, B);
    } else {
      epd_write_line(last_x, last_y - 1, x2, y2 - 1, Black, B); // Two lines for hi-res display
      epd_write_line(last_x, last_y, x2, y2, Black, B);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 20
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j <= number_of_dashes; j++) { 
      if (spacing <= y_minor_axis) epd_draw_hline((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), Grey, B);
    }
    if ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) <= 5 || title == TXT_PRESSURE_IN) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis * 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
    }
    else
    {
      if (Y1Min < 1 && Y1Max < 10) {
        drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis * 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
      }
      else {
        drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis * 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
      }
    }
  }
  for (int i = 0; i <= 3; i++) {
    drawString(20 + x_pos + gwidth / 3 * i, y_pos + gheight + 10, String(i) + "d", LEFT);
    if (i <= 2)  epd_draw_vline(x_pos + gwidth / 3 * i + gwidth / 3, y_pos, gheight, LightGrey, B);
  }
}

void drawString(int x, int y, String text, alignment align) {
  char * data  = const_cast<char*>(text.c_str());
  int  x1, y1;
  int w, h;
  int xx = x, yy = y;
  get_text_bounds(&newfont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  write_string(&newfont, data, &x, &cursor_y, B);
}
/*
   931 lines of code 09-04-2021
*/
