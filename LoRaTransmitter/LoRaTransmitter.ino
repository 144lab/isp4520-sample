#include <Arduino.h>
#include <SPI.h>
#include <SX126x-Arduino.h>
#include <bluefruit.h>

#define SCHED_MAX_EVENT_DATA_SIZE \
  APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE 60

// nRF52832 - SX126x pin configuration
int PIN_LORA_RESET = 19;  // LORA RESET
int PIN_LORA_NSS = 24;    // LORA SPI CS
int PIN_LORA_SCLK = 23;   // LORA SPI CLK
int PIN_LORA_MISO = 25;   // LORA SPI MISO
int PIN_LORA_DIO_1 = 11;  // LORA DIO_1
int PIN_LORA_BUSY = 27;   // LORA SPI BUSY
int PIN_LORA_MOSI = 26;   // LORA SPI MOSI
int RADIO_TXEN = -1;      // LORA ANTENNA TX ENABLE
int RADIO_RXEN = -1;      // LORA ANTENNA RX ENABLE
// SPI class for communication between nRF52832 and SX126x
SPIClass SPI_LORA(NRF_SPIM2, PIN_LORA_MISO, PIN_LORA_SCLK, PIN_LORA_MOSI);

#define LED1 31
#define LED2 30

// for SPI Flash(W25Q256JVFI)
const uint8_t EXT_SCK = 22;
const uint8_t EXT_MISO = 29;
const uint8_t EXT_MOSI = 13;
#define CS 6
#define RESET 5
#define HOLD 4
#define WP 8

#define RF_FREQUENCY 923000000  // Hz
#define TX_OUTPUT_POWER 14      // dBm (max:14dBm for JP)
#define LORA_BANDWIDTH 0  // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 11  // [SF7..SF12]
#define LORA_CODINGRATE 1         // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8    // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0     // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000
#define BUFFER_SIZE 64

#define APP_TX_DUTYCYCLE 1000

struct {
  uint8_t serial;
  uint32_t start;
  uint32_t posix;
} context;

static RadioEvents_t RadioEvents;
TimerEvent_t appTimer;

static void OnTxDone(void);
static void OnTxTimeout(void);
static void OnCadDone(bool cadResult);

void setup_lora() {
  lora_isp4520_init(SX1262);
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.CadDone = OnCadDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  // Set Radio TX configuration
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                    0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
}

static void OnInterval1Hz(void);
const uint8_t SIRC_Header[] = {0x02, 0x01, 0x06, 0x1A, 0xFF, 0x81, 0x03, 0x07,
                               0x80, 0xE4, 0xB2, 0x44, 0x00, 0x79, 0x0B};

uint8_t TxBuffer[9] = {0};
static uint8_t *last[2] = {&TxBuffer[1], &TxBuffer[5]};

// Sample PAYLOAD
// 02-01-06-1A-FF-81-03-07-80-E4-B2-44-00-79-0B-00-00-80-00-8C-89-A5-46-BB-2B-1D-00-00-00-C4-00
//                                              ~~~~~ 00-00:圧力計
//                                              00-01:流量計
//                                              ......80-00-8C-89-A5-46-BB-2B-SN-YY-YY-YY-ZZ-00

void OnScanCallback(ble_gap_evt_adv_report_t *report) {
  do {
    if (report->data.len >= 30) {
      if (memcmp(report->data.p_data, SIRC_Header, sizeof(SIRC_Header)) == 0) {
        uint8_t kind, sn, bat;
        uint32_t value;
        kind = uint16_t(report->data.p_data[15]);
        kind |= uint16_t(report->data.p_data[16]);
        if (kind != 0 && kind != 1) break;
        TxBuffer[0] = report->data.p_data[25];
        if (context.start > 0 && TxBuffer[0] == context.serial) {
          memcpy(last[kind], &report->data.p_data[26], 4);
        }
      }
    }
  } while (false);
  Bluefruit.Scanner.resume();
}

void initBLE(void) {
  Bluefruit.autoConnLed(false);
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);
  Bluefruit.setName("SIRC-Logger");
  // Bluefruit.setConnLedInterval(250);
  Bluefruit.Scanner.setRxCallback(OnScanCallback);
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

  Serial.setPins(17, 7);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("reset boot");
  appTimer.timerNum = 3;
  TimerInit(&appTimer, OnInterval1Hz);
  TimerSetValue(&appTimer, APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);
  initBLE();
  Serial.println("ble setup completed");
  setup_lora();
  Serial.println("lora setup completed");

  digitalWrite(LED2, 1);
  digitalWrite(LED1, 1);

  // TODO: get from dipsw
  context.serial = 1;
  context.start = millis();
}

static void OnInterval1Hz() {
  digitalWrite(LED2, 0);
  Serial.println("Timer Interrupt");
  TimerSetValue(&appTimer, APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);
  StartCad();
}

void loop() {
  Radio.IrqProcess();
  delay(0);
}

static time_t timeToSend;
static time_t cadTime;

void StartCad() {
  Radio.Standby();
  SX126xSetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10,
                     LORA_CAD_ONLY, 0);
  SX126xSetDioIrqParams(IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  cadTime = millis();
  Radio.StartCad();
  Serial.println("start cad");
}

static void OnTxDone(void) { Serial.println("OnTxDone"); }
static void OnTxTimeout(void) { Serial.println("OnTxTimeout"); }
static void OnCadDone(bool cadResult) {
  static uint8_t buffer[64];
  Serial.println("OnCadDone");
  time_t duration = millis() - cadTime;
  if (cadResult) {
    Radio.Rx(RX_TIMEOUT_VALUE);
  } else {
    Radio.Send(TxBuffer, 9);
  }
  digitalWrite(LED2, 1);
}
