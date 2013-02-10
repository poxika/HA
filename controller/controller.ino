#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <util.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "poxika.h"
#include "relays.h"

// PIN definitions
#define ONE_WIRE_BUS 2

// Pachube definitions
#define FEED "52915" 
#define KEY "AIVLMfLavxYz9-pQ7MNyPlIu8liSAKw0VThoV25wZ0xpaz0g"
#define HOST "poxika-meter.appspot.com"

#define ONE_SECOND 1000UL
#define TEN_SECONDS 10000UL
#define THIRTY_SECONDS 30000UL
#define ONE_MINUTE 60000UL
#define ONE_HOUR 3600000UL



int onboard_value; // onboard temp value
int restarts_value; // number of restarts
int errors_value;
int res;
int errors;

long last_connection_time = 0;     // last time you connected to the server, in milliseconds

const long posting_interval = THIRTY_SECONDS;  // update every 30s

boolean ready_to_send = false;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Relays initialization
Relays relays;

// onboard sensor address is 10 2F FB 21 00 00 00 40
DeviceAddress onboard_temp_sensor = { 0x10, 0x2F, 0xFB, 0x21, 0x00, 0x00, 0x00, 0x40};

// Ethernet Library setup
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x9A, 0xE0 };
EthernetClient eth;
byte ip[] = { 192, 168, 100, 5 };  

// initialize cosm
Poxika poxika(FEED,KEY);

unsigned int restarts;
unsigned long restart_delay;

void setup(void) {
  Serial.begin(9600);
  
  Serial.println("Starting devices");
  // startup the relays
  relays.sequence(ONE_MINUTE); 
  Serial.println("All devices started");
  
  // initialize DS1820 sensors
  sensors.begin();
  // resolution of 9 is 0.5C, 10 is 0.2/0.3C
  sensors.setResolution(onboard_temp_sensor, 10);
    
  // initialize Poxika client
  // to test the output, just pass Serial
  // poxika.init(&Serial);
  poxika.init(&eth);
  
  // Pass a buffer of 10 char to store the values
  onboard_value = poxika.add("t1","1234567890");
  restarts_value = poxika.add("e1","0987654321");
  errors_value = poxika.add("e2","0000000000");

  
  restarts = 0;
  errors = 0;
  Serial.println("Configuring Ethernet");
  Ethernet.begin(mac,ip);
  delay(ONE_SECOND);
}

void loop() {
  
  delay(ONE_SECOND);
  
  if (millis() - last_connection_time > posting_interval) {
    // updating sensors
    Serial.println("reading Temp");
    sensors.requestTemperatures();
    res = poxika.update(onboard_value, sensors.getTempC(onboard_temp_sensor));
    res = poxika.update(restarts_value, restarts);
    if (errors<0) errors=0;
    res = poxika.update(errors_value, errors);
    
    // sending
    if (eth.connect(HOST, 80)) {
      last_connection_time = millis();
      
      Serial.println("sending Temp");
      int result = poxika.send(HOST);
      Serial.print("Return code is ");
      Serial.println(result);
      Serial.println("Disconnecting ...");
      eth.stop();
      if (result == 200){
        restarts = 0;
        errors = 0;
      }
    } 
    else {
      Serial.println("connection failed");
      errors +=1;
      restart_delay = ONE_HOUR + (ONE_HOUR * (restarts * restarts));
      if ((millis() - last_connection_time) > restart_delay){
        Serial.println("Restarting ...");
        // adding more delay to try to solve potential issues
        relays.sequence(ONE_MINUTE+(TEN_SECONDS * restarts));
        restarts += 1; 
        Serial.println("Restarted");
      }
      delay(ONE_MINUTE);
    }
  }
}

