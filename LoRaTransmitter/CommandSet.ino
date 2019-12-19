#include <Arduino.h>

#include "TinyFlash.h"
#include "global.h"

TinyFlash flash(PIN_FLASH_CS);

uint8_t block[256] = {0};

Context context;

void StartLogging(const String &payload) {
  int pos = payload.indexOf(",");
  context.posix = payload.substring(0, pos).toInt();
  context.serial = payload.substring(pos + 1).toInt();
  context.start = millis();
  // digitalWrite(LED, 0);
  block[0] = 0xa5;  // start mark
  block[1] = (context.serial >> 0) & 0xff;
  block[2] = (context.posix >> 0) & 0xff;
  block[3] = (context.posix >> 8) & 0xff;
  block[4] = (context.posix >> 16) & 0xff;
  block[5] = (context.posix >> 24) & 0xff;
  if (!flash.eraseSector(INFO_OFFSET)) {
    Serial.println("start logging failed: flash erase failed");
    return;
  }
  if (!flash.writePage(INFO_OFFSET, block)) {
    Serial.println("start logging failed: flash write failed");
  }
}

void doCommand(const String &line) {
  int pos;
  if (line.length() < 1) {
    Serial.println("invalid command");
    return;
  }
  char cmd = line.charAt(0);
  String payload = line.substring(1);
  switch (cmd) {
    case 0x41:
      if (payload.length() < 14) {
        Serial.println("invalid command");
        return;
      }
      StartLogging(payload);
      break;
    case 0x42:
      break;
    case 0x43:
      break;
    case 0x44:
    case 0x45:
      break;
    case 0x46:
      break;
    case 0x47:
      break;
  }
}

void setupCmd() {
  setupDipSW();
  pinMode(PIN_FLASH_CS, OUTPUT);
  digitalWrite(PIN_FLASH_CS, 1);
  pinMode(PIN_FLASH_RESET, OUTPUT);
  digitalWrite(PIN_FLASH_RESET, 0);
  pinMode(PIN_FLASH_HOLD, OUTPUT);
  digitalWrite(PIN_FLASH_HOLD, 1);
  pinMode(PIN_FLASH_WP, OUTPUT);
  digitalWrite(PIN_FLASH_WP, 1);
  delay(200);
  digitalWrite(PIN_FLASH_RESET, 1);

  flash.begin();

  context.start = 0;
  context.serial = 255;

  if (flash.beginRead(INFO_OFFSET)) {
    if (flash.readNextByte() == 0xa5) {
      uint32_t posix, start;
      uint8_t serial;
      serial = flash.readNextByte() << 0;
      posix = flash.readNextByte() << 0;
      posix |= flash.readNextByte() << 8;
      posix |= flash.readNextByte() << 16;
      posix |= flash.readNextByte() << 24;
      context.start = 1;
      context.posix = posix;
      context.serial = serial;
    }
    flash.endRead();
  }
}

void loopCmd() {
  static char buff[18];
  static int index = -1;
  if (Serial.available() > 0) {
    int dat = Serial.read();
    switch (dat) {
      case STX:
        index = 0;
        break;
      case ETX:
        buff[index] = 0x00;
        doCommand(String(buff));
        index = -1;
        break;
      default:
        if (index >= 0) {
          buff[index] = (char)(dat);
          index++;
          if (index > 17) {
            Serial.println("buffer overflow");
            index = -1;
          }
        }
    }
  }
}
