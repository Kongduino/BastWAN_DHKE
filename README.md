# BastWAN_DHKE

A project demonstrating how to do Diffie Hellman Key Exchange on the BastWAN/RAK4260.

It relies on 3 libraries, which are included in the code:

* `aes.c/h` an implementation of the AES algorithm, specifically ECB, CTR and CBC mode.
* `DHKE.h` my implementation of Diffie-Hellman Key Exchange, using (for now) AES ECB.
* `LoRandom.h` my implementation of LoRa-based RNG. It is library independent.
  To use the `LoRandom.h` library in your code, you need to provide 2 functions:
  ```c
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
  ```
You need to call `void setupLoRandom()` at startup, and `uint8_t getLoRandomByte()` when you need a random byte. This code creates a stock of 256 bytes at startup to make things easier. Remember you need to reset setting when you need RNG...
