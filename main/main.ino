/* By TinLethax */
//the microgear (uGear for short, not official name) and network stuff

#include <ESP8266WiFi.h>// include wifi stuff for the nodeMCU 
#include <MicroGear.h>// include microgear library

const char* ssid     = "";// wifi
const char* password = "";// wifi's password

#define APPID   "" //app id of the project 
#define KEY     "" // the key
#define SECRET  "" //secret key for the project
#define ALIAS   "" //alias of the client (In this case is nodeMCU)
#define FEEDID  "" // feed id for the IoT feed 
WiFiClient client; // use nodeMCU as wifi client

char str[64];// store the message
int timer = 0;
MicroGear microgear(client);// use microgear as client

//The libs for the ds18b20 and Inits of the temp-sensor.
#include <OneWire.h>// include onewire library (Interfacing stuff)
#include <DallasTemperature.h>// include the dallas DS18B20 library (commanding stuff)
#define ONE_WIRE_BUS 14 //pin D5 
OneWire oneWire(ONE_WIRE_BUS); //DS18B20 is on the D4 at NodeMCU
DallasTemperature sensors(&oneWire);// se the onewire bus pin
DeviceAddress insideThermometer;// store the device address

//General use GPIOs
int pmrly = 2; //pump relay on GPIO2 or pin D4

// command interpreter
String rcvdstring;   // string received from serial
String cmdstring;   // command part of received string
String  parmstring; // parameter part of received string
int parmvalint;        // parameter value
int sepIndex;       // index of seperator
int humidThres ; // keep the minimum threshold from the server message
int pumpDura ; // keep the pump duration from the server
int humid ; // the variable for keeping the analog value of humidity sensor
int prevHumid; // store the prvious humidThres

void cmdInt(void)
{
  rcvdstring.trim();  // remove leading&trailing whitespace, if any
  // find index of separator "="
  sepIndex = rcvdstring.indexOf('=');

  // extract command and parameter
  cmdstring = rcvdstring.substring(0, sepIndex);
  parmstring = rcvdstring.substring(sepIndex + 1);
  if (cmdstring.equalsIgnoreCase("d"))   {
    humidThres = parmstring.toInt();
  }
  else if (cmdstring.equalsIgnoreCase("h"))   {
    pumpDura = parmstring.toInt();
  }
}

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {// message handler of the uGear. Do something if message arrives
  char *m = (char *)msg;
  m[msglen] = '\0';
  rcvdstring = m;
  Serial.println(rcvdstring);
  cmdInt();// extract the data from server's message 
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {//lets uGear known the alias for showing up on the sever
  Serial.println("Connected to NETPIE...");
  microgear.setAlias(ALIAS);
}

void printAddress (DeviceAddress deviceAddress)// print device address of the DS18B20
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void setup() {
  pinMode(pmrly, OUTPUT);// set D2 as output for controlling pump relay
  digitalWrite(pmrly, HIGH);// pull high because the relay is active high
  microgear.on(MESSAGE, onMsghandler);// setup the message handler
  microgear.on(CONNECTED, onConnected);// shout out "connected" to serial

  Serial.begin(115200);// set serial baud rate to 115200
  Serial.println("Starting...");
  sensors.begin();// initialize the sensor
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  sensors.isParasitePowerMode();
  Serial.println();
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
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


void pumpctrl() {
  Serial.println(humidThres);
  Serial.println(pumpDura);
//if (humidThres != prevHumid){// if the dat updated 
    if (humid <= humidThres) {// if the real humidity match the threshold 
      digitalWrite(pmrly, LOW);//turn on pump
      Serial.println("PUMP ON ");// telling us in serial that the pump on ! 
      delay(pumpDura);// for "pumpDura" second 
      digitalWrite(pmrly, HIGH);// then stop the pump
    }
   // prevHumid = humid;// ensure that it wont do again if we update the threshold.
 //}
}

void loop() {
  if (microgear.connected()) {
    Serial.println("connected");
    microgear.loop();

    if (timer >= 1000) {
      sensors.requestTemperatures();// request temperature from sensor usin 1wire 
      delay(20);//delay for .02 s
      float temp = sensors.getTempC(insideThermometer);//read the temperature and put to "temp"
      humid = analogRead(A0);// read analog value from A0 and pu the vaue to "humid"
      humid = map(humid, 100, 800, 100, 0);// analog value = 0 mean super wet 100% humid.
      pumpctrl();// get into pump control for a bit 
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
