#include <WiFi.h>        // Include WIFi Library for ESP32
#include <WebServer.h>   // Include WebSwever Library for ESP32
#include <ArduinoJson.h> // Include ArduinoJson Library
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <WebSocketsServer.h> // Include Websocket Library
#include "SocketCAN.h"
#include "driver/twai.h"
#include "drive_mode.h"
#include "TX_RX.h"
#include "LittleFS.h"
#include <ESPAsyncWebServer.h>

#define ledpin 21 


int oil_warning_temp {120},
 oil_danger_temp {130},
            gear {1},
       startType {1},
        oil_temp {100},
      water_temp {100},      
    gearbox_temp {30},
    illumination {0},
      brightness {100},       
   lastDriveMode {19},       
startupDriveMode {19},
        modeData {0}, 
          rpmLSB {0},
          rpmMSB {0}, 
     throttleMSB {0}, 
     throttleLSB {0},
      shift_Mode {20},
     shift_light {0},
       selector1 {1},
       driveMode {19},
  shift_reminder {15},    
        rpm {2500},
        throttle {0},
         lastRPM {0},
              ID {0},
           speed {0},
        shiftRPM {6500},
        ID_all {0};

String mode;
//timers
long    lastSend {0},
        rpmTimer {0},
       pgmTimer1 {0},
       pgmTimer2 {0},
            wait {0},
        ECOTimer {0},
        emTimer1 {0},
        emTimer2 {0},
        emTimer3 {0},
        emTimer4 {0},
        emTimer5 {0},
        modeTImer{0},
      illumTimer {0},
      softAP_timer {0};
//flags
bool  startFromSetMode  {false}, //combined into pgmStart
               written  {false},
               pgmMode  {false}, //combined into start_pgm
                logger  {false},
            parkButton  {false},
          parkTimerSet  {false},
        lastParkButton  {false},
                wasPGM  {false},
           blockButton  {false},
               garbage  {false},
       startUpComplete  {false},
       gearDecrease     {false},
        rpmDecrease     {false},
      speedDecrease     {false},
        oilDecrease     {false},
      waterDecrease     {false},
           lightsOn     {false};

      const int dataRate = 100;

WiFiScanClass scan;
Settings settings;
Alerts alerts;
VehicleData CANdata;
WebServer socketServer(80); 
WebServer configServer(80);                             // create instance for web server on port "80"
WebSocketsServer webSocket = WebSocketsServer(81); // create instance for webSocket server on port"81"

const char* softAP_ssid = "DLC-1";
const char* softAP_password = "passphrase12345";

const char *ssid_1 = "Pixel_6886";  // Your SSID
const char *password_1 = "fart1979"; // Your Password

const char *ssid_2 = "KG2_IoT";  // Your SSID
const char *password_2 = "elise190"; // Your Password

const char *ssid_3 = "Patroness-2.4";  // Your SSID
const char *password_3 = "EngFr33d0m"; // Your Password

String ssid_saved;  // Your SSID
String password_saved; // Your Password

const char* config = "<html><body>" //html for wifi setup page
    "<h1>WiFi Configuration</h1>"
    "<form action='/config' method='post'>"
      "<label for='ssid'>SSID:</label>"
      "<input type='text' id='ssid' name='ssid'><br><br>"
      "<label for='password'>Password:</label>"
      "<input type='password' id='password' name='password'><br><br>"
      "<input type='submit' value='Connect'>"
    "</form>"
  "</body></html>";

u8_t clientID = 0;

int interval = 100;               // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired

String jsonString;      // Temporary storage for the JSON String






