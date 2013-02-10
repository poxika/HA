#ifndef poxika_h
#define poxika_h
#include "Arduino.h"

#define MAX_DATASTREAMS_NUM  3

struct data
{
  public:
    char *id;
    char *value;
};

class Poxika : public Stream {
  private:
    Stream *serial_;
    String feed_;
    String key_;
    data streams_[MAX_DATASTREAMS_NUM];
    int nb_;
    
  public:
    Poxika(const char *feed, const char *key);
    void init(Stream *serial);
    int add(char *id, char *buff);
    bool update(unsigned int id, float f);
    int send(const char *host);
    long getTime(const char *host);
    virtual size_t write(uint8_t c);
    virtual int available() { return serial_->available(); };
    virtual int read() { return serial_->read(); };
    virtual int peek() { return serial_->peek(); };
    virtual void flush() { return serial_->flush(); };
};

#endif
