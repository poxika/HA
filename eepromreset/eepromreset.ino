#include <EEPROM.h>

void setup(void) {
  Serial.begin(9600);
  Serial.println("reseting eeprom at position 0");
  EEPROM.write(0,0);
  
}
void loop() {
  delay(ONE_SECOND);
}
