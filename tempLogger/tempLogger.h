
#define SMALLEST_ONE_PULSE    400
#define LARGEST_ONE_PULSE     700

#define SMALLEST_ZERO_PULSE  1000
#define LARGEST_ZERO_PULSE   1500
 
#define IS_ZERO(l)  ((l > SMALLEST_ZERO_PULSE ) && (l < LARGEST_ZERO_PULSE))
#define IS_ONE(l)   ((l > SMALLEST_ONE_PULSE) && (l <  LARGEST_ONE_PULSE))


float lastInT;
float lastOutT;
float lastOutH;
char buffer[15]; // 15 bytes should be ok
#define NBSENSORS 3
#define TIN  0
#define TOUT  1
#define HOUT  2
String sensor_ascii[3] = {"12345678910","12345678910","12345678910"}; // only 3 sensors.
String sensor_labels[NBSENSORS] = { "t1","t2","h"};
float s[NBSENSORS]={150,150,150}; // initialize with dummy value

const int prescale = 8;            // prescale factor (each tick 0.5 us @16MHz)
const byte prescaleBits = B010;    // tick is 0.5 us overflows in 65.536 ms
// calculate time per counter tick in ns
const long  precision = (1000000/(F_CPU/1000)) * prescale  ;   

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

