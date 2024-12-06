
//Patrick Cahill
//12-6-24
//Smart Environmental Monitoring System Upload 1

//Parameters for Gas Sensor Config
 /*Gas    | a      | b
    H2     | 987.99 | -2.162
    LPG    | 574.25 | -2.222
    CO     | 36974  | -3.109
    Alcohol| 3616.1 | -2.675
    Propane| 658.71 | -2.168
*/

#include <LiquidCrystal_I2C.h> //Library for LCD
#include "DHT.h" //Library for  DHT 11 Temp Sensor
#include <WebServer.h> // Library for web server
#include "time.h" //Time library on ESP 32
#include <WiFi.h>
#include <MQUnifiedsensor.h>

#define DHT11_PIN 18 // DHT 11 is connnected to pin 18 on ESP 32
#define Board                   ("ESP-32") // These are used to setup MQ135 got them from MQUnifiedsensor.h
#define Pin                     (33) // GPIO Pin for the MQ sensor (check esp32-wroom-32d.jpg image on ESP32 folder)
#define Voltage_Resolution      (3.3) // 3.3V
#define ADC_Bit_Resolution      (12) // 12-bit resolution for ESP32
#define RatioMQ135CleanAir      (9.83) // RS / R0 = 9.83 ppm for MQ135
#define Type                    ("MQ-135") //MQ135 instead of MQ2, for air quality sensor

const char *ssid     = "WiFi"; //Used to connect to hotspot from phone
const char *password = "Password";

const int LCD_Address = 0x27;//Default address for 20x4 LCD
const int LCD_COL = 20; // 20 characters long
const int LCD_ROW = 4; // 4 characters wide
const int switchButton = 5; //Pin for switch button
const char* ntpServer = "pool.ntp.org"; //This char represents the server where we request the time
const long  gmtOffset_sec = 0; //This long defines the offset in seconds between your time zone and GMT
const int   daylightOffset_sec = 3600; //This int defines the Daylight savings time offset

MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type); //Input values needed to setup MQ135
DHT dht(DHT11_PIN, DHT11); //Sets DHT pin to 18 and the DHT type to 11
LiquidCrystal_I2C lcd(LCD_Address,LCD_COL,LCD_ROW); //Sets LCD address to 0x27 for a 20 by 4 character LCD

void menuCycle();
void mainMenu();
void bootScreen();
void printTempC();
void printTempF();
void printTime();
void wifiStart();
float getTemp();
float getHumidity();
float getAirQuality();
void printAirQuality();
void configMQ135();
void calibrateMQ135();
void selectGasType();

int escape = 0;
int buttonState = 1; 
int currentMenu = 0;
int prevButtonState = 1;
int menuNum = 4;
String gasType = "";


void setup() {
   Serial.begin(9600);
   dht.begin(); 
   lcd.init();
   lcd.backlight();
   bootScreen();
   pinMode(switchButton, INPUT_PULLUP);
}

void loop() {
 menuCycle();
}

