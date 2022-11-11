void postDataToServer() {
 
  Serial.println("Posting JSON data to server...");
  // Block until we are able to connect to the WiFi access point

  HTTPClient http;
  http.begin("https://kqhivemind.com/api/stats/signin/nfc/");  
  http.addHeader("Content-Type", "application/json");         
     
  StaticJsonDocument<200> doc;
  // add these values into the post
  doc["scene_name"] = scene_name; // Taken from the hivemind_config
  doc["cabinet_name"] = cabinet_name; // Taken from the hivemind_config
  doc["token"] = token; // Taken from the hivemind_config
  
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