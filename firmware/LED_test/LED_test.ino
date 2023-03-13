#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel

#define RGBpin 13  // The pin on the ESP32 that controls all of the RGB devices.
#define NUMPIXELS 5 // standard for the KQ board layout, change if you use the RGB headers
Adafruit_NeoPixel pixels(NUMPIXELS, RGBpin, NEO_GRB + NEO_KHZ800);
const int Buttons[] = {32, 26, 33, 27, 25};
const int BtnLEDs[] = {0, 0, 0, 0, 0};
// Just holding the button states
int ButtonStates[] = {0,0,0,0,0};

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
    BootLights();
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

void loop() {
  for (int thisPlayer = 0; thisPlayer < 5; thisPlayer++) {
      ButtonStates[thisPlayer] = digitalRead(Buttons[thisPlayer]);

      if(ButtonStates[thisPlayer] == 0){
          pixels.setPixelColor(thisPlayer, pixels.Color(255, 255, 255));
      }
      else{
          pixels.setPixelColor(thisPlayer, pixels.Color(0, 0, 0));
      }
  }
  pixels.show();
}
