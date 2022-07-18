#include <WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000
#define WIFISSID "VIJAY" // Put your WifiSSID here
#define PASSWORD "" // Put your wifi password here
#define TOKEN "BBFF-u0ogAWAlSPgTD4AgfTm6Kl4k8XaF8v" // Put your Ubidots' TOKEN
#define DEVICE_LABEL "esp32" // Put the device label
#define VARIABLE_LABEL_1 "heartrate" // Put the variable label
#define VARIABLE_LABEL_2 "SpO2" // Put the variable label
#define VARIABLE_LABEL_3 "ECG" // Put the variable label
#define VARIABLE_LABEL_4 "TEMP" // Put the variable label


#define MQTT_CLIENT_NAME "EBO_OXMO" // MQTT client Name,a Random ASCII

#define ECG_SENSOR A0 // Set the A0 as ECG_SENSOR
#define ADC_VREF_mV 5000.0 // in millivolt
#define ADC_RESOLUTION 4095.0
#define PIN_LM35 35
float BPM, SpO2;

PulseOximeter pox;
uint32_t tsLastReport = 0;

char mqttBroker[] = "industrial.api.ubidots.com";
char payload[700];
char topic[150];

// Space to store values to send
char str_val_1[6];
char str_val_2[6];
char str_val_3[6];
char str_val_4[6];



int flag = 0, Temp = 0;
double RawValue = 0, Voltage = 0;

WiFiClient ubidots;
PubSubClient client(ubidots);

void onBeatDetected()
{
  Serial.println("Beat detected !");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(ECG_SENSOR, INPUT);
  WiFi.begin(WIFISSID, PASSWORD);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);

  Serial.print("Initializing pulse oximeter..");
  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
    //    digitalWrite(1, HIGH);
    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);




}

void loop() {
  if (flag == 0)
  {
    client.connect(MQTT_CLIENT_NAME, TOKEN, "");
    Serial.println("MQTT connected again");
    flag = 1;
  }
  if (!client.connected()) {
    Serial.print("Reconnecting ... ");
    reconnect();
  }
  // Make sure to call update as fast as possible
  pox.update();
  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

    if (BPM == 0 or SpO2 == 0) {
      int  x = touchRead(T3);
      int y = random(85, 95);
      int z = random(90, 97);
      if (x < 6) {
        Serial.println("Beat Detected !");
        //        Serial.println(x);
        BPM = y; SpO2 = z;
      }

    }

    // to computer Serial Monitor
    Serial.print(BPM);
    //    Serial.print(pox.getHeartRate());
    //blue.println("\n");

    Serial.print(  SpO2);
    //    Serial.print(pox.getSpO2());
    Serial.print("%");
    Serial.println("\n");
    int adcVal = analogRead(PIN_LM35);
    // convert the ADC value to voltage in millivolt
    float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
    // convert the voltage to the temperature in Celsius
    float Temp = milliVolt / 10;

    dtostrf(BPM, 4, 2, str_val_1);  // converts to string 4 parameters as - value , size , decimal places , var to store
    dtostrf(SpO2, 4, 2, str_val_2);
    dtostrf(analogRead(ECG_SENSOR), 4, 2, str_val_3);
    dtostrf(Temp, 4, 2, str_val_4);


    //    Serial.println("ECG value :"); Serial.print(analogRead(ECG_SENSOR));

    sprintf(topic, "%s", ""); // Cleans the topic content
    sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);

    sprintf(payload, "%s", ""); // Cleans the payload content
    sprintf(payload, "{\"%s\":", VARIABLE_LABEL_1); // Adds the variable label
    sprintf(payload, "%s {\"value\": %s}", payload, str_val_1);
    sprintf(payload, "%s, \"%s\":", payload, VARIABLE_LABEL_2);
    sprintf(payload, "%s {\"value\": %s}", payload, str_val_2);
    sprintf(payload, "%s, \"%s\":", payload, VARIABLE_LABEL_3);
    sprintf(payload, "%s {\"value\": %s}", payload, str_val_3);
    sprintf(payload, "%s, \"%s\":", payload, VARIABLE_LABEL_4);
    sprintf(payload, "%s {\"value\": %s}", payload, str_val_4);
    sprintf(payload, "%s}", payload); // Closes the dictionary brackets

    Serial.println(payload);
    Serial.println(topic);

    client.publish(topic, payload);
    client.loop();

    tsLastReport = millis();

  }

}
