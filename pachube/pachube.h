#ifndef pachube_h
#define pachube_h
#include "Arduino.h"

#define MAX_DATASTREAMS_NUM  3

struct data
{
//  public:
    char *id;
    char *value;
};

class Pachube : public Print {
  private:
    Print *serial_;
    String feed_;
    String key_;
    data streams_[MAX_DATASTREAMS_NUM];
    int nb_;
    
  public:
    Pachube(const char *feed, const char *key);
    void init(Print *serial);
    int add_stream(char *id, char *buff);
    bool update_stream(unsigned int id, float f);
    void send();
    size_t write(uint8_t c);
};

#endif
