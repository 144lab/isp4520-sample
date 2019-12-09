#include <SPI.h>
#include <SPIMemory.h>
#include <bluefruit.h>

/* For a list of EIR data types see:
 *    https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 *    Matching enum: cores/nRF5/SDK/components/softdevice/s132/headers/ble_gap.h
 */

// Sample
// 02-01-06-1A-FF-81-03-07-80-E4-B2-44-00-79-0B-00-00-80-00-8C-89-A5-46-BB-2B-1D-00-00-00-C4-00

#define LED 26
#define CS 17
#define SCK 8
#define MISO 22
#define MOSI 12
#define RESET 6
#define WP 7

SPIClass SPI1(NRF_SPIM2, MISO, SCK, MOSI);
int8_t pins[] = {SCK, MISO, MOSI, CS};
SPIFlash flash(pins);

void setup() {
  Serial.setPins(31, 30);
  Serial.begin(115200);
  while (!Serial) delay(10);  // for nrf52840 with native usb

  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  pinMode(RESET, OUTPUT);
  pinMode(WP, OUTPUT);
  digitalWrite(RESET, 0);
  digitalWrite(WP, 1);
  delay(50);
  digitalWrite(RESET, 1);
  delay(5000);

  // flash.begin();

  /*
  String inputString = "This is a test String";
  flash.writeStr(0, inputString);
  String outputString = "";
  if (flash.readStr(0, outputString)) {
    Serial.print(F("Read string: "));
    Serial.println(outputString);
  }
  */

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central =
  // 1 SRAM usage required by SoftDevice will increase dramatically with number
  // of connections
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);  // Check bluefruit.h for supported values

  // Set the device name
  Bluefruit.setName("Bluefruit52");

  // Set the LED interval for blinky pattern on BLUE LED
  Bluefruit.setConnLedInterval(250);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Filter out packet with a min rssi
   * - Interval = 100 ms, window = 50 ms
   * - Use active scan (used to retrieve the optional scan response adv packet)
   * - Start(0) = will scan forever since no timeout is given
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);
  // Bluefruit.Scanner.filterUuid(BLEUART_UUID_SERVICE); // only invoke callback
  // if detect bleuart service
  Bluefruit.Scanner.setInterval(160, 80);  // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);   // Request scan response data
  Bluefruit.Scanner.start(0);  // 0 = Don't stop scanning after n seconds

  Serial.println("Scanning ...");
}

const uint8_t header[] = {0x02, 0x01, 0x06, 0x1A, 0xFF, 0x81, 0x03, 0x07,
                          0x80, 0xE4, 0xB2, 0x44, 0x00, 0x79, 0x0B};

void scan_callback(ble_gap_evt_adv_report_t* report) {
  PRINT_LOCATION();
  uint8_t len = 0;
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));

  // Display the timestamp and device address
  // if (report->type.scan_response) {
  //  return;
  //}
  // MAC is in little endian --> print reverse
  // Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  // Serial.print("\n");

  if (report->data.len >= 30) {
    if (memcmp(report->data.p_data, header, 5) == 0) {
      // Raw buffer contents
      Serial.printf("%14s %d bytes\n", "PAYLOAD", report->data.len);
      if (report->data.len) {
        Serial.printf("%15s", " ");
        Serial.printBuffer(report->data.p_data, report->data.len, '-');
        Serial.println();
      }
    }
  }

  /*
  // RSSI value
  Serial.printf("%14s %d dBm\n", "RSSI", report->rssi);

  // Adv Type
  Serial.printf("%14s ", "ADV TYPE");
  if (report->type.connectable) {
    Serial.print("Connectable ");
  } else {
    Serial.print("Non-connectable ");
  }

  if (report->type.directed) {
    Serial.println("directed");
  } else {
    Serial.println("undirected");
  }

  // Shortened Local Name
  if (Bluefruit.Scanner.parseReportByType(
          report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, buffer,
  sizeof(buffer))) { Serial.printf("%14s %s\n", "SHORT NAME", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  // Complete Local Name
  if (Bluefruit.Scanner.parseReportByType(report,
                                          BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                          buffer, sizeof(buffer))) {
    Serial.printf("%14s %s\n", "COMPLETE NAME", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  // TX Power Level
  if (Bluefruit.Scanner.parseReportByType(
          report, BLE_GAP_AD_TYPE_TX_POWER_LEVEL, buffer,
  sizeof(buffer))) { Serial.printf("%14s %i\n", "TX PWR LEVEL",
  buffer[0]); memset(buffer, 0, sizeof(buffer));
  }

  // Check for UUID16 Complete List
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, buffer,
      sizeof(buffer));
  if (len) {
    printUuid16List(buffer, len);
  }

  // Check for UUID16 More Available List
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, buffer,
      sizeof(buffer));
  if (len) {
    printUuid16List(buffer, len);
  }

  // Check for UUID128 Complete List
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, buffer,
      sizeof(buffer));
  if (len) {
    printUuid128List(buffer, len);
  }

  // Check for UUID128 More Available List
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE,
  buffer, sizeof(buffer)); if (len) { printUuid128List(buffer, len);
  }

  // Check for BLE UART UUID
  if (Bluefruit.Scanner.checkReportForUuid(report,
  BLEUART_UUID_SERVICE)) { Serial.printf("%14s %s\n", "BLE UART", "UUID
  Found!");
  }

  // Check for DIS UUID
  if (Bluefruit.Scanner.checkReportForUuid(report,
                                           UUID16_SVC_DEVICE_INFORMATION))
  { Serial.printf("%14s %s\n", "DIS", "UUID Found!");
  }

  // Check for Manufacturer Specific Data
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer,
      sizeof(buffer));
  if (len) {
    Serial.printf("%14s ", "MAN SPEC DATA");
    Serial.printBuffer(buffer, len, '-');
    Serial.println();
    memset(buffer, 0, sizeof(buffer));
  }

  Serial.println();
  */

  // For Softdevice v6: after received a report, scanner will be
  // paused We need to call Scanner resume() to continue scanning
  Bluefruit.Scanner.resume();
}

void printUuid16List(uint8_t* buffer, uint8_t len) {
  Serial.printf("%14s %s", "16-Bit UUID");
  for (int i = 0; i < len; i += 2) {
    uint16_t uuid16;
    memcpy(&uuid16, buffer + i, 2);
    Serial.printf("%04X ", uuid16);
  }
  Serial.println();
}

void printUuid128List(uint8_t* buffer, uint8_t len) {
  (void)len;
  Serial.printf("%14s %s", "128-Bit UUID");

  // Print reversed order
  for (int i = 0; i < 16; i++) {
    const char* fm = (i == 4 || i == 6 || i == 8 || i == 10) ? "-%02X" : "%02X";
    Serial.printf(fm, buffer[15 - i]);
  }

  Serial.println();
}

void loop() {
  // nothing to do
}