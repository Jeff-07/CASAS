#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <FS.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "NodeConfig.h" 

DNSServer dnsServer;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Supply the preferred Static IP for this node as a paremeter value in the following line
IPAddress localIP(192, 168, 254, 202);

// Set your Gateway IP address
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

boolean restart = false;

// Connecting nodes to central hub through MQTT Protocol

// IP address of Raspberry Pi in the CASAS Network
IPAddress broker(192,168,254,130);

WiFiClient wclient;

PubSubClient client(wclient); // Initialize MQTT client (Specific CASAS node is one of the MQTT Clients in the system)

// File system is mounted to give access saved configurations
void initFS() {
  if (SPIFFS.begin()) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }
}

// Read File from SPIFFS or the file system
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = SPIFFS.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    //return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
    //Serial.write(file.read());
    Serial.println(fileContent);
  }
  file.close();
  //return String(file.read());
  fileContent.trim();
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = SPIFFS.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  file.close();
}

// This class is for the captive portal or network manager that automatically opens
// while the CASAS Node Access point is active to allow connecting to a different WiFi network
class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/wifimanager.html", "text/html"); 
  }
};

// Initialize node in WiFi station mode to connect to a wireless access point
// In this case, the CASAS access point provided by the separate router
bool initStationMode() {
  //if(ssid=="" || ip==""){
    if(ssid==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);

  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    // return false;
  }

  WiFi.setAutoReconnect(false);
  localIP.fromString(ip.c_str()); // IP Address given to node upon a successful connection
  localGateway.fromString(gateway.c_str()); // IP Address of gateway or the router

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid.c_str(), pass.c_str());
  delay(interval_reconnect);

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");

    return false;
  }

  Serial.println(WiFi.localIP());
  return true;
}

void initNetworkManager() {

  // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("CASAS Lighting", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
    });
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP 

    server.begin();
}

// Try to reconnect to saved network until value of interval_restart is reached if
// unable to connect after node was powered on and setup() was executed
// or if disconnected to WiFi after initially being connected

void reconnectToNetwork() {

  //Serial.println(currentMillis);

  if(WiFi.status() != WL_CONNECTED) {

    //interval_AP = 120000;
  currentMillis = millis();

  Serial.println(currentMillis);

  while (currentMillis >= interval_restart) {

    currentMillis = currentMillis - interval_reconnect;
    Serial.println(currentMillis);

     if (initStationMode()) {

       restart = false;
       break;
     }
     else {restart = true;}
     //delay(interval_reconnect);
   }
    
    //if (currentMillis - interval_AP >= interval_restart) {
    if (restart) {  
       Serial.println("Initiating restart sequence.");
       ESP.restart();
    }
    //}
  }  
}

// See NodeConfigurations.txt file for guide to editing the following
// The following is to allow the MQTT Broker running in Raspberry Pi to discover this node in the network

void sendNodeDiscoveryMsg() {
  // This is the discovery topic for this specific CASAS node
  String discoveryTopic = CONFIG_TOPIC;

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Lighting Node";
  // doc["dev_cla"] = "light";
  doc["unique_id"] = "light1";
  doc["command_topic"] = COMMAND_TOPIC;
  doc["stat_t"]   = STATE_TOPIC;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["state_on"] = "on";
  doc["state_off"] = "off";
  // doc["retain"] = true;

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

// Reconnect to MQTT Broker to allow this node to publish data and/or receive commands
void reconnectToBroker() {
  
  // Loop until we're reconnected
  //while (!client.connected()) {
    if (!(client.connected())) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if(client.connect(ID,HA_USER,HA_PASS)) {
      //client.subscribe(TOPIC);
      Serial.println("connected");

      Serial.println(digitalRead(relayPin));

      if (digitalRead(relayPin) == LOW) {

        lightState = "off";
        //Serial.println("off");
      }
      else {

        lightState = "on";
        //Serial.println("on");
      }

      sendNodeDiscoveryMsg();

      client.publish(STATE_TOPIC,lightState.c_str());
     
      client.subscribe(COMMAND_TOPIC);
      Serial.print("Subcribed");
      //Serial.println(TOPIC);
      Serial.println('\n');

    } else {

      Serial.println(" try again in 10 seconds");
      delay(10000);
      //Serial.println(currentMillis);

    } 
  }

  // client.publish(STATE_TOPIC,lightState.c_str());
  delay(10000);
}

// Handle incoming messages from the MQTT broker
void callback(char* topic, byte* payload, unsigned int length) {
  //String message = ""; // Declare the message variable
  String response;

  // Convert the payload to a string
  for (int i = 0; i < length; i++) {
    response += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(response);

    if (response == "ON") {
      // Turn on the light bulb
      turnLightOn();
      lightState = "on";
    } else if (response == "OFF") {
      // Turn off the light bulb
      turnLightOff();
      lightState = "off";
    }
  //}
  client.publish(STATE_TOPIC,lightState.c_str());

 
}
  

void setup()
{
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  interval_reconnect = 20000;

  turnLightOff();

  initFS();

  // Load values saved in LittleFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);
  
  //if(initStationMode()) {

  initStationMode();
  //}
  
  if(!(initStationMode())) {
    
    initNetworkManager(); // Initialize CASAS Node Access point if node failed to connect to WiFi
    
  }
  else {
    interval_AP = 30000; // Otherwise, change interval_AP value to shorten amount of time before network manager webpage closes
  }

  interval_reconnect = 10000;
  Serial.println(interval_reconnect);
  Serial.println(interval_AP);
  
  client.setServer(broker, 1883);
  client.setCallback(callback);
}

void loop()
{
  //Serial.println(millis());

  while (millis() <= interval_AP) {

      dnsServer.processNextRequest();
    }

  //dnsServer.processNextRequest();
  
  if (restart){
    delay(5000);
    ESP.restart();
  }  
   
   //if (millis() - previousMillis >= interval_AP) {

    //unsigned long currentMillis = millis(); // Get the current time

    reconnectToNetwork();
    reconnectToBroker();

    client.loop();
     
}

// Function to turn the light bulb on
void turnLightOn() {
    digitalWrite(relayPin, HIGH); // Turn on the relay
    Serial.println("turning on");
}

// Function to turn the light bulb off
void turnLightOff() {
    digitalWrite(relayPin, LOW); // Turn off the relay
    Serial.println("turning off");
}