void update_webpage()
{
  JsonDocument doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();
  int gear1 = gear - 4;
  String str1 = "P";
  switch (gear1)
    {
      case -3:
        str1 = ("N");
        break;

      case -2:  
        str1 = ("R");
        break; 

      case -1 ... 0:  
        str1 = ("P");
        break;

      case 1 ... 8:
        str1 = String(gear1);
        break;
    }
  
//// Data////////
  object["RPM"]           = CANdata.rpm;
  object["Gear"]          = CANdata.gearString;
  object["oil_temp"]      = CANdata.oil_temp;
  object["water_temp"]    = CANdata.water_temp;
  object["gearbox_temp"]  = CANdata.gearbox_temp;
  object["speed"]         = CANdata.speed; 
  object["driveMode"]     = mode;
  object["ethContent"]    = CANdata.ethContent;
  object["fuel_temp"]     = CANdata.fuelTemp;
  //// Alerts////////
  object["warning"]       = alerts.oilWarning;
  object["danger"]        = alerts.oilDanger;
  object["coldEngine"]    = alerts.coldEngine;
  object["remind"]        = alerts.shiftReminder;
  object["shiftLight"]    = alerts.shiftLight;
  object["lightsOn"]      = alerts.lightsOn;
  //// Settings////////
  object["startMode"]     = settings.startMode;
  object["startType"]     = settings.startType;
  object["startType"]     = settings.startType;
  object["startMode"]     = mode2string(settings.startMode); //settings.startMode;
  object["shiftRPM"]      = settings.shiftLightRPM;
  object["warning_temp"]  = settings.oil_warning_temp;
  object["danger_temp"]   = settings.oil_danger_temp;
  object["reminder_sec"]  = settings.shift_reminder_ms / 1000;
  
  serializeJson(doc, jsonString);    // serialize the object and save teh result to teh string variable.
  // Serial.println(jsonString);         // print the string for debugging.
  webSocket.broadcastTXT(jsonString); // send the JSON object through the websocket
  jsonString = "";                    // clear the String.
}
/// @brief //////////////////////////////////////////////////////////
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// This function gets a call when a WebSocket event occurs////////////////////
void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED: // enum that read status this is used for debugging.
    Serial.print("WS Type ");
    Serial.print(type);
    Serial.println(": DISCONNECTED");
    neopixelWrite(21, 0, 0, 0);
    break;

  case WStype_CONNECTED: // Check if a WebSocket client is connected or not
    Serial.print("WS Type ");
    Serial.print(type);
    Serial.println(": CONNECTED");
    neopixelWrite(21, 0, 0, 128);
    update_webpage();   // update the webpage accordingly
    break;

  case WStype_TEXT:   // check responce from client
    Serial.println(); // the payload variable stores teh status internally
    Serial.printf("Payload: %s,      Size of payload: %d", payload, length);
    Serial.println();
    // unsigned char input = payload;
    JsonDocument doc;
    deserializeJson(doc, payload);
    /* data structure:
     {"startType":1,
      "startMode":"Sport",
      "reminder":4000, (if value is 0, shift reminder is off)
      "danger":"128",
      "warning":"117",
      "shiftRPM":"5859"}*/
    settings.startType          = doc["sType"];
    settings.oil_warning_temp   = doc["OWT"];
    settings.oil_danger_temp    = doc["ODT"];
    settings.shiftLightRPM      = doc["shftRPM"];
    shift_reminder              = doc["rem_sec"];
    settings.shift_reminder_ms  = shift_reminder * 1000;
    string2mode(doc["sMode"], settings.startMode);
    
    if (!writeToEEPROM(startTypeAddr,  settings.startType)) {
      Serial.println("Failed to write starttype to EEPROM");
    }
    if (!writeToEEPROM(startModeAddr,  settings.startMode)) {
      Serial.println("Failed to write startmode to EEPROM");
    }
    if (!writeToEEPROM(shiftLightAddr, settings.shiftLightRPM)) {
      Serial.println("Failed to write shiftlight to EEPROM");
    }
    if (!writeToEEPROM(oil_warnAddr,   settings.oil_warning_temp)) {
      Serial.println("Failed to write oil warning temp to EEPROM");
    }
    if (!writeToEEPROM(oil_dangerAddr, settings.oil_danger_temp)) {
      Serial.println("Failed to write oil danger temp to EEPROM");
    }
    if (!writeToEEPROM(reminderAddr,   settings.shift_reminder_ms)) {
      Serial.println("Failed to write reminder ms to EEPROM");
    }
    
    break;
  }
}