void selectGasType() {
  Serial.println("Please Input Desired Gas To Detect");
  

  while (1) { // Remains in loop until a valid choice is entered
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please Choose One");
    lcd.setCursor(0, 1);
    lcd.print("Of The Following");
    lcd.setCursor(0, 2);
    lcd.print("Gases To Detect:");
    delay(2000); // Wait 2 seconds to ensure the user can read it
    
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1: Alcohol");
    lcd.setCursor(0, 1);
    lcd.print("2: Hydrogen");
    lcd.setCursor(0, 2);
    lcd.print("3: LP Gas");
    lcd.setCursor(0, 3);
    lcd.print("4: Carbon Monoxide");
    delay(2000); 
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("5: Propane");
    lcd.setCursor(0, 1);
    lcd.print("Enter 1-5:");
    delay(2000);
   

    if (Serial.available() > 0) {
      char choice = Serial.read(); 
      Serial.print("Input received: ");
      Serial.println(choice); 

      if (choice >= '1' && choice <= '5') {
        
        switch (choice) {
          case '1':
        configMQ135(3616.1, -2.675); // Each of these values are useed to calibrate MQ135 important to help detect certain gases in the air
        lcd.clear();
        lcd.print("Device Configured");
        lcd.setCursor(0,2);
        lcd.print("To Detect Alcohol");
        gasType = "Alcohol";
        break;

        case '2':
        configMQ135(987.99, -2.162 );
        lcd.clear();
        lcd.print("Device Configured");
        lcd.setCursor(0,2);
        lcd.print("To Detect Hydrogen");
        gasType = "Hydrogen";
        break;

        case '3':
        configMQ135(574.25,-2.222);
         lcd.clear();
        lcd.print("Device Configured");
         lcd.setCursor(0,2);
        lcd.print("To Detect LP Gas");
        gasType = "LP Gas";
        break;

        case '4':
        configMQ135(36974,-3.109);
        lcd.print("Device Configured");
         lcd.setCursor(0,2);
        lcd.print("To Detect Carbon Monoxide");
        gasType = "CO";
        break;

        case '5':
        configMQ135(658.71,-2.168);
        lcd.print("Device Configured");
         lcd.setCursor(0,2);
        lcd.print("To Detect Propane Gas");
        gasType = "Propane";
        break;
        }
        break; // Exit the loop once a valid choice is made
      } else {
        // Handle invalid input
        Serial.println("Invalid input. Please enter a number between 1 and 5.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Input");
        delay(1000); // Show the error message for 1 second
      }
    }

    delay(100); // Small delay to allow Serial buffer to process new input
  }
}


void configMQ135(float a, float b) { //Passes chosen gas type with values to setup sensor to detect certain gas
    MQ135.setRegressionMethod(1);
    MQ135.setA(a);
    MQ135.setB(b);
 }

void calibrateMQ135() {
  lcd.print("Calibrating MQ-135"); //Calibration code for MQ135 it will take 10 readings of R0 then using the average of them to set a baseline resistance for accurate air quality measurement
  delay(2000);
 float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update(); 
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir); 
    Serial.print(".");
  }
  MQ135.setR0(calcR0 / 10);
}


void menuCycle(){
  buttonState = digitalRead(switchButton);
  
  if(buttonState == HIGH && prevButtonState == LOW ){ //buttonState is an active high switch so when the button is pressed it becomes low which becomes important in loop as we set buttonState equal to prevButtonState thus fufiling our if statement
   
    currentMenu++;
    currentMenu = currentMenu % (menuNum + 1);  //Every time we press button in loop it goes back into if statement ading currentMenu. This line is used to loop back to our mainMenu

    switch(currentMenu){
    case 1:
    lcd.clear();
    while(escape == 0){
      mainMenu();
      if(digitalRead(switchButton) == LOW){ //Again active high switch so when button is pressed esscape will be 1 breaking out of loop
      escape = 1;
    }
  }
    
    escape = 0;
    break;

    case 2:
     lcd.clear();
    while(escape == 0){
      printTempC();
      if(digitalRead(switchButton) == LOW){
      escape = 1;
    }
  }
    
    escape = 0;
    break;

    case 3:
     lcd.clear();
    while(escape == 0){
      printTempF();
      if(digitalRead(switchButton) == LOW){
      escape = 1;
    }
  }

   escape = 0;
    break;

   case 4:
     lcd.clear();
    while(escape == 0){
      printAirQuality();
      if(digitalRead(switchButton) == LOW){
      escape = 1;
    }
  }
    
    escape = 0;
    break;
    }
   if (currentMenu == menuNum){
     currentMenu = 0;
     } 
  }
 
   
    }

void mainMenu(){
  lcd.setCursor(2,0);
  lcd.print("SEMS - Main Menu");
  printTime();
}

