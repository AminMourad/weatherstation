#include <PubSubClient.h>

#include <SPI.h>
#include <Wire.h>
//Library for the CO2 sensor
#include <SensirionI2CScd4x.h>
#include <DHT.h> //Library for DHT
//Libraries for the display and graphics methods
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//Library to parse JSON files
#include <Arduino_JSON.h>
//Libraries for HTTP connections (API) and WiFi connection
#include <HTTPClient.h>
#include <WiFi.h>
//Library that enables a Real-Time-Clock
#include <ESP32Time.h>


//Sets the I2C bus to PIN 8 (data) and 9 (clock)
#define I2C_SDA 8
#define I2C_SCL 9


#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//Defines the address of the display (change as desired, address can be read using the I2C scanner)
#define SCREEN_ADDRESS 0x3C

//Defines screen height and width
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Initializes the connection to the screen using a method from the display library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Class method(screen width, screen height, connection type, reset pin)


SensirionI2CScd4x scd4x; //Tell the library that a sensor is connected
uint16_t error; //initializes a variable to store occurring errors
char errorMessage[256]; //Contains all possible error messages of the CO2 sensor in a stack

//DHT mydht(10, DHT22);

//Here the link to the API is stored, change latitude, longitude and model to your needs
const char* url = "https://api.open-meteo.com/v1/forecast?latitude=YOURLATITUDE&longitude=YOURLONGITUDE&current=temperature_2m,weather_code&daily=weather_code,temperature_2m_max,temperature_2m_min&timeformat=unixtime&timezone=auto&forecast_days=3&models=icon_d2";

//Here the name of your WiFi connection is stored
const char* ssid = "YOURWIFISSID";     // Replace with your WiFi SSID
//Here the password of your WiFi connection is stored
const char* password = "YOURWIFIPASSWORD"; // Replace with your WiFi password

//Here a global variable for the queried UTC time is defined (only needed for time without NTP)
//long utctime;

//List of bitmap images for the symbols of the weather codes... //Here we define the X-coordinate of the symbols (simply to replace the value in all methods each time [See below in the code])
int icon_pos_x = 39;
//Here we define the Y-coordinate of the symbols (simply to replace the value in all methods each time [See below in the code])
int icon_pos_y = 21;
float avgTemp = 0;
float avgHum = 0;
float dhtHumid = 0;
float dhtTemp = 0;

uint16_t avgCO2 = 0;
int start = 0;
unsigned long count1=0;
unsigned long count2=0;
unsigned long currentMillis=0;

int lastvisit = 0;
int weatherCode = 0;
double temperature_2m = 0;
double maxTemp_today = 0;
double minTemp_today = 0;
double maxTemp_tomorrow = 0;
double minTemp_tomorrow = 0;

String name_tomorrow = "";

ESP32Time rtc(0);


const char* mqtt_server = "MQTTSERVER";
const int mqtt_port = MQTTPORT;
const char* mqtt_client_name;
const char* mqtt_user = "YOURUSERNAME";
const char* mqtt_password = "YOURPASSWORD";
WiFiClient espClient;
PubSubClient client(espClient);



