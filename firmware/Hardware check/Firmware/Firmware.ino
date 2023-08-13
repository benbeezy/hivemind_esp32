#include <Wire.h>
#include <SPI.h>
#include <UNIT_PN532.h> // https://github.com/elechouse/PN532
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel

#define PN532_SS 14 // The digital data pin the NFC reader is connected.
#define RGBpin 13  // The pin on the ESP32 that controls all of the RGB devices.
#define NUMPIXELS 5 // standard for the KQ board layout, change if you use the RGB headers
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

WiFiManager wm;
WiFiManagerParameter token; // global param ( for non blocking w params )
WiFiManagerParameter cabinet_ip; // global param ( for non blocking w params )
WiFiManagerParameter hivemind_ip; // global param ( for non blocking w params )
WiFiManagerParameter cab_color; // global param ( for non blocking w params )

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

    int token_length = 45; // How long can your API key be
    int cabinet_ip_length = 32; // How long can your API key be
    int hivemind_ip_length = 32; // How long can your API key be

    new (&token) WiFiManagerParameter("clienttoken", "Client token", "", token_length,"placeholder=\"Client token\"");
    new (&cabinet_ip) WiFiManagerParameter("cabinetip", "Cabinet IP", "kq.local", cabinet_ip_length,"placeholder=\"192.168.x.x\"");
    new (&hivemind_ip) WiFiManagerParameter("hivemindip", "Hivemind IP", "kqhivemind.com", hivemind_ip_length,"placeholder=\"kqhivemind.com\"");
    const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='blue' checked> Blue<br><input type='radio' name='customfieldid' value='gold'> Gold";
    new (&cab_color) WiFiManagerParameter(custom_radio_str); // custom html input
  
    wm.addParameter(&token);
    wm.addParameter(&cabinet_ip);
    wm.addParameter(&hivemind_ip);
    wm.addParameter(&cab_color);

    std::vector<const char *> menu = {"param","wifi","info","sep","restart","exit"};
    wm.setMenu(menu);

    // RESET our configs if you hold down queen while booting
    if(digitalRead(Buttons[2]) == 0){
        Serial.println("Resetting wifi");
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
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  Serial.println("Your config settings");
  Serial.println("PARAM token = " + getParam("token"));
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
    for(int i=0; i<5; i++) {
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
}

void ReadNFC(){
    Serial.println("Trying to read card!");
    boolean success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
    uint8_t uidLength;	// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
    if (success) {
      
      for(int i=0; i<NUMPIXELS; i++) {
          pixels.setPixelColor(i, pixels.Color(0, 128, 0));
      }
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(1000);
      for(int i=0; i<NUMPIXELS; i++) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }

      pixels.show(); // turn off pixels
      // Wait 1 second before continuing
      Serial.println("Found a card!");
      Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
      Serial.print("UID Value: ");
      for (uint8_t i=0; i < uidLength; i++)
      {
        Serial.print(" 0x");Serial.print(uid[i], HEX);
      }
      Serial.println("");

      delay(1000);
    }
    else
    {
      // PN532 probably timed out waiting for a card
      Serial.println("Timed out waiting for a card");
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
}
