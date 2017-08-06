#include <SoftwareSerial.h>
// I think this included with ESP8266 core

#include <SH1106Wire.h>
#include <OLEDDisplayFonts.h>
// Available on the library manager
// Search for "OLED" and it's the one by Daniel Eichhorn

#define RXS 4
#define TXS 5

SoftwareSerial BTSerial(RXS,TXS,false,128);

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D6;
const int SDC_PIN = D7;

//badge
String nut1 = "C6591CA4BF9E";
//wallet
String nut2 = "DC3D6E4C215A";
 
int numberOfDevices = 0;

struct Device{
  String name;
  String address;
  int power;
  int threshold;
  bool found;
};

#define DEVICES_NUM 10
Device seenDevices[DEVICES_NUM];

#define KNOWN_DEVICE_COUNT 2
int foundDevices = 0;
Device knownDevice[KNOWN_DEVICE_COUNT] = { 
  { "Badge", nut1, 0, -70 , false}, 
  { "Wallet", nut2, 0, -70, false}
};

char c=' ';
boolean NL = true;

long bluetoothScanDue;

SH1106Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
 
void setup() 
{
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Searching for devices");
  display.display();
    
  Serial.print("Sketch:   ");   Serial.println(__FILE__);
  Serial.print("Uploaded: ");   Serial.println(__DATE__);
  Serial.println(" ");

  BTSerial.begin(9600);  
  Serial.println("BTserial started at 9600");
  Serial.println("");

  String loadingBar = "";
  Serial.println("Starting Search");
  for ( int i = 0; i < 8; i++) {
//    display.clear();
//    display.drawString(0, 0, "Searching for devices");
//    display.drawString(0, 15, loadingBar);
//    display.display();
    sendDiscovery();
    if(foundDevices == KNOWN_DEVICE_COUNT) {
      break;
    }
    //loadingBar = loadingBar + "---";
    displayDevices(true);
    delay(200);
  }
  Serial.println("Finished Search");
  displayDevices(false);
 
}

String endString = "OK+DISCE";
String startString = "OK+DISIS";

String tempEndString = "";
bool checkForEnd(char c) {
  tempEndString = tempEndString + c;
  if (tempEndString == endString) {
    tempEndString = "";
    //Serial.println(tempEndString);
    return true;
  }
  // If the tempEndString isn't at index 0, its not going to be a match
  if( endString.indexOf(tempEndString) != 0) {
    //Serial.println(tempEndString);
    tempEndString = "";
  }

  return false;
}

void updateDevice(Device d){
  for ( int i = 0; i < KNOWN_DEVICE_COUNT; i++) {
     if (knownDevice[i].address == d.address) {
      Serial.println("Found device: " + knownDevice[i].name);
        if (knownDevice[i].power == 0 || knownDevice[i].power < d.power) {
          Serial.println("Updating Power: " + String(d.power));
          knownDevice[i].power = d.power;
          if(!knownDevice[i].found && knownDevice[i].power > knownDevice[i].threshold) {
            knownDevice[i].found = true;
            foundDevices++;
          }
        }
        return;
     }
  }

  Serial.println("Not a known device");
}

void displayDevices(bool searching) {
  int y = 0;
  display.clear();
  String message = "";
  for ( int i = 0; i < KNOWN_DEVICE_COUNT; i++) {
    message = knownDevice[i].name + ": ";
     if (knownDevice[i].power != 0) {
        if(knownDevice[i].power >= knownDevice[i].threshold ) {
          Serial.println(knownDevice[i].name + ": OK");
          message = message + "OK";
        } else {
          Serial.println(knownDevice[i].name + ": LOW");
          message = message + "LOW";  
        }

        message = message + " (" + String(knownDevice[i].power) + ")";
     } else {
        Serial.println(knownDevice[i].name + ": ??");
        if(searching) {
          message = message + "Searching..."; 
        } else {
          message = message + "MISSING"; 
        }
     }

     display.drawString(0, y, message);
     y = y + 15;
  }
  display.display();
}

void sendDiscovery() {
  Serial.println("SendDiscovery");
  String response = "";
  BTSerial.print("AT+DISI?"); 
  bool gotResponse = false;
  bool finishedResponse = false;
  long now = millis();
  char c;
  while (gotResponse || millis() - now < 3000) {
    while (BTSerial.available())
    {
      gotResponse = true;
      c = BTSerial.read();
      //Serial.print(c);
      response = response + c;
      finishedResponse = checkForEnd(c);
    }

    if(finishedResponse) {
      Serial.println(response);
      int lenghtOfStart = startString.length();
      int lenghtOfEnd = endString.length();
      int lengthOfResponse = response.length();
      response = response.substring(8, lengthOfResponse - lenghtOfEnd);
      Serial.println(response);
      if (response.length() > 0) {
        Device devices[16];
        bool parsing = true;
        int devicesCount = 0;
        int offset = 1;
        String startOfDevice = "OK+DISC";
        int startOfDeviceLength = startOfDevice.length();
        String devicesString = "";
        while (parsing) {
          int index = response.indexOf(startOfDevice, offset);
          if (index >= 0) {
            devicesString = response.substring(startOfDeviceLength, index);
            //Serial.println(devices[devicesCount]);
            response = response.substring(index);
          } else {
            devicesString = response.substring(startOfDeviceLength);
            //Serial.println(devices[devicesCount]);
            parsing = false;
          }

          int indexOfPower  = devicesString.lastIndexOf(":") + 1;
//          Serial.println(devicesString.substring(indexOfPower));
//          Serial.println(indexOfPower);
          devices[devicesCount].power = devicesString.substring(indexOfPower).toInt();
          int indexOfAddress  = devicesString.lastIndexOf(":", indexOfPower -2) + 1;
          devices[devicesCount].address = devicesString.substring(indexOfAddress, indexOfPower -1);
          
          devicesCount++;
        }

         Serial.println("Found " + String(devicesCount) + " devices.");

         for ( int i = 0; i < devicesCount; i++ ) {
          Serial.println("------");
          Serial.println("Address: " + devices[i].address);
          Serial.println("Power: " + String(devices[i].power));
          Serial.println("------");
          updateDevice(devices[i]);
         }
      }
      
      break;
    }

    delay(10); // allow WDT to feed;
  }
  Serial.println("End of loop");
}
 
void loop()
{
 
    // Read from the Bluetooth module and send to the Arduino Serial Monitor
    if (BTSerial.available())
    {
        c = BTSerial.read();
        Serial.write(c);
    }

//    long now = millis();
//
//    if(now > bluetoothScanDue) {
//      sendDiscovery();
//      bluetoothScanDue = now + 5000;
//    }
 
 
    // Read from the Serial Monitor and send to the Bluetooth module
    if (Serial.available())
    {
        c = Serial.read();
        BTSerial.write(c);   
 
        // Echo the user input to the main window. The ">" character indicates the user entered text.
        if (NL) { Serial.print(">");  NL = false; }
        Serial.write(c);
        if (c==10) { NL = true; }
    }
 
}
