#include <EmonLib.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "htmlhead.h"
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>
EnergyMonitor emon1;             // Create an instance


#define PN532_SCK (D5) // (2) 
#define PN532_MOSI (D7) // (3) 
#define PN532_SS (D3) // (4) 
#define PN532_MISO (D6) // (5) Adafruit_PN532 nfc (PN532_SS); 
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

const byte DNS_PORT = 53;
DNSServer dnsServer;
uint32_t Color1, Color2;  // What colors are in use
uint16_t TotalSteps = 255;  // total number of steps in the pattern
uint16_t Index;  // current step within the pattern

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

const int FW_VERSION = 1026;

const char* fwUrlBase = "http://www.malyon.co.uk/atomicsmart/";

byte sensorInterrupt = D5;
float calibrationFactor = 4.5;

//emon1.current(1, 111.1);       // Current: input pin, calibration.
double Irms = 0.00;

volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

volatile byte oldpulseCount;
float oldflowRate;
unsigned int oldflowMilliLitres;
unsigned long oldtotalMilliLitres;

boolean liquidflowing = false;


unsigned long oldTime;


#define ONE_WIRE_BUS D2
OneWire ds(ONE_WIRE_BUS);

IPAddress ipMulti(239, 255, 255, 250);
const unsigned int portMulti = 1900;
char packetBuffer[512];


#define PIN            D2


WiFiUDP UDP;

String GPixelCount = "";
int GPattern = 1;
int GDevtype = 1;

unsigned int loadpixel(void) {
  EEPROM.begin(512);
  delay(10);
  GPattern = EEPROM.read(304);
  GPixelCount = "";
  for (int i = 305; i < 310; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    GPixelCount += char(EEPROM.read(i));
  }

  return GPixelCount.toInt();
}




#define NUMPIXELS      250

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(loadpixel(), PIN, NEO_GRB + NEO_KHZ800);

const int relayPin = D1;
const int buttonPin = D2;
const int LEDPin = D4;

boolean firstrun = true;
boolean ignoreserver = false;

String Host = "AtomicSmart";
const char* update_username = "admin";
const char* update_password = "*****";
const char* update_path = "/firmware";
ESP8266HTTPUpdateServer httpUpdater;

unsigned long last_interrupt_time = 0;

String Mqttconnectionstatus = "";
int mqttreconnecttrycount = 0;


boolean APmode = false;

String serial;
String persistent_uuid;

String content;
int statusCode;
String st;

String Gqsid = "";
String Gqpass = "";
String Gmqtthost = "";
String Gmqttport = "";
String Gmqttuser = "";
String Gmqttpass = "";
String Gdevname = "";
boolean GAlexa = false;
boolean GMQTT = false;
boolean GLED = false;
boolean GNeoPixel = false;
String GPixelcolor = "#45bc00";


String LastTag = "";

boolean TagPresent = false;

boolean Gautoupdate = true;
double PowerFactor = 230.0;

long Gcelsius = 0.00;

long mqttpreviousMillis = 0;
long mqttinterval = 500;

long PowerpreviousMillis = 0;
long Powerinterval = 30000;

long buttonpresspreviousMillis = 0;
long buttonpressinterval = 3000;

long autoupdatepresspreviousMillis = 0;
long autoupdateinterval = 60000;

long NFCpresspreviousMillis = 0;
long NFCinterval = 5000;

long previousMillis = 0;
long interval = 60000;
boolean firstrunDS = true;
boolean firstrunAU = true;

const char* ssid = "AtomicSmart1";

const char* password = "welcomehome";


const char* mqtt_server = "************";

char NRValue[8];
String MainTopic;
const char * c;
const long togDelay = 100;  // pause for 100 milliseconds before toggling to Open
const long postDelay = 200;  // pause for 200 milliseconds after toggling to Open
int value = 0;
boolean relayState;
boolean switchenable = true;
boolean lastrelaystate;

boolean XMasState;


boolean DBellState;
boolean lastDBellState;

unsigned long PayloadTime = millis();
static unsigned long last_PayloadTime = 0;


WiFiClient client;
WiFiClient client2;
PubSubClient espclient(client2);
long lastMsg = 0;

ESP8266WebServer server ( 80 );

void callback(char* topic, byte* payload, unsigned int length) {
  String myString;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    myString += (char)payload[i];
  }
  Serial.println();
  Serial.print("DEBUG: ");
  Serial.println(("openhab/out/" + String(Gdevname) + "/command").c_str());

  if (!ignoreserver) {

    if ( strcmp(topic, ("openhab/out/" + String(Gdevname) + "/command").c_str()) == 0) {
      if (GDevtype == 7) {
        if (myString == "ON") {
          Serial.println("Actioned");
          turnOnXMas(true);

        }
        if (myString == "OFF") {
          Serial.println("Actioned1");
          turnOffXMas(true);
          //XMasState = false;
        }
      } else {
        if (myString == "ON") {
          Serial.println("Actioned");
          turnOnRelay(false);
        }
        if (myString == "OFF") {
          Serial.println("Actioned1");
          turnOffRelay(false);
        }
      }
    }
  }




  last_PayloadTime = millis();

}

void buttonPress()

{
  if (millis() - last_interrupt_time > 1000)
  {



    ignoreserver = true;
    last_interrupt_time = millis();
    if (switchenable) {

      Serial.println("Button Pressed");
      if (GDevtype == 2) {
        if (relayState == LOW) {
          digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
          relayState = true;
          if (GLED) {
            digitalWrite(LEDPin, LOW);
          }
        } else if (relayState == HIGH) {
          digitalWrite(relayPin, LOW); // turn on relay with voltage HIGH
          relayState = false;
          if (GLED) {
            digitalWrite(LEDPin, HIGH);
          }
        }
      } if (GDevtype == 3) {
        if (DBellState == true) {
          DBellState = false;
        } else if (DBellState == false) {
          DBellState = true;
        }
      }

    }
  }


}

