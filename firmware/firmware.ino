#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <SPI.h>
#include <UNIT_PN532.h>
#include "hivemind_config.h"

TaskHandle_t ReaderThread;
TaskHandle_t StatsThread;

UNIT_PN532 nfc(PN532_SS);
String lastScanID = "";

/*
 * Track the game stats
 */

const char* websockets_server_host = CAB_IP; //Enter server adress
const uint16_t websockets_server_port = 12749; // Enter server port

using namespace websockets;

WebsocketsClient client;

/*
 * Setup
 */
 
void setup() {
  /////////////Setup pins///////////////
  pinMode(skulls_button, INPUT);
  pinMode(abs_button, INPUT);
  pinMode(checks_button, INPUT);
  pinMode(queen_button, INPUT);
  pinMode(stripes_button, INPUT);

  pinMode(skulls_led, OUTPUT);
  pinMode(abs_led, OUTPUT);
  pinMode(checks_led, OUTPUT);
  pinMode(queen_led, OUTPUT);
  pinMode(stripes_led, OUTPUT);
  
  /////////////Get things setup and wifi connected with captive portal/////////////
  hivemind_wifi_setup();

  /////////////Connect to cab/////////////
  // run callback when messages are received
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  client.connect(websockets_server_host, websockets_server_port, "/");
  client.send("![k[im alive],v[1]]!");
  client.ping();
  
  postDataToServer();
  
  Serial.println("Starting reader setup");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board, make sure the little switches are right");
    while (1); // halt
  }

  // Set the max number of retry attempts.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Card reader setup and reader to scan...");
}

/*
 * Reader loop that waits and scans cards
 */
 
void ReaderLoop(void * pvParameters){
  for(;;) {
    boolean success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

    if (success) {
      Serial.println("Found a card!");
      Serial.print("UID Value: ");
      for (uint8_t i=0; i < uidLength; i++)
      {
        lastScanID = lastScanID + uid[i];
      }
      Serial.println(lastScanID);
      dimLEDs();
      delay(1000);
    }
    else
    {
      // PN532 probably timed out waiting for a card
      Serial.println("Waiting for a card");
    }
  }
}

/*
 * The always going stats loop
 */

void StatLoop(void * pvParameters){
  for(;;) {
    //client.poll();
    //client.send("![k[im alive],v[1]]!");
    //TO-DO: Make it post to hivemind, use the hivemind_stats.ino file
  }
}

void loop() {

  ////////////Card reader thread///////////////
  xTaskCreatePinnedToCore(
                    ReaderLoop,   /* Task function. */
                    "Reader thread",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &ReaderThread,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */

  ////////////Stats thread///////////////
  xTaskCreatePinnedToCore(
                    StatLoop,   /* Task function. */
                    "Stats thread",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &StatsThread,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */
    
}
