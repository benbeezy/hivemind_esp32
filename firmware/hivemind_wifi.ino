void hivemind_wifi_setup(){
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  
  Serial.begin(115200);
      
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  
  // Un-comment out the line below this to reset the wifi connections settings
  // wm.resetSettings();
  
  bool res;
  res = wm.autoConnect("Hivemind",""); // If you want to add a password you can in the second quoted section
  
  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }
}