void handlesettings() {
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
  String mqtthost = server.arg("mqtthost");
  String mqttport = server.arg("mqttport");
  String mqttuser = server.arg("mqttuser");
  String mqttpass = server.arg("mqttpass");
  String devname = server.arg("devname");


  int Devtype = server.arg("Devtype").toInt();
  boolean Alexa;
  if (GDevtype == 1 ) {
    Alexa = false;
  } else {
    if (server.arg("Alexa") == "1") {
      Alexa = true;
    } else if (server.arg("Alexa") == "0") {
      Alexa = false;
    }
  }

  boolean MQTT;
  if (server.arg("MQTT") == "1") {
    MQTT = true;
  } else if (server.arg("MQTT") == "0") {
    MQTT = false;
  }

  boolean NeoPixel;
  if (GDevtype == 1 ) {
    NeoPixel = false;
  } else {
    if (server.arg("NeoPixel") == "1") {
      NeoPixel = true;
    } else if (server.arg("NeoPixel") == "0") {
      NeoPixel = false;
    }
  }

  boolean LED;
  if (server.arg("LED") == "1") {
    LED = true;
  } else if (server.arg("LED") == "0") {
    LED = false;
  }

  boolean autoupdate;
  if (server.arg("autoupdate") == "1") {
    autoupdate = true;
  } else if (server.arg("autoupdate") == "0") {
    autoupdate = false;
  }



  if (qsid.length() > 0 && qpass.length() > 0) {
    Serial.println("clearing eeprom");
    for (int i = 0; i < 512; ++i) {
      EEPROM.write(i, 0);
    }


    for (int i = 0; i < 32; ++i)
    {
      EEPROM.write(i, qsid.charAt(i));
    }
    for (int i = 32; i < 96; ++i)
    {
      EEPROM.write(i, qpass.charAt(i - 32));
    }
    for (int i = 96; i < 128; ++i)
    {
      EEPROM.write(i, mqtthost.charAt(i - 96));
    }
    for (int i = 128; i < 160; ++i)
    {
      EEPROM.write(i, mqttport.charAt(i - 128));
    }
    for (int i = 160; i < 192; ++i)
    {
      EEPROM.write(i, mqttuser.charAt(i - 160));
    }
    for (int i = 192; i < 224; ++i)
    {
      EEPROM.write(i, mqttpass.charAt(i - 192));
    }
    for (int i = 224; i < 256; ++i)
    {
      EEPROM.write(i, devname.charAt(i - 224));
    }

    EEPROM.write(257, Alexa);
    EEPROM.write(258, MQTT);
    EEPROM.write(259, LED);
    EEPROM.write(260, NeoPixel);
    EEPROM.write(269, Devtype);
    EEPROM.write(270, autoupdate);


    Serial.print("Saving Device Type: ");
    Serial.println(Devtype);

    EEPROM.commit();
    loadSettingsFromEEPROM(false);

    content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
    statusCode = 200;
  } else {
    content = "{\"Error\":\"404 not found\"}";
    statusCode = 404;
    Serial.println("Sending 404");
  }

  content = HTMLheadref;
  content += "<h3>Please wait while Device Restarts</h3></body></html>";
  server.sendContent(content);
  delay(1000);
  ESP.restart();
}

