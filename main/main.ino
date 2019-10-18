//the microgear and network stuff

#include <ESP8266WiFi.h>
#include <MicroGear.h>

const char* ssid     = "";
const char* password = "";

#define APPID   ""
#define KEY     ""
#define SECRET  ""
#define ALIAS   "NodeMCU"
#define FEEDID  "SmartFarmDemo"
WiFiClient client;

char str[64];
int timer = 0;
MicroGear microgear(client);

//The libs for the ds18b20 and Inits of the temp-sensor.
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2 //pin D4 
OneWire oneWire(ONE_WIRE_BUS); //DS18B20 is on the D4 at NodeMCU
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;


void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message -->");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setAlias(ALIAS);
}

void printAddress (DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void setup() {

  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);

  Serial.begin(115200);
  Serial.println("Starting...");
  sensors.begin();
  Serial.println();
  Serial.print("DallasTempID:");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, 12);

  if (WiFi.begin(ssid, password)) {// attemp to connect to the AP
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  microgear.init(KEY, SECRET, ALIAS);
  microgear.connect(APPID);
}
int humid ;
float temp;
void loop() {
  if (microgear.connected()) {
    Serial.println("connected");
    microgear.loop();

    if (timer >= 1000) {
      sensors.requestTemperatures();
      temp = sensors.getTempC(insideThermometer);
      humid = analogRead(A0);
      humid = map(humid, 100, 800, 100, 0);// analog value = 0 mean super wet 100% humid. 
      sprintf(str, "%d,%f", humid, temp);
      Serial.print("Temp C: ");
      Serial.print(temp);
      Serial.print("     ");
      Serial.print("Moisture Value");
      Serial.print(humid);
      Serial.println();
      Serial.print("Sending -->");
      microgear.publish("/sfs", str);
      String data = "{\"Soil humidity\":";
      data += humid ;
      data += ", \"Air temp\":";
      data += temp ;
      data += "}";
      microgear.writeFeed(FEEDID, data);
      

      timer = 0;
    }
    else timer += 200;
  }
  else {
    Serial.println("connection lost, reconnect...");
    if (timer >= 5000) {
      microgear.connect(APPID);
      timer = 0;
    }
    else timer += 200;
  }
  delay(200);
}