void setup()
{
  neopixelWrite(21, 0, 0, 0);
  GPIO_OUTPUT_SET(modeUp, 0);
  GPIO_OUTPUT_SET(modeDown, 0);
  GPIO_OUTPUT_SET(DSC, 0);
  Serial.begin(115200);       // Init Serial for Debugging.
  initLittleFS();
  InitEEPROM();
  Serial.println("EEPROM started");
  settings.startType = readStartModes(startTypeAddr);
  settings.startMode = readStartModes(startModeAddr);
  settings.shiftLightRPM = readStartModes(shiftLightAddr);
  settings.oil_warning_temp = readStartModes(oil_warnAddr);
  settings.oil_danger_temp = readStartModes(oil_dangerAddr);
  settings.shift_reminder_ms = readStartModes(reminderAddr);

  ssid_saved = EEPROM.get (SSIDAddr, ssid_saved);
  password_saved = EEPROM.get (passwordAddr, password_saved);

  WiFi.setTxPower(WIFI_POWER_2dBm);

  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(local_ip, gateway, subnet);

  if (ssid_saved != "") {
    WiFi.begin(ssid_saved, password_saved);
  }

  (WiFi.softAP(softAP_ssid, softAP_password)); 
  Serial.println("softAP started"); // setup softAP so user can enter their network credentials

  // int n = WiFi.scanNetworks();
  // Serial.println("scan done");
  //   for (int i = 0; i < n; ++i) {
  //   if (WiFi.status() != WL_CONNECTED) {
  //     if (WiFi.SSID(i) == ssid_1) {
  //       WiFi.begin(ssid_1, password_1);
  //       Serial.printf("connected to %s", ssid_1);
  //       break;
  //     }
  //     else if (WiFi.SSID(i) == ssid_2) {
  //       WiFi.begin(ssid_2, password_2);
  //       Serial.printf("connected to %s", ssid_2);
  //       break;
  //     }
  //     else if (WiFi.SSID(i) == ssid_3) {
  //       WiFi.begin(ssid_3, password_3);
  //       Serial.printf("connected to %s", ssid_3);
  //       break;
  //       delay(200);
  //     }
  //   }
  // }

  // while (WiFi.status() != WL_CONNECTED) {
  //   neopixelWrite(21, 50, 50, 0);
  // }
  (WiFi.status() == WL_CONNECTED)? neopixelWrite(21, 0, 128, 128): neopixelWrite(21, 0, 128, 0);

  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
  // Initialize a web server on the default IP address. and send the webpage as a response.
  if (!MDNS.begin("esp32")) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(500);
    }
  }
  Serial.println("mDNS responder started");

  // if (!WiFi.begin(ssid_saved, password_saved)) {
  //   Serial.println("Failed to connect to WiFi");
  // }
  configServer.on("/config", HTTP_GET, []() {
              Serial.println("Received request for /");
              configServer.send(200, "text/html", config);
            });
  socketServer.on("/data", [&]() { 
              File file = LittleFS.open("/index.html");
              String fileContent = file.readString();
              socketServer.send(200,"text/html", fileContent ); 
              file.close();
              fileContent = ""; //empty the string
            });
  delay(200);
 configServer.on("/config", HTTP_POST, []() {
      // Handle the form submission and store the network credentials
      String ssid = configServer.arg(0);
      Serial.println(ssid);
      String password = configServer.arg(1);
      Serial.println(password);
      // Store the network credentials in EEPROM
      EEPROM.put(SSIDAddr, ssid);
      EEPROM.put(passwordAddr, password);
      EEPROM.commit();
      configServer.send(200, "text/html", "<h1>Form Submitted</h1>");

      // Connect to the Wi-Fi network as a client
      WiFi.begin(ssid.c_str(), password.c_str());
 });
  socketServer.begin();                    // init the server
  configServer.begin();                    // init the server
  Serial.println("HTTP server started");
  webSocket.begin();                 // init the Websocketserver
  webSocket.onEvent(webSocketEvent); // init the webSocketEvent function when a websocket event occurs
  delay(200);
}