void handleRoot() {
  //  digitalWrite ( led, 1 );
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;


  String messageBody = HTMLhead;

  messageBody += "<div class=\"tab\">";
  messageBody += "<button class=\"tablinks\" onclick=\"openTab(event, 'Home')\">Home</button>";
  messageBody += "<button class=\"tablinks\" onclick=\"openTab(event, 'Settings')\">Settings</button>";
  if (GNeoPixel) {
    messageBody += "<button class=\"tablinks\" onclick=\"openTab(event, 'NeoPixel')\">NeoPixel</button>";
  }

  if (GDevtype == 7) {
    messageBody += "<button class=\"tablinks\" onclick=\"openTab(event, 'XMas')\">XMas</button>";
  }

  messageBody += "<button class=\"tablinks\" onclick=\"openTab(event, 'Update')\">Update</button>";
  messageBody += "</div>";

  messageBody += "<div id=\"Settings\" class=\"tabcontent\"><h3>Settings</h3>";
  messageBody += "<table width=\"800px\">";
  messageBody += "<form method='get' action='setting'>";

  messageBody += "<tr><td>Device Type:</td><td>";

  messageBody += "<select name=\"Devtype\">";
  messageBody += "<option value=\"1\"";
  if (GDevtype == 1) {
    messageBody += "selected";
  }
  messageBody += ">Socket</option>";
  messageBody += "<option value=\"2\"";
  if (GDevtype == 2) {
    messageBody += "selected";
  }
  messageBody += ">Light Switch</option>";

  messageBody += "<option value=\"3\"";
  if (GDevtype == 3) {
    messageBody += "selected";
  }
  messageBody += ">DoorBell</option>";
  messageBody += "<option value=\"4\"";
  if (GDevtype == 4) {
    messageBody += "selected";
  }
  messageBody += ">Thermostat</option>";

  messageBody += "<option value=\"5\"";
  if (GDevtype == 5) {
    messageBody += "selected";
  }
  messageBody += ">Flowmeter</option>";


  messageBody += "<option value=\"6\"";
  if (GDevtype == 6) {
    messageBody += "selected";
  }
  messageBody += ">Powermeter</option>";

  messageBody += "<option value=\"7\"";
  if (GDevtype == 7) {
    messageBody += "selected";
  }
  messageBody += ">Xmas Lights</option>";

  messageBody += "<option value=\"8\"";
  if (GDevtype == 8) {
    messageBody += "selected";
  }
  messageBody += ">NFC Reader</option>";

  messageBody += "</select></td></tr>";


  messageBody += "<tr><td>Enable MQTT Support:</td><td>";

  if (GMQTT) {
    messageBody += "<input type=\"hidden\" name=\"MQTT\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"MQTT\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td>MQTT Server Status:</td><td>";
  if (GMQTT) {
    messageBody += Mqttconnectionstatus;
  } else if (!GMQTT) {
    messageBody += "Disabled";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td align=\"centre\"><label>Wifi SSID: </label></td><td align=\"centre\"><input name='ssid' length=32 value='";
  messageBody += Gqsid;
  messageBody += "' required></td><td align=\"centre\"><label>Wifi Password: </label></td><td align=\"centre\"><input name='pass' length=32 value='";
  messageBody += Gqpass;
  messageBody += "' required></td></tr>";
  messageBody += "<tr><td align=\"centre\"><label>MQTT Host: </label></td><td align=\"centre\"><input name='mqtthost' length=32 value='";
  messageBody += Gmqtthost;
  messageBody += "' required></td><td align=\"centre\"><label>MQTT Port: </label></td><td align=\"centre\"><input name='mqttport' length=32 value='";
  messageBody += Gmqttport;
  messageBody += "' required></td><tr>";
  messageBody += "<tr><td align=\"centre\"><label>MQTT Username: </label></td><td align=\"centre\"><input name='mqttuser' length=32 value='";
  messageBody += Gmqttuser;
  messageBody += "' required></td><td align=\"centre\"><label>MQTT Password: </label></td><td align=\"centre\"><input name='mqttpass' length=32 value='";
  messageBody += Gmqttpass;
  messageBody += "' required></td><tr>";
  messageBody += "<tr><td align=\"centre\"><label>Device Name: </label></td><td align=\"centre\"><input name='devname' length=32 value='";
  messageBody += Gdevname;
  messageBody += "' required></td></tr>";




  if (GDevtype == 2 || GDevtype == 1 ) {
    messageBody += "<tr><td>Enable Alexa Support:</td><td>";
    if (GAlexa) {
      messageBody += "<input type=\"hidden\" name=\"Alexa\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
    } else {
      messageBody += "<input type=\"hidden\" name=\"Alexa\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
    }
    messageBody += "</td>";

  } else {
    messageBody += "<input type=\"hidden\" name=\"Alexa\" value=\"0\">";
  }
  if (GDevtype == 3 ) {
    messageBody += "<td>Enable Neopixel:</td><td>";

    if (GNeoPixel) {
      messageBody += "<input type=\"hidden\" name=\"NeoPixel\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
    } else {
      messageBody += "<input type=\"hidden\" name=\"NeoPixel\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
    }

    messageBody += "</td></tr>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"NeoPixel\" value=\"0\">";

  }

  messageBody += "<tr><td></td></tr>";


  messageBody += "<tr><td>Enable Status LED:</td><td>";

  if (GLED) {
    messageBody += "<input type=\"hidden\" name=\"LED\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"LED\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td>Enable Auto Update:</td><td>";

  if (Gautoupdate) {
    messageBody += "<input type=\"hidden\" name=\"autoupdate\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"autoupdate\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td></td><td></td><td></td><td align=\"centre\"><input type='submit'></form></td></tr>";
  messageBody += "<tr><td></td></tr>";


  messageBody += "<tr><td></td></tr>";
  messageBody += "<tr><td><form method='get' action='factoryreset'><label>Factory Reset: </label><input type='hidden' name='reset' value='true'></td><td><input type='button' onClick=\"confSubmit(this.form);\" value='Reset'></form><br></td>";

  messageBody += "<tr><td><form method='get' action='mqttreconnect'><input type='submit' value='Reconnect'></form></td>";
  messageBody += "<td><form method='get' action='reboot'><input type='submit' value='Restart'></form></td>";

  messageBody += "</tr></table></div>";

  messageBody += "<div id=\"Home\" class=\"tabcontent\"><h3>Home</h3>";
  if (GDevtype == 1 || GDevtype == 2) {
    messageBody += "<form method='get' action='switchaction'><label>Switch State: </label>";
    messageBody += "<label class=\"switch\">";
    //messageBody += "<input name='switchstate' type=\"checkbox\" value=\"1\">";
    if (relayState) {
      messageBody += "<input type=\"hidden\" name=\"switchstate\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
    } else {
      messageBody += "<input type=\"hidden\" name=\"switchstate\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
    }
    messageBody += "<span class=\"slider round\"></span>";
    messageBody += "</label><input type='submit'></form>";

    messageBody += "<form method='get' action='switchupdate'><label>Switch Enable: </label>";
    messageBody += "<label class=\"switch\">";
    //messageBody += "<input name='switchstate' type=\"checkbox\" value=\"1\">";
    if (switchenable) {
      messageBody += "<input type=\"hidden\" name=\"switchenable\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
    } else {
      messageBody += "<input type=\"hidden\" name=\"switchenable\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
    }
    messageBody += "<span class=\"slider round\"></span>";
    messageBody += "</label><input type='submit'></form>";

  } else if (GDevtype == 3) {
    messageBody += "Device is in Doorbell Mode";
  } else if (GDevtype == 4) {
    messageBody += "Temperature: ";
    messageBody += Gcelsius;
    messageBody += "&deg<br>";
  }

  else if (GDevtype == 5) {
    messageBody += "Total liquid: ";
    messageBody += totalMilliLitres;
    messageBody += "mL<br>";
  }
  else if (GDevtype == 6) {
    messageBody += "Power: ";
    messageBody += Irms * PowerFactor;
    messageBody += "W<br>";
  }
  else if (GDevtype == 7) {
    messageBody += "<form method='get' action='XMasaction'><label>XMas State: </label>";
    messageBody += "<label class=\"switch\">";
    //messageBody += "<input name='switchstState' type=\"checkbox\" value=\"1\">";
    if (XMasState) {
      messageBody += "<input type=\"hidden\" name=\"XMasState\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
    } else {
      messageBody += "<input type=\"hidden\" name=\"XMasState\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
    }
    messageBody += "<span class=\"slider round\"></span>";
    messageBody += "</label><input type='submit'></form><br>";
    messageBody += pixels.numPixels();

  }
  else if (GDevtype == 8) {
    messageBody += "Last Tag Scanned: ";
    messageBody += LastTag;
    messageBody += "<br>";

  }





  messageBody += "</div>";



  messageBody += "<div id=\"Update\" class=\"tabcontent\"><h3>Update</h3>";
  messageBody += "<table><tr><td>";
  messageBody += "<form id='data' action='/firmware' method='POST' enctype='multipart/form-data'>";
  messageBody += "<input type='file' accept='.bin' class='custom-file-input' id='inputGroupFile04' name='update'><label class='custom-file-label' for='inputGroupFile04'>Choose file</label></td><td><button class='btn btn-danger' type='submit'>Update!</button><td></tr>";
  messageBody += "<tr><td><button type='button' class='btn btn-Primary' onclick='window.location.href=\"/\"'>Back</button></td></td>";
  messageBody += "</form></table></div>";


  messageBody += "<div id=\"NeoPixel\" class=\"tabcontent\"><h3>NeoPixel</h3>";
  messageBody += "<form method='get' action='neopixel'>";
  messageBody += "<table><tr><td>";
  messageBody += "<input type=\"color\" name=\"pixelcolor\" value=\"";
  messageBody += GPixelcolor;
  messageBody += "\"></td>";
  messageBody += "<td><input type=\"submit\"></td></tr>";
  messageBody += "</form></table></div>";


  messageBody += "<div id=\"XMas\" class=\"tabcontent\"><h3>XMas</h3>";
  messageBody += "<form method='get' action='XMas'>";
  messageBody += "<table><tr><td>";

  messageBody += "Pattern:</td><td>";
  messageBody += "<select name=\"Pattern\">";
  messageBody += "<option value=\"1\"";
  if (GPattern == 1) {
    messageBody += "selected";
  }
  messageBody += ">XMas Chase</option>";

  messageBody += "<option value=\"2\"";
  if (GPattern == 2) {
    messageBody += "selected";
  }
  messageBody += ">XMas Flash</option>";

  messageBody += "</select></td></tr>";
  messageBody += "<tr><td align=\"centre\"><label>Number of Pixels: </label></td><td align=\"centre\"><input name='pixelcount' length=32 value='";
  messageBody += GPixelCount;
  messageBody += "'></td><tr>";

  messageBody += "<td><input type=\"submit\"></td></tr>";
  messageBody += "</form></table></div>";


  messageBody += "<script> openTab(event, \"Home\")</script></body>";
  messageBody += "<br>Hostname: ";
  messageBody += String(Gdevname);
  messageBody += "<br>";
  messageBody += "<br>ESP ChipID: ";
  messageBody += String(ESP.getChipId());
  messageBody += "<br><br>";
  messageBody += "MAC Address: ";
  messageBody += String(getMAC());
  messageBody += "<br><br>";
  messageBody += "Firmware Version: ";
  messageBody += String( FW_VERSION );

  messageBody += "</html>";
  server.setContentLength(messageBody.length());
  server.send(200, "text/html", "");
  server.sendContent(messageBody);

  //  digitalWrite ( led, 0 );
}

void handleNotFound() {
  //digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  //  digitalWrite ( led, 0 );
}


void setup() {
  Serial.begin(115200);
  //  EEPROM.begin(512);
  //  delay(10);
  ESP.wdtEnable(30000);
  Serial.println("Reading EEPROM");
  loadSettingsFromEEPROM(false);

  pinMode(relayPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  if (GDevtype == 2 || GDevtype == 3 ) {
    pinMode(buttonPin, INPUT_PULLUP);
  }
  pinMode(D3, OUTPUT);

  digitalWrite(LEDPin, HIGH);
  if (GDevtype == 3 || GDevtype == 7 ) {
    pixels.begin(); // This initializes the NeoPixel library.
  }
  Serial.println("Startup");
  // read eeprom for ssid and pass

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  if (GDevtype == 5) {
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }


  if (GDevtype == 6) {
    emon1.current(A0, 111.1);
  }



  String esid = Gqsid;
  Serial.print("SSID: ");
  Serial.println(esid);
  String epass = Gqpass;
  Serial.print("PASS: ");
  Serial.println(epass);

  if (GDevtype == 2 || GDevtype == 3 ) {
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, FALLING);
  }


  if (GDevtype == 4) {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 10);
    display.println("AtomicSmart");
    display.println(millis());
    display.display();
  }

  if (GDevtype == 8) {
    nfc.begin();
  }

  uint32_t uniqueSwitchId = ESP.getChipId() + 80;
  char uuid[64];
  sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
            (uint16_t) ((uniqueSwitchId >> 16) & 0xff),
            (uint16_t) ((uniqueSwitchId >>  8) & 0xff),
            (uint16_t)   uniqueSwitchId        & 0xff);
  serial = String(uuid);
  persistent_uuid = "Socket-1_0-" + serial + "-80";

  if ( esid.length() > 1 ) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {


      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
      }
      Serial.println("");
      Serial.println("WiFi connected");

      MDNS.begin((String(Gdevname)).c_str());


      ArduinoOTA.setHostname((String(Gdevname)).c_str());
      ArduinoOTA.onStart([]() {
        Serial.println("Start");
      });
      ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });
      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });
      ArduinoOTA.begin();
      Serial.println("Ready");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());


      server.on ( "/mqttreconnect", reconnect );

      server.on( "/reboot", []() {
        content = HTMLhead;
        content += "<h3 style=\"color:#45BC00;\">Restarting Device </h3></body></html>";
        server.sendContent(content);
        delay(1000);
        ESP.restart();
      });

      server.on ( "/", handleRoot );
      server.on ( "/inline", []() {
        server.send ( 200, "text/plain", "this works as well" );
      } );
      server.on("/factoryreset", []() {
        if (server.arg("reset")) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 512; ++i) {
            EEPROM.write(i, 0);
          }
          EEPROM.commit();
          content = HTMLhead;
          content += "<h3 style=\"color:#45BC00;\">Please Restart the Device </h3></body></html>";
          server.sendContent(content);

        }
      });

      server.on("/neopixel", [&]() {
        String pixelcolor = server.arg("pixelcolor");
        Serial.print("Pixel Colour: ");
        Serial.println(pixelcolor);
        for (int i = 261; i < 268; ++i)
        {
          EEPROM.write(i, pixelcolor.charAt(i - 261));
        }
        EEPROM.commit();
        GPixelcolor = pixelcolor;
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });



      server.on("/XMas", [&]() {
        int Pattern = server.arg("Pattern").toInt();
        String TPixelCount = server.arg("pixelcount");

        Serial.print("Pattern: ");
        Serial.println(Pattern);
        Serial.print("PixelCount: ");
        Serial.println(server.arg("pixelcount"));

        EEPROM.write(304, Pattern);

        for (int i = 305; i < 310; ++i)
        {
          EEPROM.write(i, server.arg("pixelcount").charAt(i - 305));
        }
        EEPROM.commit();
        GPattern = Pattern;
        GPixelCount = server.arg("pixelcount").toInt();
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });





      server.on("/checkForUpdates", [&]() {
        checkForUpdates();
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });


      if (GAlexa) {

        server.on("/setup.xml", [&]() {
          handleSetupXml();
        });

        server.on("/upnp/control/basicevent1", [&]() {
          handleUpnpControl();
        });

        server.on("/eventservice.xml", [&]() {
          handleEventservice();
        });

        if (UDP.beginMulticast(WiFi.localIP(), ipMulti, portMulti)) {
          Serial.print("Udp multicast server started at ");
          Serial.print(ipMulti);
          Serial.print(":");
          Serial.println(portMulti);


        }
        else {
          Serial.println("Connection failed");
        }


      }
      server.on("/setting", handlesettings);


      server.on("/XMasaction", []() {
        if (GDevtype == 7) {
          Serial.println(server.arg("XMasState"));
          if (server.arg("XMasState") == "1") {
            turnOnXMas(true);
          }
          if (server.arg("XMasState") == "0") {
            turnOffXMas(true);
          }
        }
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });

      server.on("/switchaction", []() {
        if (GDevtype == 1 || GDevtype == 2) {
          Serial.println(server.arg("switchstate"));
          if (server.arg("switchstate") == "1") {
            turnOnRelay(true);
          }
          if (server.arg("switchstate") == "0") {
            turnOffRelay(true);
          }
        }
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });

      server.on("/switchupdate", []() {
        if (GDevtype == 1 || GDevtype == 2) {
          Serial.println(server.arg("switchenable"));
          if (server.arg("switchenable") == "1") {
            switchenable = true;
            EEPROM.write(300, 1);
            EEPROM.commit();
          }
          if (server.arg("switchenable") == "0") {
            switchenable = false;
            EEPROM.write(300, 0);
            EEPROM.commit();
          }
        }
        content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
        server.sendContent(content);
      });



      server.onNotFound ( handleNotFound );
      httpUpdater.setup(&server, update_path, update_username, update_password);
      server.begin();
      Serial.println ( "HTTP server started" );
      MDNS.addService("http", "tcp", 80);
      Serial.print("DNS Hostname: ");
      Serial.println((String(Gdevname)).c_str());

      if (GMQTT) {

        if (Gmqtthost.length() > 0) {

          char buf[100];
          snprintf( buf, sizeof(buf) - 1, "%s", Gmqtthost.c_str() );

          espclient.setServer(buf, 1883);
          espclient.setCallback(callback);
          if (!espclient.connected()) {
            reconnect();
          }
          espclient.loop();
          espclient.publish(("openhab/out/" + Gdevname + "/command").c_str(), "Awake");
          Serial.println(("openhab/out/" + Gdevname + "/command").c_str());

        }
      }
      return;
    } else {
      setupAP();
    }
  } else {
    setupAP();
  }

}

