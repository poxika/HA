#ifndef relays_h
#define relays_h
#include "Arduino.h"

// Relays are connected 
class Relays {
public:
    Relays();
    void allOn();
    void allOff();
    void on(int relay);
    void off(int relay);
    void sequence(long d, int seq[]);
};
#endif
