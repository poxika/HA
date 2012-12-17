unsigned char relayPin[4] = {4,5,6,7};

void setup()
{
  int i;
  Serial.begin(9600); 
  // should be replaced by :
  // sets bits 7, 6, 5, 4 to 1 (OUTPUT)
  // DDRD = DDRD | B11110000;
  // sets bits 7, 6, 5, 4 to 1 (HIGH) : power off
  // PORTD = PORTD | B11110000;
  for(i = 0; i < 4; i++)
  {
    pinMode(relayPin[i],OUTPUT);
    digitalWrite(relayPin[i],HIGH);
  }
  Serial.println("All off");
  digitalWrite(relayPin[3],LOW);
  Serial.println("1 On");
  Serial.println("Waiting 1min");
  delay(60000L);
  digitalWrite(relayPin[2],LOW);
  Serial.println("2 On");
  Serial.println("Waiting 1min");
  delay(60000L);
  digitalWrite(relayPin[1],LOW);
  Serial.println("3 On");
  Serial.println("Waiting 1min");
  delay(60000L);
  digitalWrite(relayPin[0],LOW);
  Serial.println("4 On");
}

void loop()
{
 delay(10*1000);
 Serial.println("Waiting ...");
}