void setupAP() {

  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  String NewSSID = "AtomicSmart";
  String ESPID = String(ESP.getChipId());
  NewSSID += ESPID.substring(4);
  const char* ssidAP = (NewSSID).c_str();

  String NewPass = "welcomehome";
  const char* passAP = (NewPass).c_str();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passAP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.print("SSID:");
  Serial.println(ssidAP);

  dnsServer.start(DNS_PORT, "*", myIP);

  //  String ESPID = String(ESP.getChipId());
  Host += ESPID.substring(4);
  const char* MHost = (Host).c_str();
  MDNS.begin(MHost);
  MDNS.addService("http", "tcp", 80);

  server.on("/", handleRoot);
  server.on("/factoryreset", []() {
    if (server.arg("reset")) {
      Serial.println("clearing eeprom");
      for (int i = 0; i < 512; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      content = HTMLhead;
      content += "<h3>Please Restart the Device </h3></body></html>";
      server.sendContent(content);

    }
  });
  server.on("/setting", handlesettings);

  if (GAlexa) {

    server.on("/setup.xml", [&]() {
      handleSetupXml();
    });

    server.on("/upnp/control/basicevent1", [&]() {
      handleUpnpControl();
    });

    server.on("/eventservice.xml", [&]() {
      handleEventservice();
    });

  }
  httpUpdater.setup(&server, update_path, update_username, update_password);
  server.begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);
  Serial.print("DNS Hostname: ");
  Serial.println(MHost);
  APmode = true;
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void reconnect() {
  // Loop until we're reconnected
  espclient.disconnect();
  if (millis() - mqttpreviousMillis > mqttinterval || firstrun) {
    mqttpreviousMillis = millis();
    firstrun = false;

    Serial.print("Connecting to MQTT Broker: ");
    Serial.println(Gmqtthost);
    Serial.print("Username: ");
    Serial.println(Gmqttuser);
    Serial.print("Password: ");
    Serial.println(Gmqttpass);
    if (espclient.connect((String(ESP.getChipId())).c_str(), (String(Gmqttuser)).c_str(), (String(Gmqttpass)).c_str())) {
      espclient.subscribe(("openhab/out/" + Gdevname + "/command").c_str());
      Mqttconnectionstatus = "Connected";
      Serial.println("Connected");
      mqttreconnecttrycount = 0;
    } else {
      mqttreconnecttrycount += 1;
      Mqttconnectionstatus = "Failed to Connect";
      Serial.println("Failed to Connect");
      if (mqttreconnecttrycount > 10) {
        ESP.restart();
      }
    }
  }
}

void turnOnXMas(bool report) {
  XMasState = true;

  relayState = true;

  if (report) {
    if (GMQTT) {
      espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "ON");
    }
  }
  if (GLED) {
    digitalWrite(LEDPin, LOW);
  }
}

