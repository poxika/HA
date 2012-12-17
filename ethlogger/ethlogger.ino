#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>

#include <pachube.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// PIN definitions
#define ONE_WIRE_BUS 2
#define GREEN_LED  6
#define RED_LED  7

// Pachube definitions
#define FEED "52915" 
#define KEY "AIVLMfLavxYz9-pQ7MNyPlIu8liSAKw0VThoV25wZ0xpaz0g"
#define HOST "api.cosm.com"

int onboard; // onboard temp value
long last_connection_time = 0;     // last time you connected to the server, in milliseconds
const int posting_interval = 10000;  // update every 10s

boolean ready_to_send = false;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// onboard sensor address is 10 2F FB 21 00 00 00 40
DeviceAddress onboard_temp_sensor = { 0x10, 0x2F, 0xFB, 0x21, 0x00, 0x00, 0x00, 0x40};

// Ethernet Library setup
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x9A, 0xE0 };
EthernetClient eth;

// initialize cosm
Pachube cosm(FEED,KEY);

unsigned long startTime;

void setup(void) {
  Serial.begin(9600);
  // initialize DS1820 sensors
  sensors.begin();
  // resolution of 9 is 0.5C, 10 is 0.2/0.3C
  sensors.setResolution(onboard_temp_sensor, 10);
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  // initialize Pachube client
  // to test the output, just pass Serial
  // cosm.init(&Serial);
  cosm.init(&eth);
  
  // Pass a buffer of 10 char to store the values
  onboard = client.add_stream("t1","1234567890");
  
  int success = 0;
  for (;success==0;){
    Serial.println("Configuring Ethernet");
    if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP Trying again");
      digitalWrite(RED_LED, HIGH);
      // Maybe the network is restarting, wait for 1 min
      delay(60000L);
    } else {
      success = 1;
      digitalWrite(RED_LED, LOW);
    }
  }
}

void loop() {
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
  if (millis() - last_connection_time > posting_interval) {
    Serial.println("reading Temp");
    // get sensor readings
    sensors.requestTemperatures();
    cosm.update_stream(onboard,sensors.getTempC(onboard_temp_sensor));
    
    if (eth.connect(HOST, 80)) {
      Serial.println("sending Temp");
      digitalWrite(GREEN_LED, HIGH);
      cosm.send();
      // may need to read response to check return code.
      Serial.println("Disconnecting ...");
      eth.stop();
    } 
    else {
       // if you couldn't make a connection:
       Serial.println("connection failed");
    }
  }
}

