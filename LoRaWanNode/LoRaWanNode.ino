#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include <SPI.h>

#define SCHED_MAX_EVENT_DATA_SIZE \
  APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE \
  60 /**< Maximum number of events in the scheduler queue. */

#define LORAWAN_ADR_ON                                                       \
  1 /**< LoRaWAN Adaptive Data Rate enabled (the end-device should be static \
       here). */
#define LORAWAN_ADR_OFF 0 /**< LoRaWAN Adaptive Data Rate disabled. */

#define LORAWAN_APP_DATA_BUFF_SIZE \
  64 /**< Size of the data to be transmitted. */
#define LORAWAN_APP_TX_DUTYCYCLE                                              \
  10000 /**< Defines the application data transmission duty cycle. 10s, value \
           in [ms]. */
#define APP_TX_DUTYCYCLE_RND                                              \
  1000 /**< Defines a random delay for application data transmission duty \
          cycle. 1s, value in [ms]. */
#define JOINREQ_NBTRIALS 3

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

#define LED1 30
#define LED2 31

// for SPI Flash(W25Q256JVFI)
const uint8_t EXT_SCK = 22;
const uint8_t EXT_MISO = 29;
const uint8_t EXT_MOSI = 13;
#define CS 6
#define RESET 5
#define HOLD 4
#define WP 8

static void lorawan_has_joined_handler(void);
static void lorawan_rx_handler(lmh_app_data_t *app_data);
static void lorawan_confirm_class_handler(DeviceClass_t Class);
static void send_lora_frame(void);
static uint32_t timers_init(void);

// APP_TIMER_DEF(lora_tx_timer_id); ///< LoRa tranfer timer instance.
TimerEvent_t appTimer;  ///< LoRa tranfer timer instance.
static uint8_t m_lora_app_data_buffer
    [LORAWAN_APP_DATA_BUFF_SIZE];  ///< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = {
    m_lora_app_data_buffer, 0, 0, 0,
    0};  ///< Lora user application data structure.

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
 */
static lmh_param_t lora_param_init = {LORAWAN_ADR_ON, LORAWAN_DEFAULT_DATARATE,
                                      LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS,
                                      LORAWAN_DEFAULT_TX_POWER};

/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
 */
static lmh_callback_t lora_callbacks = {
    BoardGetBatteryLevel,       BoardGetUniqueId,
    BoardGetRandomSeed,         lorawan_rx_handler,
    lorawan_has_joined_handler, lorawan_confirm_class_handler};

uint8_t nodeDeviceEUI[8] = {0x00, 0x95, 0x64, 0x1F, 0xDA, 0x91, 0x19, 0x0B};
uint8_t nodeAppEUI[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x02, 0x01, 0xE1};
uint8_t nodeAppKey[16] = {0x07, 0xC0, 0x82, 0x0C, 0x30, 0xB9, 0x08, 0x70,
                          0x0C, 0x0F, 0x70, 0x06, 0x00, 0xB0, 0xBE, 0x09};
uint32_t nodeDevAddr = 0x260116F8;
uint8_t nodeNwsKey[16] = {0x7E, 0xAC, 0xE2, 0x55, 0xB8, 0xA5, 0xE2, 0x69,
                          0x91, 0x51, 0x96, 0x06, 0x47, 0x56, 0x9D, 0x23};
uint8_t nodeAppsKey[16] = {0xFB, 0xAC, 0xB6, 0x47, 0xF3, 0x58, 0x45, 0xC7,
                           0x50, 0x7D, 0xBF, 0x16, 0x8B, 0xA8, 0xC1, 0x7C};

void setup_lora() {
  appTimer.timerNum = 3;
  TimerInit(&appTimer, tx_lora_periodic_handler);
  uint32_t err_code = lora_isp4520_init(SX1262);
  if (err_code != 0) {
    Serial.printf("lora_hardware_init failed - %d\n", err_code);
  }
  // Setup the EUIs and Keys
  lmh_setDevEui(nodeDeviceEUI);
  lmh_setAppEui(nodeAppEUI);
  lmh_setAppKey(nodeAppKey);
  lmh_setNwkSKey(nodeNwsKey);
  lmh_setAppSKey(nodeAppsKey);
  lmh_setDevAddr(nodeDevAddr);

  // Initialize LoRaWan
  err_code = lmh_init(&lora_callbacks, lora_param_init);
  if (err_code != 0) {
    Serial.printf("lmh_init failed - %d\n", err_code);
  }

  if (!lmh_setSubBandChannels(1)) {
    Serial.println("lmh_setSubBandChannels failed. Wrong sub band requested?");
  }

  // Start Join procedure
  lmh_join();
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
  setup_lora();
  Serial.println("lora setup completed");
  digitalWrite(LED1, 1);
  digitalWrite(LED2, 1);
}

void loop() { Radio.IrqProcess(); }

static void lorawan_has_joined_handler(void) {
  Serial.println("Network Joined");
  lmh_class_request(CLASS_A);
  TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);
}

static void lorawan_rx_handler(lmh_app_data_t *app_data) {
  Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d\n",
                app_data->port, app_data->buffsize, app_data->rssi,
                app_data->snr);

  switch (app_data->port) {
    case 3:
      // Port 3 switches the class
      if (app_data->buffsize == 1) {
        switch (app_data->buffer[0]) {
          case 0:
            lmh_class_request(CLASS_A);
            break;

          case 1:
            lmh_class_request(CLASS_B);
            break;

          case 2:
            lmh_class_request(CLASS_C);
            break;

          default:
            break;
        }
      }
      break;

    case LORAWAN_APP_PORT:
      // YOUR_JOB: Take action on received data
      break;

    default:
      break;
  }
}

static void lorawan_confirm_class_handler(DeviceClass_t Class) {
  Serial.printf("switch to class %c done\n", "ABC"[Class]);

  // Informs the server that switch has occurred ASAP
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = LORAWAN_APP_PORT;
  lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
}

static void tx_lora_periodic_handler(void) {
  TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
  TimerStart(&appTimer);
  send_lora_frame();
}

static void send_lora_frame(void) {
  digitalWrite(LED1, 0);
  Serial.print("Sending frame: ");
  if (lmh_join_status_get() != LMH_SET) {
    Serial.println("Did not join network, skip sending frame");
    return;
  }

  uint32_t i = 0;
  m_lora_app_data.port = LORAWAN_APP_PORT;
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'e';
  m_lora_app_data.buffer[i++] = 'l';
  m_lora_app_data.buffer[i++] = 'l';
  m_lora_app_data.buffer[i++] = 'o';
  m_lora_app_data.buffer[i++] = ' ';
  m_lora_app_data.buffer[i++] = 'w';
  m_lora_app_data.buffer[i++] = 'o';
  m_lora_app_data.buffer[i++] = 'r';
  m_lora_app_data.buffer[i++] = 'l';
  m_lora_app_data.buffer[i++] = 'd';
  m_lora_app_data.buffer[i++] = '!';
  m_lora_app_data.buffsize = i;

  lmh_error_status error = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
  // error == LMH_SUCCESS : success
  Serial.printf("lmh_send result %d\n", error);
  digitalWrite(LED1, 1);
}