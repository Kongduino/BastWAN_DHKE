#define CBC 0
#define CTR 1
#define ECB 2

uint8_t myMode = CBC;

#include "aes.c"

unsigned char randomStock[256];
// We'll build a stock of random bytes for use in code
uint8_t randomIndex = 0;
char myUUID[36];

void hexDump(unsigned char *buf, uint16_t len) {
  String s = "|", t = "| |";
  Serial.println(F("  |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f |"));
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
  for (uint16_t i = 0; i < len; i += 16) {
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j >= len) {
        s = s + "   "; t = t + " ";
      } else {
        char c = buf[i + j];
        if (c < 16) s = s + "0";
        s = s + String(c, HEX) + " ";
        if (c < 32 || c > 127) t = t + ".";
        else t = t + (char)c;
      }
    }
    uint8_t index = i / 16;
    Serial.print(index, HEX); Serial.write('.');
    Serial.println(s + t + "|");
    s = "|"; t = "| |";
  }
  Serial.println(F("  +------------------------------------------------+ +----------------+\n"));
}

void hexDump64(uint64_t n) {
  uint8_t buff[8], realBuff[8], t;
  *(int64_t *)buff = n;
  for (t = 0; t < 8; t++) realBuff[t] = buff[7 - t];
  hexDump((unsigned char*)realBuff, 8);
}

uint64_t P = 0xffffffffffffffc5;
uint64_t G = 5;

union BigKey {
  uint8_t oneChunk[32]; // occupies 32 bytes
  uint64_t fourNumbers[4]; // occupies 32 bytes
}; // the whole union  occupies 32 bytes

char firstLayerKey[32];
char encBuf[512];

// Power function to return value of a ^ b mod P
uint64_t power(uint64_t a, uint64_t b) {
  if (b == 1) return a;
  uint64_t t = power(a, b / 2);
  if (b % 2 == 0) return (t * t) % P;
  else return (((t * t) % P) * a) % P;
}

uint64_t randomint64() {
  uint64_t a = 0;
  uint8_t j;
  for (j = 0; j < 8; j++) {
    uint8_t x = randomStock[randomIndex++];
    a = (a << 8) | x;
  }
  randomIndex += 8;
  return a;
}

void *fourTo32(uint64_t*what, uint8_t myKey[32]) {
  *(uint64_t*)myKey = what[3];
  memcpy(myKey + 24, myKey, 8);
  *(uint64_t*)myKey = what[2];
  memcpy(myKey + 16, myKey, 8);
  *(uint64_t*)myKey = what[1];
  memcpy(myKey + 8, myKey, 8);
  *(uint64_t*)myKey = what[0];
}

char oneHalfByte(uint8_t c) {
  if (c < 10) return ((char)c + 48);
  else return ((char)c + 87);
}

void array2hex(uint8_t *buf, size_t sLen, char *x, uint8_t dashFreq = 0) {
  size_t i, len, n = 0;
  const char * hex = "0123456789ABCDEF";
  for (i = 0; i < sLen; ++i) {
    x[n++] = hex[(buf[i] >> 4) & 0xF];
    x[n++] = hex[buf[i] & 0xF];
    if (dashFreq > 0 && i != sLen - 1) {
      if ((i + 1) % dashFreq == 0) x[n++] = '-';
    }
  }
  x[n++] = 0;
}

void array2hex(BigKey *bk, size_t sLen, char *x) {
  array2hex((uint8_t *)bk->oneChunk, sLen, x);
}

void hex2array(uint8_t *src, uint8_t *dst, size_t sLen) {
  size_t i, n = 0;
  for (i = 0; i < sLen; i += 2) {
    uint8_t x, c;
    c = src[i];
    if (c != '-') {
      if (c > 0x39) c -= 55;
      else c -= 0x30;
      x = c << 4;
      c = src[i + 1];
      if (c > 0x39) c -= 55;
      else c -= 0x30;
      dst[n++] = (x + c);
    }
  }
}

void hex2array(uint8_t *src, BigKey *bk, size_t sLen) {
  hex2array(src, bk->oneChunk, sLen);
}

class buddy {
  public:
    BigKey PublicKey;
    buddy();
    uint16_t encrypt(unsigned char *, uint16_t, buddy, char *);
    void decrypt(unsigned char *, size_t, buddy, unsigned char *);
  private:
    void calculate(BigKey BobPublic);
    bool _inited = false;
    BigKey SecretKey;
    BigKey PrivateKey;
};

buddy::buddy() {
  // We need 32 bytes, ie 256 bits, for AES-256
  Serial.println("In buddy init...");
  memcpy(PrivateKey.oneChunk, randomStock + randomIndex, 32);
  randomIndex += 32;
  uint8_t i = 0;
  PublicKey.fourNumbers[i] = power(G, PrivateKey.fourNumbers[i]);
  i++;
  PublicKey.fourNumbers[i] = power(G, PrivateKey.fourNumbers[i]);
  i++;
  PublicKey.fourNumbers[i] = power(G, PrivateKey.fourNumbers[i]);
  i++;
  PublicKey.fourNumbers[i] = power(G, PrivateKey.fourNumbers[i]);
}

