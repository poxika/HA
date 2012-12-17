#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define LED 5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;
// My MAC
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x9A, 0xE0};

// initialize the library instance:
EthernetClient client;

long lastConnectionTime = 0;        // last time you connected to the server, in milliseconds
const int postingInterval = 30000;  // update every 20s

void setup(void)
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  // start serial port
  Serial.begin(9600);

  Serial.println("Initializing DS1820");
  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  
  Serial.println("Configuring Ethernet Shield");
    if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(1000); 

  float tempF = sensors.getTempF(insideThermometer);
  // Connect after a given # of seconds
  if (millis() - lastConnectionTime > postingInterval) {
    digitalWrite(LED, HIGH);
    sendData(tempF);
    digitalWrite(LED, LOW);
  }
}

// this method makes a HTTP connection to the server:
void sendData(float thisData) {
  // if there's a successful connection:
  Serial.print("Sending ");
  Serial.println(thisData);
  if (client.connect("api.pachube.com", 80)) {
    char buffer[10]; // 10 bytes should be ok
    Serial.println("connecting to pachube...");
    // send the HTTP PUT request. 
    // fill in your feed address here:
    client.print("PUT /v2/feeds/48299/datastreams/T HTTP/1.1\n");
    client.print("Host: api.pachube.com\n");
    client.print("X-PachubeApiKey: PIygqqUiPASLLqufNNnsLmBiZ12SAKw1Vys3THpLUmZNWT0g\n");
    
    client.print("Content-Length: ");

    client.println(6, DEC);

    // last pieces of the HTTP PUT request:
    client.print("Content-Type: text/csv\n");
    client.println("Connection: close\n");

    // here's the actual content of the PUT request:
    client.println(thisData,DEC);

    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println("disconnecting.");
    client.stop();
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}


// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:

int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten, 
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
