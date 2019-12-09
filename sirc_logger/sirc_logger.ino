#include <stdio.h>

#define LED 26
#define CS 6
#define SCK 10
#define MISO 29
#define MOSI 9
#define RESET 5
#define HOLD 28
#define WP 8

#include "TinyFlash.h"

TinyFlash flash(CS);

#define STX 0x02
#define ETX 0x03

struct {
  uint8_t serial;
  uint32_t start;
  uint32_t posix;
  uint32_t write0Index;
  uint32_t write1Index;
} context;

void Send(const String &payload) {
  Serial.printf("\x02%s\x03", payload.c_str());
}
void Message(const String &message) {
  Serial.printf("\x02\x39%s\x03", message.c_str());
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
  // Serial.setPins(31, 30);
  Serial.begin(115200);

  // pinMode(SCK, OUTPUT);
  // pinMode(MOSI, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, 0);
  pinMode(HOLD, OUTPUT);
  digitalWrite(HOLD, 1);
  pinMode(WP, OUTPUT);
  digitalWrite(WP, 1);
  delay(200);
  digitalWrite(RESET, 1);

  Message("reset boot");
  uint32_t cap = flash.begin();
  Message(String("flash begin:") + String(cap));

  context.serial = 255;
  context.write0Index = -1;
  context.write1Index = -1;
}

void loop() {
  static char buff[16];
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
        buff[index] = (char)(dat);
        index++;
        if (index > 15) {
          Message("buffer overflow");
          index = -1;
        }
    }
  }
}

void doCommand(const String &line) {
  int pos;
  if (line.length() < 1) {
    Message("invalid command");
    return;
  }
  char cmd = line.charAt(0);
  String payload = line.substring(1);
  switch (cmd) {
    case 0x41:
      if (payload.length() < 14) {
        Message("invalid command");
        return;
      }
      StartLogging(payload);
      break;
    case 0x42:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Erase();
      break;
    case 0x43:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Check();
      break;
    case 0x44:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Get(100);
      break;
    case 0x45:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Get(-1);
      break;
    case 0x46:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Next();
      break;
  }
}

void StartLogging(const String &payload) {
  int pos = payload.indexOf(",");
  context.posix = payload.substring(0, pos - 1).toInt();
  context.serial = payload.substring(pos + 1).toInt();
  context.start = millis();
  digitalWrite(LED, 0);
}

void Erase() {
  if (!flash.eraseChip()) {
    Message("flash erase failed");
  }
  context.write0Index = 0;
  context.write1Index = 0;
}
void Check() {
  static char buff[9];
  String s = "0";
  sprintf(buff, "%08X", context.write0Index);
  s += String(buff);
  sprintf(buff, "%08X", context.write1Index);
  s += String(buff);
  Send(s);
}

uint32_t getIndex;
uint32_t maxIndex;

void Get(int maxNum) {
  static char buff[9];
  String s = "1";
  sprintf(buff, "%02X", context.serial);
  s += buff;
  sprintf(buff, "%08X", context.posix + ((millis() - context.start) / 1000));
  s += buff;
  uint32_t bigger = max(context.write0Index, context.write1Index);
  sprintf(buff, "%08X", bigger);
  s += buff;
  Send(s);
  getIndex = 0;
  if (maxNum < 0) {
    maxIndex = bigger;
  } else {
    maxIndex = uint32_t(maxNum);
  }
}

void Next() {
  for (uint32_t i = 0; i < 100; i++) {
    bool any = false;
    char buff[11] = {0};
    if (getIndex + i <= context.write0Index) {
      uint32_t tm = 0;   // TODO: read from SPI-memory
      uint32_t val = 0;  // TODO: read from SPI-memory
      buff[0] = (tm >> 0) & 255;
      buff[1] = (tm >> 8) & 255;
      buff[2] = (tm >> 16) & 255;
      buff[3] = (val >> 0) & 255;
      buff[4] = (val >> 8) & 255;
      any |= true;
    }
    if (getIndex + i <= context.write1Index) {
      uint32_t tm = 0;   // TODO: read from SPI-memory
      uint32_t val = 0;  // TODO: read from SPI-memory
      buff[5] = (tm >> 0) & 255;
      buff[6] = (tm >> 8) & 255;
      buff[7] = (tm >> 16) & 255;
      buff[8] = (val >> 0) & 255;
      buff[9] = (val >> 8) & 255;
      buff[10] = (val >> 16) & 255;
      any |= true;
    }
    Serial.write(buff);
    if (!any) {
      break;
    }
    getIndex++;
  }
}
