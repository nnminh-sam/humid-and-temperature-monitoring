#include <ESP8266WiFi.h>        // * Wifi connection for ESP8266 board
#include "mqtt_secrets.h"       // * Credential for MQTT and Wifi connection
#include <DHT.h>                // * Library for DHT11
#include <ESP8266HTTPClient.h>  // * HTTP Client library for ESP8266 board
#include <WiFiClientSecure.h>   // * Secure client for HTTPS requests
#include <ArduinoJson.h>

const String apiKey = CHANNEL_WRITE_API_KEY;
const String thingSpeakServer = "http://api.thingspeak.com/update";
const String emailServer = "https://helpless-anette-nnminh-haise-7aa662f8.koyeb.app/api/v1/emails/notify";
const String thingSpeakServerReadTemperatureThresholdUrl = "https://api.thingspeak.com/channels/2711831/fields/3.json?api_key=O0QM4ZXS290A7SUM&results=1";
const String thingSpeakServerReadHumidThresholdUrl = "https://api.thingspeak.com/channels/2711831/fields/4.json?api_key=O0QM4ZXS290A7SUM&results=1";
const String thingSpeakReadFeedUrl = "https://api.thingspeak.com/channels/2711831/feeds.json?api_key=O0QM4ZXS290A7SUM&results=1";
const String localServer = "http://159.223.76.203";
const String localChannelId = "672adc592317e2b5f060181c";
const String localServerWriteKey = "e3699a5657b5d2e25525259b32c2e5225af3cdd177e7e702fd9a56cb7d8911f1";
const String localServerReadKey = "8a197a16080c1b20d5a9e395eb42317f64e2b593c7175ffa1dde23c0bfcdbc9f";


#define DHTPIN D2         // * DHT11 Data pin
#define DHTTYPE DHT11     // * Sensor type
#define Relay D7          // * Relay control pin
#define DEFAULT_TEMP_THRESHOLD 30
#define DEFAULT_HUMID_THRESHOLD 60
DHT dht(DHTPIN, DHTTYPE); // * Initialize a DHT object with D2 data pin and DHT sensor type

#define RELAY_DURATION_IN_SEC 3

struct Data {
  float temperature;
  float humid;
};

Data thresholdData = {DEFAULT_TEMP_THRESHOLD, DEFAULT_HUMID_THRESHOLD};

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(SECRET_WIFI_NAME, SECRET_WIFI_PASSWORD); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }
  Serial.println("Connected to WiFi");

  pinMode(Relay, OUTPUT); // * Configuring Relay as output of ESP8266 board
  dht.begin();
}

// void getDataFromServer() {
//   WiFiClientSecure httpsClient;
//   httpsClient.setInsecure();
//   HTTPClient httpClient;
  
//   const String localServerThresholdUrl = localServer + "/api/v1/feeds/thresholds?channel-id=" + localChannelId + "&read-key=" + localServerReadKey;
//   httpClient.begin(httpsClient, localServerThresholdUrl);
//   int httpCode = httpClient.GET();

//   if (httpCode <= 0) {
//     Serial.println("Failed to get data from server. Error: " + String(httpClient.errorToString(httpCode).c_str()));
//     httpClient.end();
//   }

//   String payload = httpClient.getString();
//   Serial.println("Data received from server:");
//   Serial.println(payload);

//   const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1) + 340;
//   DynamicJsonDocument doc(capacity);
//   DeserializationError error = deserializeJson(doc, payload);

//   if (error) {
//     Serial.print("Failed to parse JSON: ");
//     Serial.println(error.c_str());
//     return;
//   }

//   float tempThreshold = doc["data"]["temperatureThreshold"].isNull() ? -1 : doc["data"]["temperatureThreshold"].as<float>();
//   float humidThreshold = doc["data"]["humidityThreshold"].isNull() ? -1 : doc["data"]["humidityThreshold"].as<float>();
//   Serial.println("temp threshold:" + String(tempThreshold));
//   Serial.println("humid threshold:" + String(humidThreshold));

//   if (tempThreshold != -1) {
//     Serial.println("Temperature Threshold (field3): " + String(tempThreshold));
//     thresholdData.temperature = tempThreshold;
//   } else {
//     Serial.println("Temperature threshold data (field3) is null.");
//     thresholdData.temperature = DEFAULT_TEMP_THRESHOLD;
//   }

//   if (humidThreshold != -1) {
//     Serial.println("Humidity Threshold (field4): " + String(humidThreshold));
//     thresholdData.humid = humidThreshold;
//   } else {
//     Serial.println("Humidity threshold data (field4) is null.");
//     thresholdData.humid = DEFAULT_HUMID_THRESHOLD;
//   }
  
//   httpClient.end();
// }

