/*
  Decodes the transmission from La Crosse TX7U
  
  see http://www.f6fbb.org/domo/sensors/tx3_th.php
  and http://kjordahl.net/weather.html

*/
//#define DEBUG 1

const int inputCapturePin = 8;
const int greenLedPin     = 6;
const int redLedPin       = 7;

#define SMALLEST_ONE_PULSE    450
#define LARGEST_ONE_PULSE     600

#define SMALLEST_ZERO_PULSE  1200
#define LARGEST_ZERO_PULSE   1500
 
#define IS_ZERO(l)  ((l > SMALLEST_ZERO_PULSE ) && (l < LARGEST_ZERO_PULSE))
#define IS_ONE(l)   ((l > SMALLEST_ONE_PULSE) && (l <  LARGEST_ONE_PULSE))



const int prescale = 8;            // prescale factor (each tick 0.5 us @16MHz)
const byte prescaleBits = B010;    // tick is 0.5 us overflows in 65.536 ms
// calculate time per counter tick in ns
const long  precision = (1000000/(F_CPU/1000)) * prescale  ;   

#ifdef DEBUG
// To store pulse metrics
const int numberOfEntries  = 128;    // the max number of pulses to measure
volatile byte index = 0;             // index to the stored readings
volatile unsigned int results[numberOfEntries]; // note this is 16 bit value
#endif

volatile unsigned int timeValue;
volatile unsigned int packetReady = 0;
volatile unsigned int overflows;
volatile unsigned int readByte;
volatile unsigned int state;
volatile unsigned int errorCount;
byte packet[5];
byte finishedPacket[5];
volatile int bitCount = 0;

// Statistics
volatile int minZero = LARGEST_ZERO_PULSE ;
volatile int maxZero;
volatile int minOne = LARGEST_ONE_PULSE ;
volatile int maxOne;


// temperature
#define TO_F(t) (t*9/5 + 32)

/* Overflow interrupt vector */
ISR(TIMER1_OVF_vect){                 // here if no input pulse detected
   overflows++;                       // incriment overflow count  
}

/* ICR interrupt vector */
ISR(TIMER1_CAPT_vect)
{
  volatile long pulseLength;
  TCNT1 = 0;                            // reset the counter
  timeValue = ICR1;                     // grab the timer value
  if(bitRead(TCCR1B ,ICES1) == false)   // falling edge detected
  {
    int val;
    pulseLength = (timeValue * precision)/1000;
    if IS_ZERO(pulseLength) { // Got a zero
       readByte = readByte << 1;
       if (pulseLength < minZero)
         minZero = pulseLength;
       if (pulseLength > maxZero)
         maxZero = pulseLength;   
       bitCount ++;
    } else if IS_ONE(pulseLength) { // got one
      readByte = readByte << 1;
      readByte = readByte | 1;
      if (pulseLength < minOne)
        minOne = pulseLength;
      if (pulseLength > maxOne)
        maxOne = pulseLength;   
      bitCount ++;
    } else { // drop 
      readByte = 0;
      if (state > 0)
        digitalWrite(redLedPin, HIGH);
      state = 0; // drop packet
      bitCount = 0;
    }

    if (((readByte == 0xA0) || (readByte == 0xAE)) && (state == 0)){ //got first byte
      packet[0] = readByte;
      state = 1;
      bitCount = 8;
      digitalWrite(redLedPin, LOW);
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
      digitalWrite(greenLedPin, HIGH);
      memcpy(&finishedPacket,&packet,5);
      errorCount=0;
    }
    #ifdef DEBUG
    if(index < numberOfEntries)
    {      
       results[index] = pulseLength ;
       index++;
    } 
    #endif
  }

  TCCR1B ^= _BV(ICES1);            // toggle bit to trigger on the other edge
}

void setup() {
  Serial.begin(9600);
  pinMode(greenLedPin, OUTPUT); 
  pinMode(redLedPin, OUTPUT); 
  pinMode(inputCapturePin, INPUT); // ICP pin (digital pin 8 on Arduino) as input

  TCCR1A = 0 ;                    // Normal counting mode
  TCCR1B = prescaleBits ;         // set prescale bits
  TCCR1B |= _BV(ICES1);           // enable input capture
  bitSet(TIMSK1,ICIE1);           // enable input capture interrupt for timer 1 
  bitSet(TIMSK1,TOIE1);           // enable overflow interrupt to detect missing input pulses
  
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

void loop() 
{
  delay(1000);
  float value = 0;
  if(packetReady > 0 )
  {
    for( byte i=0; i < 5; i++)
    {
      Serial.print(finishedPacket[i],HEX);
      Serial.print(" ");
    }
    Serial.println("");
    if (finishedPacket[0] == 0xA0){
      value = getT(finishedPacket[2],finishedPacket[3]);
      Serial.print("Temperature :");
      Serial.print(value);
      Serial.print(" C / ");
      Serial.print(TO_F(value));
      Serial.println(" F");
    } 
    if (finishedPacket[0] == 0xAE){
      value = getH(finishedPacket[2],finishedPacket[3]);
     Serial.print("Humidity :");
     Serial.println(value);
    } 
    packetReady=0;
    digitalWrite(greenLedPin, LOW);
  }
  #ifdef DEBUG
  Serial.print("Min 0: ");
  Serial.print(minZero);
  Serial.print(" Max 0: ");
  Serial.print(maxZero);
    Serial.print(" Min 1: ");
  Serial.print(minOne);
  Serial.print(" Max 1: ");
  Serial.println(maxOne);
  if (index > 0){
    for( byte i=0; i < index; i++)
    {
       Serial.print(results[i]);
       if IS_ZERO(results[i]) {
         Serial.println("  - 0");
       } else if IS_ONE(results[i]) {
         Serial.println("  - 1");
       } else
         Serial.println("");
    }
    index = 0;
    overflows = 0;
  }
  #endif
}
