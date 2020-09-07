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
  fillRandom(randomStock, 256);
  hexDump(randomStock, 256);
  Serial.println("\nMy UUID:");
  array2hex(randomStock, 16, myUUID, 4);
  hexDump((unsigned char*)myUUID, 36);
  randomIndex = 16;

  Serial.println("The value of P:"); hexDump64(P);
  Serial.println("The value of G:"); hexDump64(G);
  buddy Alice;
  Serial.println("\nAlicePublic:");
  hexDump((unsigned char*)Alice.PublicKey.oneChunk, 32);
  Serial.println("\nSetting up Bob");
  BigKey BobPublic;
  buddy Bob;

  myMode = ECB;
  for (uint8_t i = 0; i < 4; i++) hexDump64(BobPublic.fourNumbers[i]);
  String pkt = "This is ån ECB éncryption and dècryption test...";
  Serial.println("\n" + pkt);
  uint16_t len = pkt.length() + 1;
  // +1 = don't forget to account for the '\0' at the end...
  unsigned char pktBuf[256];
  char finalArray[256];
  pkt.toCharArray((char *)pktBuf, len);
  Serial.println("Size of message packet: " + String(len));
  Serial.println("\nEncryption with Alice's private key and Bob's public key.");
  size_t olen = Alice.encrypt((unsigned char*)pktBuf, len, Bob, finalArray);
  //encBuf contains the message, encrypted and Base64-encoded, length olen
  Serial.println("finalArray:");
  hexDump((unsigned char*)finalArray, olen);
  Serial.println("\nDecryption with Bob's private key and Alice's public key.");
  Bob.decrypt((unsigned char *)finalArray, olen, Alice, pktBuf);
  Serial.println((char*)pktBuf);
  
  myMode = CBC;
  pkt = "This is à CBC êncryption and dëcryption test...";
  Serial.println("\n" + pkt);
  len = pkt.length() + 1;
  // +1 = don't forget to account for the '\0' at the end...
  pkt.toCharArray((char *)pktBuf, len);
  Serial.println("Size of message packet: " + String(len));
  Serial.println("\nEncryption with Alice's private key and Bob's public key.");
  olen = Alice.encrypt((unsigned char*)pktBuf, len, Bob, finalArray);
  //encBuf contains the message, encrypted and Base64-encoded, length olen
  Serial.println("finalArray:");
  hexDump((unsigned char*)finalArray, olen);
  Serial.println("\nDecryption with Bob's private key and Alice's public key.");
  Bob.decrypt((unsigned char *)finalArray, olen, Alice, pktBuf);
  Serial.println((char*)pktBuf);
  randomIndex += 16;
  // Now that we have encrypted AND decrypted, we can increment the randomIndex pointer.
  // This is not sustainable though. It'd be better to save the iv before encryption.
  // Just sayin'...
  
  myMode = CTR;
  pkt = "This is à CTR ęncryption and dēcryption tėst...";
  Serial.println("\n" + pkt);
  len = pkt.length() + 1;
  // +1 = don't forget to account for the '\0' at the end...
  pkt.toCharArray((char *)pktBuf, len);
  Serial.println("Size of message packet: " + String(len));
  Serial.println("\nEncryption with Alice's private key and Bob's public key.");
  olen = Alice.encrypt((unsigned char*)pktBuf, len, Bob, finalArray);
  //encBuf contains the message, encrypted and Base64-encoded, length olen
  Serial.println("finalArray:");
  hexDump((unsigned char*)finalArray, olen);
  Serial.println("\nDecryption with Bob's private key and Alice's public key.");
  Bob.decrypt((unsigned char *)finalArray, olen, Alice, pktBuf);
  Serial.println((char*)pktBuf);
}

void loop() {
}