// 'wi-day-sunny', 20x20px
static const unsigned char PROGMEM clearsky[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x04,
	0x02, 0x00, 0x00, 0x60, 0x00, 0x01, 0xf8, 0x00, 0x01, 0x08, 0x00, 0x19, 0x09, 0x80, 0x01, 0x08,
	0x00, 0x01, 0x08, 0x00, 0x00, 0xf0, 0x00, 0x06, 0x06, 0x00, 0x04, 0x02, 0x00, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-cloudy', 20x20px
static const unsigned char PROGMEM cloudy[] = {
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x01, 0x01, 0x80, 0x00, 0x18, 0x00, 0x03,
	0xe6, 0x00, 0x0c, 0x62, 0x00, 0x08, 0x22, 0x60, 0x18, 0x3a, 0x00, 0x20, 0x06, 0x00, 0x40, 0x06,
	0x00, 0x40, 0x02, 0x00, 0x60, 0x06, 0x80, 0x3f, 0xfc, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-fog', 20x20px
static const unsigned char PROGMEM fog[] = {
	0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x80, 0xc0, 0x00, 0x1c, 0x80, 0x03,
	0xf6, 0x00, 0x06, 0x21, 0x00, 0x04, 0x11, 0x30, 0x1c, 0x1d, 0x00, 0x30, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x1f, 0xfe, 0x00, 0x1f, 0xfe, 0x40, 0x7f, 0xf8, 0x00, 0x7f, 0xf8, 0x00, 0x0f, 0xff, 0x00,
	0x1f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-drizzle', 20x20px
static const unsigned char PROGMEM drizzle[] = {
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x01, 0x01, 0x80, 0x00, 0x18, 0x00, 0x03,
	0xe4, 0x00, 0x0c, 0x62, 0x00, 0x08, 0x22, 0x60, 0x18, 0x3a, 0x00, 0x20, 0x06, 0x00, 0x40, 0x02,
	0x00, 0x42, 0x82, 0x00, 0x66, 0xa6, 0x80, 0x35, 0xac, 0x00, 0x05, 0x00, 0x00, 0x01, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-rain', 20x20px
static const unsigned char PROGMEM rain[] = {
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x01, 0x01, 0x80, 0x00, 0x18, 0x00, 0x03,
	0xe4, 0x00, 0x0c, 0x62, 0x00, 0x08, 0x22, 0x60, 0x18, 0x3a, 0x00, 0x20, 0x06, 0x00, 0x40, 0x02,
	0x00, 0x42, 0x82, 0x00, 0x66, 0xa6, 0x80, 0x35, 0xac, 0x00, 0x05, 0x60, 0x00, 0x05, 0x40, 0x00,
	0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-snow', 20x20px
static const unsigned char PROGMEM snow[] = {
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x01, 0x01, 0x80, 0x00, 0x19, 0x00, 0x03,
	0xfc, 0x00, 0x0c, 0x62, 0x00, 0x08, 0x22, 0x60, 0x18, 0x3a, 0x00, 0x20, 0x06, 0x00, 0x40, 0x06,
	0x00, 0x40, 0x02, 0x00, 0x60, 0x06, 0x80, 0x30, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-showers', 20x20px
static const unsigned char PROGMEM rain_showers[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe0, 0x00, 0x03, 0x18, 0x00, 0x02, 0x08, 0x00, 0x06, 0x0e, 0x00, 0x08, 0x01, 0x00, 0x10, 0x00,
	0x80, 0x10, 0x80, 0x80, 0x18, 0x89, 0x80, 0x0c, 0x03, 0x00, 0x00, 0x40, 0x00, 0x01, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-snow', 20x20px
static const unsigned char PROGMEM snow_showers[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe0, 0x00, 0x03, 0x18, 0x00, 0x02, 0x08, 0x00, 0x06, 0x0e, 0x00, 0x08, 0x01, 0x00, 0x10, 0x00,
	0x80, 0x10, 0x00, 0x80, 0x18, 0x01, 0x80, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'wi-day-thunderstorm', 20x20px
static const unsigned char PROGMEM thunderstorm[] = {
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x01, 0x01, 0x80, 0x00, 0x18, 0x00, 0x03,
	0xfc, 0x00, 0x0c, 0x62, 0x00, 0x08, 0x22, 0x60, 0x18, 0x3a, 0x00, 0x20, 0x06, 0x00, 0x40, 0x06,
	0x00, 0x44, 0x82, 0x00, 0x6c, 0xa6, 0x80, 0x3d, 0xac, 0x00, 0x1d, 0x60, 0x00, 0x1d, 0x40, 0x00,
	0x09, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM tyfun[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x03,
	0xfc, 0x00, 0x03, 0xfc, 0x00, 0x07, 0xfe, 0x00, 0x07, 0xfe, 0x00, 0x00, 0xf2, 0x00, 0x00, 0x60,
	0x00, 0x00, 0x04, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// adjust to your timezone, daylight offset is only used during the daylight saving time period
const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


void setup() {
  


    Serial.begin(115200);

    // Connect to WiFi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid,password); // Connect to WiFi network

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Wire.begin(I2C_SDA, I2C_SCL);

    //Checking for OLED display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }


  
    display.setRotation(1); // Set rotation to portrait mode
    display.setTextWrap(false);

    //Starte Anzeigen der Inhalte
    display.display();
    delay(200);
    display.clearDisplay();
    

    //Starting connection to CO2 Sensor
    scd4x.begin(Wire);

    //Stopping any measurement already going on
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
      Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    }
    //Starting periodic measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    }

    //mydht.begin(); 

/*
    HTTPClient http;

    Serial.println("http://worldtimeapi.org/api/ip");
    http.begin("http://worldtimeapi.org/api/ip");
    delay(3000);
    int httpCode = http.GET();
    delay(3000);
    JSONVar gtime;
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
        gtime = JSON.parse(payload);
      }
      else {
        Serial.println("HTTP error code: " + String(httpCode));
      }
    } else {
      Serial.println("HTTP request failed");
    }
    http.end();

    utctime = gtime["unixtime"];

    rtc.setTime(utctime);
    
*/
configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


  String mqtt_client_name_str = "WetterstationIoT";
    mqtt_client_name = mqtt_client_name_str.c_str();
    client.setServer(mqtt_server, mqtt_port);
    Serial.print("Connecting to MQTT Server");
    while (!client.connected()) {
      Serial.print(".");
      if(client.connect(mqtt_client_name, mqtt_user, mqtt_password))
        Serial.println("\nMQTT connected!");
      else {
        Serial.print("\nMQTT Connection failed with state: ");
        Serial.println(client.state());
        delay(2000);
      }
  }
}

void loop() {
  currentMillis=millis();
  if(currentMillis-count1 > 7000){
    if(!client.connected()){
      client.connect(mqtt_client_name, mqtt_user, mqtt_password);
    }

    error = scd4x.readMeasurement(avgCO2,avgTemp,avgHum);
    if (error) {
        Serial.print("Error trying to execute readMeasurements(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    //dhtTemp =   mydht.readTemperature();
    //dhtHumid =  mydht.readHumidity();
    //if (!isnan(dhtTemp)){
    //avgTemp = dhtTemp;
    //avgHum = dhtHumid;
    //}

    if (avgCO2 != 0){
      client.publish(String("YOURCO2FEED").c_str(), String(avgCO2).c_str());
      client.publish(String("YOURHUMIDITYFEED").c_str(), String(avgHum).c_str());
      client.publish(String("YOURTEMPERATUREFEED").c_str(), String(avgTemp).c_str());
    }
    count1=millis();
  }



    if(currentMillis-count2 > 15000){
      if (WiFi.status() != WL_CONNECTED)
        WiFi.reconnect();
      HTTPClient http;
      http.begin(url); // Initialize HTTPClient for the request
      int httpCode = http.GET(); // Send the request
      if (httpCode) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString(); // Get the response payload
          Serial.println("Received payload:");
          Serial.println(payload);
  
          // Parse JSON
          JSONVar data = JSON.parse(payload);
  
          // Accessing individual values and saving them
          weatherCode = data["current"]["weather_code"];
          temperature_2m = data["current"]["temperature_2m"];
          maxTemp_today = data["daily"]["temperature_2m_max"][0];
          minTemp_today = data["daily"]["temperature_2m_min"][0];
          maxTemp_tomorrow = data["daily"]["temperature_2m_max"][1];
          minTemp_tomorrow = data["daily"]["temperature_2m_min"][1];

      } else {
          Serial.print("HTTP request failed with error code: ");
          Serial.println(httpCode);
        }
    } else {
      Serial.println("HTTP request failed.");
    }
    http.end(); // Free resources
    count2=millis();
    }

    delay(100);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(3,0);
    delay(10);
    String timestring = rtc.getTime();
    String cutString = timestring.substring(0, 5);
    display.print(cutString);
    display.setTextSize(1);
    display.setCursor(0,20);
    display.println("Now:");
    display.println(" " + String((int)temperature_2m) + char(247) + "C");
    display.setCursor(0,40);
    display.println("Today:");
    display.println(" " + String((int)minTemp_today) + " to " + String((int)maxTemp_today) + char(247) + "C");

    switch (rtc.getDayofWeek()) {
    case 0:
      name_tomorrow = "Monday:";
      break;
    case 1:
      name_tomorrow = "Tuesday:";
      break;
    case 2:
      name_tomorrow = "Wednesday:";
      break;
    case 3:
      name_tomorrow = "Thursday:";
      break;
    case 4:
      name_tomorrow = "Friday:";
      break;
    case 5:
      name_tomorrow = "Saturday:";
      break;
    case 6:
      name_tomorrow = "Sunday:";
      break;
    }

    display.setCursor(0,60);
    display.println(name_tomorrow);
    display.println(" " + String((int)minTemp_tomorrow) + " to " + String((int)maxTemp_tomorrow)+ char(247) + "C");


    if(weatherCode >= 1 && weatherCode <= 3){
      display.drawBitmap(icon_pos_x ,icon_pos_y,cloudy,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 45 && weatherCode <= 48){
      display.drawBitmap(icon_pos_x ,icon_pos_y,fog,20,20,SSD1306_WHITE);
      }
    else if(weatherCode >= 51 && weatherCode <= 57){
      display.drawBitmap(icon_pos_x ,icon_pos_y,drizzle,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 61 && weatherCode <= 67){
      display.drawBitmap(icon_pos_x ,icon_pos_y,rain,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 71 && weatherCode <= 77){
      display.drawBitmap(icon_pos_x ,icon_pos_y,snow,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 80 && weatherCode <= 82){
      display.drawBitmap(icon_pos_x ,icon_pos_y,rain_showers,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 85 && weatherCode <= 86){
      display.drawBitmap(icon_pos_x ,icon_pos_y,snow_showers,20,20,SSD1306_WHITE);
    }
    else if(weatherCode >= 95 && weatherCode <= 96){
      display.drawBitmap(icon_pos_x ,icon_pos_y,thunderstorm,20,20,SSD1306_WHITE);
    }
    else if(weatherCode == 99){
      display.drawBitmap(icon_pos_x ,icon_pos_y,tyfun,20,20,SSD1306_WHITE);
    }
    else{
      display.drawBitmap(icon_pos_x ,icon_pos_y,clearsky,20,20,SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setCursor(0,108);
    display.println(String(avgTemp).substring(0,4) + char(247) + "C" + "  " + String((int)avgHum) + "%");
    display.println(String(avgCO2) + "ppm CO2");
    display.display();
}
