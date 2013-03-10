#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <util.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#include "poxika.h"
#include "relays.h"

//#define TEST 1
//#define NOSENSOR 1
//#define NOETH 1

// PIN definitions
#define ONE_WIRE_BUS 2

// Pachube definitions
#define FEED "52915" 
#define KEY "AIVLMfLavxYz9-pQ7MNyPlIu8liSAKw0VThoV25wZ0xpaz0g"
#define HOST "poxika-meter.appspot.com"

//Time constants
#ifdef TEST
#define ONE_SECOND 100UL
#define TEN_SECONDS 1000UL
#define THIRTY_SECONDS 3000UL
#define ONE_MINUTE 6000UL
#define ONE_HOUR 36000UL
#else
#define ONE_SECOND 1000UL
#define TEN_SECONDS 10000UL
#define THIRTY_SECONDS 30000UL
#define ONE_MINUTE 60000UL
#define ONE_HOUR 3600000UL
#endif

// relay assignment
#define COMCAST 4
#define ATT 3
#define LINKSYS 2
#define UNUSED 1
// start sequence
int seq[4] = {COMCAST,ATT,LINKSYS,UNUSED};

// metrics
int onboard_temp; // onboard temp value
int restart_count; // number of restarts
int error_count;

boolean success;

unsigned int restarts;
unsigned int errors;

// timers
unsigned long next_post;
unsigned long next_restart;
unsigned long last_connection_time = 0;     // last time you connected to the server, in milliseconds
unsigned long restart_interval;
const long posting_interval = THIRTY_SECONDS;  // update every 30s

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Relays initialization
Relays relays;

unsigned char init_count;

// onboard sensor address is 10 2F FB 21 00 00 00 40
DeviceAddress onboard_temp_sensor = { 0x10, 0x2F, 0xFB, 0x21, 0x00, 0x00, 0x00, 0x40};

// Ethernet Library setup
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x9A, 0xE0 };
EthernetClient eth;
byte ip[] = { 192, 168, 100, 5 };  

// initialize cosm
Poxika poxika(FEED,KEY);


void setup(void) {
  Serial.begin(9600);
  
  init_count = EEPROM.read(0);
  Serial.print("Init Count ");
  Serial.println(init_count);
  if (init_count > 254) init_count =0;
  ++init_count;
#ifndef TEST
  EEPROM.write(0,init_count);
#else
  Serial.print("New Init Count ");
  Serial.println(init_count);
#endif

  // start the relays
  Serial.println("Starting devices");
  relays.sequence(ONE_MINUTE, seq); 
  Serial.println("All devices started");
  
#ifndef NOSENSOR 
  // initialize DS1820 sensors
  sensors.begin();
  // resolution of 9 is 0.5C, 10 is 0.2/0.3C
  sensors.setResolution(onboard_temp_sensor, 10);
#endif

#ifndef NOETH
  // initialize Poxika client
  // to test the output, just pass Serial
  // poxika.init(&Serial);
  poxika.init(&eth);
#else
  poxika.init(&Serial);
#endif

  // Pass a buffer of 10 char to store the values
  // all buffers need to be different to avoid optimization
  onboard_temp = poxika.add("t1","1111111111");
  restart_count = poxika.add("e1","2222222222");
  error_count = poxika.add("e2","0000000000");


  
#ifndef NOETH
  Serial.println("Configuring Ethernet");
  Ethernet.begin(mac,ip);
#endif

  last_connection_time = millis();
  restarts = 0;
  errors = 0;
  
}

void loop() {
  delay(TEN_SECONDS);
  
  restart_interval = ONE_HOUR + (ONE_HOUR * (restarts * restarts));
  next_restart = last_connection_time + restart_interval;
  next_post =  last_connection_time + posting_interval;
#ifdef TEST
  Serial.print((long)(millis() - next_restart));
  Serial.print(">=");
  Serial.println(0);
#endif

  if ((long) (millis() - next_post) >= 0) {
    
    // updating metrics
    Serial.println("reading Temp");
#ifndef NOSENSOR
    sensors.requestTemperatures();
    success = poxika.update(onboard_temp, sensors.getTempC(onboard_temp_sensor));
#else
    success = poxika.update(onboard_temp,(float) 12.5);
#endif
    
    //updating counters
    success = poxika.update(restart_count, restarts);
    if (errors<0) errors=0;
    success = poxika.update(error_count, errors);
    
    // sending
    Serial.println("Connecting ...");
#ifndef NOETH
    boolean res = eth.connect(HOST, 80);
#else
    boolean res = true;
#endif
    if (res) {
      Serial.println("sending ...");
      int result = poxika.send(HOST);
      Serial.print("Return code is ");
      Serial.println(result);
      Serial.println("Disconnecting ...");
#ifndef NOETH
      eth.stop();
#endif
      Serial.println("Disconnected ...");
      if (result == 200){
        last_connection_time = millis(); 
        restarts = 0;
        errors = 0;
      } else {
        Serial.println("sending error");
        errors += 1;
      }
    }
    else {
      Serial.println("connection failed");
      errors += 1;
    }
  }

  if ((long)(millis() - next_restart) >= 0)
  {
     Serial.println("Restarting ...");
     // adding more delay to try to solve potential issues
#ifdef TEST
     Serial.println(ONE_MINUTE+(TEN_SECONDS * restarts));
#endif
     relays.sequence(ONE_MINUTE+(TEN_SECONDS * restarts), seq);
     restarts += 1; 
     Serial.println("Restarted");
  }
}

