#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include <CAN.h>

bool speedSet = false;
const long canBaudRate = 250E3;
const long esp32BaudRate = 115200;
const long canId = 0x85103203; // CAN ID
const int speedLimit = 35; // Speed limit in km/h
int wheelSize = 0; // Wheel size in mm (calculated to match 0xC0 and 0x2B)
int wheelPerimeter = 0; // Wheel perimeter in mm (calculated to match 0xCE and 0x08)
bool logOnlyMode = true; // Set to true for log-only mode
const int CAN_TX_PIN = 5; // Set your CAN TX pin
const int CAN_RX_PIN = 4; // Set your CAN RX pin
String logBuffer = ""; // Buffer for logging messages

const char* ssid     = "ESP32-Velo";
const char* password = "123456789";

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(8,8,8,8); // Google Public DNS

WebServer server(80);

// Forward declarations for handler functions
void handle_OnConnect();
void handle_led1on();
void handle_led1off();
void handle_led2on();
void handle_led2off();
void handle_NotFound();
void handle_setSpeed();
void handle_readSpeed();
void handle_toggleLogMode();
void appendToLog(const String& message);
void printRepeatedMessage(const char* message, int count);
void writeToCan(int speedLimit, int wheelSize, int wheelPerimeter);
String SendHTML(uint8_t led1stat, uint8_t led2stat);

uint8_t LED1pin = 22; // Correct led
bool LED1status = LOW;

uint8_t LED2pin = 5;
bool LED2status = LOW;

void setup() {
  Serial.begin(esp32BaudRate);
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet, dns); // Added DNS server configuration
  
  Serial.println("AP started");
  appendToLog("AP started");
  Serial.print("AP IP address: ");
  appendToLog("AP IP address: "); 
  Serial.println(WiFi.softAPIP());
  appendToLog(WiFi.softAPIP().toString());

  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  server.on("/setSpeed", handle_setSpeed); // New route for speed setting
  server.on("/readSpeed", handle_readSpeed); // New route for reading speed
  server.on("/toggleLogMode", handle_toggleLogMode); // New route for toggling log mode
  server.on("/log", []() {
    server.send(200, "text/plain", logBuffer); // Send the log buffer as plain text
  });
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
  appendToLog("HTTP server started");

  // Set CAN pins
  CAN.setPins(CAN_TX_PIN, CAN_RX_PIN);

  if (!CAN.begin(canBaudRate)) {
    Serial.println("Starting CAN failed!");
    appendToLog("Starting CAN failed!");
    while (1);
  }

  Serial.println("CAN setup ok");
  appendToLog("CAN setup ok");

  // if (!logOnlyMode) { //why?
  //   writeToCan(speedLimit, wheelSize, wheelPerimeter);
  // }
}

void loop() {
  server.handleClient();
  if(LED1status)
  {digitalWrite(LED1pin, HIGH);}
  else
  {digitalWrite(LED1pin, LOW);}
  
  if(LED2status)
  {digitalWrite(LED2pin, HIGH);}
  else
  {digitalWrite(LED2pin, LOW);}
}

void appendToLog(const String& message) {
  logBuffer += message + "\n"; // Add the message to the buffer
  if (logBuffer.length() > 1000) { // Limit buffer size to 1000 characters
    logBuffer = logBuffer.substring(logBuffer.length() - 1000);
  }
}

void handle_OnConnect() {
  LED1status = HIGH;
  LED2status = HIGH;
  Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,LED2status)); 
}

void handle_led1on() {
  LED1status = LOW;
  Serial.println("GPIO4 Status: ON");
  server.send(200, "text/html", SendHTML(true,LED2status)); 
}

void handle_led1off() {
  LED1status = HIGH;
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", SendHTML(false,LED2status)); 
}

void handle_led2on() {
  LED2status = LOW;
  Serial.println("GPIO5 Status: ON");
  server.send(200, "text/html", SendHTML(LED1status,true)); 
}

