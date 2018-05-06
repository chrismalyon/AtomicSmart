#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "htmlhead.h"
#include <ESP8266HTTPUpdateServer.h>

IPAddress ipMulti(239, 255, 255, 250);
const unsigned int portMulti = 1900;
char packetBuffer[512];

#define MAX_SWITCHES 14
int numOfSwitchs = 0;

WiFiUDP UDP;

const int relayPin = D1;
const int buttonPin = D2;
const int LEDPin = D4;

boolean firstrun = true;
boolean ignoreserver = false;

String Host = "AtomicSmart";
const char* update_username = "admin";
const char* update_password = "*******";
const char* update_path = "/firmware";
ESP8266HTTPUpdateServer httpUpdater;

unsigned long last_interrupt_time = 0;

String Mqttconnectionstatus = "";

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

long mqttpreviousMillis = 0;
long mqttinterval = 10000;

long buttonpresspreviousMillis = 0;
long buttonpressinterval = 3000;



const char* ssid = "AtomicSmart1";

const char* password = "welcomeAtomicSmart";

const char* mqtt_server = "mqtt.Server.co.uk";

char NRValue[8];
String MainTopic;
const char * c;
const long togDelay = 100;  // pause for 100 milliseconds before toggling to Open
const long postDelay = 200;  // pause for 200 milliseconds after toggling to Open
int value = 0;
boolean relayState;
boolean lastrelaystate;
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

  last_PayloadTime = millis();

}


void buttonPress()

{
  if (millis() - last_interrupt_time > 1000)
  {
    ignoreserver = true;
    last_interrupt_time = millis();
    Serial.println("Button Pressed");
    if (relayState == LOW) {
      digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
      relayState = true;
      if (GLED){
      digitalWrite(LEDPin, LOW);
      }



    } else if (relayState == HIGH) {
      digitalWrite(relayPin, LOW); // turn on relay with voltage HIGH
      relayState = false;
      if (GLED){
      digitalWrite(LEDPin, HIGH);
      }

    }

  }


}


