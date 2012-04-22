
#include "rnxv.h"

RnXv::RnXv(const unsigned int pin) : pin_(pin)
{
   pinMode(pin_, OUTPUT);
   // used to figure out if rnxv is on
   pinMode(8, OUTPUT);
}

void RnXv::power_on(){
  digitalWrite(pin_, HIGH);
  digitalWrite(8, HIGH);
  // wait for RN XV to initialize
  // should monitor GPIO 4 instead
  delay(1000); 
}

void RnXv::power_off(){
  digitalWrite(pin_, LOW);
  digitalWrite(8, LOW);
}

void RnXv::init(Print *serial) {
    serial_ = serial;
}

// enter command mode
bool RnXv::command_mode() {
  delay(RNXV_COMMAND_GUARD);
  serial_->println("$$$");
  delay(RNXV_COMMAND_GUARD);
  // should check ver or some other command
  return true;
}

// connect to server
bool RnXv::connect(const char *server, unsigned int port) {
 
  serial_->print("open ");
  serial_->print(server);
  serial_->print(" ");
  serial_->println(port);
  //should wait to receive "*OPEN*"
  return true;
}

void RnXv::disconnect(){
  serial_->println("close");
}

size_t RnXv::write(uint8_t c){
  return serial_->write(c);
}
