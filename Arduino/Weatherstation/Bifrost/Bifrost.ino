/*!
  Bifrost.ino
  Code for the Bifrost Weather Station,
  a RF enabled, solar charged weather station
  generating it's own power and storing it
  inside a P18650 battery unit.
 
  \version 0.3
  \package POWAH
  \authors Charlie Nederhoed, Thomas Verschoof
 */

//! Arduino Core Libraries
#include <SPI.h>
#include <Wire.h>

//! Third Party Libraries
#include <DHT.h>
#include "LowPower.h"
#include "RF24.h"
#include "RF24_config.h"
#include "nRF24L01.h"
#include "SFE_BMP180.h"

//! Definitions
#define DEBUG // (Un)comment to toggle Debugging

#define ALTITUDE 0.1 // Altitude at location
#define DHT_PIN 2

/*! Global objects and variables */	

//! Radio
RF24 radio(3,4); // CE = 3, CSN = 4
const uint64_t pipe = 0xE8E8F0F0E1LL;
const int messageSize = 64;

//! DHT Sensor
DHT dht;

//! BMP180 Sensor
SFE_BMP180 bmp180;

//! Variables
int sleepCounter 	= 0;
int intervalSeconds	= 296; /*! in seconds */
int interval		= (int) ( intervalSeconds / 8.0 );
int bmpOversampling = 3; // 0 - 3 for oversampling in the BMP180

/*!
	Measurements,
	return -999.0 if not measured / failing
*/
double humidity 	= -999.0;
double dhtTemp		= -999.0; //! Not used yet
double bmpTemp		= -999.0;
double mbarAbs		= -999.0;
double mbarRel		= -999.0;
double altitude 	= -999.0;

/*!
	Power down the radio unit (RF24) to conserve energy
	\param sleepBefore an integer that sets the delay before calling powerDown
	\param sleepAfter an integer that sets the delay after calling powerDown
*/
void radioOff( int sleepBefore = 500, int sleepAfter = 4500 )
{
	#ifdef DEBUG
		Serial.println( "TURNING OFF RADIO" );
	#endif

	delay( sleepBefore );
	radio.powerDown();
	delay( sleepAfter );
}

//! Power up the radio unit
void radioOn()
{
	#ifdef DEBUG
		Serial.println( "TURNING ON RADIO" );
	#endif

	radio.powerUp();
	delay(500); /*! Always delay half a second, to init RF24 */
}

/*!
	Put the Arduino into low power mode (sleep 8 seconds),
	also increment the global int sleepCounter with 1
*/
void sleep8s()
{
	#ifdef DEBUG
		Serial.println( "TURNING ARDUINO INTO LOW POWER MODE" );
		delay(50);
	#endif 

	LowPower.powerDown( SLEEP_8S, ADC_OFF, BOD_OFF );
	sleepCounter++;
}

/*!
	Check if the interval expired
	\returns boolean
*/
bool intervalExpired( void )
{
	#ifdef DEBUG
		Serial.println( "CHECK IF INTERVAL DISABLED" );
	#endif
	if( sleepCounter >= interval )
	{
		resetCounter();
		return true;
	}
	return false;
}

//! Reset the sleep counter
void resetCounter( void )
{
	#ifdef DEBUG
		Serial.println( "RESET COUNTER" );
	#endif
	sleepCounter = 0;
}

//! DHT Sensor do all measurements
void dhtDoMeasurement( void )
{
	#ifdef DEBUG
		Serial.println( "START DHT MEASUREMENTS" );
	#endif
	dhtMeasureTemp();
	dhtMeasureHumidity();
}

//! DHT Sensor measure humidity, save to global
double dhtMeasureHumidity( void )
{
	#ifdef DEBUG
		Serial.println( "MEASURE HUMIDITY" );
	#endif
	humidity = dht.getHumidity();
	return humidity;
}

//! DHT Sensor measure temperature, save to global
double dhtMeasureTemp( void )
{
	#ifdef DEBUG
		Serial.println( "MEASURE DHT TEMP" );
	#endif
	dhtTemp = dht.getTemperature();
	return dhtTemp;
}

//! BMP Sensor do all all measurement
void bmpDoMeasurement( void )
{
	#ifdef DEBUG
		Serial.println( "STARTING BMP MEASUREMENTS" );
	#endif
	bmpMeasureTemp();
	bmpMeasureMBarAbs();
	bmpMeasureMBarRel();
	bmpMeasureAltitude();
}

