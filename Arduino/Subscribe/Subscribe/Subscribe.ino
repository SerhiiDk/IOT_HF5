#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include "arduino_secrets.h"
#include <ArduinoUniqueID.h>
#include "MessageType.h"
#include <rgb_lcd.h>
#include <Arduino_JSON.h>

char ssid[] = SECRET_SSID;   
char pass[] = SECRET_PASS; 
char api[] = SECRET_API; 
int apiPort = SECRET_API_PORT;
char broker[] = SECRET_BROKER;
int brokerPort = SECRET_BROKER_PORT;

WiFiClient client;
MqttClient mqttClient(client);

char alarmTopic[]  = "Alarm";
int alarmLength;

rgb_lcd lcd;

int button_press = 0;
const int buttonPin = 5;
const int led = 4;

unsigned long previousMillis = 0;
long loopInterval; 

void GetIndentifier(){
  UniqueIDdump(Serial);
  // for debuging
	// Serial.print("UniqueID: ");
	for (size_t i = 0; i < UniqueIDsize; i++)
	{
		if (UniqueID[i] < 0x10)
    {
			// Serial.print("0");
    }
		// Serial.print(UniqueID[i], HEX);
	}
}


void GetSettings()
{
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
      // for debuging
      // Serial.println("Response body");
      // Serial.println(responseBody);
      JSONVar result = JSON.parse(responseBody);

      // Check if parsing was successful
      if (JSON.typeof(result) != "array") 
      {
        // Serial.println("Failed to parse JSON or not an array.");
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
        else if (name = "loopInterval")
        {
           loopInterval = (value + "00").toInt();
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


void RunAlarm(){
  DisplayMessage("Alarm", MessageType::Alarm);
  int count = 0;
  while(count < alarmLength)
  {
    unsigned long currentMillis = millis();

    button_press = digitalRead(buttonPin);

    if (currentMillis - previousMillis >= loopInterval) 
    {
      previousMillis = currentMillis;
      digitalWrite(led, !digitalRead(led)); 
      count++;
    }
    if(button_press == 1)
    {
      digitalWrite(led, LOW);
      DisplayMessage("No Alarm", MessageType::NoAlarm);
      return;
    } 
  }
  // Serial.println("Alarm is stopped");
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //Led port settings
  pinMode(led, OUTPUT);
  pinMode(buttonPin, INPUT);

  lcd.begin(16, 2);
  DisplayMessage("No alarm", MessageType::NoAlarm);
  
  while (!Serial) {
    // Serial.print("No Serial ");
    ; // wait for serial port to connect. Needed for native USB port only
  }

  GetIndentifier();

  // attempt to connect to Wifi network:
  // Serial.print("Attempting to connect to SSID: ");
  // Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    // Serial.print(".");
    delay(5000);
  }
  GetSettings();
  // Serial.println("You're connected to the network");
  // Serial.println();

  // Serial.print("Attempting to connect to the MQTT broker: ");
  // Serial.println(broker);

  if (!mqttClient.connect(broker, brokerPort)) {
    // Serial.print("MQTT connection failed! Error code = ");
    // Serial.println(mqttClient.connectError());
    while (1);
  }

  // Serial.println("You're connected to the MQTT broker!");

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);
  // Serial.print("Subscribing to topic: ");
  // Serial.println(alarmTopic);
  
  // subscribe to a topic
  mqttClient.subscribe(alarmTopic);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);

  // Serial.print("Topic: ");
  // Serial.println(alarmTopic);
}

void loop() {
  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alive which avoids being disconnected by the broker
  mqttClient.poll();
}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  // for debuging
  // Serial.println("Received a message with topic '");
  // Serial.print(mqttClient.messageTopic());
  // Serial.print("', length ");
  // Serial.print(messageSize);
  // Serial.println(" bytes:");

  String message = "";

  while (mqttClient.available()) {
    message += (char)mqttClient.read();
  }

  // Optionally print it
  // Serial.println(message);
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    // Serial.print((char)mqttClient.read());
  }
  
  RunAlarm();
}