void turnOffXMas(bool report) {
  XMasState = false;

  relayState = false;
  if (report) {
    if (GMQTT) {
      espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "OFF");
    }
  }
  if (GLED) {
    digitalWrite(LEDPin, HIGH);
  }
}

void turnOnRelay(bool report) {
  digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH

  relayState = true;

  if (report) {
    if (GMQTT) {
      espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "ON");
    }
  }
  if (GLED) {
    digitalWrite(LEDPin, LOW);
  }
}

void turnOffRelay(bool report) {
  digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW

  relayState = false;
  if (report) {
    if (GMQTT) {
      espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "OFF");
    }
  }
  if (GLED) {
    digitalWrite(LEDPin, HIGH);
  }
}

void loop() {
  ESP.wdtFeed();
  dnsServer.processNextRequest();

  if (GDevtype == 8) {


    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
    // the UID, and uidLength will indicate the size of the UUID (normally 7)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

    if (success) {
      // Display some basic information about the card
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc.PrintHex(uid, uidLength);
      Serial.println("");
      String NewUID = "";
      for ( uint8_t i = 0; i < uidLength; i++) {
        NewUID += String(uid[i], HEX);
      }
      Serial.println(NewUID);

      unsigned long NFCcurrentMillis = millis();
      if (NFCcurrentMillis - NFCpresspreviousMillis > NFCinterval) {
        espclient.publish("openhab/in/NFC/state", String(NewUID).c_str());
        LastTag = String(NewUID).c_str();
        NFCpresspreviousMillis = NFCcurrentMillis;
      }

    }
  }



  if (GMQTT) {
    espclient.loop();
  }
  ArduinoOTA.handle();
  server.handleClient();


  if (Gautoupdate) {
    unsigned long autoupdatecurrentMillis = millis();

    if (autoupdatecurrentMillis - autoupdatepresspreviousMillis > autoupdateinterval || firstrunAU) {
      firstrunAU = false;
      autoupdatepresspreviousMillis = autoupdatecurrentMillis;
      checkForUpdates();
    }
  }



  if (GDevtype == 5) {
    if ((millis() - oldTime) > 1000)   // Only process counters once per second
    {
      // Disable the interrupt while calculating flow rate and sending the value to
      // the host
      detachInterrupt(sensorInterrupt);

      // Because this loop may not complete in exactly 1 second intervals we calculate
      // the number of milliseconds that have passed since the last execution and use
      // that to scale the output. We also apply the calibrationFactor to scale the output
      // based on the number of pulses per second per units of measure (litres/minute in
      // this case) coming from the sensor.
      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

      // Note the time this processing pass was executed. Note that because we've
      // disabled interrupts the millis() function won't actually be incrementing right
      // at this point, but it will still return the value it was set to just before
      // interrupts went away.
      oldTime = millis();

      // Divide the flow rate in litres/minute by 60 to determine how many litres have
      // passed through the sensor in this 1 second interval, then multiply by 1000 to
      // convert to millilitres.
      flowMilliLitres = (flowRate / 60) * 1000;

      // Add the millilitres passed in this second to the cumulative total
      totalMilliLitres += flowMilliLitres;

      unsigned int frac;

      // Print the flow rate for this second in litres / minute
      Serial.print("Flow rate: ");
      Serial.print(int(flowRate));  // Print the integer part of the variable
      Serial.print(".");             // Print the decimal point
      // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
      frac = (flowRate - int(flowRate)) * 10;
      Serial.print(frac, DEC) ;      // Print the fractional part of the variable
      Serial.print("L/min");
      // Print the number of litres flowed in this second
      Serial.print("  Current Liquid Flowing: ");             // Output separator
      Serial.print(flowMilliLitres);
      Serial.print("mL/Sec");

      // Print the cumulative total of litres flowed since starting
      Serial.print("  Output Liquid Quantity: ");             // Output separator
      Serial.print(totalMilliLitres);
      Serial.println("mL");

      // Reset the pulse counter so we can start incrementing again
      pulseCount = 0;

      // Enable the interrupt again now that we've finished sending output
      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);


      if (frac > 0.00 ) {
        //        EEPROMWritelong(271, totalMilliLitres);
        liquidflowing = true;
        espclient.publish(("openhab/out/" + Gdevname + "flowRate/state").c_str(), String(flowRate).c_str());
        espclient.publish(("openhab/out/" + Gdevname + "flowMilliLitres/state").c_str(), String(flowMilliLitres).c_str());
        espclient.publish(("openhab/out/" + Gdevname + "totalMilliLitres/state").c_str(), String(totalMilliLitres).c_str());
        oldpulseCount = pulseCount;
      } else {
        if (liquidflowing) {
          espclient.publish(("openhab/out/" + Gdevname + "flowRate/state").c_str(), String(flowRate).c_str());
          espclient.publish(("openhab/out/" + Gdevname + "flowMilliLitres/state").c_str(), String(flowMilliLitres).c_str());
          espclient.publish(("openhab/out/" + Gdevname + "totalMilliLitres/state").c_str(), String(totalMilliLitres).c_str());
          liquidflowing = false;
        }
      }

    }
  }

  if (GDevtype == 6) {
    if (millis() - PowerpreviousMillis > Powerinterval) {
      Irms = emon1.calcIrms(1480);  // Calculate Irms only
      espclient.publish(("openhab/out/" + Gdevname + "power/state").c_str(), String(Irms * PowerFactor).c_str());
      PowerpreviousMillis = millis();
    }
  }

  if (!GLED) {
    digitalWrite(LEDPin, HIGH);
  }



  if (GDevtype == 7) {
    if (XMasState) {
      //RainbowCycleUpdate();
      //      xmas(0);
            if (GPattern == 1) {
              RainbowCycleUpdate();
            }
            if (GPattern == 2) {
              xmas(0);
            }
    }
    else {
      for (uint16_t j = 0; j < pixels.numPixels(); j++) {
        pixels.setPixelColor(j  , 0, 0, 0); // Draw new pixel
      }
      pixels.show();
    }

  }

  if (GAlexa) {
    serverLoop();
  }
  if (GMQTT) {
    if (relayState != lastrelaystate) {
      if (relayState == true) {
        espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "ON");
      } else if (relayState == false) {
        espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "OFF");
      }
      lastrelaystate = relayState;
    }
  }
  if (GDevtype == 3) {
    if ( DBellState != lastDBellState) {

      espclient.publish(("openhab/in/" + Gdevname + "/state").c_str(), "ON");
      Serial.println("DoorBell Pressed");
      lastDBellState = DBellState;
      digitalWrite(LEDPin, LOW);
      delay(1000);
      digitalWrite(LEDPin, HIGH);
    }
  }
  if (ignoreserver) {
    if (millis() - last_interrupt_time > 5000)
    {
      ignoreserver = false;
    }
  }
  delay(25);
  if (APmode) {
    return;
  } else {

    if (GMQTT) {
      if (Gmqtthost.length() > 0) {
        if (!espclient.connected()) {
          reconnect();
        }
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi Disconnected...");
      Serial.println("Reconnecting Wifi...");
      WiFi.begin((Gqsid).c_str(), (Gqpass).c_str());
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      }
      reconnect();
    }
    delay(25);
  }

  if (GDevtype == 4) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval || firstrunDS) {
      previousMillis = currentMillis;
      firstrunDS = false;

      byte i;
      byte present = 0;
      byte type_s;
      byte data[12];
      byte addr[8];
      float celsius, fahrenheit;

      if ( !ds.search(addr))
      {
        ds.reset_search();
        delay(250);
        return;
      }


      if (OneWire::crc8(addr, 7) != addr[7])
      {
        Serial.println("CRC is not valid!");
        return;
      }
      Serial.println();

      // the first ROM byte indicates which chip
      switch (addr[0])
      {
        case 0x10:
          type_s = 1;
          break;
        case 0x28:
          type_s = 0;
          break;
        case 0x22:
          type_s = 0;
          break;
        default:
          Serial.println("Device is not a DS18x20 family device.");
          return;
      }

      ds.reset();
      ds.select(addr);
      ds.write(0x44, 1);        // start conversion, with parasite power on at the end
      delay(1000);
      present = ds.reset();
      ds.select(addr);
      ds.write(0xBE);         // Read Scratchpad

      for ( i = 0; i < 9; i++)
      {
        data[i] = ds.read();
      }

      // Convert the data to actual temperature
      int16_t raw = (data[1] << 8) | data[0];
      if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10)
        {
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      }
      else
      {
        byte cfg = (data[4] & 0x60);
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

      }
      celsius = (float)raw / 16.0 - 6;
      Gcelsius = celsius;

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println(celsius);
      display.display();

      espclient.publish(("openhab/out/" + String(Gdevname) + "/state").c_str(), String(celsius).c_str());

      Serial.println( String(celsius).c_str());

      Serial.println("Waiting…");

    }

  }

}

