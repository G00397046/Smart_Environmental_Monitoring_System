//Patrick Cahill
//3-25-25
//Smart Environmental Monitoring System Upload 1

//Parameters for Gas Sensor Config
/*Gas    | a      | b
    H2     | 987.99 | -2.162
    LPG    | 574.25 | -2.222
    CO     | 605.18 | -3.937  
    Alcohol| 3616.1 | -2.675
    Propane| 658.71 | -2.1681
*/

#include <LiquidCrystal_I2C.h> //Library for LCD1
#include "DHT.h" //Library for  DHT 11 Temp Sensor
#include "time.h" //Time library on ESP 32
#include <MQUnifiedsensor.h>
#include <Adafruit_BMP280.h> //Library for BMP 280
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "homepage.h"

#define DHT11_PIN 18 // DHT 11 is connnected to pin 18 on ESP 32
#define Board                   ("ESP-32") // These are used to setup MQ135 got them from MQUnifiedsensor.h
#define Pin                     (33) // GPIO Pin for the MQ sensor (check esp32-wroom-32d.jpg image on ESP32 folder)
#define Voltage_Resolution      (3.3) // 3.3V
#define ADC_Bit_Resolution      (12) // 12-bit resolution for ESP32
#define RatioMQ135CleanAir      (3.6) // RS / R0 = 3.6 ppm for MQ135
#define Type                    ("MQ-135") //MQ135 instead of MQ2, for air quality sensor
Adafruit_BMP280 bmp; // I2C

const char * ssid = "WiFi"; //Used to connect to hotspot from phone
const char * password = "Password";

const int LCD_Address = 0x27; //Default address for 20x4 LCD
const int BMP_Address = 0x76; //Address for BMP 280
const int LCD_COL = 20; // 20 characters long
const int LCD_ROW = 4; // 4 characters wide
const int switchButton = 5; //Pin for switch button
const int buttonTwo = 16;
const char * ntpServer = "pool.ntp.org"; //This char represents the server where we request the time1
const long gmtOffset_sec = 0; //This long defines the offset in seconds between your time zone and GMT
const int daylightOffset_sec = 3600; //This int defines the Daylight savings time offset
const float seaLevelPressure = 1013.25; // Standard pressure at sea level used for calculating altitude

MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type); //Input values needed to setup MQ135
DHT dht(DHT11_PIN, DHT11); //Sets DHT pin to 18 and the DHT type to 11
LiquidCrystal_I2C lcd(LCD_Address, LCD_COL, LCD_ROW); //Sets LCD address to 0x27 for a 20 by 4 character LCD

int escape = 0;
int loaded = 0;
int buttonState = 1;
int currentMenu = 0;
int prevButtonState = 1;
int menuNum = 4;
String gasType = "";
int printFlag = 1;


// Async Web Server and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(9600);
  dht.begin();
  bmp.begin(BMP_Address);
  lcd.init();
  lcd.backlight();
  bootScreen();
  pinMode(switchButton, INPUT_PULLUP); //Using pullup resistors in ESP-32  
  pinMode(buttonTwo, INPUT_PULLUP);
}

