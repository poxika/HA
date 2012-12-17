
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#define DEBUG
#include "tempLogger.h"

#define RF_PIN  8
#define ONE_WIRE_BUS 2
#define GREEN_LED  6
#define RED_LED  7

#define LC1 0x8E
#define LC2 0xF2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress Tin;

// My MAC
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x9A, 0xE0};

// initialize the library instance:
EthernetClient client;

long lastConnectionTime = 0;        // last time you connected to the server, in milliseconds
const int postingInterval = 30000;  // update every 30s


/* Overflow interrupt vector */
ISR(TIMER1_OVF_vect){                 // here if no input pulse detected
   overflows++;                       // incriment overflow count  
}

/* ICR interrupt vector */
ISR(TIMER1_CAPT_vect)
{
  volatile long pulseLength;
  TCNT1 = 0;                            // reset the counter
  timeValue = ICR1 + (overflows * 0xffff);    // grab the timer value
  overflows = 0;
  if((bitRead(TCCR1B ,ICES1) == false) && (timeValue > 600))  // falling edge detected
  {
    pulseLength = (timeValue * precision)/1000;
    if IS_ZERO(pulseLength) { // Got a zero
       readByte = readByte << 1;
    } else if IS_ONE(pulseLength) { // got one
      readByte = readByte << 1;
      readByte = readByte | 1;
    } else { // drop 
      readByte = 0;
      state = 0; // drop packet
      bitCount = 0;
    }

    if (((readByte == 0xA0) || (readByte == 0xAE)) && (state == 0)){ //got first byte
      packet[0] = readByte;
      state = 1;
      bitCount = 8;
    }
    if ((state == 1) && (bitCount == 16)) { // got sensor address
      state = 2;
      packet[1] = readByte;
    }
    if ((state == 2) && (bitCount == 24)){ // got high byte or measure
      state = 3;
      packet[2] = readByte;
    }
    if ((state == 3) && (bitCount == 32)){ // got last part of measure
      state = 4;
      packet[3] = readByte;
    }
    if ((state == 4) && (bitCount == 40)){
      state = 0;
      packet[4] = readByte;
      packetReady=1;
      memcpy(&finishedPacket,&packet,5);
      errorCount=0;
    }
  }
  TCCR1B ^= _BV(ICES1);            // toggle bit to trigger on the other edge
}

void setup(void)
{
  Serial.begin(9600);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(RF_PIN, INPUT);
  
  digitalWrite(GREEN_LED, LOW);
  

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

  if (!sensors.getAddress(Tin, 0)) Serial.println("Unable to find address for Device 0"); 

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(Tin, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(Tin), DEC); 
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
  
  initRF();
}

void initRF() {
  pinMode(RF_PIN, INPUT); // ICP pin (digital pin 8 on Arduino) as input
//  PORTB &= ~(1<<PORTB0);
  
  TCCR1A = 0 ;                    // Normal counting mode
  TCCR1B = prescaleBits ;         // set prescale bits

  TCCR1B |= _BV(ICES1);           // enable input capture
//  TCCR1B |= _BV(ICNC1);
  bitSet(TIMSK1,ICIE1);           // enable input capture interrupt for timer 1 
  bitSet(TIMSK1,TOIE1);           // enable overflow interrupt to detect longer pulses
}

float getT(byte h, byte l){
  // converts temperature expressed in BCD as used in la crosse sensors
  // for example h=0x70 l=0x37  returns  20.3 
  float dec =  ((float)(l >> 4)) /10;
  float value = (h >> 4) *10 ;// isolate 4 MSB (tens)
  value = value + (h & 0x0f); // isolate 4 LSB (units)
  value = (value + dec ) -50; // substract 50
  return value;
}

float getH(byte h, byte l){
  // converts humidity expressed in BCD as used in la crosse sensors
  // for example h=0x33 l=0x03  returns  33.0 
  float dec =  ((float)(l >> 4)) /10;
  float value = (h >> 4) *10 ;// isolate 4 MSB (tens)
  value = value + (h & 0x0f); // isolate 4 LSB (units)
  value = (value + dec );
  return value;
}

boolean isValid(byte *p){
  byte s=0;
  byte old = p[4] & 0x0f;
  for (byte i=0; i<4; i++){
    s = s + (p[i]>>4) + (p[i] & 0x0f);
  }
  s = s + (p[4]>>4);
  s = s & 0x0f;
  byte hh = p[2];
  byte ll = (p[3]<<4) | (p[4]>>4);
  if ((s==old) && (hh == ll))
    return 1;
  else
    return 0;
}

void printT(char *s, float t){
  Serial.print(s);
  Serial.print(t);
  Serial.print(" C / ");
  Serial.print(TO_F(t));
  Serial.println(" F");
}

void loop() 
{
  delay(1000);
  if ((packetReady > 0) && (isValid(finishedPacket) == 1)){
#ifdef DEBUG
    for( byte i=0; i < 5; i++)
    {
      Serial.print(finishedPacket[i],HEX);
      Serial.print(" ");
    }
    Serial.println("");
#endif
    if (finishedPacket[0] == 0xA0){
      if (LC1 == (finishedPacket[1] &0xfe))
          s[TOUT] = getT(finishedPacket[2],finishedPacket[3]);
    }else if (finishedPacket[0] == 0xAE){
      s[HOUT] = getH(finishedPacket[2],finishedPacket[3]);
    }
    packetReady=0;
  }
  if (millis() - lastConnectionTime > postingInterval) {
    digitalWrite(GREEN_LED, HIGH);
    sensors.requestTemperatures(); // Send the command to get temperatures
    delay(1000);
    s[TIN] = sensors.getTempC(Tin);
    sendData(s, NBSENSORS);
#ifdef DEBUG
      printT("Temperature In:",s[TIN]);
      printT("Temperature Out:",s[TOUT]);
      Serial.print("Humidity :");
      Serial.println(s[HOUT]);
#endif
    lastConnectionTime = millis();
    digitalWrite(GREEN_LED, LOW);
  }
  

}

// this method makes a HTTP connection to the server:
void sendData(float s[], int nbvalues) {

  for (int i = 0; i< nbvalues; i++)
      sensor_ascii[0]="";
  int payload_length = 0;
  Serial.println("Sending ");

  for (int i = 0; i< nbvalues; i++) {
     if (s[i] < 150){ 
       dtostrf(s[i],0,1,buffer);
       sensor_ascii[i] = sensor_labels[i] +","+ String(buffer) + "\r\n";
       payload_length = payload_length + sensor_ascii[i].length();
       Serial.print(sensor_ascii[i]);
     }
  }

  if (client.connect("api.pachube.com", 80)) {

    Serial.println("connecting to pachube...");
    // send the HTTP PUT request. 
    // fill in your feed address here:
    client.print("PUT /v2/feeds/52915 HTTP/1.1\n");
    client.print("Host: api.pachube.com\n");
    client.print("X-PachubeApiKey: AIVLMfLavxYz9-pQ7MNyPlIu8liSAKw0VThoV25wZ0xpaz0g\n");
    
    client.print("Content-Length: ");

    client.println(payload_length, DEC); 

    // last pieces of the HTTP PUT request:
    client.print("Content-Type: text/csv\n");
    client.println("Connection: close\n");
   
    for (int i = 0; i< nbvalues; i++) {
       if (s[i] < 150) 
          client.print(sensor_ascii[i]);
    }
    
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

