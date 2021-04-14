const uint32_t uvi_width = 30;
const uint32_t uvi_height = 30;
const uint8_t uvi_data[(30*30)/2] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x65, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x38, 0xFB, 0xFF, 0xFF, 0xCF, 0x10, 0xFD, 0xFF, 0xFF, 0xAF, 0xA3, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x03, 0xB0, 0xFF, 0xFF, 0xFF, 0xCB, 0xFF, 0xFF, 0xFF, 0x0B, 0x40, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x1B, 0x10, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xC1, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xCF, 0x01, 0xF8, 0xFF, 0xCF, 0xBA, 0xFD, 0xFF, 0x7F, 0x20, 0xFD, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x8C, 0xFE, 0x9F, 0x03, 0x00, 0x30, 0xFA, 0xDF, 0xD8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x04, 0x00, 0x00, 0x00, 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0x00, 0xA4, 0xDD, 0x3A, 0x00, 0xF6, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x09, 0x70, 0xFF, 0xFF, 0xFF, 0x06, 0xB0, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0xF4, 0xFF, 0xFF, 0xFF, 0x4F, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xBE, 0xBB, 0xFC, 0xCF, 0x00, 0xFB, 0xFF, 0xFF, 0xFF, 0xAF, 0x10, 0xFE, 0xCF, 0xBB, 0xFB, 
  0x04, 0x00, 0xB0, 0xAF, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0xCF, 0x00, 0xFC, 0x0A, 0x00, 0x60, 
  0x06, 0x00, 0xC1, 0xAF, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xCF, 0x00, 0xFC, 0x0B, 0x00, 0x70, 
  0xDF, 0xCC, 0xFD, 0xDF, 0x00, 0xFB, 0xFF, 0xFF, 0xFF, 0x9F, 0x10, 0xFE, 0xDF, 0xDC, 0xFD, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xF4, 0xFF, 0xFF, 0xFF, 0x3F, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x09, 0x60, 0xFF, 0xFF, 0xFF, 0x05, 0xB0, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xA3, 0xCC, 0x39, 0x00, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x7B, 0xFE, 0xAF, 0x04, 0x00, 0x41, 0xFB, 0xDF, 0xC7, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xAF, 0x00, 0xF8, 0xFF, 0xEF, 0xCC, 0xFE, 0xFF, 0x7F, 0x10, 0xFC, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x0A, 0x10, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x01, 0xC1, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x02, 0xC1, 0xFF, 0xFF, 0xFF, 0xBA, 0xFF, 0xFF, 0xFF, 0x1B, 0x40, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x5A, 0xFD, 0xFF, 0xFF, 0xCF, 0x00, 0xFD, 0xFF, 0xFF, 0xCF, 0xA5, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x76, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  };