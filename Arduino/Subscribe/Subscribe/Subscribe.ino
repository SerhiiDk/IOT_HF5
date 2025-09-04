#include <Arduino.h>

#include "secrets.h"
#include "MessageType.h"

#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <ArduinoUniqueID.h>
#include <rgb_lcd.h>
#include <Arduino_JSON.h>

char ssid[] = SECRET_SSID;   
char pass[] = SECRET_PASS; 
char api[] = SECRET_API; 
int apiPort = SECRET_API_PORT;
char broker[] = SECRET_BROKER;
int brokerPort = SECRET_BROKER_PORT;

WiFiClient client;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

char alarmTopic[]  = "Alarm";
int alarmLength;
int updateSettingInterval;
rgb_lcd lcd;

int button_press = 0;
const int buttonPin = 5;
const int led = 4;
const int buzzerPin = 8;

unsigned long previousMillis = 0;
short ledInterval; 
short buzzerSound;


unsigned long start = 0;

// get device identifier
void GetIndentifier(){
  UniqueIDdump(Serial);
}

void GetSettings()
{
  mqttClient.poll();
  char request[200]; // Buffer for request route
  char* route = "/getSettings";
  // Make Post Request with route value 
  snprintf(request, sizeof(request),
          "GET %s HTTP/1.1", 
          route);

  if(WiFi.status()== WL_CONNECTED)
  { 
    int index = 0;
    // try to connect to client and check if client connected to server successfully
    if (client.connect(api, apiPort)) 
    {
      String host = strcat("Host: " , api);
      client.println(request);
      client.println(host);
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println(); // end HTTP request header

      bool isBody = false;
      String responseBody = "";

      while (client.connected()) 
      {
        while (client.available()) 
        {
          // read response line
          String line = client.readStringUntil('\n');
          line.trim(); // Remove leading/trailing whitespace and \r

          // check if body
          if (!isBody) {
            if (line.length() == 0) {
              isBody = true; // End of headers
            }
            continue;
          }
          // filter body
          if (line.length() <= 4 && line.indexOf("[") == -1 && line.indexOf("{") == -1) 
          {
            continue;
          }
          responseBody += line;
        }
      }
      client.stop();
      JSONVar result = JSON.parse(responseBody);

      // Check if parsing was successful
      if (JSON.typeof(result) != "array") 
      {
        return;
      }

      for (int i = 0; i < result.length(); i++)
      {
        // Access first object in array
        JSONVar obj = result[i];

        // Extract values
        String name = (const char*)obj["name"];
        String value = (const char*)obj["value"];
        if(name == "AlarmLength")
        {
          alarmLength = value.toInt();
        }
        else if (name == "LedInterval")
        {
           ledInterval = (value + "00").toInt();
        }
        else if (name == "BuzzerSound")
        {
           buzzerSound = value.toInt();
        }
        else if(name == "UpdateSettingInterval")
        {
          updateSettingInterval = (value + "000").toInt();
        } 
      }
    }
    else
    {
      // Serial.println("Error connecting to server");
    }
  }
  else{
    // Serial.println("Error");
  } 
}

void DisplayMessage(char message[], MessageType type)
{
  lcd.clear();
  switch (type)
  {
    case MessageType::NoAlarm:
        lcd.setRGB(0, 255, 0);
        lcd.print(message);
      break;
    case MessageType::Warning:
      break;
    case MessageType::Alarm:
        lcd.setRGB(255, 0, 0);
        lcd.print(message);
      break;
    default:
      break;
  }
}

void RunAlarm() {
  DisplayMessage("Alarm", MessageType::Alarm);
  int count = 0;
  previousMillis = millis(); // initialize previousMillis before loop starts

  while (count < alarmLength) {
    tone(buzzerPin, buzzerSound);  // start tone
    
    unsigned long currentMillis = millis();
    button_press = digitalRead(buttonPin);  // update button state here

    if (currentMillis - previousMillis >= ledInterval) {
      previousMillis = currentMillis;
      digitalWrite(led, !digitalRead(led)); // toggle LED
      count++;
    }

    if (button_press == HIGH) {
      digitalWrite(led, LOW);
      noTone(buzzerPin);  // stop tone when button pressed
      DisplayMessage("No Alarm", MessageType::NoAlarm);
      break;
    }
  }

  DisplayMessage("No Alarm", MessageType::NoAlarm);
  noTone(buzzerPin);  // stop tone when alarm ends
  digitalWrite(led, LOW); // turn off LED at the end
}

void onMqttMessage(int messageSize) 
{
  // we received a message from subscription
  RunAlarm();
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //Led port settings
  pinMode(led, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  lcd.begin(16, 2);
  DisplayMessage("No alarm", MessageType::NoAlarm);
  
  while (!Serial)
  {
    //no serial port
  }

  GetIndentifier();
  // attempt to connect to Wifi network:
  while (WiFi.begin(ssid, pass) != WL_CONNECTED)
  {
      //can not connect to WIFI
  }
  GetSettings();

  if (!mqttClient.connect(broker, brokerPort))
  {
    while (1);
  }
  // subscribe to a topic
  mqttClient.subscribe(alarmTopic);
  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);
  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);
}

void loop() 
{
  mqttClient.poll();

  unsigned long current = millis();

  if (current - start > updateSettingInterval)
  {
    GetSettings();
    start = current;
  }
}
