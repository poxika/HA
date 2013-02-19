#include "arduino.h"
#include "relays.h"

int relayPin[4] = {4,5,6,7};

Relays::Relays()
{
  for(int i = 0; i < 4; i++)
  {
    pinMode(relayPin[i],OUTPUT);
  }
}

void Relays::on(int relay)
{
  digitalWrite(relayPin[relay-1],LOW);
}

void Relays::off(int relay)
{
  digitalWrite(relayPin[relay-1],HIGH);
}

void Relays::allOff()
{
  for(int i = 1; i < 5; i++)
  {
    off(i);
  }
}

void Relays::allOn()
{
  for(int i = 1; i < 5; i++)
  {
    on(i);
  }
}

void Relays::sequence(long d, int seq[]){
  allOff();
  for (int i =0; i < 4; i++)
  {
    on(seq[i]);
    delay(d);
  }
}
