#include <bluefruit.h>
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
#define MAX_ENTRIES 2796202

const uint8_t header[] = {0x02, 0x01, 0x06, 0x1A, 0xFF, 0x81, 0x03, 0x07,
                          0x80, 0xE4, 0xB2, 0x44, 0x00, 0x79, 0x0B};
typedef struct {
  int32_t count;
  int32_t wrote;
  int index;
  uint8_t buffer[256];
} Buffer;

struct {
  uint8_t serial;
  uint32_t start;
  uint32_t posix;
  Buffer buffer[2];
} context;

uint32_t getIndex;
uint32_t maxIndex;

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
  digitalWrite(CS, 1);
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, 0);
  pinMode(HOLD, OUTPUT);
  digitalWrite(HOLD, 1);
  pinMode(WP, OUTPUT);
  digitalWrite(WP, 1);
  delay(200);
  digitalWrite(RESET, 1);

  // Message("reset boot");
  uint32_t cap = flash.begin();
  // Message(String("flash begin:") + String(cap));

  context.start = 0;
  context.serial = 255;
  context.buffer[0].count = 0;
  context.buffer[0].wrote = 0;
  context.buffer[1].count = 0;
  context.buffer[1].wrote = 0;
  getIndex = 0;
  maxIndex = 0;

  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);
  Bluefruit.setName("SIRC-Logger");
  Bluefruit.setConnLedInterval(250);
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);
  Bluefruit.Scanner.setInterval(160, 80);  // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);   // Request scan response data
  Bluefruit.Scanner.start(0);  // 0 = Don't stop scanning after n seconds

  startTimer(1000000);
}

void loop() {
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
            Message("buffer overflow");
            index = -1;
          }
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
    case 0x47:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      StopLogging();
      break;
    case 0x48:
      if (payload.length() > 0) {
        Message("invalid command");
        return;
      }
      Dump();
      break;
  }
}

void StartLogging(const String &payload) {
  int pos = payload.indexOf(",");
  context.posix = payload.substring(0, pos).toInt();
  context.serial = payload.substring(pos + 1).toInt();
  context.start = millis();
  // digitalWrite(LED, 0);
}

void StopLogging() {
  if (context.buffer[0].index > 0) {
    _flash(&context.buffer[0], 0);
  }
  if (context.buffer[1].index > 0) {
    _flash(&context.buffer[1], 16777216);
  }
  context.start = 0;
  // digitalWrite(LED, 1);
}

void Erase() {
  context.start = 0;
  digitalWrite(LED, 0);
  if (!flash.eraseChip()) {
    Message("flash erase failed");
  }
  for (int i; i < 2; i++) {
    context.buffer[i].count = 0;
    context.buffer[i].wrote = 0;
    context.buffer[i].index = 0;
  }
  digitalWrite(LED, 1);
}

void Check() {
  static char buff[9];
  String s = "0";
  sprintf(buff, "%08X", context.buffer[0].count);
  s += String(buff);
  sprintf(buff, "%08X", context.buffer[1].count);
  s += String(buff);
  Send(s);
}

