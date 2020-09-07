/*
  SX1276 Register (address)     Register bit field (bit #)      Values    Note
  RegOpMode (0x01)              LongRangeMode[7]                ‘1’       LoRa mode enable
                                Mode[2:0]                       ‘101’     Receive Continuous mode
  ------------------------------------------------------------------------------------------------------------------
  RegModemConfig1 (0x1D)        Bw[7:4]                         ‘0111’    ‘0111’ for 125kHz modulation Bandwidth
                                CodingRate[3:1]                 ‘001’     4/5 error coding rate
                                ImplicitHeaderModeOn[0]         ‘0’       Packets have up-front header
  ------------------------------------------------------------------------------------------------------------------
  RegModemConfig2 (0x1E)        SpreadingFactor[7:4]            ‘0111’    ‘0111’ (SF7) = 6kbit/s

  To generate an N bit random number, perform N read operation of the register RegRssiWideband (address 0x2c)
  and use the LSB of the fetched value. The value from RegRssiWideband is derived from a wideband (4MHz) signal strength
  at the receiver input and the LSB of this value constantly and randomly changes.
*/

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

void fillRandom(unsigned char *x, size_t len) {
  size_t i;
  for (i = 0; i < len; i++) {
    x[i] = getLoRandomByte();
  }
}
