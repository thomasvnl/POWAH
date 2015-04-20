#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <RF24.h>
#include <string>
 
using namespace std;
 
// spi device, spi speed, ce gpio pin
RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
std::string theMessage;

void setup(void)
{
  printf("\nPreparing interface\n");
    cout << "setup" << endl;
    // init radio for reading
    radio.begin();
    cout << "begin done" << endl;
    radio.setChannel(76);
    radio.openReadingPipe(1,0xE8E8F0F0E1LL);
    cout << "pipe opened" << endl;
    radio.startListening();
    cout << "out setup" << endl;
}
 
void loop(void)
{
  //cout << "in loop" << endl;
  if (radio.available())
  {
    int msg[1];
    radio.read(msg, 1);
    char theChar = msg[0];    
    //cout << theCheck << endl;
    if (theChar != '#')
    {
      //cout << theChar << endl;
      theMessage += theChar;
      //cout << theMessage << endl;
    }    
    else 
    {
      time_t t = time(0);
      struct tm * now = localtime( & t );
      cout << "start streaming" << endl;
      std::ofstream myfile;
      myfile.open ("/home/pi/weather/rf24weather.txt", std::ios_base::app);
      myfile << theMessage << " < > " << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << " -- " << now->tm_mday << '-' << (now->tm_mon + 1) << '-'<< (now->tm_year + 1900) << "\r\n";
      cout << theMessage << " < > " << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << " -- " << now->tm_mday << '-' << (now->tm_mon + 1) << '-'<< (now->tm_year + 1900) << endl;
      myfile.close();
      theMessage = "";
    }
  }
}
 
int main(int argc, char** argv) 
{
    cout << "Going in to setup" << endl;
    setup();
    cout << "main" << endl;
    while(1)
        loop();
 
    return 0;
}