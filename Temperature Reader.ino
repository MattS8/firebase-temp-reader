/*
 Name:		Temperature_Reader.ino
 Created:	7/2/2022
 Author:	Matthew Steinhardt
*/

#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <FirebaseFS.h>
#include <Firebase.h>
#include "RTDBHelper.h"
#include "TokenHelper.h"
#include "Credentials.h"

/*** -------- Firebase Connectivity Variables -------- ***/
WiFiManager gWifiManager;
FirebaseData gSendData;
FirebaseAuth gAuth;
FirebaseConfig gFirebaseConfig;
std::string gDevicePath = "systems/solar_temp_reader";

/*** -------- Temperature Reader Variables -------- ***/
#define DHTPIN 4     // GPIO4 Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 3 (on the right) of the sensor to GROUND (if your sensor has 3 pins)
// Connect pin 4 (on the right) of the sensor to GROUND and leave the pin 3 EMPTY (if your sensor has 4 pins)
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Time Between info updates
const double TRANSMISSION_INTRERVAL = 10 * 1000 * 1000;

void connectToWiFi()
{
	gWifiManager.autoConnect("Temp-Reader");
}

void setupFirebase()
{
	gFirebaseConfig.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
	gFirebaseConfig.database_url = FIREBASE_DB_URL;
	gFirebaseConfig.api_key = FIREBASE_API;

	gAuth.user.email = FIREBASE_USER_EMAIL;
	gAuth.user.password = FIREBASE_USER_PASS;

	Firebase.reconnectWiFi(true);
	Firebase.setDoubleDigits(5);

	Firebase.begin(&gFirebaseConfig, &gAuth);
	Firebase.reconnectWiFi(true);
}

void setupDHTSensor()
{
	dht.begin();
}

void setup() {
	Serial.begin(115200);

	connectToWiFi();

	setupFirebase();

	setupDHTSensor();
}

void sendTemps(float temperature, float humidity, float heatIndex)
{
	FirebaseJson json;
	json.set("Timestamp/.sv", "timestamp");
	json.set("Temperature", temperature);
	json.set("Humidity", humidity);
	json.set("Heat_Index", heatIndex);
	if (!Firebase.RTDB.pushJSON(&gSendData, "systems/solar_temp_reader/readings", &json))
	{
		Serial.printf("Error sending temp readings: %s\n", gSendData.errorReason().c_str());
	}
	
}

// the loop function runs over and over again until power down or reset
void loop() {
	static unsigned long nextTransmission = 0;

	if (millis() > nextTransmission)
	{
		// Read temperature, humidity, and heat index
		// NOTE: Reading temperature or humidity takes about 250 milliseconds!
		//	Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float humidity = dht.readHumidity();
		float temperature = dht.readTemperature(true);

		if (isnan(humidity) || isnan(temperature))
		{
			Serial.println("Failed to read from DHT sensor!");
			return;
		}

		float heatIndex = dht.computeHeatIndex(temperature, humidity);
		sendTemps(temperature, humidity, heatIndex);
		nextTransmission = millis() + TRANSMISSION_INTRERVAL;
	}
}
