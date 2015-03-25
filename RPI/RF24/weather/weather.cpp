#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include "/home/pi/rf24libs/RF24/RF24.h"
 
using namespace std;
 
// spi device, spi speed, ce gpio pin
RF24 radio(RPI_V2_GPIO_P1_22,BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);

void setup(void)
{
    // init radio for reading
    radio.begin();
    radio.openReadingPipe(1,0xE8E8F0F0E1LL);
    radio.startListening();
    cout << "setup";
}
 
void loop(void)
{
 
  if (radio.available())
  {
    std::string theMessage;
    int msg[1];
      
    time_t rawtime;
    time (&rawtime);
    radio.read(msg, 1);
    char theChar = msg[0];
    if (msg[0] != 2)
    {
      theMessage += theChar;
    }    
    else 
    {
      ofstream myfile;
      myfile.open ("/home/pi/weather/rf24weather.txt");
      myfile << theMessage << ':' << rawtime << endl;
      myfile.close();
      theMessage = "";
    }
  }
}
 
int main(int argc, char** argv) 
{
    setup();
    cout << "main";
    while(1)
        loop();
 
    return 0;
}