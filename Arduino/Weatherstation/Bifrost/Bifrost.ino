#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <LowPower.h>
#include <DHT.h>

#define ALTITUDE 0.1
#define DHTPIN 2
#define DHTTYPE DHT11

RF24 radio(3,4);
const uint64_t pipe = 0xE8E8F0F0E1LL;
SFE_BMP180 pressure;
DHT dht(DHTPIN,DHTTYPE);

int sleepCount = 0;

void setup(void)
{
	Serial.begin(9600);
	radio.begin();
	radio.openWritingPipe(pipe);	
	if (pressure.begin())
	{
	Serial.println("BMP180 init success");
	}
	else
	{
		Serial.println("BMP180 init fail\n\n");
    	while(1); // Pause forever.
	}
delay(5000);
radio.powerDown();
dht.begin();
delay(2000);
}

void loop(void)
{
  //Sleep for 8 sec with ADC and BOD off
  LowPower.powerDown(SLEEP_8S,ADC_OFF,BOD_OFF);
  sleepCount++;
  if (sleepCount == 37)
  {
  	String theMessage = baro();
  	theMessage = theMessage + tempsens();
	sendRadio(theMessage);
  	theMessage = "";
  	sleepCount = 0;
  }
}

String baro()
{
	char status;
	double T,P,p0,a;
	char buf[32];
	double checkD = (ALTITUDE,0);
	dtostrf(checkD,2,2,buf);
	String checkS = String(buf);

	String tempMessage = "Provided altitude: " + checkS + " meters \n";

	status = pressure.startTemperature();
	if (status != 0)
	{
    // Wait for the measurement to complete:
    	delay(status);

    	status = pressure.getTemperature(T);
    	if (status != 0)
    	{
      // Print out the measurement:      
      		checkD = T,2;
      		dtostrf(checkD,2,2,buf);
      		checkS = String(buf);
      
      		tempMessage = tempMessage + "Temperature: " + checkS + " deg C \n";

      // Start a pressure measurement:

      		status = pressure.startPressure(3);
      		if (status != 0)
      		{
        // Wait for the measurement to complete:
        		delay(status);

        // Retrieve the completed pressure measurement:

        		status = pressure.getPressure(P,T);
        		if (status != 0)
        		{
          // Print out the measurement:
          			checkD = P,2;
          			dtostrf(checkD,2,2,buf);
          			checkS = String(buf);

          			tempMessage = tempMessage + "Absolute pressure: " + checkS + " mb \n";

          			p0 = pressure.sealevel(P,ALTITUDE);
          
          			checkD = p0,2;
          			dtostrf(checkD,2,2,buf);
          			checkS = String(buf);

          			tempMessage = tempMessage + "Relative (sea-level) pressure : " + checkS + " mb \n";

          			a = pressure.altitude(P,p0);

          			checkD = a,2;
          			dtostrf(checkD,2,2,buf);
          			checkS = String(buf);

          			tempMessage = tempMessage + "Computed altitude : " + checkS + " meters \n";
          			return tempMessage;
  				}
  				else
  				{ 
  					Serial.println("error retrieving pressure measurement\n");
  				}
			}
			else 
			{
				Serial.println("error starting pressure measurement\n");
			}
		}
		else
		{
			Serial.println("error retrieving temperature measurement\n");
		}
	}
	else 
	{
		Serial.println("error starting temperature measurement\n");
	}
}

String tempsens()
{
	char buf[8];
	double hum;
	double temp;
	String theMessage;

	hum = dht.readHumidity();
	temp = dht.readTemperature();

	dtostrf(hum,2,2,buf);
	theMessage = "Humidity : " + String(buf) + "%";

	dtostrf(temp,2,2,buf);
	theMessage = theMessage + " Temperature: " + String(buf) + " C \n";
	return theMessage;
}

void sendRadio(String message)
{	
	int msg[1];
	int messageSize = message.length();
	radio.powerUp();
	delay(500);
	for (int i = 0; i < messageSize; i++)
	{
		int charToSend[1];
		charToSend[0] = message.charAt(i);
		radio.write(charToSend,1);
	}
	msg[0] =2;
	radio.write(msg,1);
	radio.powerDown();
}


