//inspired by http://code.google.com/p/pachubelibrary/

#include "pachube.h"

char send_buffer[100];

Pachube::Pachube(const char *feed, const char *key) : key_(key), feed_(feed)
{}

void Pachube::init(Print *serial){
  serial_ = serial;
}


int Pachube::add_stream(char *id, char *b){
  if (nb_ < MAX_DATASTREAMS_NUM){
    streams_[nb_].id = id;
    streams_[nb_].value = b;
    return (nb_ ++);
  } else {
    return -1;
  }
}

void Pachube::send(){
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
  serial_->println("Host: api.cosm.com");
  
  serial_->print("X-PachubeApiKey: ");
  serial_->println(key_);

  serial_->print("Content-Length: ");
  serial_->println(strlen(send_buffer), DEC); 

  serial_->print("Content-Type: text/csv\n");
  serial_->println("Connection: close\n");
  serial_->print(send_buffer);

}

bool Pachube::update_stream(unsigned int id, float f){
  if (id >= 0  && id < nb_ ){
     dtostrf(f,0,1,streams_[id].value);
     return true;
  }else
    return false;
}
size_t Pachube::write(uint8_t c){
  return serial_->write(c);
}
