#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
String apiKey = "SSU3AUBCRN9PKI96";

const char *server = "snms-100e5.web.app";
void sendData(String);

RF24 radio(4, 5);
const uint64_t address = 0xF0F0F0F0E1LL;
WiFiManager wm;

struct MyVariable
{
  byte soilmoisturepercent;
  byte nitrogen;
  byte phosphorous;
  byte potassium;
  byte temperature;
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
  if (recvData())

  {
    // create json string from struct
    String postStr = "{\"soilmoisturepercent\":";
    postStr += variable.soilmoisturepercent;
    postStr += ",\"nitrogen\":";
    postStr += variable.nitrogen;
    postStr += ",\"phosphorous\":";
    postStr += variable.phosphorous;
    postStr += ",\"potassium\":";
    postStr += variable.potassium;
    postStr += ",\"temperature\":";
    postStr += variable.temperature;
    postStr += "}";
    Serial.print("Posting: ");
    Serial.println(postStr);

    sendData(postStr);
    // Serial.println("Data Received:");

    // Serial.print("Soil Moisture: ");
    // Serial.print(variable.soilmoisturepercent);
    // Serial.println("%");

    // Serial.print("Nitrogen: ");
    // Serial.print(variable.nitrogen);
    // Serial.println(" mg/kg");
    // Serial.print("Phosphorous: ");
    // Serial.print(variable.phosphorous);
    // Serial.println(" mg/kg");
    // Serial.print("Potassium: ");
    // Serial.print(variable.potassium);
    // Serial.println(" mg/kg");

    // Serial.print("Temperature: ");
    // Serial.print(variable.temperature);
    // Serial.println("*C");

    // Serial.println();

    // if (client.connect(server, 80))
    // {
    //   String postStr = apiKey;
    //   postStr += "&field1=";
    //   postStr += String(variable.soilmoisturepercent);
    //   postStr += "&field2=";
    //   postStr += String(variable.nitrogen);
    //   postStr += "&field3=";
    //   postStr += String(variable.phosphorous);
    //   postStr += "&field4=";
    //   postStr += String(variable.potassium);
    //   postStr += "&field5=";
    //   postStr += String(variable.temperature);
    //   postStr += "\r\n\r\n\r\n\r\n\r\n";

    //   client.print("POST /update HTTP/1.1\n");
    //   client.print("Host: api.thingspeak.com\n");
    //   client.print("Connection: close\n");
    //   client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    //   client.print("Content-Type: application/x-www-form-urlencoded\n");
    //   client.print("Content-Length: ");
    //   client.print(postStr.length());
    //   client.print("\n\n");
    //   client.print(postStr);
    //   delay(1000);
    //   Serial.println("Data Sent to Server");
    // }
    // client.stop();
  }
}

void sendData(String data)
{

  Serial.println("Sending data to server");
  if (client.connect(server, 80))
  {
    client.println("POST /update HTTP/1.1");
    client.println("Host: snms-100e5.web.app/api/v1/snms/add");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(data.length()));
    client.println();
    client.println(data);
    Serial.println("Data Sent to Server");
  }
  client.stop();
}