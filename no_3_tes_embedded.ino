#include <ModbusMaster.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Library for creating JSON objects

// WiFi credentials
const char* ssid = "ssid_test";
const char* password = "password_test"; 

// MQTT broker details
const char* mqttServer = "192.168.1.100"; // Mosquitto IP
const int mqttPort = 1883;
const char* mqttClientID = "node1";
const char* mqttTopic = "DATA/LOCAL/SENSOR/PANEL_1";

// Create WiFi and MQTT Client objects
WiFiClient espClient;
PubSubClient client(espClient);

float v_read, i_read, p_read, temp_read;
int fan_stat;
float v_raw[2], i_raw[2], p_raw[2], temp_raw[2];
uint8_t res_v, res_i, res_p, res_temp;
float room_temp = 27.0;

unsigned long prev_mil;
unsigned long curr_mil;
int interval = 1000;

ModbusMaster pm_v, pm_i, pm_p, temp_sens;

float f_2uint_float(unsigned int uint1, unsigned int uint2);
void setupWiFi();
void reconnectMQTT();
String createJsonPayload();

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);

  setupWiFi();
  client.setServer(mqttServer, mqttPort); //set MQTT server and port

  pm_v.begin(1, Serial1);
  pm_i.begin(1, Serial1);
  pm_p.begin(1, Serial1);
  temp_sens.begin(1, Serial2);

  pinMode(15, OUTPUT); //relay fan
}

void loop() {
  if (!client.connected())  ///reconnecting to MQTT
  {
    reconnectMQTT();
  }

  //read and send sensor data begins
  curr_mil = millis();
  if (curr_mil - prev_mil >= interval)
  {
    prev_mil = curr_mil;
    res_v = pm_v.readInputRegisters(0x00, 2); //address v 30000 
    res_i = pm_i.readInputRegisters(0x10, 2); //address i 30016
    res_p = pm_p.readInputRegisters(0x18, 2); //address p 30024
    res_temp = temp_sens.readInputRegisters(0x03E8, 2); //address temp C 301001

    if (res_v == pm_v.ku8MBSuccess) 
    {
      v_raw[0] = pm_v.getResponseBuffer(0); 
      v_raw[1] = pm_v.getResponseBuffer(1); 
      v_read = f_2uint_float(v_raw[0], v_raw[1]);
    }
    if (res_i == pm_i.ku8MBSuccess) 
    {
      i_raw[0] = pm_i.getResponseBuffer(0); 
      i_raw[1] = pm_i.getResponseBuffer(1); 
      i_read = f_2uint_float(i_raw[0], i_raw[1]);
    }
    if (res_p == pm_p.ku8MBSuccess) 
    {
      p_raw[0] = pm_p.getResponseBuffer(0); 
      p_raw[1] = pm_p.getResponseBuffer(1); 
      p_read = f_2uint_float(p_raw[0], p_raw[1]);
    }
    if (res_temp == temp_sens.ku8MBSuccess) 
    {
      temp_raw[0] = pm_v.getResponseBuffer(0); 
      temp_raw[1] = pm_v.getResponseBuffer(1); 
      temp_read = f_2uint_float(temp_raw[0], temp_raw[1]);
    }
    
    if(temp_read >= (room_temp + ((2/100) * room_temp) ) )
    {
      fan_stat = 1;
    }
    else
    {
      fan_stat = 0;
    }
    digitalWrite(15, fan_stat);

    String payload = createJsonPayload();     // Create the JSON payload
    client.publish(mqttTopic, payload.c_str());   // Publish the JSON payload to the MQTT topic
  }

}

float f_2uint_float(unsigned int uint1, unsigned int uint2) {    // reconstruct the float from 2 unsigned integers

  union f_2uint {
    float f;
    uint16_t i[2];
  };

  union f_2uint f_number;
  f_number.i[0] = uint1;
  f_number.i[1] = uint2;

  return f_number.f;

}

void setupWiFi() {
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Try to connect
    if (client.connect(mqttClientID)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

String createJsonPayload() {
  // Create a JSON object
  StaticJsonDocument<200> doc;

  // Add the status and deviceID fields
  doc["status"] = "OK";
  doc["deviceID"] = "yourname";

  // Add the data field with voltage, current, power, fan status, and temperature
  JsonObject data = doc.createNestedObject("data");
  data["v"] = String(v_read);
  data["i"] = String(i_read);
  data["pa"] = String(p_read);
  data["fan"] = fan_stat;
  data["temp"] = String(temp_read);

  // Serialize JSON to String and return it
  String output;
  serializeJson(doc, output);
  return output;
}
