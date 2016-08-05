// Adafruit IO REST API access with ESP8266
//
// For use with ESP8266 Arduino from:
//   https://github.com/esp8266/Arduino
//
// Works great with ESP8266 modules like the Adafruit Huzzah ESP:
//  ----> https://www.adafruit.com/product/2471
//
// Written by Tony DiCola for Adafruit Industries.  
// MIT license, all text above must be included in any redistribution.
#include <ESP8266WiFi.h>
#include "Adafruit_IO_Client.h"

#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_9DOF.h>

#include <Adafruit_INA219.h>

// Configure WiFi access point details.
#define WLAN_SSID  "barf"
#define WLAN_PASS  "barf"

// Configure Adafruit IO access.
#define AIO_KEY    "barf"

Adafruit_INA219 ina219;

/* Assign a unique ID to the sensors */
Adafruit_9DOF dof = Adafruit_9DOF();
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);

/* Update this with the correct SLP for accurate altitude measurements */
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;


// Create an ESP8266 WiFiClient class to connect to the AIO server.
WiFiClient client;

// Create an Adafruit IO Client instance.  Notice that this needs to take a
// WiFiClient object as the first parameter, and as the second parameter a
// default Adafruit IO key to use when accessing feeds (however each feed can
// override this default key value if required, see further below).
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

// Finally create instances of Adafruit_IO_Feed objects, one per feed.  Do this
// by calling the getFeed function on the Adafruit_IO_FONA object and passing
// it at least the name of the feed, and optionally a specific AIO key to use
// when accessing the feed (the default is to use the key set on the
// Adafruit_IO_Client class).
Adafruit_IO_Feed testFeed = aio.getFeed("esptestfeed");

// Alternatively to access a feed with a specific key:
//Adafruit_IO_Feed testFeed = aio.getFeed("esptestfeed", "...esptestfeed key...");



struct BarfDataType {
    int timestamp;
    
    float roll;
    float pitch;
    float heading;
    
    float shuntvoltage;
    float busvoltage;
    float current_mA;
    float loadvoltage;
};

void initIMU() {
  if(!accel.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
    while(1);
  }
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
}


void readIMU() {
  sensors_event_t accel_event;
  sensors_event_t mag_event;
  sensors_vec_t   orientation;

  /* Calculate pitch and roll from the raw accelerometer data */
  accel.getEvent(&accel_event);
  if (dof.accelGetOrientation(&accel_event, &orientation))
  {
    /* 'orientation' should have valid .roll and .pitch fields */
    Serial.print(F("Roll: "));
    Serial.print(orientation.roll);
    Serial.print(F("; "));
    Serial.print(F("Pitch: "));
    Serial.print(orientation.pitch);
    Serial.print(F("; "));
  }
  
  /* Calculate the heading using the magnetometer */
  mag.getEvent(&mag_event);
  if (dof.magGetOrientation(SENSOR_AXIS_Z, &mag_event, &orientation))
  {
    /* 'orientation' should have valid .heading data now */
    Serial.print(F("Heading: "));
    Serial.print(orientation.heading);
    Serial.print(F("; "));
  }

  Serial.println(F(""));
}


void readINA219(BarfDataType &sensorReadings) {
  sensorReadings.shuntvoltage = ina219.getShuntVoltage_mV();
  sensorReadings.busvoltage = ina219.getBusVoltage_V();
  sensorReadings.current_mA = ina219.getCurrent_mA();
  sensorReadings.loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltage:   "); Serial.print(sensorReadings.busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(sensorReadings.shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(sensorReadings.loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(sensorReadings.current_mA); Serial.println(" mA");
  Serial.println("");
  
  // Turn off the system when this drops below threshold.
  return busvoltage;
}


void setup() {
  // Setup serial port access.
  Serial.begin(115200);
  delay(10);
  Serial.println(); Serial.println();
  Serial.println(F("Adafruit IO ESP8266 test!"));

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");  
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  
  // Initialize the Adafruit IO client class (not strictly necessary with the
  // client class, but good practice).
  aio.begin();

  uint32_t currentFrequency;
  ina219.begin();

  initIMU();

  pinMode(12, OUTPUT);
    
  Serial.println(F("Ready!"));
}

void loop() {
  // Create local readings data.
  BarfDataType readings;

  barfnoises.timestamp = NTP_or_something();
  digitalWrite(12, HIGH);
  delay(100);
  digitalWrite(12, LOW);
  readINA219(readings);
  readIMU();
  
  // Write the data values, they should match the print statements from the loop.
  Serial.print("^Bus Voltage:   "); Serial.print(readings.busvoltage); Serial.println(" V");
  Serial.print("^Shunt Voltage: "); Serial.print(readings.shuntvoltage); Serial.println(" mV");
  Serial.print("^Load Voltage:  "); Serial.print(readings.loadvoltage); Serial.println(" V");
  Serial.print("^Current:       "); Serial.print(readings.current_mA); Serial.println(" mA");
  Serial.println("");
  
  // To write a value just call the feed's send function and pass it the value.
  // Send will create the feed on Adafruit IO if it doesn't already exist and
  // then add the value to it.  Send returns a boolean that's true if it works
  // and false if it fails for some reason.
  if (testFeed.send(barfer)) {
    Serial.print(F("Wrote value to feed: ")); Serial.println(barfer, DEC);
  }
  else {
    Serial.println(F("Error writing value to feed!"));
  }

  // Now wait 10 seconds and read the current feed value.
  //Serial.println(F("Waiting 10 seconds and then reading the feed value."));
  //delay(15000);

  // To read the latest feed value call the receive function on the feed.
  // The returned object will be a FeedData instance and you can check if it's
  // valid (i.e. was successfully read) by calling isValid(), and then get the
  // value either as a text value, or converted to an int, float, etc.
  FeedData latest = testFeed.receive();
  if (latest.isValid()) {
    Serial.print(F("Received value from feed: ")); Serial.println(latest);
    // By default the received feed data item has a string value, however you
    // can use the following functions to attempt to convert it to a numeric
    // value like an int or float.  Each function returns a boolean that indicates
    // if the conversion succeeded, and takes as a parameter by reference the
    // output value.
    int i;
    if (latest.intValue(&i)) {
      Serial.print(F("Value as an int: ")); Serial.println(i, DEC);
    }
    // Other functions that you can use include:
    //  latest.uintValue() (unsigned int)
    //  latest.longValue() (long)
    //  latest.ulongValue() (unsigned long)
    //  latest.floatValue() (float)
    //  latest.doubleValue() (double)
  }
  else {
    Serial.print(F("Failed to receive the latest feed value!"));
  }

  // Now wait 10 more seconds and repeat.
  //Serial.println(F("Waiting 10 seconds and then writing a new feed value."));
  delay(15000);
}
