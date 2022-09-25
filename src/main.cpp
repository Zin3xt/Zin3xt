#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <time.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyBaC-nDYGGweVndczqJbwantLzacnR5AyI"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "vzichri@gmail.com"
#define USER_PASSWORD "Killnext21"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://console.firebase.google.com/u/0/project/iot-smns/database/iot-smns-default-rtdb/data/~2F"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String parentPath;
String uid;
String databasePath;
const char *ntpServer = "pool.ntp.org";

int timestamp;
FirebaseJson json;

String tempPath = "/temperature";
String humPath = "/humidity";
String nitroPath = "/nitrogen";
String phosPath = "/phosphorous";
String potasPath = "/potassium";
String timePath = "/timestamp";
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

// Function that gets current epoch time
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

RF24 radio(4, 5);
const uint64_t address = 0xF0F0F0F0E1LL;
WiFiManager wm;

struct MyVariable
{
  float soilmoisturepercent;
  float nitrogen;
  float phosphorous;
  float potassium;
  float temperature;
};
MyVariable variable;

WiFiClient client;
int timeout = 120; // seconds to run for

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  pinMode(0, INPUT_PULLUP);
  Serial.begin(115200);
  radio.begin();                     // Starting the Wireless communication
  radio.openReadingPipe(0, address); // Setting the address at which we will receive the data
  radio.setPALevel(RF24_PA_MIN);     // You can set this as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.startListening();            // This sets the module as receiver

  bool res;
  Serial.println("Receiver Started....");
  Serial.print("Connecting to ");
  res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap

  if (!res)
  {

    Serial.println("Failed to connect");

    ESP.restart();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  configTime(0, 0, ntpServer);

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
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

int recvData()
{
  if (radio.available())
  {
    radio.read(&variable, sizeof(MyVariable));
    return 1;
  }
  return 0;
}

void loop()
{

  if (digitalRead(0) == LOW)
  {
    // reset settings - for testing
    // wm.resetSettings();

    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("OnDemandAP"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(variable.soilmoisturepercent));
    json.set(humPath.c_str(), String(variable.temperature));
    json.set(nitroPath.c_str(), String(variable.nitrogen));
    json.set(phosPath.c_str(), String(variable.phosphorous));
    json.set(potasPath.c_str(), String(variable.potassium));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
