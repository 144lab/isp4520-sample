#include <Arduino.h>
#include <SPI.h>
#include <SX126x-Arduino.h>

#include "global.h"

#ifndef RX_CHANNEL
#define RX_CHANNEL 0
#endif
#define CHANNEL RX_CHANNEL

// SPI class for communication between nRF52832 and SX126x
SPIClass SPI_LORA(NRF_SPIM2, PIN_LORA_MISO, PIN_LORA_SCLK, PIN_LORA_MOSI);

#define LED1 13

static time_t timeToSend;
static time_t cadTime;

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Serial.printBuffer(payload, size, '-');
  Serial.print(",");
  Serial.print(rssi);
  Serial.print(",");
  Serial.println(snr);
  Radio.Rx(RX_TIMEOUT_VALUE);
}
void onRxTimeout(void) { Radio.Rx(RX_TIMEOUT_VALUE); }
void onRxError(void) { Radio.Rx(RX_TIMEOUT_VALUE); }

void setupLoRa(uint8_t ch) {
  static RadioEvents_t RadioEvents;
  lora_isp4520_init(SX1262);
  RadioEvents.RxDone = onRxDone;
  RadioEvents.RxTimeout = onRxTimeout;
  RadioEvents.RxError = onRxError;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(CHANNELS[ch]);
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

  Serial.setPins(8, 6);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  setupLoRa(CHANNEL);
  Serial.println("setup completed");

  digitalWrite(LED1, 1);
}

void loop() { Radio.IrqProcess(); }