void loadSettingsFromEEPROM(bool first)
{

  Gqsid = "";
  for (int i = 0; i < 32; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gqsid += char(EEPROM.read(i));
  }
  Gqpass = "";
  for (int i = 32; i < 96; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gqpass += char(EEPROM.read(i));
  }
  Gmqtthost = "";
  for (int i = 96; i < 128; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gmqtthost += char(EEPROM.read(i));
  }
  Gmqttport = "";
  for (int i = 128; i < 160; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gmqttport += char(EEPROM.read(i));
  }
  Gmqttuser = "";
  for (int i = 160; i < 192; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gmqttuser += char(EEPROM.read(i));
  }
  Gmqttpass = "";
  for (int i = 192; i < 224; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gmqttpass += char(EEPROM.read(i));
  }
  Gdevname = "";
  for (int i = 224; i < 256; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    Gdevname += char(EEPROM.read(i));
  }


  GAlexa = EEPROM.read(257);
  GMQTT = EEPROM.read(258);
  GLED = EEPROM.read(259);
  GNeoPixel = EEPROM.read(260);
  GDevtype = EEPROM.read(269);
  Gautoupdate = EEPROM.read(270);

  totalMilliLitres = 0;
  for (int i = 271; i < 303; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    totalMilliLitres += char(EEPROM.read(i));
  }

  GPattern = EEPROM.read(304);

  GPixelCount = "";
  for (int i = 305; i < 310; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    GPixelCount += char(EEPROM.read(i));
  }


  GPixelcolor = "";
  for (int i = 261; i < 268; ++i)
  {
    if (EEPROM.read(i) == 0) break;
    GPixelcolor += char(EEPROM.read(i));
  }
  Serial.print("Pixel Colour: ");
  Serial.println(GPixelcolor);

  switchenable = EEPROM.read(300);


}


