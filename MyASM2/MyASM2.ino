// Define Blynk device
#define BLYNK_TEMPLATE_ID "TMPL6oNWKJzsl"
#define BLYNK_TEMPLATE_NAME "ASM"
#define BLYNK_AUTH_TOKEN "jcgWH1LvRF1eed2ZGuRJUxkubvXa1Zsz"
// Library
#include <Servo.h>  
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Phong 10"
#define WIFI_PASSWORD "phong10vip"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAgzThl26clcSurSNNeqO-Qn2Az36m6K1E"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "thangquangtran12@gmail.com"
#define USER_PASSWORD "01223166976"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://asm2-ad522-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
String timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
String getTime() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
 	struct tm *ptm = gmtime ((time_t *)&epochTime);
 	int monthDay = ptm->tm_mday;
 	int currentMonth = ptm->tm_mon+1;
 	int currentYear = ptm->tm_year+1900;
 	String formattedTime = timeClient.getFormattedTime();
 	return String(monthDay) + "-" + String(currentMonth) + "-" +
  String(currentYear) + " " + formattedTime;
}

// define the dht11 sensor
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float sensorTemp = 0;
float sensorHum = 0;

// Here LED is uses as HEATER and DC MOTOR is uses as Cooler.
Servo servo;

// Setting the range of the desire tempersture
float MaxTemp = 35; // Room temp [20-35]

// You should get Auth Token in the Blynk App.
char Bauth[] = BLYNK_AUTH_TOKEN;

void setup() {
  // Connect to Blynk device
   Blynk.begin(Bauth, WIFI_SSID, WIFI_PASSWORD);

  // put your setup code here, to run once:
   Serial.begin(9600);

  // Initialize BME280 sensor
  initWiFi();
  timeClient.begin();

  // run dht sensor
  dht.begin();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

  //Set output for the heater and fan
  servo.attach(0);
  delay(2000);
}


void loop() {
  //Output temperature
  sensorTemp= dht.readTemperature();
  sensorHum= dht.readHumidity();
  Serial.println("Sensor Reading: ");
  Serial.print(sensorTemp); Serial.println(" C");
  Serial.print(sensorHum); Serial.println("%");
  

  // If sensorTemp > Maximum temperature and sensorHum<40 notify of risk of fire
   if (sensorTemp>MaxTemp){
    Serial.println("Beware of fire!");
    delay(2000);
  }
  // If sensorTemp < Minimum temperature then Heater will turn on
  else if(sensorTemp>MaxTemp && sensorHum<30){
    Serial.print("Temp is HIGH");
    Serial.println("There is fire in the store");
    Serial.println("Open EXIT doors!");
    
    servo.write(0);  
    delay(1000);
    // If sensorTemp > Minimum temperature then Heater will turn off
    
  }
  else if(sensorTemp<MaxTemp){
    Serial.print("Now Temp is OK!");
    Serial.println("Close EXIT doors!");
    servo.write(0);  
    delay(2000);
    } 
  else{
    Serial.println("Temp is NORMAL!");
    delay(1000);
  }

  delay(1000);
  Blynk.run();
  Blynk.virtualWrite(V0, sensorTemp);
  Blynk.virtualWrite(V2, sensorHum);

    // put your main code here, to run repeatedly:
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath=databasePath +"/"+  String(timestamp);

    json.set(tempPath.c_str(), String(sensorTemp));
    json.set(humPath.c_str(), String(sensorHum));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