void getDataFromServer() {
  WiFiClient httpClient; // Use WiFiClient for HTTP connections
  HTTPClient http;

  // Construct the URL for the local server
  const String localServerThresholdUrl = localServer + "/api/v1/feeds/thresholds?channel-id=" + localChannelId + "&read-key=" + localServerReadKey;
  
  // Begin the HTTP connection
  http.begin(httpClient, localServerThresholdUrl);
  int httpCode = http.GET(); // Send the GET request

  if (httpCode > 0) { // Check if the request was successful
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Data received from server:");
      Serial.println(payload);

      const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(1) + 200; // Adjusted memory allocation
      DynamicJsonDocument doc(capacity);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }

      // Extract threshold values
      JsonObject data = doc["data"];
      float tempThreshold = data["temperatureThreshold"].is<float>() ? data["temperatureThreshold"].as<float>() : -1;
      float humidThreshold = data["humidityThreshold"].is<float>() ? data["humidityThreshold"].as<float>() : -1;

      // Log and assign values, falling back to defaults if needed
      if (tempThreshold != -1) {
        Serial.println("Temperature Threshold: " + String(tempThreshold));
        thresholdData.temperature = tempThreshold;
      } else {
        Serial.println("Temperature threshold data is null. Using default.");
        thresholdData.temperature = DEFAULT_TEMP_THRESHOLD;
      }

      if (humidThreshold != -1) {
        Serial.println("Humidity Threshold: " + String(humidThreshold));
        thresholdData.humid = humidThreshold;
      } else {
        Serial.println("Humidity threshold data is null. Using default.");
        thresholdData.humid = DEFAULT_HUMID_THRESHOLD;
      }
    } else {
      Serial.println("Error in response. HTTP code: " + String(httpCode));
    }
  } else {
    Serial.println("Failed to get data from server. Error: " + String(http.errorToString(httpCode).c_str()));
  }

  http.end(); // End the HTTP connection
}

void sendDataToThingSpeakServer(float temperature, float humid) {
  WiFiClient wifiClient;
  HTTPClient http;

  const String thingSpeakHttpPostRequest = thingSpeakServer + "?api_key=" + apiKey + "&field1=" + String(temperature) + "&field2=" + String(humid) + "&field3=" + String(thresholdData.temperature) + "&field4=" + String(thresholdData.humid);
  http.begin(wifiClient, thingSpeakHttpPostRequest);

  int thingSpeakHttpCode = http.POST("");
  if (thingSpeakHttpCode > 0) {
    String payload = http.getString();
    Serial.println("ThingSpeak Server request sent successfully.");
    Serial.println("Server response code: " + String(thingSpeakHttpCode));
  } else {
    Serial.println("Failed to send HTTP POST request. Error: " + String(http.errorToString(thingSpeakHttpCode).c_str()));
  }

  http.end();

  const String localServerGetRequest = localServer + "/api/v1/feeds/create?channel-id=" + localChannelId + "&write-key=" + localServerWriteKey + "&temperature=" + String(temperature) + "&humidity=" + String(humid) + "&temperature-threshold=" + String(thresholdData.temperature) + "&humidity-threshold=" + String(thresholdData.humid);
  http.begin(wifiClient, localServerGetRequest);

  int localServerHttpGet = http.GET();
  if (localServerHttpGet > 0) {
    String payload = http.getString();
    Serial.println("Local Server request sent successfully.");
    Serial.println("Server response code: " + String(localServerHttpGet));
  } else {
    Serial.println("Failed to send HTTP POST request. Error: " + String(http.errorToString(localServerHttpGet).c_str()));
  }

  http.end();
}

void requestToSendEmail(float temperature, float humid) {
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient httpClientForSendingEmail;

  const String emailSubject = "Humid+and+Temperature+Alert";
  const String emailContent = "Temperature+=+" + String(temperature) + "+ Celsious |+Humid+=+" + String(humid) + "%";
  String mailingHttpsServerGetRequest = String(emailServer) + "?to=" + NOTIFICATION_RECEIVER + "&subject=" + emailSubject + "&content=" + emailContent;

  httpClientForSendingEmail.begin(httpsClient, mailingHttpsServerGetRequest);
  int mailingServiceHttpsCode = httpClientForSendingEmail.GET();
  if (mailingServiceHttpsCode > 0) {
    String getPayload = httpClientForSendingEmail.getString();
    Serial.println("Mailing Server request sent successfully.");
    Serial.println("Server response code: " + String(mailingServiceHttpsCode));
    Serial.println("Response payload: " + getPayload);
  } else {
    Serial.println("Failed to send HTTPS GET request. Error: " + String(httpClientForSendingEmail.errorToString(mailingServiceHttpsCode).c_str()));
  }

  httpClientForSendingEmail.end();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Cannot read data from DHT sensor!");
    return;
  }

  Serial.print("Humid: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");

  delay(2000);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Trying to reconnect...");
    return;
  }

  getDataFromServer();

  sendDataToThingSpeakServer(t, h);

  if (t >= thresholdData.temperature) {
    Serial.println("Turning on Relay");
    digitalWrite(Relay, LOW);
    delay(RELAY_DURATION_IN_SEC * 1000);
    digitalWrite(Relay, HIGH);
    Serial.println("Turned off Relay");

    requestToSendEmail(t, h);

    Serial.println("Waiting 2 seconds to send next request ...");
    delay(2000);
  } else {
    digitalWrite(Relay, HIGH);
  }
}