void handleEventservice() {
  Serial.println(" ########## Responding to eventservice.xml ... ########\n");

  String eventservice_xml = "<scpd xmlns=\"urn:Belkin:service-1-0\">"
                            "<actionList>"
                            "<action>"
                            "<name>SetBinaryState</name>"
                            "<argumentList>"
                            "<argument>"
                            "<retval/>"
                            "<name>BinaryState</name>"
                            "<relatedStateVariable>BinaryState</relatedStateVariable>"
                            "<direction>in</direction>"
                            "</argument>"
                            "</argumentList>"
                            "</action>"
                            "</actionList>"
                            "<serviceStateTable>"
                            "<stateVariable sendEvents=\"yes\">"
                            "<name>BinaryState</name>"
                            "<dataType>Boolean</dataType>"
                            "<defaultValue>0</defaultValue>"
                            "</stateVariable>"
                            "<stateVariable sendEvents=\"yes\">"
                            "<name>level</name>"
                            "<dataType>string</dataType>"
                            "<defaultValue>0</defaultValue>"
                            "</stateVariable>"
                            "</serviceStateTable>"
                            "</scpd>\r\n"
                            "\r\n";

  server.send(200, "text/plain", eventservice_xml.c_str());
}

void handleUpnpControl() {
  Serial.println("########## Responding to  /upnp/control/basicevent1 ... ##########");

  //for (int x=0; x <= HTTP.args(); x++) {
  //  Serial.println(HTTP.arg(x));
  //}

  String request = server.arg(0);
  Serial.print("request:");
  Serial.println(request);

  Serial.println("Responding to Control request");

  String response_xml = "";

  if (request.indexOf("<BinaryState>1</BinaryState>") > 0) {
    Serial.println("Got Turn on request");
    turnOnRelay(true);
    response_xml =  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                    "<s:Body>"
                    "<u:SetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
                    "<BinaryState>1</BinaryState>"
                    "</u:SetBinaryStateResponse>"
                    "</s:Body>"
                    "</s:Envelope>\r\n"
                    "\r\n";
  }

  if (request.indexOf("<BinaryState>0</BinaryState>") > 0) {
    Serial.println("Got Turn off request");
    turnOffRelay(true);
    response_xml =  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                    "<s:Body>"
                    "<u:SetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
                    "<BinaryState>0</BinaryState>"
                    "</u:SetBinaryStateResponse>"
                    "</s:Body>"
                    "</s:Envelope>\r\n"
                    "\r\n";
  }

  server.send(200, "text/xml", response_xml.c_str());
  Serial.print("Sending :");
  Serial.println(response_xml);
}

void handleSetupXml() {
  Serial.println(" ########## Responding to setup.xml ... ########\n");

  IPAddress localIP = WiFi.localIP();
  char s[16];
  sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

  String setup_xml = "<?xml version=\"1.0\"?>"
                     "<root xmlns=\"urn:Belkin:device-1-0\">"
                     "<specVersion>"
                     "<major>1</major>"
                     "<minor>0</minor>"
                     "</specVersion>"
                     "<device>"
                     "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
                     "<friendlyName>" + Gdevname + "</friendlyName>"
                     "<manufacturer>Belkin International Inc.</manufacturer>"
                     "<modelName>Emulated Socket</modelName>"
                     "<modelNumber>3.1415</modelNumber>"
                     "<manufacturerURL>http://www.belkin.com</manufacturerURL>"
                     "<modelDescription>Belkin Plugin Socket 1.0</modelDescription>"
                     "<modelURL>http://www.belkin.com/plugin/</modelURL>"
                     "<UDN>uuid:" + persistent_uuid + "</UDN>"
                     "<serialNumber>" + serial + "</serialNumber>"
                     "<binaryState>0</binaryState>"
                     "<serviceList>"
                     "<service>"
                     "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
                     "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
                     "<controlURL>/upnp/control/basicevent1</controlURL>"
                     "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
                     "<SCPDURL>/eventservice.xml</SCPDURL>"
                     "</service>"
                     "</serviceList>"
                     "</device>"
                     "</root>\r\n"
                     "\r\n";

  server.send(200, "text/xml", setup_xml.c_str());

  Serial.print("Sending :");
  Serial.println(setup_xml);
}

String getAlexaInvokeName() {
  return Gdevname;
}

void respondToSearch(IPAddress senderIP, unsigned int senderPort) {
  Serial.println("");
  Serial.print("Sending response to ");
  Serial.println(senderIP);
  Serial.print("Port : ");
  Serial.println(senderPort);

  IPAddress localIP = WiFi.localIP();
  char s[16];
  sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

  String response =
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=86400\r\n"
    "DATE: Sat, 26 Nov 2016 04:56:29 GMT\r\n"
    "EXT:\r\n"
    "LOCATION: http://" + String(s) + ":80/setup.xml\r\n"
    "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
    "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
    "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
    "ST: urn:Belkin:device:**\r\n"
    "USN: uuid:" + persistent_uuid + "::urn:Belkin:device:**\r\n"
    "X-User-Agent: redsonic\r\n\r\n";

  UDP.beginPacket(senderIP, senderPort);
  UDP.write(response.c_str());
  UDP.endPacket();

  Serial.println("Response sent !");
}

