// #define DHTPIN 4     // esp8266 D2 pin map as 4 in Arduino IDE
#define DHTPIN D7
#define DHTTYPE DHT22 

// Credentials for authentication when connecting to MQTT Broker
// running on Raspberry Pi to send/receive data from/to input/output nodes
const char *HA_USER = "casasHub";
const char *HA_PASS = "ALECS-013";

// Connecting nodes to central hub through MQTT Protocol

char ID[20] = "temperatureNode";  // Unique name for specific node in the system
// const char *CONFIG_TOPIC = "homeassistant/sensor/temperatureNode/config";
const char *STATE_TOPIC = "homeassistant/sensor/temperatureNode/state";
const char *STATUS_TOPIC = "tempNode/status"; 

unsigned long currentMillis;
unsigned long previousMillis = 0; 

// Uptime (in milliseconds) of the Access Point if node can't connect to WiFi
unsigned long interval_AP = 120000; 

// Node will restart if the amount of time reaches this threshold while there is no network connection 
unsigned long interval_restart = 60000;

// Interval before each attempt to connect to WiFi
unsigned long interval_reconnect; 

// Following lines are for the Network Manager webpage that automatically opens
// with CASAS Node Access Point

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";