void getSensorData(){
  struct tm timeinfo; //Created the structure to store local time info
  getLocalTime(&timeinfo); //Saving all the details of the local time to the time structure
  float tempC = dht.readTemperature(); //readTemperature() is a function in DHT library used to activate temperature readings
  float tempF = (tempC * 1.8) + 32;
  float humidity = dht.readHumidity();
  float pressure = (bmp.readPressure() / 100); //BMP 280 outputs pressue in Pa must divide by 100 to convert to hPa commonly used in weather measurements
  float altitude = bmp.readAltitude(seaLevelPressure); //Uses standard pressure at sea level to make an accurate altitude measurement
  MQ135.update(); //Mq135 function from library updates internal state of MQ135
  float PPM = MQ135.readSensor(); //Another MQ135 function sets airQuality equal to analog vaule given from MQ135
  float AQI;
  String quality = ""; 

  if (PPM >= 0 && PPM <= 4.4) { //If PPM is from 0-4.4 Pass The Concentration of CO along with breakpoints of PPM and the AQI scale range
    AQI = calculateCOAQI(PPM, 0, 4.4, 0, 50);
    quality = "Good                ";
    
  } else if (PPM >= 4.5 && PPM <= 9.4) {
    AQI = calculateCOAQI(PPM, 4.5, 9.4, 51, 100);
    quality = "Moderate            "; //Tabs ensure no overlap. Example if AQI went from Moderate to Good the message would be Goodrate
  
  } else if (PPM >= 9.5 && PPM <= 12.4) {
    AQI = calculateCOAQI(PPM, 9.5, 12.4, 101, 150);
    quality = "Harmful To Sensitive";
    
  } else if (PPM >= 12.5 && PPM <= 15.4) {
    AQI = calculateCOAQI(PPM, 12.5, 15.4, 151, 200);
    quality = "Harmful             ";
   
  } else if (PPM >= 15.5 && PPM <= 30.4) {
    AQI = calculateCOAQI(PPM, 15.5, 30.4, 201, 300);
    quality = "Very Harmful        ";
    
  } else if (PPM >= 30.5 && PPM <= 40.4) {
    AQI = calculateCOAQI(PPM, 30.5, 40.4, 301, 400);
    quality = "Hazardous           ";
    
  } else if (PPM > 40.5) {
    AQI = calculateCOAQI(PPM, 40.5, PPM, 401, 500);
    quality = "Very Hazardous      ";

 }
 

handleSensorData(tempC, tempF, humidity, pressure, altitude, PPM, AQI , quality);
printDataType(&timeinfo, tempC, tempF, humidity, pressure, altitude, PPM, AQI , quality);
}

void handleSensorData(float tempC, float tempF, float humidity, float pressure, float altitude, float PPM, float AQI, String quality){
  Serial.println(AQI);
  Serial.println(PPM);
   StaticJsonDocument<200> jsonDoc;
   jsonDoc["TemperatureC"] = tempC;
   jsonDoc["TemperatureF"] = tempF;
   jsonDoc["Humidity"] = humidity;
   jsonDoc["Pressure"] = pressure;
   jsonDoc["Altitude"] = altitude;
   jsonDoc["PPM"] = PPM;
   jsonDoc["AQI"] = AQI;
   jsonDoc["Quality"] = quality;

   String message;
   serializeJson(jsonDoc, message);
   ws.textAll(message);

}

void printDataType(struct tm* timeinfo, float tempC, float tempF, float humidity, float pressure, float altitude, float PPM, float AQI, String quality){
  switch(printFlag){
    case 1:
    mainMenu(timeinfo);
    break;
    case 2: 
    printTemp(tempC, tempF, humidity);
    break;
    case 3:
    printBMPData(pressure, altitude);
    break;
    case 4:
    printCOAQI(PPM, AQI, quality);
    break;
    case 5: //Used this article to group cases together https://stackoverflow.com/questions/4494170/grouping-switch-statement-cases-together
    case 6:
    case 7:
      printAirQuality(PPM);
    
    break;
    }

}

void loop() {
  menuCycle();  
}

void configMQ135(float a, float b) { //Recieves chosen gas type with values to setup sensor to detect certain gas
  MQ135.setRegressionMethod(1);
  MQ135.setA(a);
  MQ135.setB(b);
}

void calibrateMQ135() { //Got this from MQ135 Library
  lcd.print("Calibrating MQ-135"); //Calibration code for MQ135 it will take 10 readings of R0 then using the average of them to set a baseline resistance for accurate air quality measurement
  delay(2000);
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0 / 10);
  configMQ135(605.18, -3.937); //Call configMQ135 and pass these values to setup detection of CO. Must be configed before subMenu otherwise it wont update webserver until we reach subMenu
}

