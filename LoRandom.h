#define RegOpMode 0x01
#define RegModemConfig1 0x1D
#define RegModemConfig2 0x1E
#define RegRssiWideband 0x2C

void writeRegister(uint8_t reg, uint8_t value);
uint8_t readRegister(uint8_t reg);
// Provide your own functions, which will depend on your library

void setupLoRandom() {
  writeRegister(RegOpMode, 0b10001101);
  writeRegister(RegModemConfig1, 0b01110010);
  writeRegister(RegModemConfig2, 0b01110000);
}

uint8_t getLoRandomByte() {
  uint8_t x = 0;
  for (uint8_t j = 0; j < 8; j++) {
    x += (readRegister(RegRssiWideband) & 0b00000001);
    x = x << 1;
    delay(1);
  }
  return x;
}
