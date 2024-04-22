/*
  TheRecieverCode.ino

  This program uses the ArduinoBLE library to set-up an Arduino Nano 33 BLE
  as a peripheral device and specifies a service and a characteristic. Depending
  of the value of the specified characteristic, one of two actuators is controlled,
  indexing up and down.

  This program also uses the NanoBLEFlashPrefs library to save the position of the
  actuators late in the pages of flash space of the Arudino Nano to make small
  space of non-volitle memory so that the actuators do not initialize at a position
  that they are not at when the device is powered on and off.

  This version should be able to connect to ios app to change the indexing distance,
  d, and also change the limits that the stroke can reach

  The circuit:
  - Arduino Nano 33 BLE.
  - 2 Actuonix linear actuators

  THIS IS FOR REVEICER ARDUINO (ACTUATOR)
  this receives a byte of data named gesture (should be only transmitted when the 'gesture' is differnt from the last one)

  NOTES
  - will not compline rn 
  - need to only save the index bounds and the index dist when a button on the phone is pressed.
  - need to finish making chr for the different values that pass into
  - need to figure out what is wrong with the other actuator by seeing if it will even just be the shifter
  - have python script to make uuid - use these so its better
    - ﻿﻿93833b0f-2d0f-482d-8932-1f718712d262
    - 91060507-cc22-45bc-a908-198877f3aafd
    - 38e2d3ec-1a0c-4649-a42e-19e8305764fa
    - 43bbc472-b24a-4984-b93a-08b37347cf71
    - c312850b-d1ad-4e52-9892-22c7d3098eaf
    - 546dc242-907f-42b2-9017-51350811e86b
    - 86bba77d-82e2-4230-a8da-f2c55491f79c
    - 5e3a7a7d-8367-4c00-8465-47ae51833334
    - b257e5e2-8f58-45e2-9d74-b24c6f58d9b3

*/

#include <ArduinoBLE.h>
#include <Servo.h>
#include <NanoBLEFlashPrefs.h>
Servo myServo;
Servo myServo2;
#define PIN_SERVO (5) // change to 5 for it to work
#define PIN_SERVO2 (6) // change to 6 to make work

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";

const char* adjustServiceUuid = "43bbc472-b24a-4984-b93a-08b37347cf71";
const char* upperlimitCharacteristicUuid = "93833b0f-2d0f-482d-8932-1f718712d262";
const char* lowerlimitCharacteristicUuid = "91060507-cc22-45bc-a908-198877f3aafd";
const char* indexCharacteristicUuid = "38e2d3ec-1a0c-4649-a42e-19e8305764fa";


int d = 10;
int lowerlimit = 0;
int upperlimit = 100;
int i = 50;
int i2 = 50;
int VRy = 0;
int gesture = 0;

//struct for flash storage method (stores i, or position of actuator 1)
typedef struct positionStruct {
  uint8_t actuatorpos; // 0-255 avalibile, 1 byte
} positionprefs;
positionprefs prefs;

// struct for flash storage of settings from phone
typedef struct adjustStruct {
  uint8_t upperlimit; // 0-255 avalibile, 1 byte
  uint8_t lowerlimit; // 0-255 avalibile, 1 byte
  uint8_t index;      // 0-255 avalibile, 1 byte
} adjustprefs;
adjustprefs prefs2;

NanoBLEFlashPrefs myFlashPrefs;

BLEService gestureService(deviceServiceUuid);
BLEByteCharacteristic gestureCharacteristic(deviceServiceCharacteristicUuid, BLERead | BLEWrite);


