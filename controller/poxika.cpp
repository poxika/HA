//inspired by http://code.google.com/p/pachubelibrary/
//use a proxy on google app engine to connect to pachube/cosm in a reliable way

#include "poxika.h"

char send_buffer[150];

Poxika::Poxika(const char *feed, const char *key) : key_(key), feed_(feed)
{}

void Poxika::init(Stream *serial){
  serial_ = serial;
  nb_ = 0;
}


int Poxika::add(char *id, char *b){
  if (nb_ < MAX_DATASTREAMS_NUM){
    int index = nb_;
    streams_[index].id = id;
    streams_[index].value = b;
    nb_ += 1;
    return (index);
  } else {
    return -1;
  }
}

int Poxika::send(const char *host){
  int r = 0;
  send_buffer[0]='\0';
  for (int i=0; i<nb_; i++){
    strcat(send_buffer,streams_[i].id);
    strcat(send_buffer,","); 
    strcat(send_buffer,streams_[i].value);
    strcat(send_buffer,"\r\n"); 
  }

  serial_->print("PUT /v2/feeds/");
  serial_->print(feed_);
  serial_->println(" HTTP/1.1");
  serial_->print("Host: ");
  serial_->println(host);
  
  serial_->print("X-ApiKey: ");
  serial_->println(key_);

  serial_->print("Content-Length: ");
  serial_->println(strlen(send_buffer), DEC); 

  serial_->print("Content-Type: text/csv\n");
  serial_->println("Connection: close\n");
  serial_->print(send_buffer);

  serial_->setTimeout(1000L);
  delay(1000);
  if (serial_->find("HTTP/1.1 ")) {
    r = serial_->parseInt();
  }
  return r;
}

long Poxika::getTime(const char *host){

  serial_->println("GET / HTTP/1.1");
  serial_->print("Host: ");
  serial_->println(host);
  serial_->println();
  
  Serial.println("Reading response");
  unsigned long x = 0L;
  serial_->setTimeout(2000L);
  Serial.println(serial_->available());
  delay(1000);
  if (serial_->find("\r\n")) {
    Serial.println(serial_->available());
    x = serial_->parseInt();
    Serial.println(x);
  } else {
    Serial.println("Cannot find return code");
  }
  return x;
}

bool Poxika::update(unsigned int id, float f){
  if (id >= 0  && id < nb_ ){
     dtostrf(f,0,1,streams_[id].value);
     return true;
  }else
    return false;
}
bool Poxika::update(unsigned int id, int i){
  if (id >= 0  && id < nb_ ){
     itoa(i,streams_[id].value,10);
     return true;
  }else
    return false;
}
bool Poxika::update(unsigned int id, unsigned int i){
  if (id >= 0  && id < nb_ ){
     itoa(i,streams_[id].value,10);
     return true;
  }else
    return false;
}
size_t Poxika::write(uint8_t c){
  return serial_->write(c);
}
