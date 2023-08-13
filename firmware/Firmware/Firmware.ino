#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>
#include <Wire.h>
#include <SPI.h>
#include <UNIT_PN532.h> // https://github.com/elechouse/PN532
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#define PN532_SS 14 // The digital data pin the NFC reader is connected.
#define RGBpin 13  // The pin on the ESP32 that controls all of the RGB devices.
#define NUMPIXELS 5 // standard for the KQ board layout, change if you use the RGB headers

#ifdef ESP32
  #include <SPIFFS.h>
#endif

Adafruit_NeoPixel pixels(NUMPIXELS, RGBpin, NEO_GRB + NEO_KHZ800);
UNIT_PN532 nfc(PN532_SS);

/*
All arrays are in stripes, abs, queen, skulls, checks order.
*/
const int Buttons[] = {32, 26, 33, 27, 25};
const int BtnLEDs[] = {0, 0, 0, 0, 0};
const bool FlipLights = false;

// Just holding the button states
int ButtonStates[] = {0,0,0,0,0};
int PlayerSignInStates[] = {0,0,0,0,0};

// In RGB values, can go up to 255 but it pulls a lot of power an can cause bad logic
int LightColor[3] = {128, 128, 128};

TaskHandle_t NFCloop;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);


  // INITIALIZE all the pins
  for (int thisPlayer = 0; thisPlayer < 5; thisPlayer++) {
    pinMode(Buttons[thisPlayer], INPUT_PULLUP);
  }

  // INITIALIZE NeoPixel strip object
  pixels.begin();
  pixels.clear();
  wm.setClass("invert"); // set dark theme

  WiFiManagerParameter token("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter scene_name("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter cabinet_ip("apikey", "API token", api_token, 32);
  WiFiManagerParameter ("apikey", "API token", api_token, 32);

  wm.addParameter(&token);
  wm.addParameter(&scene_name);
  wm.addParameter(&cabinet_ip);
  wm.addParameter(&hivemind_ip);
  wm.addParameter(&cab_color);

  std::vector<const char *> menu = {"param","wifi","info","sep","restart","exit"};
  wm.setMenu(menu);

  // RESET our configs if you hold down queen while booting
  if(digitalRead(Buttons[2]) == 0){
      Serial.println("Formatting file system...")
      SPIFFS.format();
      Serial.println("Resetting wifi...");
      wm.resetSettings(); // Rest all of the wifi settings, this includes hivemind info.
      Wifi_fail();
      Wifi_succeed();
  }
  wm.setSaveParamsCallback(saveParamCallback);
  // Boot lights let us know everything has started and we finishing our boot process
  BootLights();
  pixels.clear();

  bool res;
  res = wm.autoConnect("KQ_HiveMind","killerqueen"); // password protected ap

  if(!res) {
      Wifi_fail();
  } 
  else {
      Wifi_succeed();
  }

  // INITIALIZE our NFC reader
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    NFC_fail();
    while (1); // halt
  }

  Serial.print("NFC reader ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A card");


  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    ReadNFC,       // Task function.
                    "NFCloop",     // name of task.
                    10000,         // Stack size of task
                    NULL,          // parameter of the task
                    1,             // priority of the task
                    &NFCloop,      // Task handle to keep track of created task
                    0);            // pin task to core 0   
          
  delay(500); 
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void postDataToServer() {
 
  Serial.println("Posting JSON data to server...");
  // Block until we are able to connect to the WiFi access point

  HTTPClient http;
  http.begin("https://kqhivemind.com/api/stats/signin/nfc/");  
  http.addHeader("Content-Type", "application/json");         
     
  StaticJsonDocument<200> doc;
  // add these values into the post
  doc["scene_name"] = getParam("scene"); // Taken from the hivemind_config
  doc["cabinet_name"] = getParam("cabinet_name"); // Taken from the hivemind_config
  doc["token"] = getParam("token"); // Taken from the hivemind_config
  
  doc["action"] = "sign_in"; // This needs to be set depending on if that button is signed in already or not
  doc["card"] = "04584b91720000"; //This is Bens fixed card ID just for testing
  doc["player"] = "1"; // This value needs to change so that it's the right player not static
     
  String requestBody;
  serializeJson(doc, requestBody);
     
  int httpResponseCode = http.POST(requestBody);
 
  if(httpResponseCode>0){
       
    String response = http.getString();                       
       
    Serial.println(httpResponseCode);   
    Serial.println(response); 
  }
}

void saveParamCallback(){
  Serial.println("Your config settings");
  Serial.println("PARAM token = " + getParam("token"));
  Serial.println("PARAM scene_name = " + getParam("scene_name"));
  Serial.println("PARAM cabinet_name = " + getParam("cabinet_name"));
  Serial.println("PARAM cabinet_ip = " + getParam("cabinet_ip"));
  Serial.println("PARAM hivemind_ip = " + getParam("hivemind_ip"));
  Serial.println("PARAM cab_color = " + getParam("cab_color"));
}

void NFC_fail(){
    Serial.println("NFC failed to connect"); // We are going to blink red 3 times if NFC fails to connect
    for(int i=0; i<3; i++) {
        for(int i=0; i<NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(128, 0, 0));
        }
        pixels.show();
        delay(1000);
        pixels.clear();
        delay(1000);
    }
}

void Wifi_fail(){
    Serial.println("Failed to connect");
    for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(128, 0, 0));
    }
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(1000);
    pixels.clear();
    ESP.restart(); // Will reboot the board if wifi can't connect.
}

void Wifi_succeed(){
    Serial.println("Hivemind online!");
    for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 128, 0));
    }
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(1000);
        for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show(); // turn off pixels
}

void BootLights(){
    for(int i=0; i<1; i++) {
        for(int i=0; i<NUMPIXELS; i++) { // this will be our boot animation
            pixels.setPixelColor(i, pixels.Color(128, 200, 0));
            pixels.show();   // Send the updated pixel colors to the hardware.
            delay(100);
        }
        for(int i=0; i<NUMPIXELS; i++) { // this will be our boot animation
            pixels.setPixelColor(i, pixels.Color(0, 0, 128));
            pixels.show();   // Send the updated pixel colors to the hardware.
            delay(100);
        }
    }
    for(int i=0; i<NUMPIXELS; i++) { // this will be our boot animation
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(100);
    }
}

void ReadNFC(void * pvParameters){
    Serial.print("NFC running on core ");
    Serial.println(xPortGetCoreID());
    for(;;){
        boolean success;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
        uint8_t uidLength;	// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
        if (success) {
          // Wait 1 second before continuing
          Serial.println("Found a card!");
          Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
          Serial.print("UID Value: ");
          for (uint8_t i=0; i < uidLength; i++)
          {
            Serial.print(uid[i], HEX);
          }
          Serial.println("");

          //this is just to turn the lights on
          BootLights();
          delay(1000);
        }
    }
}

void SignInData(int player){
    // We also need to pass the uid of the player card when we are ready for it.
    PlayerSignInStates[player] = 1;
    pixels.setPixelColor(player, pixels.Color(128, 128, 128));
}

void SignOutData(int player){
    PlayerSignInStates[player] = 0;
    pixels.setPixelColor(player, pixels.Color(0, 0, 0));
}

void loop() {
  /*
  ReadNFC();
  for (int thisPlayer = 0; thisPlayer < 5; thisPlayer++) {
      ButtonStates[thisPlayer] = digitalRead(Buttons[thisPlayer]);

      if(ButtonStates[thisPlayer] == 0){
          SignInData(thisPlayer);
      }
      else{
          SignOutData(thisPlayer);
      }
  }
  pixels.show();
  */
}