void handleRoot() {
  //  digitalWrite ( led, 1 );
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;


  String messageBody = HTMLhead;

  messageBody += "<div class=\"tab\">\
  <button class=\"tablinks\" onclick=\"openTab(event, 'Home')\">Home</button>\
  <button class=\"tablinks\" onclick=\"openTab(event, 'Settings')\">Settings</button>\
  <button class=\"tablinks\" onclick=\"openTab(event, 'Update')\">Update</button>\
  </div>";
  messageBody += "<div id=\"Settings\" class=\"tabcontent\"><h3>Settings</h3>";
  messageBody += "<table width=\"800px\">";
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
  messageBody += "<form method='get' action='setting'><tr><td align=\"centre\"><label>Wifi SSID: </label></td><td align=\"centre\"><input name='ssid' length=32 value='";
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
  messageBody += "'<tr><td>Enable Alexa Support:</td><td>";

  if (GAlexa) {
    messageBody += "<input type=\"hidden\" name=\"Alexa\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"Alexa\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td></td><td></td><td></td><td align=\"centre\"></td></tr>";
  messageBody += "<tr><td></td></tr>";


  messageBody += "'<tr><td>Enable Status LED:</td><td>";

  if (GLED) {
    messageBody += "<input type=\"hidden\" name=\"LED\" value=\"1\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" checked>";
  } else {
    messageBody += "<input type=\"hidden\" name=\"LED\" value=\"0\"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\">";
  }

  messageBody += "</td></tr>";
  messageBody += "<tr><td></td><td></td><td></td><td align=\"centre\"><input type='submit'></form></td></tr>";
  messageBody += "<tr><td></td></tr>";


  messageBody += "<tr><td></td></tr>";
  messageBody += "<tr><td><form method='get' action='factoryreset'><label>Factory Reset: </label><input type='hidden' name='reset' value='true'></td><td><input type='button' onClick=\"confSubmit(this.form);\" value='Reset'></form><br></td></tr></table></div>";

  messageBody += "<div id=\"Home\" class=\"tabcontent\"><h3>Home</h3>";

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
  messageBody += "</div>";



  messageBody += "<div id=\"Update\" class=\"tabcontent\"><h3>Update</h3>";
  messageBody += "<table><tr><td>";
  messageBody += "<form id='data' action='/firmware' method='POST' enctype='multipart/form-data'>";
  messageBody += "<input type='file' accept='.bin' class='custom-file-input' id='inputGroupFile04' name='update'><label class='custom-file-label' for='inputGroupFile04'>Choose file</label></td><td><button class='btn btn-danger' type='submit'>Update!</button><td></tr>";
  messageBody += "<tr><td><button type='button' class='btn btn-Primary' onclick='window.location.href=\"/\"'>Back</button></td></td>";
  messageBody += "</form></table></div></main></body></html>";


  messageBody += "<script> openTab(event, \"Home\")</script></body>";
  messageBody += "<br>ESP ChipID: ";
  messageBody += String(ESP.getChipId());

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
  EEPROM.begin(512);
  delay(10);
  ESP.wdtEnable(30000);
  pinMode(relayPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(D3, OUTPUT);
  
  digitalWrite(LEDPin, HIGH);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM");
  loadSettingsFromEEPROM(false);
  String esid = Gqsid;
  Serial.print("SSID: ");
  Serial.println(esid);
  String epass = Gqpass;
  Serial.print("PASS: ");
  Serial.println(epass);

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, FALLING);


  uint32_t uniqueSwitchId = ESP.getChipId() + 80;
  char uuid[64];
  sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
            (uint16_t) ((uniqueSwitchId >> 16) & 0xff),
            (uint16_t) ((uniqueSwitchId >>  8) & 0xff),
            (uint16_t)   uniqueSwitchId        & 0xff);
  serial = String(uuid);
  persistent_uuid = "Socket-1_0-" + serial + "-80";

  if ( esid.length() > 1 ) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {


      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
      }
      Serial.println("");
      Serial.println("WiFi connected");


      String ESPID = String(ESP.getChipId());
      Host += ESPID.substring(4);
      const char* MHost = (Host).c_str();
      MDNS.begin(MHost);


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
      server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        String mqtthost = server.arg("mqtthost");
        String mqttport = server.arg("mqttport");
        String mqttuser = server.arg("mqttuser");
        String mqttpass = server.arg("mqttpass");
        String devname = server.arg("devname");
        boolean Alexa;
        if (server.arg("Alexa") == "1") {
          Alexa = true;
        } else if (server.arg("Alexa") == "0") {
          Alexa = false;
        }
        boolean MQTT;
        if (server.arg("MQTT") == "1") {
          MQTT = true;
        } else if (server.arg("MQTT") == "0") {
          MQTT = false;
        }

        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 512; ++i) {
            EEPROM.write(i, 0);
          }
          boolean LED;
          if (server.arg("LED") == "1") {
            LED = true;
          } else if (server.arg("LED") == "0") {
            LED = false;
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

          EEPROM.commit();
          loadSettingsFromEEPROM(false);

          content = "<html><head><meta http-equiv=\"refresh\" content=\"0;URL='./'\" /></head></html>";
          statusCode = 200;
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        content = HTMLhead;
        content += "<h3>Please Restart the Device </h3></body></html>";
        server.sendContent(content);
        //        digitalWrite(D3,HIGH);
        //ESP.reset();
      });

      server.on("/switchaction", []() {
        Serial.println(server.arg("switchstate"));
        if (server.arg("switchstate") == "1") {
          turnOnRelay(true);
        }
        if (server.arg("switchstate") == "0") {
          turnOffRelay(true);
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
      Serial.println(MHost);

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
    }
  }
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  String NewSSID = "AtomicSmart";
  String ESPID = String(ESP.getChipId());
  NewSSID += ESPID.substring(4);
  const char* ssidAP = (NewSSID).c_str();

  String NewPass = "welcomehome";
  const char* passAP = (NewPass).c_str();

  WiFi.softAP(ssidAP, passAP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.print("SSID:");
  Serial.println(ssidAP);


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
  server.on("/setting", []() {
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    String mqtthost = server.arg("mqtthost");
    String mqttport = server.arg("mqttport");
    String mqttuser = server.arg("mqttuser");
    String mqttpass = server.arg("mqttpass");
    String devname = server.arg("devname");
    boolean Alexa;
    if (server.arg("Alexa") == "1") {
      Alexa = true;
    } else if (server.arg("Alexa") == "0") {
      Alexa = false;
    }
    boolean MQTT;
    if (server.arg("MQTT") == "1") {
      MQTT = true;
    } else if (server.arg("MQTT") == "0") {
      MQTT = false;
    }
    boolean LED;
    if (server.arg("LED") == "1") {
      LED = true;
    } else if (server.arg("LED") == "0") {
      LED = false;
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

      EEPROM.commit();
      loadSettingsFromEEPROM(false);

      content = HTMLhead;
      content += "<h3 style=\"color:#45BC00;\">Please Restart the Device </h3></body></html>";
      statusCode = 200;
    } else {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
    }
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

    } else {
      Mqttconnectionstatus = "Failed to Connect";
      Serial.println("Failed to Connect");
    }
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
  if (GLED){
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
  if (GLED){
  digitalWrite(LEDPin, HIGH);
  }
}

void loop() {
  ESP.wdtFeed();
  if (GMQTT) {
    espclient.loop();
  }
  ArduinoOTA.handle();
  server.handleClient();

  if (!GLED){
      digitalWrite(LEDPin, HIGH);
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
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
    }
    delay(25);
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
    // Serial.println("----------");
    // Serial.println(request);
    // Serial.println("-----------");
    if (request.indexOf('M-SEARCH') > 0) {
      if ((request.indexOf("urn:Belkin:device:**") > 0) || (request.indexOf("ssdp:all") > 0) || (request.indexOf("upnp:rootdevice") > 0)) {
        Serial.println("Got UDP Belkin Request..");
        respondToSearch(senderIP, senderPort);
      }
    }
  }
}

