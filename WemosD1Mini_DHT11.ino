#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
 
// start telnet server (do NOT put in setup())
const uint16_t aport = 23; // standard port
WiFiServer TelnetServer(aport);
WiFiClient TelnetClient;

// replace with your channel’s thingspeak API key and your SSID and password
String apiKey = "HZC7ZLCMHRNHR2RS";
const char* ssid = "AZ_ZAHRA_PELNI"; //"My ASUS"; "AZ_ZAHRA_PELNI";
const char* password = "azzahra4579";
const char* server = "api.thingspeak.com";

#define DHTPIN D1
#define DHTTYPE DHT11 
 
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

int t, t1, h, h1 = 0;
bool t_stat, h_stat = false;
int minSecsTolerance = 10000; // 1sec = 1000ms, 1min = 60000ms
int minReadingInterval = 300000; // 1min = 60000ms, suggest 5min = 300000ms

void setup() 
{
  Serial.begin(9600);
  
  TelnetServer.begin();
  TelnetServer.setNoDelay(true);
 
  dht.begin();
  Serial.println();
  Serial.println();
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
 
void loop() 
{
  ArduinoOTA.handle();

  reading();
  delay(minReadingInterval);
}

void reading(){
  t = dht.readTemperature();
  h = dht.readHumidity();
  delay(minSecsTolerance);
  t1 = dht.readTemperature();
  h1 = dht.readHumidity();

  if (!t_stat){
    if (t==t1){
      t_stat = true;
    }
  }
  if (!h_stat){
    if (h==h1){
      h_stat = true;
    }
  }

  if (t_stat==true && h_stat==true){
    update_cloud(t, h);
    
    t_stat = false;
    h_stat = false;
  }else{
    reading();
  }
}

void Debug(String output) {
  if (!TelnetClient)  // otherwise it works only once
        TelnetClient = TelnetServer.available();
  if (TelnetClient.connected()) {
    TelnetClient.println(output);
  }  
}

void update_cloud(int t, int h){
  Serial.print("Update to Cloud => temp: " + String(t) + ", hum: " + String(h));
  Debug("Update to Cloud => temp: " + String(t) + ", hum: " + String(h));
  if (client.connect(server,80)) {
    String postStr = apiKey;
    postStr +="&field1="; postStr += String(t);
    postStr +="&field2="; postStr += String(h);
    postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println(" (Success)");
    Debug(" (Success)");
  } else {
    Serial.println(" (Failed)");
    Debug(" (Failed)");
  }
  client.stop();
}
