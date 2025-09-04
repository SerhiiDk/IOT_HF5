#include <Arduino.h>

#include "secrets.h"
#include "MessageType.h"

#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <rgb_lcd.h>
#include <I2CKeyPad.h>
#include <Wire.h>
#include <Keypad.h>
#include <ArduinoUniqueID.h>
#include <Arduino_JSON.h>

char response[16];  // to hold "valid" or other short response
int responseIndex = 0;

// Your WiFi credentials (SSID and password)
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;  
char api[] = SECRET_API; 
int apiPort = SECRET_API_PORT;
char broker[] = SECRET_BROKER;
int brokerPort = SECRET_BROKER_PORT;

char cardScanTopic[] = "CardScan";
char alarmTopic[] = "Alarm";

char deleteKeySymbol = 'C';
char enterKeySymbol = 'E';

bool keyPadActive = false;
WiFiClient client; // WIFI Client

char password[10];
int keyPadPresses = 0;
int passwordAttempt = 0;
int passwordAttemptLimit;
int shortWait;
int longWait;
int updateSettingInterval;
char arduinoIdentifier[UniqueIDsize * 2 + 1];

rgb_lcd lcd;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long start = 0;

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'0','F','E','D'}
};

byte rowPins[ROWS] = {2,3,4,5};  
byte colPins[COLS] = {6,7,8,9};

// For debuging 
// unsigned long loopCount;
// unsigned long startTime;

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void SendToTopic(char topicName[], String message)
{
  mqttClient.poll();
  mqttClient.beginMessage(topicName);
  mqttClient.print(message);
  mqttClient.endMessage();
} 

void DisplayLCDMessage(char message[], int red, int green, int blue)
{
  lcd.clear();
  lcd.setRGB(red, green, blue);
  lcd.print(message);
}

void DisplayMessage(char message[], MessageType type)
{
  lcd.clear();
  switch (type)
  {
    case MessageType::Success:
        DisplayLCDMessage(message, 0, 255, 0);
      break;
    case MessageType::Info:
        DisplayLCDMessage(message, 105, 105, 105);
      break;
    case MessageType::Warning:
        DisplayLCDMessage(message, 255, 255, 0);
      break;
    case MessageType::Error:
        DisplayLCDMessage(message, 255, 0, 0);
      break;
    default:
      break;
  }
}

// "live" timer
void WaitFor(unsigned long ms) 
{
  unsigned long start = millis();
  //do someting while waiting
  while (millis() - start < ms) 
  {

  }
}