void loop()
{

  
  configServer.handleClient();             // webserver method that handles all Client
  socketServer.handleClient();            // webserver method that handles all Client
  webSocket.loop();                       // websocket server method that handles all Client
  unsigned long currentMillis = millis(); // call millis and Get snapshot of time
  

  //EMULATION Section
  ///RPM and Gear stuff    
  if (millis() - emTimer2 > 1)
  {
    emTimer2 = millis();
    if (CANdata.rpm >= 7000)
    {
      if (CANdata.gear >= 12)
      {
      gearDecrease = true;
      rpmDecrease = true;
      }
      CANdata.rpm = (rpmDecrease)? CANdata.rpm : 3000;
      CANdata.gear = (gearDecrease)? CANdata.gear : CANdata.gear+=1;
      
    }
   
    if (CANdata.rpm <= 1500)
    {
      if (CANdata.gear <= 5) 
      {
        rpmDecrease = false;
        gearDecrease = false;
        CANdata.gear = 6;
      }  
      CANdata.rpm = (rpmDecrease)? 3000 : CANdata.rpm;
      CANdata.gear = (gearDecrease)? CANdata.gear-=1 : CANdata.gear;
    }
  CANdata.rpm = (rpmDecrease)? CANdata.rpm -=3 : CANdata.rpm +=5;
  alerts.shiftReminder = rpmDecrease? alerts.shiftReminder : false;
  alerts.shiftLight = (CANdata.rpm > settings.shiftLightRPM)? true : false;
  }

///OIL TEMP STUFF
  if (millis() - emTimer3 > 450)
  {
    emTimer3 = millis();
    if (CANdata.oil_temp > 135)  
    {
    oilDecrease = true;
    }
    else if (CANdata.oil_temp < 70 )
    {
      oilDecrease = false;
    }
    CANdata.oil_temp = (oilDecrease)? CANdata.oil_temp-=1 : CANdata.oil_temp+=1;
  }
  alerts.oilWarning = (CANdata.oil_temp>settings.oil_warning_temp)? true:false;
  alerts.oilDanger = (CANdata.oil_temp>settings.oil_danger_temp)? true:false;
  alerts.coldEngine = (CANdata.oil_temp<75)? true:false;

  ///COOLANT TEMP STUFF
  if (millis() - emTimer4 > 800)
  {
    emTimer4 = millis();
    if (CANdata.water_temp > 110)  
    {
    waterDecrease = true;
    }
    else if (CANdata.water_temp < 85 )
    {
      waterDecrease = false;
    }
    CANdata.water_temp = (waterDecrease)? CANdata.water_temp-=1 : CANdata.water_temp+=1;
  }

  ///SPEED STUFF
  if (millis() - emTimer5 > 120)
  {
    emTimer5 = millis();
    if (CANdata.speed > 100)  
    {
    speedDecrease = true;
    }
    else if (CANdata.speed < 25 )
    {
      speedDecrease = false;
    }
    CANdata.speed = (speedDecrease)? CANdata.speed-=1 : CANdata.speed+=1;
  }
  
  // for testing
  CANdata.gearbox_temp  =  CANdata.water_temp; 
  CANdata.ethContent = CANdata.water_temp - 75;

  long now = millis();
  if (now - ECOTimer > settings.shift_reminder_ms && !alerts.shiftReminder) {
    alerts.shiftReminder = true;
    ECOTimer = now;
  }
  else if (now - ECOTimer > settings.shift_reminder_ms && alerts.shiftReminder) {
    alerts.shiftReminder = false;
    ECOTimer = now;
  }

  now = millis();
  if (now - illumTimer > 20000) {
    lightsOn = !lightsOn;
    illumTimer = now;
  }

  ////theme change every 20s
  now = millis();
  if (now - modeTImer > 6000) {
    mode = (modeData == 0)? "ECO"
                          :(modeData == 1)? "Comfort"
                          :(modeData == 2)? "Sport"
                          :(modeData == 3)? "Sport +"
                          :(modeData == 4)? "Traction":"DSC Off";
    modeData++;
    modeData = (modeData > 5)? 0:modeData;                      
    modeTImer = now;
  }

  // Send data update every 50ms
  if ((unsigned long)(currentMillis - previousMillis) >= dataRate)
  {                                 // How much time has passed, accounting for rollover with subtraction!
    update_webpage();  
    previousMillis = currentMillis; // Use the snapshot to set track time until next event
  }
  
  if (millis() - rpmTimer > 1000)
  {
    lastRPM = rpm;
    rpmTimer = millis();
  }
}


