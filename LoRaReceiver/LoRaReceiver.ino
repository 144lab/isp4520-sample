#include <Arduino.h>
#include <SPI.h>
#include <SX126x-Arduino.h>

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

void OnTxDone(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxTimeout(void);
void OnRxTimeout(void);
void OnRxError(void);
void OnCadDone(bool cadResult);

#define RF_FREQUENCY 923000000  // Hz
#define TX_OUTPUT_POWER 14      // dBm (max:14dBm for JP)
#define LORA_BANDWIDTH 0  // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 11  // [SF7..SF12]
#define LORA_CODINGRATE 1        // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 10000
#define TX_TIMEOUT_VALUE 3000
#define BUFFER_SIZE 64

#define APP_TX_DUTYCYCLE 1000

static RadioEvents_t RadioEvents;
TimerEvent_t appTimer;

void setup_lora() {
  lora_isp4520_init(SX1262);
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = OnCadDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  // Set Radio RX configuration
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0,
                    0, LORA_IQ_INVERSION_ON, true);

  // Start LoRa
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void setup() {
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, 0);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, 0);

  Serial.setPins(8, 6);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("reset boot");
  setup_lora();
  Serial.println("lora setup completed");

  digitalWrite(LED2, 1);
  digitalWrite(LED1, 1);
}

void loop() { Radio.IrqProcess(); }

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
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Serial.print(millis());
  Serial.print(" ");
  Serial.printBuffer(payload, size, '-');
  Serial.print(" (RSSI:");
  Serial.print(rssi);
  Serial.print(", SNR:");
  Serial.print(snr);
  Serial.println(")");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
void OnRxTimeout(void) {
  //Serial.println("OnRxTimeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
void OnRxError(void) {
  Serial.println("OnRxError");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
void OnCadDone(bool cadResult) {
  static uint8_t buffer[64];
  Serial.println("OnCadDone");
  time_t duration = millis() - cadTime;
  if (cadResult) {
    Radio.Rx(RX_TIMEOUT_VALUE);
  } else {
    buffer[0] = 'P';
    buffer[1] = 'I';
    buffer[2] = 'N';
    buffer[3] = 'G';
    Radio.Send(buffer, 4);
  }
  digitalWrite(LED2, 1);
}