void RunPasswordCheck(char text[]){
  int statusCodeStringStartPos = 9;
  int statusCodeStringEndPos = 12;

  char result[4]; // Status code 200/401
  int j = 0;

  // for getting status code
  for (int i = statusCodeStringStartPos; i < statusCodeStringEndPos; i++) 
  {
    result[j++] = text[i];
  }
  result[j] = '\0';

  if(strcmp(result, "200") == 0)
  {
    passwordAttempt = 0;
    DisplayMessage("Door is open", MessageType::Success);
    keyPadActive = false;
    WaitFor(shortWait);
    DisplayMessage("Scan card: ", MessageType::Info);
    keyPadPresses = 0;
    memset(password, 0, sizeof(password)); // reset password 
    return;
  }
  else
  {
    passwordAttempt++;
    
    if(passwordAttempt >= passwordAttemptLimit)
    {
      String message = strcat(arduinoIdentifier, ",Password Alarm");
      SendToTopic(alarmTopic,  message);
      DisplayMessage("Door is blocked ", MessageType::Error);
      passwordAttempt = 0;
      keyPadActive = false;
      WaitFor(longWait);
      DisplayMessage("Scan card: ", MessageType::Info);
      return;
    }

    DisplayMessage("Invalid Password", MessageType::Warning); //15 lcd 
    keyPadPresses = 0;
    memset(password, 0, sizeof(password)); // reset password
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

        if(name == "PasswordLimit")
        {
          passwordAttemptLimit = value.toInt();
        }
        else if(name == "ShortWait")
        {
            shortWait = (value + "000").toInt();
        }
        else if(name == "LongWait")
        {
            longWait = (value + "000").toInt();
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

void VerifyPassword()
{
  if (strcmp(password, "") == 0)
  {
    return;
  } 
  
  char request[200]; // Buffer for request route
  char* route = "/verifyPassword";
  // Make Post Request with route value 
  snprintf(request, sizeof(request),
          "POST %s HTTP/1.1", 
          route);

  JSONVar body;
  body[0] = arduinoIdentifier;
  body[1] = password;

  String jsonBody = JSON.stringify(body);
  
  if (JSON.typeof(body) == "undefined") 
  {
    //Serial.println("Parsing input failed!");
    return;
  }
   // Check wifi connection 
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
      client.print("Content-Length: ");
      client.println(jsonBody.length()); // If Length not correct, it will give a error (400), 
      client.println("Connection: close");
      client.println(); // end HTTP request header
      client.print(jsonBody);

      while (client.connected()) 
      {
        while (client.available()) 
        {
          char c = client.read();

          if (index < sizeof(response) - 1) 
          { 
              response[index] = c;
              response[index + 1] = '\0'; 
          }
          index++;
        }
      }
      client.stop();
      RunPasswordCheck(response);
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

void GetIndentifier() {
  for (size_t i = 0; i < UniqueIDsize; i++) 
  {
    sprintf(&arduinoIdentifier[i * 2], "%02X", UniqueID[i]);
  }
  arduinoIdentifier[UniqueIDsize * 2] = '\0';
}

void DisplayPasswordSymbol()
{
  lcd.setCursor(0, 2);
  int length = strlen(password); 

  for (int i = 0; i < length; i++) 
  {
    lcd.print("*");
  } 
}

void DeleteLastPasswordSymbol()
{
  int length = strlen(password); 

  if(length == 0)
  {
    lcd.setCursor(0, 2);
    lcd.print(" ");
  }
  else
  {
    lcd.setCursor(keyPadPresses, 2);
    lcd.print(" ");
  }
}

void AddCharToPassword(char symbol)
{
  password[keyPadPresses] = symbol;
  keyPadPresses++;
  DisplayPasswordSymbol();
}

void onMqttMessage(int messageSize) {
  // for debuging, we received a message
  String message = "";

  while (mqttClient.available()) 
  {
    message += (char)mqttClient.read();
  }

  keyPadActive = true;
  DisplayMessage("Enter password: ", MessageType::Success);
}

void SubsribeOnTopic(){
  mqttClient.subscribe(cardScanTopic);
  mqttClient.onMessage(onMqttMessage);
  DisplayMessage("Scan card: ", MessageType::Info);
}

void setup() 
{
  Serial.begin(9600); 

  GetIndentifier();

  Wire.begin();
  Wire.setClock(400000);

  lcd.begin(16, 2);
  DisplayMessage("Booting", MessageType::Info);

  // Connect to WiFi
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) 
  {
    // can not connect to WIFI
  }

  GetSettings();

  if (!mqttClient.connect(broker, brokerPort)) 
  {
    while (1);  // Stop here if connection failed
  }

  SubsribeOnTopic();
}


void loop() 
{
  // For debuging ()
  //loopCount++;
  mqttClient.poll();
    
  unsigned long current = millis();

  // run after giveb time  (getting from DB) minutes to uptade setting
  if (current - start > updateSettingInterval)
  {
    GetSettings();
    start = current;
  }

  if (kpd.getKeys())
  {
    if(keyPadActive == true)
    {
      for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
      {
        if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
        {
          switch (kpd.key[i].kstate) 
          {  // Report active key state : PRESSED, HOLD
            case PRESSED:
              // delete latst char/press
              if(kpd.key[i].kchar == deleteKeySymbol)
              {
                if(strlen(password) == 0)
                {
                  return;
                }
                password[keyPadPresses-1] = '\0';
                keyPadPresses--;
                DeleteLastPasswordSymbol();
              }
              // submit password
              else if(kpd.key[i].kchar == enterKeySymbol)
              {
                  VerifyPassword();
                  keyPadPresses = 0;
                  memset(password, 0, sizeof(password));
              }
              // add char/press to password
              else
              {
                AddCharToPassword(kpd.key[i].kchar);
              }
              break;
          }        
        }
      }
    }
  }
}
