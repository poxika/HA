// Inspired by https://github.com/jcrouchley/WiFly-Shield

#include <OneWire.h>
#include <DallasTemperature.h>
#include "pachube.h"
#include "rnxv.h"

// PIN definitions
#define BEE_EN_PIN 5
#define ONE_WIRE_BUS 2

// Pachube definitions
#define FEED "52915" 
#define KEY "AIVLMfLavxYz9-pQ7MNyPlIu8liSAKw0VThoV25wZ0xpaz0g"
#define HOST "api.pachube.com"

// outdoor sensor address is 28 5B 4B C2 03 00 00 8B

int inside;
int outside;

long last_connection_time = 0;     // last time you connected to the server, in milliseconds
const int posting_interval = 1000;  // update every 30s

boolean ready_to_send = false;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// outdoor sensor address is 28 5B 4B C2 03 00 00 8B
DeviceAddress outside_temp_sensor = { 0x28, 0x5B, 0x4B, 0xC2, 0x03, 0x00, 0x00, 0x8B};

Pachube client(FEED,KEY);

RnXv wifi(BEE_EN_PIN);

void setup(void) {
  Serial.begin(9600);
  // initialize DS1820 sensors
  sensors.begin();
//  sensors.setResolution(inside_temp_sensor, 9);
// resolution of 9 is 0.5C, 10 is 0.2/0.3C
  sensors.setResolution(outside_temp_sensor, 10);
  
  // initial RNXV
  wifi.init(&Serial);
  
  // initialize Pachube client
  client.init(&wifi);
//  inside = client.add_stream("t1","1234567890");
//  if (inside < 0) while(1);
  outside = client.add_stream("t2","1234567890");
  if (inside < 0) while(1);
}


void loop() {
  if (millis() - last_connection_time > posting_interval) {

    // get sensor readings
    sensors.requestTemperatures();
//    client.update_stream(inside,sensors.getTempC(inside_temp_sensor));
    client.update_stream(outside,sensors.getTempC(outside_temp_sensor));
    
    // power up RN XV
    wifi.power_on();
    if (wifi.connect("api.pachube.com", 80)){
      client.send();
      last_connection_time = millis();
      wifi.disconnect();
    }
    // power off RN XV
    wifi.power_off();
  }
}
