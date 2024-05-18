#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

#include <WiFiManager.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>

const int idNumber = 1;
const int led_PIN = 4;

const char* WIFI_SSID = "PLDTHOMEFBR2a2e0";
const char* WIFI_PASS = "PLDTWIFItaz3n";
 
WebServer server(80);
 
 
static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);

/*
// Set your Static IP address
IPAddress local_IP(192, 168, 254, 152);
// Set your Gateway IP address
IPAddress gateway(192, 168, 254, 254);

IPAddress subnet(255, 255, 254, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
*/

//mqtt details

const char* HA_User = "casasHub";
const char* HA_Password = "013-alecs";
const char* HA_IPServer = "192.168.254.130";
IPAddress broker(192,168,254,130);

char ID[20] = "ESP_CAMERA_node_1";  // Unique name for specific node in the system
const char *CONFIG_TOPIC = "homeassistant/ESP_CAM_1/node/config";
const char *STATE_TOPIC = "homeassistant/ESP_CAM_1/node/state";
const char *COMMAND_TOPIC = "homeassistant/ESP_CAM_1/node/command";


WiFiClient wclient;
PubSubClient client(wclient);

/*
// Handle incoming messages from the MQTT broker
void callback(char* topic, byte* payload, unsigned int length) {
  String response;

  for (int i = 0; i < length; i++) {
    response += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);  
  Serial.print("] ");
  Serial.println(response);
 
}

void sendNodeDiscoveryMsg() {
  // This is the discovery topic for this specific CASAS node
  String discoveryTopic = CONFIG_TOPIC;

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "ESP_CAM_" + String(idNumber);
  doc["platform"] = "generic"; 
  doc["still_image_url"] = "http://" + String(WiFi.localIP()) + "/cam-hi.jpg";
  doc["Frame_Rate"] = "500";

  doc["command_topic"] = COMMAND_TOPIC;
  doc["stat_t"]   = STATE_TOPIC;
  //doc["payload_open"] = "";
  //doc["payload_close"] = "";
  //doc["state_open"] = "";
  //doc["state_closed"] = "";
  //doc["retain"] = true;

  // I'm sending a JSON object as the state of this MQTT device
  // so we'll need to unpack this JSON object to get a single value
  // for this specific sensor.
  //doc["val_tpl"] = "{{ value_json.state }}";

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
    if(client.connect(ID,HA_User,HA_Password)) {
      //client.subscribe(TOPIC);
      Serial.println("connected");
      sendNodeDiscoveryMsg();
      // client.publish(STATE_TOPIC); // check NodeConfigurations.txt file and then uncomment this
     
      client.subscribe(COMMAND_TOPIC);
      Serial.print("Subcribed");

    } else {

      Serial.println(" try again in 1 second/s");
      delay(1000);

    } 
  }
}*/

void serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));
 
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}
 
void handleJpgLo()
{
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}
 
void handleJpgHi()
{
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}
 
void handleJpgMid()
{
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}
 
 
void  setup(){
  Serial.begin(115200);

  pinMode(led_PIN, OUTPUT);

  for (int i = 0; i<=5; i++) {
    digitalWrite(led_PIN, HIGH);
    delay(200);
    digitalWrite(led_PIN, LOW);
    delay(200);
  }
  Serial.println();
  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);
 
    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
  }

  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //wm.resetSettings();


  //wm.setShowStaticFields(true); // force show static ip fields


  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("CASAS CAMERA NODE","password"); // password protected ap

  if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
  } 
  else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");

  }


  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");
 
  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-mid.jpg", handleJpgMid);
 

  //client.setServer(broker, 1883);
  //client.setCallback(callback);
  server.begin();
}
 
void loop()
{
  server.handleClient();
  //reconnectToBroker();
  //client.loop();
}