#ifndef rnxv_h
#define rnxv_h
#include "Arduino.h"

#define RNXV_COMMAND_GUARD  250

class RnXv : public Print
{
  private:
    Print *serial_;
    const int pin_;
  public:
    RnXv(const unsigned int pin);
    void init(Print *serial);
    bool connect(const char* host, unsigned int port);
    void disconnect();
    bool command_mode();
    void power_on();
    void power_off();
    size_t write(uint8_t c);
};
#endif