uint16_t buddy::encrypt(unsigned char *what, uint16_t myLen, buddy Bob, char *x) {
  calculate(Bob.PublicKey);
  // First we need to compute the SecretKey between my PrivateKey and Bob's PublicKey
  // Then we can encrypt the message
  uint16_t len = myLen;
  Serial.println("len before adjustment = " + String(len));
  if (len % 16 > 0) {
    if (len < 16) len = 16;
    else len += 16 - (len % 16);
  }
  Serial.println("len after adjustment = " + String(len));
  memset(encBuf, (len - myLen), 256);
  memcpy(encBuf, what, myLen);
  Serial.println("\nencBuf, normalized:");
  hexDump((unsigned char*)encBuf, len);
  struct AES_ctx ctx;
  double t0, t1;
  if (myMode == CBC) {
    Serial.println("CBC mode");
    uint8_t iv[16];
    // IVs should be unique. Repeat. IVs should be unique.
    memcpy(iv, randomStock + randomIndex, 16);
    // let's keep randomIndex at the same spot: we're gonna reuse it with decrypt
    Serial.print("iv at offset: ");
    Serial.println(randomIndex);
    hexDump((unsigned char*)iv, 16);
    t0 = millis();
    AES_init_ctx_iv(&ctx, SecretKey.oneChunk, iv);
    AES_CBC_encrypt_buffer(&ctx, (uint8_t*)encBuf, len);
    t1 = millis();
  } else if (myMode == CTR) {
    Serial.println("CTR mode");
    // void AES_CTR_xcrypt_buffer(struct AES_ctx* ctx, uint8_t* buf, uint32_t length);
    uint8_t iv[16];
    // IVs should be unique. Repeat. IVs should be unique.
    memcpy(iv, randomStock + randomIndex, 16);
    // We're reusing randomIndex from encrypt
    Serial.print("iv at offset: ");
    Serial.println(randomIndex);
    hexDump((unsigned char*)iv, 16);
    t0 = millis();
    AES_init_ctx_iv(&ctx, SecretKey.oneChunk, iv);
    AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)encBuf, len);
    t1 = millis();
  } else {
    Serial.println("ECB mode");
    uint8_t rounds = len / 16, steps = 0;
    printf("%u block(s) to encrypt\n", rounds);
    t0 = millis();
    AES_init_ctx(&ctx, SecretKey.oneChunk);
    for (uint8_t ix = 0; ix < rounds; ix++) {
      //void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);
      AES_ECB_encrypt(&ctx, (uint8_t*)encBuf + steps);
      steps += 16;
      // encrypts in place, 16 bytes at a time
    }
    t1 = millis();
  }
  Serial.println("Time: " + String(t1 - t0) + " ms");
  Serial.println("\nencBuf, encrypted:");
  hexDump((unsigned char*)encBuf, len);
  array2hex((uint8_t*)encBuf, len, x);
  return len * 2;
}

void buddy::decrypt(unsigned char *what, size_t myLen, buddy Bob, unsigned char *finalArray) {
  calculate(Bob.PublicKey);
  // First we need to compute the SecretKey between my PrivateKey and Bob's PublicKey
  // Then we decrypt the message
  memcpy(finalArray, what, myLen);
  //  Serial.println("\nfinalArray, as received:");
  //  hexDump((unsigned char*)finalArray, myLen);
  hex2array(finalArray, (unsigned char*)encBuf, myLen);
  uint16_t olen = myLen / 2;
  Serial.println("\nencBuf, hex decoded:");
  hexDump((unsigned char*)encBuf, olen);
  struct AES_ctx ctx;
  double t0, t1;
  if (myMode == CBC) {
    Serial.println("CBC mode");
    uint8_t iv[16];
    // IVs should be unique. Repeat. IVs should be unique.
    memcpy(iv, randomStock + randomIndex, 16);
    // We're reusing randomIndex from encrypt
    Serial.print("iv at offset: ");
    Serial.println(randomIndex);
    hexDump((unsigned char*)iv, 16);
    t0 = millis();
    AES_init_ctx_iv(&ctx, SecretKey.oneChunk, iv);
    AES_CBC_decrypt_buffer(&ctx, (uint8_t*)encBuf, olen);
    t1 = millis();
  } else if (myMode == CTR) {
    Serial.println("CTR mode");
    // void AES_CTR_xcrypt_buffer(struct AES_ctx* ctx, uint8_t* buf, uint32_t length);
    uint8_t iv[16];
    // IVs should be unique. Repeat. IVs should be unique.
    memcpy(iv, randomStock + randomIndex, 16);
    // We're reusing randomIndex from encrypt
    Serial.print("iv at offset: ");
    Serial.println(randomIndex);
    hexDump((unsigned char*)iv, 16);
    t0 = millis();
    AES_init_ctx_iv(&ctx, SecretKey.oneChunk, iv);
    AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)encBuf, olen);
    t1 = millis();
  } else {
    Serial.println("ECB mode");
    t0 = millis();
    AES_init_ctx(&ctx, SecretKey.oneChunk);
    uint8_t rounds = olen / 16, steps = 0;
    for (uint8_t ix = 0; ix < rounds; ix++) {
      //void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);
      AES_ECB_decrypt(&ctx, (uint8_t*)encBuf + steps);
      steps += 16;
      // encrypts in place, 16 bytes at a time
    }
    t1 = millis();
  }
  Serial.println("Time: " + String(t1 - t0) + " ms");
  Serial.println("\nencBuf, decrypted:");
  hexDump((unsigned char*)encBuf, olen);
  memcpy(finalArray, encBuf, olen);
}

void buddy::calculate(BigKey BobPublic) {
  uint8_t i;
  for (i = 0; i < 4; i++) {
    // We need 32 bytes, ie 256 bits, for AES-256
    // hence the loop
    // Generating the secret key after the exchange of keys
    uint64_t ka = power(BobPublic.fourNumbers[i], PrivateKey.fourNumbers[i]);
    SecretKey.fourNumbers[i] = ka;
  }
  _inited = true;
}