void printTime(){
  lcd.setCursor(0,3);
  struct tm timeinfo; //Created the structure to store local time info
  getLocalTime(&timeinfo); //Saving all the details of the local time to the time structure
  lcd.print(&timeinfo, " %a, %b %d  %H:%M "); //To access the members of timeinfo you use the following specifiers
}

void printTempC(){
 float tempC = getTemp(); //Calls getTemp which returns current temp readings and sets tempC equal to them
 float humi = getHumidity();
 float heatIndex;
 
  lcd.setCursor(3,0);
  lcd.print("Temperature(C) ");
  lcd.setCursor(0,1);
  lcd.print("Celcius:      ");
  lcd.print(tempC);
  lcd.print("C");
  lcd.setCursor(0,2);
  lcd.print("Humidity:     ");
  lcd.print(humi);
  lcd.print("%"); 
}
void printTempF(){
  float tempF = (getTemp() * 1.8) + 32;
  float humi = getHumidity();


  lcd.setCursor(3,0);
  lcd.print("Temperature(F) ");
  lcd.setCursor(0,1);
  lcd.print("Fahernheit:   ");
  lcd.print(tempF);
  lcd.print("F");
  lcd.setCursor(0,2);
  lcd.print("Humidity:     ");
  lcd.print(humi);
  lcd.print("%"); 
}
void bootScreen(){
  lcd.setCursor(0,0); //setCursor function allows to place the cursor on (row,column) in this case the cursor is at position (0,0)
  lcd.print("Welcome to the Smart");
  lcd.setCursor(0,1);
  lcd.print("Environment Monitor");
  lcd.setCursor(7,2);
  lcd.print("System");
  delay(4500); 
  lcd.clear(); 
  lcd.print("Features");
  lcd.setCursor(0,1);
  lcd.print("Button Cycles Menu's");
  lcd.setCursor(0,2);
  lcd.print("Users Can Config Air");
  lcd.setCursor(0,3);
  lcd.print("Quality Sensor");
  lcd.setCursor(7,2);
  delay(5500);
  lcd.clear();
  lcd.print("Current Menu's");
  lcd.setCursor(0,1);
  lcd.print("1: Home Screen");
   lcd.setCursor(0,2);
  lcd.print("2: Temp(C)");
   lcd.setCursor(0,3);
  lcd.print("3: Temp(F)");
  delay(2500);
  lcd.clear();
  lcd.print("4: Air Quality");
  delay(2000);
  wifiStart();
  lcd.clear();
  calibrateMQ135();
  selectGasType();
  lcd.clear();
  lcd.print("Press Button To");
  lcd.setCursor(0,1);
  lcd.print("Finalize Startup");

}

float getTemp(){
  float temperature = dht.readTemperature(); //readTemperature() is a function in DHT library used to activate temperature readings
  return temperature;
}

float getHumidity(){
  float humidity = dht.readHumidity(); //readHumidity() is a function in DHT library used to activate humidity readings
  return humidity;
}

float getAirQuality(){
  MQ135.update(); //Mq135 function from library updates internal state of MQ135
  float airQuality = MQ135.readSensor(); //Another MQ135 function sets airQuality equal to analog vaule given from MQ135
  delay(500); //Waits a half second to take next reading 
  return airQuality;
}

void printAirQuality(){
lcd.setCursor(0, 0);  
lcd.print("Gas Type: ");
lcd.print(gasType);  //Global variable set earlier to print Gas Type sensor is detecting

lcd.setCursor(0, 2);  
lcd.print("Air Quality:");

lcd.setCursor(0, 3);  
lcd.print(getAirQuality()); //Printing air quality derived from getAirQuality
lcd.print(" PPM");  
  }

void wifiStart(){
  lcd.clear();
  WiFi.begin(ssid, password);
   lcd.print("Connecting to ");
   lcd.print(ssid);
   while ( WiFi.status() != WL_CONNECTED ) {}
   lcd.clear();
   lcd.setCursor(2,0);
   lcd.print("Connection Made");
   delay(1000);
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Config the time with the proper offsets and NTP server address
}
