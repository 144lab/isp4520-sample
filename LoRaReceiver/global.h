#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifdef REGION_AS923
/* JP channels
923.2 - SF7BW125 to SF12BW125
923.4 - SF7BW125 to SF12BW125
922.2 - SF7BW125 to SF12BW125
922.4 - SF7BW125 to SF12BW125
922.6 - SF7BW125 to SF12BW125
922.8 - SF7BW125 to SF12BW125
923.0 - SF7BW125 to SF12BW125
922.0 - SF7BW125 to SF12BW125
*/
const uint32_t CHANNELS[] = {
    923200000, 923400000, 922200000, 922400000,
    922600000, 922800000, 923000000, 922000000,
};
#endif
#ifdef REGION_EU868
/* EU channels
868.1 - SF7BW125 to SF12BW125
868.3 - SF7BW125 to SF12BW125 and SF7BW250
868.5 - SF7BW125 to SF12BW125
867.1 - SF7BW125 to SF12BW125
867.3 - SF7BW125 to SF12BW125
867.5 - SF7BW125 to SF12BW125
867.7 - SF7BW125 to SF12BW125
867.9 - SF7BW125 to SF12BW125
*/
const uint32_t CHANNELS[] = {
    868100000, 868300000, 868500000, 867100000,
    867300000, 867500000, 867700000, 867900000,
};
#endif

// LoRa Parameters
#define TX_OUTPUT_POWER 14  // dBm (max:14dBm for JP & EU)
#define LORA_BANDWIDTH 0    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 12  // [SF7..SF12]
#define LORA_CODINGRATE 1         // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8    // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0     // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

// nRF52832 - SX126x pin configuration
#define PIN_LORA_RESET 19  // LORA RESET
#define PIN_LORA_NSS 24    // LORA SPI CS
#define PIN_LORA_SCLK 23   // LORA SPI CLK
#define PIN_LORA_MISO 25   // LORA SPI MISO
#define PIN_LORA_DIO_1 11  // LORA DIO_1
#define PIN_LORA_BUSY 27   // LORA SPI BUSY
#define PIN_LORA_MOSI 26   // LORA SPI MOSI
#define RADIO_TXEN -1      // LORA ANTENNA TX ENABLE
#define RADIO_RXEN -1      // LORA ANTENNA RX ENABLE

// Board pin definitions
#define PIN_FLASH_CS 6
#define PIN_FLASH_SCLK 22
#define PIN_FLASH_MISO 13
#define PIN_FLASH_MOSI 9
#define PIN_FLASH_RESET 5
#define PIN_FLASH_HOLD 4
#define PIN_FLASH_WP 8

typedef struct {
  uint8_t id;
  uint8_t ch;
  uint8_t serial;
  uint32_t start;
  uint32_t posix;
} Context;
extern Context context;

#define STX 0x02
#define ETX 0x03
// 最大レコード数　1ブロック分少なくして末尾の１ブロックは開始情報の記録に使う
#define MAX_ENTRIES (2796202 - 43)
// FLASH末尾ブロックの先頭オフセット
#define INFO_OFFSET 33554176
// 後半へのオフセット
#define OFFSET 16777216

// Counter is in 32kHz ticks. (NRF_WDT->CRV + 1)/32768 seconds
#define WATCHDOG_COUNTER (32768 * 2)

#define WDT_Setup()                                                 \
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos); \
  NRF_WDT->CRV = WATCHDOG_COUNTER;                                  \
  NRF_WDT->RREN = WDT_RREN_RR0_Enabled << WDT_RREN_RR0_Pos;         \
  NRF_WDT->TASKS_START = 1

#define WDT_Clear() (NRF_WDT->RR[0] = WDT_RR_RR_Reload)

extern uint8_t *lastValues[2];

#endif  // _GLOBAL_H