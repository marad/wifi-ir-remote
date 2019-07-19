#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

const char* ssid = "SSID";
const char* password = "PASSWORD";

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
WiFiServer server(80);
String header;
String currentLine;

void connectToWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFiConnected");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
}

void readLine(WiFiClient &client) {
  currentLine = "";
  while(client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        break;
      }
      currentLine += c;
    }
  }
}

void setup() {
  irsend.begin();
  // Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.begin(9600);
  // irsend.sendNEC(0x20DF10EF);
  connectToWifi();
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Client connected.");
    readLine(client); // read HTTP header

    // Ignore rest of the request
    while(client.connected()) {
      if(client.available()) {
        client.read();
      } else {
        break;
      }
    }

    Serial.print("Client request: ");
    Serial.println(currentLine);

    // extract type
    int typeStart = currentLine.indexOf('/') + 1;
    int typeEnd = currentLine.indexOf('/', typeStart);
    String type = currentLine.substring(typeStart, typeEnd);
    // extract code
    int codeStart = currentLine.indexOf('/', typeEnd) + 1;
    int codeEnd = currentLine.indexOf(' ', codeStart);
    String code = currentLine.substring(codeStart, codeEnd);
    long intCode = strtol(code.c_str(), NULL, 16);

    Serial.print("Send "); Serial.print(type); Serial.print(" code:");
    Serial.print(code);
    Serial.print("("); Serial.print(intCode); Serial.println(")");

    // match request path
    boolean sent = false;
    if (currentLine.startsWith("GET /nec/")) {
      irsend.sendNEC(intCode);
      sent = true;
    }

    // send response
    if (sent) {
      client.println("HTTP/1.1 200 OK");
      client.println("Access-Control-Allow-Origin:*");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.println("Code sent!");
      client.println();
      client.println();
    } else {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Access-Control-Allow-Origin:*");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.println("Bad request");
      client.println();
      client.println();
    }

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println();
  }
}
