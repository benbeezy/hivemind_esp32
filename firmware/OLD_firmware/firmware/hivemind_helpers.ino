bool absStatus = false;
bool stripesStatus = false;
bool queenStatus = false;
bool checksStatus = false;
bool skullsStatus = false;

void flashLEDs(){
  digitalWrite(skulls_led, HIGH);
  digitalWrite(abs_led, HIGH);
  digitalWrite(queen_led, HIGH);
  digitalWrite(checks_led, HIGH);
  digitalWrite(stripes_led, HIGH);
  delay(500);
  digitalWrite(skulls_led, LOW);
  digitalWrite(abs_led, LOW);
  digitalWrite(queen_led, LOW);
  digitalWrite(checks_led, LOW);
  digitalWrite(stripes_led, LOW);
  delay(500);
}

void setLEDs(){
  //abs
  if(absStatus == true){digitalWrite(abs_led, HIGH);}
  else{digitalWrite(abs_led, LOW);}
  
  //stripes
  if(stripesStatus == true){digitalWrite(stripes_led, HIGH);}
  else{digitalWrite(stripes_led, LOW);}

  //queen
  if(queenStatus == true){digitalWrite(queen_led, HIGH);}
  else{digitalWrite(queen_led, LOW);}

  //checks
  if(checksStatus == true){digitalWrite(checks_led, HIGH);}
  else{digitalWrite(checks_led, LOW);}

  //skulls
  if(skullsStatus == true){digitalWrite(skulls_led, HIGH);}
  else{digitalWrite(abs_led, LOW);}
}