void menuCycle() {
   if (loaded == 0) { //Allows us to enter the menu right away without waiting for button press
    prevButtonState = LOW;
    int loaded = 1;
  }
  buttonState = digitalRead(switchButton);

  if (buttonState == HIGH && prevButtonState == LOW) { //buttonState is an active high switch so when the button is pressed it becomes low which becomes important in loop as we set buttonState equal to prevButtonState thus fufiling our if statement
   
    currentMenu++; //Every time we press button in loop it goes back into if statement adding currentMenu. 
    currentMenu = currentMenu % (menuNum + 1); //This line is used to loop back to case 1 
    
    switch (currentMenu) {
    case 1:
      lcd.clear();
      while (escape == 0) {
        getSensorData();
        if (digitalRead(switchButton) == LOW) { //Again active high switch so when button is pressed esscape will be 1 breaking out of loop
        printFlag++;
        escape = 1;
        }
      }

      escape = 0;
      break;

    case 2:
      lcd.clear();
      while (escape == 0) {
        getSensorData();
        if (digitalRead(switchButton) == LOW) { 
          printFlag++;
          escape = 1;
        }
      }

      escape = 0;
      break;

    case 3:
      lcd.clear();
      while (escape == 0) {
        getSensorData();
        if (digitalRead(switchButton) == LOW) {
          printFlag++;
          escape = 1;
        }
      }

      escape = 0;
      break;

    case 4:
      lcd.clear();
      while (escape == 0) {
        if (subMenu() == 1) { //Keep calling subMenu until we recieve 1
          currentMenu = 0; //Resets currentMenu to ensure we always go back to case 1
          printFlag = 1;
          escape = 1; //Break out of loop
        }

      }
      escape = 0;
      break;
    }
    if (currentMenu == menuNum) {
      currentMenu = 0;
    }
  }
  prevButtonState = buttonState; // Set prevButtonState to buttonState which will be low on button press allowing user to enter if

}

float subMenu() {
  if (loaded == 0) { //Allows us to enter the menu right away without waiting for button press
    prevButtonState = LOW;
    int loaded = 1;
  }

  buttonState = digitalRead(buttonTwo);

  if (buttonState == HIGH && prevButtonState == LOW) {
    currentMenu++;
    currentMenu = currentMenu % (menuNum + 1);
    
    switch (currentMenu) {
    case 1:
      lcd.clear();
      while (escape == 0) {
        getSensorData();
        if (digitalRead(buttonTwo) == LOW) {
          printFlag++;
          escape = 1;
        }
        if (digitalRead(switchButton) == LOW) {
          return 1; // If switchButton is pressed return 1 returning us to menuCycle
        }
      }
      escape = 0;
      break;

    case 2:
      lcd.clear();
      while (escape == 0) {
        configMQ135(574.25, -2.222); //Call configMQ135 and pass these values to setup detection of LP Gas
        gasType = "LP Gas";
        getSensorData();
        if (digitalRead(buttonTwo) == LOW) {
          printFlag++;
          escape = 1;
        }
        if (digitalRead(switchButton) == LOW) {
          return 1;
        }
      }
      escape = 0;
      break;

    case 3:
      lcd.clear();
      while (escape == 0) {
        configMQ135(658.71, -2.168); //Call configMQ135 and pass these values to setup detection of Propane
        gasType = "Propane";
        getSensorData();
        Serial.println(printFlag);
        if (digitalRead(buttonTwo) == LOW) {
          printFlag++;
          escape = 1;

        }
        if (digitalRead(switchButton) == LOW) {
          return 1;
        }
      }
      escape = 0;
      break;

    case 4:
      lcd.clear();
      while (escape == 0) {
        configMQ135(3616.1, -2.675); //Call configMQ135 and pass these values to setup detection of Alcohol
        gasType = "Alcohol";
        Serial.println(printFlag);
        getSensorData();
        if (digitalRead(buttonTwo) == LOW) {
          printFlag++;
          escape = 1;
        }
        if (digitalRead(switchButton) == LOW) {
          return 1;
        }
      }
      escape = 0;
      break;
    }

    if (currentMenu == menuNum) {
      printFlag = 4;
      currentMenu = 0;
    }
  }

  prevButtonState = buttonState;
  return 0; //Returns 0 to ensure if statement continues to check for 1
}