void serverLoop() {

  int packetSize = UDP.parsePacket();
  if (packetSize > 0)
  {
    IPAddress senderIP = UDP.remoteIP();
    unsigned int senderPort = UDP.remotePort();

    // read the packet into the buffer
    UDP.read(packetBuffer, packetSize);

    // check if this is a M-SEARCH for WeMo device
    String request = String((char *)packetBuffer);
    Serial.println("----------");
    Serial.println(request);
    Serial.println("-----------");
    if (request.indexOf('M-SEARCH') > 0) {
      if ((request.indexOf("urn:Belkin:device:**") > 0) || (request.indexOf("ssdp:all") > 0) || (request.indexOf("upnp:rootdevice") > 0)) {
        Serial.println("Got UDP Belkin Request..");
        respondToSearch(senderIP, senderPort);
      }
    }
  }
}


String getMAC()
{
  uint8_t mac[6];
  char result[14];

  snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );

  return String( result );
}

void checkForUpdates() {
  String mac = String(ESP.getChipId());
  String fwURL = String( fwUrlBase );
  fwURL.concat( "AtomicSmart" );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  Serial.print( "MAC address: " );
  Serial.println( String(mac) );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if ( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );



    int newVersion = newFWVersion.toInt();

    if ( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );
      content = HTMLhead;
      content += "<h3>Updating. Please wait... </h3></body></html>";
      server.sendContent(content);

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
  }
  httpClient.end();
}

void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}


void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  byte aone = ((value >> 32) & 0xFF);
  byte bone = ((value >> 40) & 0xFF);
  byte cone = ((value >> 48) & 0xFF);
  byte done = ((value >> 56) & 0xFF);
  byte eone = ((value >> 64) & 0xFF);
  byte fone = ((value >> 72) & 0xFF);



  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
  EEPROM.write(address + 4, aone);
  EEPROM.write(address + 5, bone);
  EEPROM.write(address + 6, cone);
  EEPROM.write(address + 7, done);
  EEPROM.write(address + 8, eone);
  EEPROM.write(address + 9, fone);
}

static void chase(uint32_t c) {
  for (uint16_t i = 0; i < pixels.numPixels() + 4; i++) {
    pixels.setPixelColor(i  , c); // Draw new pixel
    pixels.setPixelColor(i - 4, 0); // Erase pixel a few steps back
    pixels.show();
    delay(25);
  }
}
void (*OnComplete)();  // Callback on completion of pattern
void Increment()
{

  Index++;
  if (Index >= TotalSteps)
  {
    Index = 0;
    if (OnComplete != NULL)
    {
      OnComplete(); // call the comlpetion callback
    }
  }
}


uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
void RainbowCycleUpdate()
{
  for (int i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + Index) & 255));
  }
  espclient.loop();
  pixels.show();
  Increment();
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, c);  //turn every third pixel on
      }
      pixels.show();
      espclient.loop();
      delay(wait);

      for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
    espclient.loop();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    espclient.loop();
    pixels.show();
    delay(wait);
  }
}
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    espclient.loop();
    pixels.show();
    delay(wait);
  }
}
//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      pixels.show();
      espclient.loop();
      delay(wait);

      for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3) {
        pixels.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}
static void xmas(int c) {
  espclient.loop();
  int posi = 0;
  int pixcount = 15;
  uint32_t red = (255, 0, 0);
  uint32_t green = (0, 255, 0);
  uint32_t blue = (0, 0, 255);
  theaterChase(pixels.Color(127, 127, 127), 50); // White
  espclient.loop();
  theaterChase(pixels.Color(0, 0, 255), 50); // Blue
  espclient.loop();
  colorWipe(pixels.Color(255, 0, 0), 50); // Red
  espclient.loop();
  colorWipe(pixels.Color(0, 255, 0), 50); // Green
  espclient.loop();
  colorWipe(pixels.Color(0, 0, 255), 50); // Blue
  espclient.loop();
  colorWipe(pixels.Color(127, 127, 127), 50); // White
  espclient.loop();
  for (int k = 0; k < 2; k++) {
    for (uint16_t j = c; j < pixels.numPixels(); j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 30  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 60  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 90  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 120  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 150  , 255, 0, 0); // Draw new pixel

      pixels.setPixelColor(j + 15  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 45  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 75  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 105  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 135  , 0, 255, 0); // Draw new pixel
      pixels.show();
      delay(25);
    }

    for (uint16_t j = c; j < pixels.numPixels(); j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 30  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 60  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 90  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 120  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j + 150  , 0, 255, 0); // Draw new pixel

      pixels.setPixelColor(j + 15  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 45  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 75  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 105  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j + 135  , 255, 0, 0); // Draw new pixel
      pixels.show();
      delay(25);
    }
  }
  delay(1000);
  for (int k = 0; k < 2; k++) {
    espclient.loop();
    for (uint16_t j = 0; j < pixels.numPixels() / 2; j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel

    }

    for (uint16_t j = pixels.numPixels() / 2; j < pixels.numPixels(); j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel

    }
    pixels.show();
    delay(1000);
    for (uint16_t j = 0; j < pixels.numPixels() / 2; j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
    }

    for (uint16_t j = pixels.numPixels() / 2; j < pixels.numPixels(); j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
    }
    pixels.show();
    delay(1000);
  }


  for (int k = 0; k < 2; k++) {
    espclient.loop();
    for (uint16_t j = 0; j < pixels.numPixels() / 4; j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel

    }

    for (uint16_t j = pixels.numPixels() / 4; j < pixels.numPixels() / 2; j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel

    }
    pixels.show();
    delay(1000);
    for (uint16_t j = pixels.numPixels() / 2; j < (pixels.numPixels() / 2) + pixels.numPixels() / 4; j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
      pixels.setPixelColor(j  , 0, 255, 0); // Draw new pixel
    }

    for (uint16_t j = (pixels.numPixels() / 2) + pixels.numPixels() / 4; j < pixels.numPixels(); j++) {
      espclient.loop();
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel
      pixels.setPixelColor(j  , 255, 0, 0); // Draw new pixel

    }
    pixels.show();
    delay(1000);

  }
}