//! BMP Sensor measure temperature
double bmpMeasureTemp( void )
{
	#ifdef DEBUG
		Serial.println( "MEASURE BMP TEMP" );
	#endif
	char status;
	
	status = bmp180.startTemperature();
	if( status != 0 )
	{
		//! Wait for measurement to complete
		delay( status );
		status = bmp180.getTemperature( bmpTemp );

		if( status != 0 )
		{
			return bmpTemp;
		}
	}

	return -999.0;
}

//! BMP Sensor measure abs mbar
double bmpMeasureMBarAbs()
{
	#ifdef DEBUG
		Serial.println( "MEASURE MBAR ABS" );
	#endif

	char status;

	status = bmp180.startPressure( bmpOversampling );
	if( status != 0 )
	{
		//! Wait for measurement to complete
		delay( status );
		status = bmp180.getPressure( mbarAbs, bmpTemp );
		if( status != 0 )
		{
			return mbarAbs;
		}
	}

	return -999.0;
}

//! BMP Sensor measure rel mbar
double bmpMeasureMBarRel()
{
	#ifdef DEBUG
		Serial.println( "MEASURE MBAR REL" );
	#endif
	mbarRel = bmp180.sealevel( mbarAbs, ALTITUDE );
	return mbarRel;
}

//! BMP Sensor measure computed altitude
double bmpMeasureAltitude()
{
	#ifdef DEBUG
		Serial.println( "MEASURE ALTITUDE" );
	#endif

	altitude = bmp180.altitude( mbarAbs, mbarRel );
	return altitude;
}

/*!
	Builds the message in the needed format by using the global
	variables
	\returns String a string containing the whole message
*/
String buildMessage( )
{
	#ifdef DEBUG
		Serial.println( "BUILDING MSG" );
	#endif
	char buf[ messageSize ];
	sprintf( buf, "hum:%3.2f;temp0:%3.2f;temp1:%3.2f;mbarAbs:%4.2f;mbarRel:%4.2f;alt:%4.2f", humidity, dhtTemp, bmpTemp, mbarAbs, mbarRel, altitude );
	String msg = buf;
	return msg;
}

/*!
	Powers the RF24 module, sends the message,
	and powers down the module again.
	\param buffer containing the message( 64bytes )
*/
void broadcastMessage( String msg )
{
	#ifdef DEBUG
		Serial.println( "STARTING BROADCAST" );
	#endif
	//! Power up the radio
	radioOn();

	//! Iterate through buffer until ending 0 found
	for( int i = 0; i < msg.length(); i++ )
	{
		int msgChar[1] = { msg.charAt(i) };
		radio.write( msgChar, 1 );
	}

	//! End Broadcast
	int endBroadcast[1];
	endBroadcast[0] = 2;
	radio.write( endBroadcast , 1 ); 

	//! Power down the radio
	radioOff();

	#ifdef DEBUG
		Serial.println( "FINISHED BROADCAST" );
	#endif
}


//! Setup before Loop
void setup( void )
{
	#ifdef DEBUG
		Serial.begin( 9600 );
		/*!
			WARNING

			If DEBUG is defined but no serial connection is opened,
			this part will loop forever until said connection is opened!
		*/
		while( !Serial ){ ; } //! Loop until serial connection is opened

		Serial.println( "RUNNING SETUP" );
	#endif

	//! Init the BMP180 Sensor
	#ifdef DEBUG
		if( bmp180.begin() ) Serial.println( "BMP180 INITIALIZED" );
		else
		{
			Serial.println( "BMP180 INITIALIZATION FAILED" );
			while( 1 ){ ; } //! Loop until reset
		}
	#else
		bmp180.begin();
	#endif

	//! Init the DHT Sensor
	dht.setup( DHT_PIN );

	//! Init the Radio
	radio.begin();
	radio.openWritingPipe( pipe );

	// Power down the radio
	radioOff();

	#ifdef DEBUG
		Serial.println( "END OF SETUP" );
	#endif
}

//! Arduino loop method (equivalent to while(1){ METHODS } )
void loop( void )
{
	if( intervalExpired() )
	{
		#ifdef DEBUG 
			Serial.println( "INTERVAL EXPIRED" );
		#endif
		//! Disabled until DHT Sensor connected
		//dhtDoMeasurement(); 
		bmpDoMeasurement();
		broadcastMessage( buildMessage() );
	}
	sleep8s();
}