BLEService adjustService(deviceServiceUuid);  // change this to the adjust service uuid
BLEByteCharacteristic upperlimitCharacteristic(upperlimitCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic lowerlimitCharacteristic(lowerlimitCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic indexCharacteristic(indexCharacteristicUuid, BLERead | BLEWrite);


void setup() {
  myServo.attach(PIN_SERVO);
  myServo2.attach(PIN_SERVO2);

  Serial.begin(9600);
  delay(5000);

  Serial.println("Read record...");
  int rc = myFlashPrefs.readPrefs(&prefs, sizeof(prefs));
  if (rc == FDS_SUCCESS)
  {
    Serial.println("Preferences found: ");
    Serial.println(prefs.actuatorpos);
    i = prefs.actuatorpos;
  }
  else
  {
    Serial.print("No preferences found. Return code: ");
    Serial.print(rc);
    Serial.print(", ");
    Serial.println(myFlashPrefs.errorString(rc));
  }

  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setLocalName("Arduino Nano 33 BLE (Peripheral)");
  // shifting service remote to receiver
  BLE.setAdvertisedService(gestureService);
  gestureService.addCharacteristic(gestureCharacteristic);
  BLE.addService(gestureService);
  gestureCharacteristic.writeValue(-1);

  // adjust shifting service iphone to receiver with multiple characteristics
  BLE.setAdvertisedService(adjustService);
  adjustService.addCharacteristic(indexCharacteristic);
  adjustService.addCharacteristic(lowerlimitCharacteristic);
  adjustService.addCharacteristic(upperlimitCharacteristic);

  BLE.advertise();

  Serial.println("Nano 33 BLE (Peripheral Device)");
  Serial.println(" ");

  myFlashPrefs.garbageCollection();
}

void loop() {
  BLEDevice central = BLE.central();
  Serial.println("- Discovering central device...");
  delay(500);

  if (central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
    Serial.println(" ");

    while (central.connected()) {
      if (gestureCharacteristic.written()) {
        gesture = gestureCharacteristic.value();
        writeGesture(gesture);
      }
    }

    Serial.println("* Disconnected to central device!");
  }
}
void SetStrokePerc(float strokePercentage) // do if statment tree in this with myservo
{
  if ( strokePercentage >= 1.0 && strokePercentage <= 99.0 ) // clamps stroke percentage 1-99 so no actuator strain
  {
    int usec = 1000 + strokePercentage * ( 2000 - 1000 ) / 100.0 ;
    myServo.writeMicroseconds( usec );
  }
}

void SetStrokePerc2(float strokePercentage)
{
  if ( strokePercentage >= 1.0 && strokePercentage <= 99.0 ) // clamps stroke percentage 1-99 so no actuator strain
  {
    int usec = 1000 + strokePercentage * ( 2000 - 1000 ) / 100.0 ;
    myServo2.writeMicroseconds( usec );
  }
}
SetStrokeMM(int strokeReq, int strokeMax)
{
  SetStrokePerc( ((float)strokeReq) / strokeMax );
}

void writeGesture(int gesture) {
  Serial.println(" - Characteristic <gesture_type> has changed!");
  if (i < lowerlimit) { //limits i vals so we dont have i=300 etc, is used to adjust tension with app
    i = lowerlimit;
  }
  if (i > upperlimit) {
    i = upperlimit;
  }
  if (gesture == 0) // if joystick is in the downshift position(ie, p = 0)
  {
    i -= d; // subtracts d from stroke percentage of actuator
    SetStrokePerc(i);// sets to the new stroke percentage
    delay(100);// delay so you get a shift and not just continuous motion
  }
  else if (gesture == 2) // if the joystick is below its mapped value of 55, then the actuator position is increased by d
  {
    i += d; // adds d from stroke percentage of actuator
    SetStrokePerc(i); // sets to the new stroke percentage
    delay(100); // delay so you get a shift and not just continuous motion
  }
  else if (gesture == 3)
  {
    //if (i2 == 45)
    //{
      //SetStrokePerc2(i2);
    //}
    SetStrokePerc2(i2); // write to other actuator to power dropper post

    Serial.println("Dropper post!"); // comes back to where it was different pin than setstroke perc
  }
}

  SetStrokePerc(i);
  Serial.print("i=");
  Serial.print(i);

  prefs.actuatorpos = i;
  Serial.println("Write ActuatorPos...");
  myFlashPrefs.deletePrefs();
  myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
}
