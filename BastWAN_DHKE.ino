/*
  #include <string.h>
  #include <sstream>
  #include <iomanip>
*/
#include <stdio.h>
#include "DHKE.h"
#include <SPI.h>
#include <LoRa.h>
#include "LoRandom.h"


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

void writeRegister(uint8_t reg, uint8_t value) {
  LoRa.writeRegister(reg, value);
}

uint8_t readRegister(uint8_t reg) {
  return LoRa.readRegister(reg);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.flush();
  Serial.print(F("\n\n\n[SX1276] Initializing ... "));
  delay(1000);
  LoRa.setPins(SS, RFM_RST, RFM_DIO0);
  Serial.println("SS: " + String(SS));
  Serial.println("RFM_RST: " + String(RFM_RST));
  Serial.println("RFM_DIO0: " + String(RFM_DIO0));
  Serial.println("RFM_SWITCH: " + String(RFM_SWITCH));
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.print("Setting up LoRa ");
  pinMode(RFM_SWITCH, OUTPUT);
  digitalWrite(RFM_SWITCH, 1);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setPreambleLength(8);
  setupLoRandom();
  Serial.println("[o]");
  pinMode(PIN_PA28, OUTPUT);
  digitalWrite(PIN_PA28, HIGH);
  Serial.println("End of setup\n\n");
  uint8_t x = 0;
  x = LoRa.readRegister(0x01);
  Serial.print("RegOpMode: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);
  x = LoRa.readRegister(0x1D);
  Serial.print("RegModemConfig1: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);
  x = LoRa.readRegister(0x1E);
  Serial.print("RegModemConfig2: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);

  Serial.println("\nBuilding a stock of random numbers:");
  uint16_t i;
  for (i = 0; i < 256; i++) {
    uint8_t x = getLoRandomByte();
    randomStock[i] = x;
  }
  randomIndex = 0;
  hexDump(randomStock, 256);

  Serial.println("The value of P:"); hexDump64(P);
  Serial.println("The value of G:"); hexDump64(G);
  buddy Alice;
  Serial.println("\nAlicePublic:");
  hexDump((unsigned char*)Alice.PublicKey.oneChunk, 32);
  Serial.println("\nSetting up Bob");
  BigKey BobPublic;
  buddy Bob;
  for (uint8_t i = 0; i < 4; i++) hexDump64(BobPublic.fourNumbers[i]);
  Serial.println("\nThis is an encryption and decryption test...");
  String pkt = "This is an encryption and decryption test...";
  uint16_t len = pkt.length() + 1;
  // +1 = don't forget to account for the '\0' at the end...
  unsigned char pktBuf[256];
  pkt.toCharArray((char *)pktBuf, len);
  Serial.println("Size of message packet: " + String(len));
  Serial.println("\nEncryption with Alice's private key and Bob's public key.");
  char finalArray[256];
  size_t olen = Alice.encrypt((unsigned char*)pktBuf, len, Bob, finalArray);
  //encBuf contains the message, encrypted and Base64-encoded, length olen
  Serial.println("finalArray:");
  hexDump((unsigned char*)finalArray, olen);
  Serial.println("\nDecryption with Bob's private key and Alice's public key.");
  Bob.decrypt((unsigned char *)finalArray, olen, Alice, pktBuf);
}

void loop() {
}
