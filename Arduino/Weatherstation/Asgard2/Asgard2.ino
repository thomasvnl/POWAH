#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>
/*
This is the corresponding sketch to the 'basicSend' sketch.
the nrf24l01 will listen for numbers 0-255, and light the red LED
whenever a number in the sequence is missed.  Otherwise,
it lights the green LED
*/
int msg[1];
RF24 radio(3,4);
const uint64_t pipe = 0xE8E8F0F0E1LL;
String theMessage;
 
void setup(void){
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1,pipe);
  radio.startListening();
}
 
void loop(void)
{

  if (radio.available())
  {
    bool done = false;  
      
    //while (!done)
    //{
      done = radio.read(msg, 1);
      char theChar = msg[0];
      if (msg[0] != 2)
      {
        theMessage.concat(theChar);
      }    
      else 
      {
      Serial.println(theMessage);
      theMessage = "";
      }
     //}
   }
}