void handle_led2off() {
  LED2status = HIGH;
  Serial.println("GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,false)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handle_setSpeed() {
  if (server.hasArg("speed")) {
    String speed = server.arg("speed");
    Serial.print("Speed selected: ");
    appendToLog("Speed selected: ");
    Serial.println(speed);
    appendToLog(speed);

    // Confirm writing to device with LED1
    LED1status = LOW;
    Serial.println("GPIO4 Status: ON");
    server.send(200, "text/html", SendHTML(true,LED2status)); 
    
    // Add logic to process the speed value, e.g., send it to the motor controller
    if (!logOnlyMode && !speedSet && wheelSize > 0 && wheelPerimeter > 0) {
      speedSet = true;
      printRepeatedMessage("------ WRITING SPEED ------", 6);
    
      // Convert the string to an integer
      writeToCan(speed.toInt(), wheelSize, wheelPerimeter);
    
      printRepeatedMessage("----------- DONE ----------", 6);
    } else {
      if (speedSet) {
      Serial.println("Speed already set.");
      appendToLog("Speed already set.");
      }
      if (logOnlyMode) {
      Serial.println("Log-only mode is enabled.");
      appendToLog("Log-only mode is enabled.");
      }
      if (wheelSize <= 0) {
      Serial.println("Wheel size is not set.");
      appendToLog("Wheel size is not set.");
      }
      if (wheelPerimeter <= 0) {
      Serial.println("Wheel perimeter is not set.");
      appendToLog("Wheel perimeter is not set.");
      }
    }
  } else {
    Serial.println("No speed value received");
    appendToLog("No speed value received");
  }
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

void handle_readSpeed() {
  // Logic to read the current speed from the device

  appendToLog("Reading speed from CAN bus...");
  int packetSize = CAN.parsePacket();
  speedSet = false; // Reset speedSet flag for reading speed

  if (packetSize) {
    if (CAN.packetExtended()) {
      Serial.print("Extended ID: 0x");
      Serial.print(CAN.packetId(), HEX);
      appendToLog("Extended ID: 0x" + String(CAN.packetId(), HEX));
    } else {
      Serial.print("Standard ID: 0x");
      Serial.print(CAN.packetId(), HEX);
      appendToLog("Standard ID: 0x" + String(CAN.packetId(), HEX));
    }

    Serial.print(" DLC: ");
    Serial.print(packetSize);
    appendToLog("DLC: " + String(packetSize));

    while (CAN.available()) {
      Serial.print(" ");
      Serial.print(CAN.read(), HEX);
      appendToLog(" " + String(CAN.read(), HEX));
    }
    Serial.println();

    // Print human-readable data if packetid ends with 0x3203
    if (packetSize >= 6 && (CAN.packetId() & 0xFFFF) == 0x3203) {
      CAN.beginPacket(canId);
      int speedHigh = CAN.read();
      int speedLow = CAN.read();
      int speed = (speedHigh << 8) | speedLow;
      Serial.print("Speed Limit: ");
      Serial.print(speed / 100.0);
      Serial.println(" km/h");
      appendToLog("Speed Limit: " + String(speed / 100.0) + " km/h");

      int wheelSizeLow = CAN.read();
      int wheelSizeHigh = CAN.read();
      int wheelSize = ((wheelSizeHigh & 0x0F) << 4) | (wheelSizeLow & 0x0F);
      Serial.print("Wheel Size: ");
      Serial.print(wheelSize * 10);
      Serial.println(" mm");
      appendToLog("Wheel Size: " + String(wheelSize * 10) + " mm");

      int wheelPerimeterLow = CAN.read();
      int wheelPerimeterHigh = CAN.read();
      int wheelPerimeter = (wheelPerimeterHigh << 8) | wheelPerimeterLow;
      Serial.print("Wheel Perimeter: ");
      Serial.print(wheelPerimeter);
      Serial.println(" mm");
      appendToLog("Wheel Perimeter: " + String(wheelPerimeter) + " mm");
    }
  }

  // Send the current speed back to the client
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

void handle_toggleLogMode() {
  logOnlyMode = !logOnlyMode; // Toggle the logOnlyMode
  String status = logOnlyMode ? "enabled" : "disabled";
  Serial.println("Log-only mode " + status);
  appendToLog("Log-only mode " + status);
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

void writeToCan(int speedLimit, int wheelSize, int wheelPerimeter) {
  int speed = speedLimit * 100; // Convert km/h to the required format
  uint32_t temp_wheel_size = (wheelSize / 10) & 0x0F;
  temp_wheel_size = temp_wheel_size << 4;
  temp_wheel_size += (wheelSize % 10);

  CAN.beginExtendedPacket(canId);
  CAN.write((speed >> 8) & 0xFF); // Speed limit high byte
  CAN.write(speed & 0xFF);        // Speed limit low byte
  CAN.write(temp_wheel_size);     // Wheel size low byte
  CAN.write((wheelSize >> 8) & 0xFF); // Wheel size high byte
  CAN.write(wheelPerimeter & 0xFF);   // Wheel perimeter low byte
  CAN.write((wheelPerimeter >> 8) & 0xFF); // Wheel perimeter high byte
  CAN.endPacket();
}

void printRepeatedMessage(const char* message, int count) {
  for (int i = 0; i < count; i++) {
    Serial.println(message);
    appendToLog(message);
  }
}

String SendHTML(uint8_t led1stat, uint8_t led2stat) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Speed Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: inline-block;width: 200px;background-color: #3498db;border: none;color: white;padding: 15px 0;text-decoration: none;font-size: 20px;margin: 10px auto;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="#log {text-align: left; margin: 20px auto; width: 90%; height: 200px; overflow-y: scroll; border: 1px solid #ccc; padding: 10px; background-color: #f9f9f9;}\n";
  ptr +="</style>\n";
  ptr +="<script>\n";
  ptr +="function fetchLog() {\n";
  ptr +="  fetch('/log').then(response => response.text()).then(data => {\n";
  ptr +="    document.getElementById('log').innerText = data;\n";
  ptr +="  });\n";
  ptr +="}\n";
  ptr +="setInterval(fetchLog, 1000);\n"; // Refresh log every second
  ptr +="</script>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Veloretti Speed Settings</h1>\n";

  // Add current speed display and read button
  ptr +="<p>Current Speed: <span id=\"speed\">Unknown</span></p>\n";
  ptr +="<form action=\"/readSpeed\" method=\"GET\">\n";
  ptr +="<input type=\"submit\" value=\"Read from bike\" class=\"button\">\n";
  ptr +="</form>\n";
  ptr +="<br><br>\n";

  ptr +="<form action=\"/setSpeed\" method=\"GET\">\n";
  ptr +="<label for=\"speed\">Select Speed:</label>\n";
  ptr +="<select id=\"speed\" name=\"speed\">\n";
  for (int i = 25; i <= 37; i++) {
    ptr += "  <option value=\"" + String(i) + "\">" + String(i) + " km/h</option>\n";
  }
  ptr +="</select>\n";
  ptr +="<br><br>\n";
  ptr +="<input type=\"submit\" value=\"Write to bike\" class=\"button\">\n";
  ptr +="</form>\n";

  // Add toggle for log-only mode as a checkbox
  ptr +="<form action=\"/toggleLogMode\" method=\"GET\">\n";
  ptr +="<label for=\"logMode\">Log-Only Mode:</label>\n";
  ptr +="<input type=\"checkbox\" id=\"logMode\" name=\"logMode\" onchange=\"this.form.submit()\"";
  if (logOnlyMode) {
    ptr +=" checked";
  }
  ptr +=">\n";
  ptr +="</form>\n";

  // Add log display
  ptr +="<h3>Log Output</h3>\n";
  ptr +="<div id=\"log\">Loading...</div>\n";

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