void Get(int maxNum) {
  static char buff[9];
  String s = "1";
  sprintf(buff, "%02X", context.serial);
  s += buff;
  sprintf(buff, "%08X", context.posix);
  s += buff;
  uint32_t bigger = max(context.buffer[0].wrote, context.buffer[1].wrote);
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

void Dump() {
  static uint8_t buff[256];
  if (flash.beginRead(0)) {
    for (int i = 0; i < 256; i++) {
      buff[i] = flash.readNextByte();
    }
    flash.endRead();
    Serial.write(buff, sizeof(buff));
  }
}

void Next() {
  for (uint32_t i = 0; i < 100; i++) {
    bool any = false;
    char buff[11] = {0};
    noInterrupts();
    if (getIndex < context.buffer[0].wrote) {
      if (flash.beginRead(getIndex * 6)) {
        buff[0] = flash.readNextByte();
        buff[1] = flash.readNextByte();
        buff[2] = flash.readNextByte();
        buff[3] = flash.readNextByte();
        buff[4] = flash.readNextByte();
        flash.endRead();
      }
      any |= true;
    }
    interrupts();
    noInterrupts();
    if (getIndex < context.buffer[1].wrote) {
      if (flash.beginRead(getIndex * 6 + 16777216)) {
        buff[5] = flash.readNextByte();
        buff[6] = flash.readNextByte();
        buff[7] = flash.readNextByte();
        buff[8] = flash.readNextByte();
        buff[9] = flash.readNextByte();
        buff[10] = flash.readNextByte();
        flash.endRead();
      }
      any |= true;
    }
    interrupts();
    Serial.write(buff, sizeof(buff));
    if (!any || maxIndex < getIndex) {
      break;
    }
    getIndex++;
  }
}

void _flash(Buffer *buffer, uint32_t offset) {
  int32_t count = buffer->count;
  if (!flash.writePage((count * 6 + offset) & 0xffffff00, buffer->buffer)) {
    Message("write failed:" + String(count));
  }
  memset(buffer->buffer, 0, sizeof(buffer->buffer));
  buffer->index = 0;
  buffer->wrote = count;
}

void write(uint8_t kind, uint32_t value) {
  if (kind != 0 && kind != 1) return;
  uint32_t offset = uint32_t(kind) * 16777216;
  Buffer *buffer = &(context.buffer[kind]);
  uint32_t sec = (millis() - context.start) / 1000;
  if (buffer->count < MAX_ENTRIES) {
    buffer->buffer[buffer->index++] = (sec >> 0) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->buffer[buffer->index++] = (sec >> 8) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->buffer[buffer->index++] = (sec >> 16) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->buffer[buffer->index++] = (value >> 0) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->buffer[buffer->index++] = (value >> 8) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->buffer[buffer->index++] = (value >> 16) & 0xff;
    if (buffer->index == 256) _flash(buffer, offset);
    buffer->count++;
  }
}

static uint32_t sirc_value[2] = {0};

extern "C" void TIMER2_IRQHandler(void) {
  int32_t ret = 0;
  if ((NRF_TIMER2->EVENTS_COMPARE[0] != 0) &&
      ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk) != 0)) {
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;  // Clear compare register 0 event
  }
  if (context.start > 0) {
    write(0, sirc_value[0]);
    write(1, sirc_value[1]);
  }
}

void startTimer(unsigned long us) {
  NRF_TIMER2->TASKS_STOP = 1;
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;  // Set the timer in Counter Mode
  NRF_TIMER2->TASKS_CLEAR = 1;  // clear the task first to be usable for later
  NRF_TIMER2->PRESCALER = 4;    // Set prescaler. Higher number gives slower
                                // timer.
  NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit
                        << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER2->CC[0] = us;  // Set value for TIMER2 compare register 0

  // Enable interrupt on Timer 2, both for CC[0] and CC[1] compare match events
  NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Enabled
                         << TIMER_INTENSET_COMPARE0_Pos;
  // Clear the timer when COMPARE0 event is triggered
  NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Enabled
                       << TIMER_SHORTS_COMPARE0_CLEAR_Pos;

  NRF_TIMER2->TASKS_START = 1;  // Start TIMER
  NVIC_EnableIRQ(TIMER2_IRQn);
}

void stopTimer() { NRF_TIMER2->TASKS_STOP = 1; }

// Sample PAYLOAD
// 02-01-06-1A-FF-81-03-07-80-E4-B2-44-00-79-0B-00-00-80-00-8C-89-A5-46-BB-2B-1D-00-00-00-C4-00
//                                              ~~~~~ 00-00:圧力計
//                                              00-01:流量計
//                                              ......80-00-8C-89-A5-46-BB-2B-SN-YY-YY-YY-ZZ-00

void scan_callback(ble_gap_evt_adv_report_t *report) {
  do {
    if (report->data.len >= 30) {
      if (memcmp(report->data.p_data, header, sizeof(header)) == 0) {
        uint8_t kind, sn, bat;
        uint32_t value;
        kind = uint16_t(report->data.p_data[15]);
        kind |= uint16_t(report->data.p_data[16]);
        if (kind != 0 && kind != 1) break;
        sn = report->data.p_data[25];
        if (context.start > 0 && sn == context.serial) {
          value = report->data.p_data[28] << 0;
          value |= report->data.p_data[27] << 8;
          value |= report->data.p_data[26] << 16;
          bat = report->data.p_data[29];
          sirc_value[kind] = value;
          /*
          Message(String("kind:") + String(kind) + String(", ") +
                  String("sn:") + String(sn) + String(", ") + String("value:") +
                  String(value) + String(", ") + String("bat:") + String(bat) +
                  String(", "));
          */
        }
      }
    }
  } while (false);
  // For Softdevice v6: after received a report, scanner will be
  // paused We need to call Scanner resume() to continue scanning
  Bluefruit.Scanner.resume();
}
