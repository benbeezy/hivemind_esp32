#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <UNIT_PN532.h>

//Wi-fi settings//
#define SECRET_SSID "exampleSSID"
#define SECRET_PASS "examplePASSWORD"
#define CAB_IP "192.168.1.125"

//Define the pins for SPI communication.
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (14)
#define PN532_MISO (19)

TaskHandle_t ReaderThread;
TaskHandle_t StatsThread;

UNIT_PN532 nfc(PN532_SS);

/*
 * 
 * Track the game stats
 * 
 */

const char* websockets_server_host = CAB_IP; //Enter server adress
const uint16_t websockets_server_port = 12749; // Enter server port

using namespace websockets;

void onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}


/*
 * 
 * Posting card data back to hivemind for sign-in
 * 
 */
 
void postDataToServer() {
 
  Serial.println("Posting JSON data to server...");
  // Block until we are able to connect to the WiFi access point

  HTTPClient http;
  http.begin("https://kqhivemind.com/api/stats/signin/nfc/");  
  http.addHeader("Content-Type", "application/json");         
     
  StaticJsonDocument<200> doc;
  // Add values in the document
  //
  doc["scene_name"] = "slc";
  doc["cabinet_name"] = "hive";
  doc["token"] = "G_LkYIJ6MuuC2bQdbR3MH3bjxgxPMXF1dEJhM5R2rT0";
  doc["action"] = "sign_in";
  doc["card"] = "04584b91720000";
  doc["player"] = "1";
     
  String requestBody;
  serializeJson(doc, requestBody);
     
  int httpResponseCode = http.POST(requestBody);
 
  if(httpResponseCode>0){
       
    String response = http.getString();                       
       
    Serial.println(httpResponseCode);   
    Serial.println(response); 
  }
}

/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiMulti wifiMulti;
WebsocketsClient client;

/*
 * 
 * Setup
 * 
 */
 
void setup() {
  /////////////Get things setu-up and wifi connected/////////////
  Serial.begin(9600);
   
  delay(4000);
  wifiMulti.addAP(ssid, pass);
  delay(4000);

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
 * 
 * The loop that keeps on going with multi threading
 * 
 */
 
void ReaderLoop(void * pvParameters){
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++)
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX);
    }
    Serial.println("");
    // Wait 1 second before continuing
    delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
}
void StatLoop(void * pvParameters){
  client.poll();
  client.send("![k[im alive],v[1]]!");
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