void mainMenu(struct tm* timeinfo) {
  lcd.setCursor(2, 0);
  lcd.print("SEMS - Main Menu");
  lcd.setCursor(0, 3);
  getLocalTime(timeinfo); //Saving all the details of the local time to the time structure
  lcd.print(timeinfo, " %a, %b %d  %H:%M "); //To access the members of timeinfo you use the following specifiers
}

void printTemp(float tempC, float tempF, float humidity) {
  if (digitalRead(buttonTwo) == HIGH) {
    lcd.setCursor(3, 0);
    lcd.print("Temperature(C) ");
    lcd.setCursor(0, 1);
    lcd.print("Celcius:      ");
    lcd.print(tempC);
    lcd.print("C");
    lcd.setCursor(0, 2);
    lcd.print("Humidity:     ");
    lcd.print(humidity);
    lcd.print("%");
  }
  else {
  lcd.setCursor(3, 0);
  lcd.print("Temperature(F) ");
  lcd.setCursor(0, 1);
  lcd.print("Fahernheit:   ");
  lcd.print(tempF);
  lcd.print("F");
  lcd.setCursor(0, 2);
  lcd.print("Humidity:     ");
  lcd.print(humidity);
  lcd.print("%");
}
        
}

void printBMPData(float pressure, float altitude){
  lcd.setCursor(0,0);
  lcd.print("Pressure: ");
  lcd.setCursor(0,1);
  lcd.print(pressure);
  lcd.print(" hPa");
  lcd.setCursor(0,2);
  lcd.print("Altitiude: ");
   lcd.setCursor(0,3);
  lcd.print(altitude);
  lcd.print(" m");
}
void bootScreen() {
  lcd.setCursor(0, 0); //setCursor function allows to place the cursor on (row,column) in this case the cursor is at position (0,0)
  lcd.print("Smart Envirnomental");
  lcd.setCursor(0, 1);
  lcd.print(" Monitoring System");
  delay(3000);
  lcd.clear();
  calibrateMQ135();
  wifiStart();
  lcd.clear();
}
float calculateCOAQI(float ppm, float cl, float ch, float il, float ih) { //Note: BP means breakpoint and the index corresponds to the Concentration(BP) and Concentration is current PPM
  return ((ih - il) / (ch - cl)) * (ppm - cl) + il; // AQI = (Index(high) - Index(low)) / (Concentration(BP)(high) - Concentration(BP)(low)) * (Concentration -  Concentration(BP)(low)) + Index(low) 

}

void printAirQuality(float PPM) {
  Serial.println("Inside");
  lcd.setCursor(0, 0);
  lcd.print("Gas Type: ");
  lcd.print(gasType); //Global variable set earlier to print Gas Type sensor is detecting
  lcd.setCursor(0, 2);
  lcd.print("Air Quality:");
  lcd.setCursor(0, 3);
  lcd.print(PPM); //Printing air quality derived from getAirQuality NEED PPM
  lcd.print(" PPM");
}

void printCOAQI(float PPM, float AQI, String quality) { //Values from getCOAQI to be printed on LCD
  lcd.setCursor(0, 0);
  lcd.print("Air Quality:");
  lcd.setCursor(0, 1);
  lcd.setCursor(0, 1);
  lcd.print(quality);
  lcd.setCursor(0, 2);
  lcd.print("AQI: ");
  lcd.print(AQI);
  lcd.setCursor(0, 3);
  lcd.print(PPM);
  lcd.print(" PPM");
}

void wifiStart() {
  lcd.clear();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.print("Connecting to ");
  lcd.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {}
  server.on("/", handleRoot);
  //ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Connection Made");
  delay(1000);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Config the time with the proper offsets and NTP server address
  lcd.clear();
  lcd.print("Configuring Time");
  delay(2000);
}

void handleRoot(AsyncWebServerRequest *request){
  request->send_P(200, "text/html", homePagePart1.c_str());
}






