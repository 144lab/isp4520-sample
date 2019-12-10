#ifndef _TINYFLASH_H_
#define _TINYFLASH_H_

#include <Arduino.h>

class TinyFlash {
 public:
  TinyFlash(uint8_t cs = 6);
  uint32_t begin(void);
  boolean beginRead(uint32_t addr);
  boolean writePage(uint32_t addr, uint8_t *data);
  boolean eraseChip(void);
  boolean eraseSector(uint32_t addr);
  uint8_t readNextByte(void);
  void endRead(void);
  void Enable();
  void Disable();

 private:
  boolean waitForReady(uint32_t timeout = 100L), writeEnable(void);
  void writeDisable(void), cmd(uint8_t c);
  uint8_t cs_pin;
};

#endif  // _TINYFLASH_H_
