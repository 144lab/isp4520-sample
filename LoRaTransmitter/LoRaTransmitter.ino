#include <Arduino.h>
#include <SPI.h>
#include <SX126x-Arduino.h>
#include <bluefruit.h>
#include <system/utilities.h>

#include "global.h"

#define APP_TX_DUTYCYCLE 1000

#define PIN_DIPSW1 10
#define PIN_DIPSW2 12
#define PIN_DIPSW3 14
#define PIN_DIPSW4 15
#define PIN_DIPSW5 16
#define PIN_DIPSW6 18
#define PIN_DIPSW7 20

static const uint8_t DIPSW[] = {PIN_DIPSW1, PIN_DIPSW2, PIN_DIPSW3, PIN_DIPSW4,
                                PIN_DIPSW5, PIN_DIPSW6, PIN_DIPSW7};
void setupDipSW() {
  for (int i = 0; i < 7; i++) {
    pinMode(DIPSW[i], INPUT_PULLUP);
  }
}

uint8_t dipsw() {
  uint8_t v = 0;
  for (int i = 0; i < 7; i++) {
    v |= (digitalRead(DIPSW[i]) & 1) << i;
  }
  return v;
}

// SPI class for communication between nRF52832 and SX126x
SPIClass SPI_LORA(NRF_SPIM2, PIN_LORA_MISO, PIN_LORA_SCLK, PIN_LORA_MOSI);

#define LED1 31
#define LED2 30

TimerEvent_t appTimer;
static bool sending;
static time_t timeToSent;
static time_t sendingStart;
static time_t sendingDuration = 1000;
uint8_t txBuffer[11] = {0};
uint8_t *lastValues[2] = {&txBuffer[1], &txBuffer[5]};

static void onTxDone(void) {
  timeToSent = millis();
  sendingDuration = timeToSent - sendingStart;
  Serial.print("send: ");
  Serial.printBuffer(txBuffer, 11, '-');
  Serial.print(" (");
  Serial.print(uint32_t(sendingDuration));
  Serial.println("ms)");
  digitalWrite(LED2, 1);
  sending = false;
}
static void onTxTimeout(void) {
  digitalWrite(LED2, 1);
  Serial.println("onTxTimeout");
  sending = false;
}
static void onCadDone(bool cadResult) {
  txBuffer[0] = context.id;
  uint16_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += txBuffer[i];
  }
  txBuffer[10] = ~(sum - 1) & 0xff;
  if (cadResult) {
    Radio.Rx(RX_TIMEOUT_VALUE);
  } else {
    delay(randr(5, 1005));
    digitalWrite(LED2, 0);
    sendingStart = millis();
    Radio.Send(txBuffer, 11);
  }
}

void setupLoRa(uint8_t ch) {
  static RadioEvents_t RadioEvents;
  lora_isp4520_init(SX1262);
  RadioEvents.TxDone = onTxDone;
  RadioEvents.TxTimeout = onTxTimeout;
  RadioEvents.CadDone = onCadDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(CHANNELS[ch]);
  // Set Radio TX configuration
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                    0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
}

void startCad() {
  Radio.Standby();
  SX126xSetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10,
                     LORA_CAD_ONLY, 0);
  SX126xSetDioIrqParams(IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  Radio.StartCad();
}

static void onInterval1Hz() {
  TimerSetValue(&appTimer, APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);
  if (!sending && sendingDuration * 10 < millis() - timeToSent) {
    sending = true;
    startCad();
  }
}

// Sample PAYLOAD
// 02-01-06-1A-FF-81-03-07-80-E4-B2-44-00-79-0B-00-00-80-00-8C-89-A5-46-BB-2B-1D-00-00-00-C4-00
//                                              ~~~~~ 00-00:圧力計
//                                              00-01:流量計
//                                              ......80-00-8C-89-A5-46-BB-2B-SN-YY-YY-YY-ZZ-00

void onScanCallback(ble_gap_evt_adv_report_t *report) {
  const uint8_t SIRC_Header[] = {0x02, 0x01, 0x06, 0x1A, 0xFF, 0x81, 0x03, 0x07,
                                 0x80, 0xE4, 0xB2, 0x44, 0x00, 0x79, 0x0B};
  do {
    if (report->data.len >= 30) {
      if (memcmp(report->data.p_data, SIRC_Header, sizeof(SIRC_Header)) == 0) {
        uint8_t kind, sn, bat;
        uint32_t value;
        kind = uint16_t(report->data.p_data[15]);
        kind |= uint16_t(report->data.p_data[16]);
        if (kind != 0 && kind != 1) break;
        sn = report->data.p_data[25];
        if (context.start > 0 && sn == context.serial) {
          Serial.print("recv(");
          Serial.print(kind);
          Serial.print("): ");
          Serial.printBuffer(&report->data.p_data[25], 5, '-');
          Serial.println();
          lastValues[kind][0] = sn;
          lastValues[kind][1] = report->data.p_data[26];
          lastValues[kind][2] = report->data.p_data[27];
          if (kind) {
            lastValues[kind][3] = report->data.p_data[28];
          }
          lastValues[kind][3 + kind] = report->data.p_data[29];
        }
      }
    }
  } while (false);
  Bluefruit.Scanner.resume();
}

void setupBLE(void) {
  Bluefruit.autoConnLed(false);
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);
  Bluefruit.setName("SIRC-Logger");
  Bluefruit.Scanner.setRxCallback(onScanCallback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);
  Bluefruit.Scanner.setInterval(160, 80);  // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);   // Request scan response data
  Bluefruit.Scanner.start(0);  // 0 = Don't stop scanning after n seconds
}

void setup() {
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, 0);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, 0);
  setupDipSW();

  Serial.setPins(17, 7);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  context.id = dipsw();
  context.ch = context.id % 3;

  setupBLE();
  setupLoRa(context.ch);
  setupCmd();

  appTimer.timerNum = 3;
  TimerInit(&appTimer, onInterval1Hz);
  TimerSetValue(&appTimer, APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);

  Serial.print("ID: ");
  Serial.println(context.id);
  Serial.print("Channel: ");
  Serial.println(context.ch);
  Serial.print("Serial: ");
  Serial.println(context.serial);
  Serial.println("setup completed");

  digitalWrite(LED2, 1);
  digitalWrite(LED1, 1);

  // WDT_Setup();
}

void loop() {
  Radio.IrqProcess();
  loopCmd();
  WDT_Clear();
}
