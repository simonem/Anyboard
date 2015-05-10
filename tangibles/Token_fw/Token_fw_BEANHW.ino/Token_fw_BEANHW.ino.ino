// Token's firmware working copy
// Code based on RFDUINO hardware, uses C char arrays 

#include <ArduinoJson.h>

//Test JSON strings
char OFF[]= "{\"device\":\"LED\",\"event\":\"off\"}";
char GREEN[] = "{\"device\":\"LED\",\"event\":\"on\",\"color\":\"green\"}";
char RED[] = "{\"device\":\"LED\",\"event\":\"on\",\"color\":\"red\"}";
char BLUE[] = "{\"device\":\"LED\",\"event\":\"on\",\"color\":\"blue\"}";
char WHITE[] = "{\"device\":\"LED\",\"event\":\"on\",\"color\":\"white\"}";

void setup() {
  Serial.begin(57600);  
  Serial.setTimeout(25);
 
  // Test LED on startup
  parseJSON(GREEN);
  delay(300);
  parseJSON(RED);
  delay(300);
  parseJSON(BLUE);
  delay(300);
  parseJSON(WHITE);
  delay(300);
  parseJSON(OFF);
  delay(300);
  
  //Initialise bluetooth

}

void loop() {
String payload;
if(Serial.available()>0)
{
  payload = Serial.readString();
  // say what you got:
  Serial.print("I received: ");
  Serial.println(payload);
  parseJSON(payload);
  Serial.println("JSON succesfully parsed");
}
}





//Parses JSON messages
void parseJSON(String payload) {
  char sel;
  char json[200]; 
  payload.toCharArray(json, 50);
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);

 // Serial.print("DEBUG: "); Serial.println(json);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  const char* device = root["device"];
  const char* event = root["event"];
  const char* color = root["color"];

  if(strcmp(device, "LED") == 0)
  {
    if(strcmp(event, "on") == 0)
    {
     sel = color[0];
     ledON(sel);
    } 
    if (strcmp(event, "off") == 0)
    {
      ledOFF();
    }
  }
}

// Turns the LED off
void ledOFF(){
  Bean.setLed(0, 0, 0);
}

// Turns on the LED on a specific color: r=red, g=gree, osv..
void ledON(char sel){
  switch(sel)
  {
        case 'r':
    {
        Bean.setLed(255, 0, 0);
        Serial.println("DEBUG: LED RED ON");
        break;
    }
        case 'g':
    {
        Bean.setLed(0, 255, 0);
        Serial.println("DEBUG: LED GREEN ON");
        break;
    }
        case 'b':
    {
        Bean.setLed(0, 0, 255);
        Serial.println("DEBUG: LED BLUE ON");
        break;
    }
        case 'w':
    {
        Bean.setLed(255, 255, 255);
        Serial.println("DEBUG: LED WHITE ON");
        break;
    }    
  }
}
