#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <mcp_can.h>

bool testmode = true; // Set to true for testing mode
bool speedSet = true;
const long canBaudRate = 250E3;
const long esp32BaudRate = 115200;
const long canId = 0x85103203; // CAN ID
int speed = 0;
int wheelSize = 0; // 11040; // Wheel size in mm (calculated to match 0xC0 and 0x2B)
int wheelPerimeter = 0; // 2254; // Wheel perimeter in mm (calculated to match 0xCE and 0x08)
bool logOnlyMode = true; // Set to true for log-only mode
const int SPI_CS_PIN = 5;  // CS pin for MCP2515
String logBuffer = ""; // Buffer for logging messages

// Create MCP_CAN instance
MCP_CAN CAN0(SPI_CS_PIN);

const char* ssid     = "Veloretti-ESP";
const char* password = "123456789";

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

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
  WiFi.softAPConfig(local_ip, gateway, subnet); 
  
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

  // Print MOSI, MISO, SCK of board
  appendToLog("MOSI pin:" + String(MOSI));
  appendToLog("MISO pin:" + String(MISO));
  appendToLog("SCK pin:" + String(SCK));

  // Initialize MCP2515 running at 8MHz with a baudrate of 250kb/s
  if(CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
    appendToLog("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    appendToLog("Error Initializing MCP2515...");
    if(!testmode) {
      while(1);
    }
  }

  // Set operation mode to normal
  CAN0.setMode(MCP_NORMAL);
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
    Serial.println(speed);
    appendToLog("Speed selected: " + speed);
    
    // Confirm writing to device with LED1
    LED1status = LOW;
    Serial.println("GPIO4 Status: ON");
    server.send(200, "text/html", SendHTML(true,LED2status)); 
    
    // Add logic to process the speed value, e.g., send it to the motor controller
    if (!logOnlyMode && !speedSet && wheelSize > 0 && wheelPerimeter > 0) {
      printRepeatedMessage("------ WRITING SPEED ------", 6);
    
      // Convert the string to an integer
      writeToCan(speed.toInt(), wheelSize, wheelPerimeter);
      speedSet = true;

      printRepeatedMessage("----------- DONE ----------", 6);
    } else {
      if (speedSet || wheelSize <= 0 || wheelPerimeter <= 0) {
      Serial.println("First read settings from the bike");
      appendToLog("First read settings from the bike.");
      }
      if (logOnlyMode) {
      Serial.println("Log-only mode is enabled, cannot write to bike.");
      appendToLog("Log-only mode is enabled, cannot write to bike.");
      }
    }
  } else {
    Serial.println("No speed value received");
    appendToLog("No speed value received");
  }
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

void handle_readSpeed() {
  appendToLog("Reading speed from CAN bus...");
  speedSet = false; // Reset speedSet flag for reading speed

  byte len = 0;
  byte buf[8];
  long unsigned int rxId;

  if(CAN0.readMsgBuf(&rxId, &len, buf) == CAN_OK) {
    if(len >= 6 && (rxId & 0xFFFF) == 0x3203) {
      // Log full CAN message in the exact format
      String messageInfo = String(rxId, HEX);
      messageInfo += " " + String(len);
      
      for(int i = 0; i < len; i++) {
        messageInfo += " " + String(buf[i], HEX);
      }
      appendToLog(messageInfo);

      // Extract speed (bytes 0-1)
      int speedHigh = buf[0];
      int speedLow = buf[1];
      speed = (speedHigh << 8) | speedLow;
      appendToLog("Speed Limit on bike: " + String(speed / 100.0) + " km/h");

      // Extract wheel size (bytes 2-3)
      int wheelSizeLow = buf[2];
      int wheelSizeHigh = buf[3];
      wheelSize = ((wheelSizeHigh & 0x0F) << 4) | (wheelSizeLow & 0x0F);
      appendToLog("Wheel Size: " + String(wheelSize * 10) + " mm");
      appendToLog("Wheel Size (Hex): 0x" + String(wheelSize, HEX));

      // Extract wheel perimeter (bytes 4-5)
      int wheelPerimeterLow = buf[4];
      int wheelPerimeterHigh = buf[5];
      wheelPerimeter = (wheelPerimeterHigh << 8) | wheelPerimeterLow;
      appendToLog("Wheel Perimeter: " + String(wheelPerimeter) + " mm");
      appendToLog("Wheel Perimeter (Hex): 0x" + String(wheelPerimeter, HEX));
    } else {
      appendToLog("Received non-speed packet or invalid packet size");
    }
  } else if (testmode) {
    speed = random(2500, 3700); // 25-37 km/h
    wheelSize = random(20, 25); // 20-25 inch wheels
    wheelPerimeter = random(1800, 2200); // Typical wheel perimeters
    appendToLog("Test mode: Using simulated values");
  } else {
    appendToLog("No CAN packet received");
  }

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

  byte data[6];
  data[0] = (speed >> 8) & 0xFF;     // Speed limit high byte
  data[1] = speed & 0xFF;            // Speed limit low byte
  data[2] = temp_wheel_size;         // Wheel size low byte
  data[3] = (wheelSize >> 8) & 0xFF; // Wheel size high byte
  data[4] = wheelPerimeter & 0xFF;   // Wheel perimeter low byte
  data[5] = (wheelPerimeter >> 8) & 0xFF; // Wheel perimeter high byte

  // Send the CAN message
  byte sndStat = CAN0.sendMsgBuf(canId, 1, 6, data); // 1 indicates extended ID
  
  if(sndStat == CAN_OK) {
    // Log the complete CAN message in standard format: ID DLC DATA_BYTES
    String canMessage = String(canId, HEX) + " " + 
                       String(6) + " " + // DLC (Data Length Code) is 6 bytes
                       String(data[0], HEX) + " " + 
                       String(data[1], HEX) + " " +
                       String(data[2], HEX) + " " + 
                       String(data[3], HEX) + " " +
                       String(data[4], HEX) + " " + 
                       String(data[5], HEX);
    
    // Ensure all hex values are uppercase and padded to 2 digits
    canMessage.toUpperCase();
    appendToLog("CAN Message sent: " + canMessage);
  } else {
    appendToLog("Error sending CAN message");
  }
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
  ptr +="<title>Veloretti Speed Control</title>\n";
  ptr +="<style>\n";
  ptr +="html { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body { margin-top: 20px; background-color: #f5f5f5; }\n";
  ptr +="h1 { color: #2c3e50; margin: 20px auto 30px; font-size: 2em; }\n";
  ptr +="h3 { color: #34495e; margin-bottom: 20px; }\n";
  ptr +=".container { max-width: 800px; margin: 0 auto; padding: 20px; background: white; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n";
  ptr +=".status-panel { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 20px 0; }\n";
  ptr +=".status-item { background: #f8f9fa; padding: 15px; border-radius: 8px; border: 1px solid #dee2e6; }\n";
  ptr +=".status-label { color: #6c757d; font-size: 0.9em; margin-bottom: 5px; }\n";
  ptr +=".status-value { color: #2c3e50; font-size: 1.2em; font-weight: bold; }\n";
  ptr +=".button { display: inline-block; width: 200px; background-color: #3498db; border: none; color: white; padding: 12px 0; text-decoration: none; font-size: 16px; margin: 10px; cursor: pointer; border-radius: 6px; transition: background-color 0.3s; }\n";
  ptr +=".button:hover { background-color: #2980b9; }\n";
  ptr +=".button:active { background-color: #2472a4; }\n";
  ptr +=".button-off { background-color: #95a5a6; }\n";
  ptr +=".button-off:hover { background-color: #7f8c8d; }\n";
  ptr +=".control-panel { margin: 20px 0; padding: 20px; background: #f8f9fa; border-radius: 8px; }\n";
  ptr +="select { padding: 8px; margin: 10px; border-radius: 4px; border: 1px solid #ddd; font-size: 16px; }\n";
  ptr +="#log { text-align: left; margin: 20px auto; width: 100%; height: 200px; overflow-y: auto; border: 1px solid #dee2e6; padding: 10px; background-color: #f8f9fa; border-radius: 6px; font-family: monospace; font-size: 14px; }\n";
  ptr +=".log-entry { margin: 5px 0; padding: 5px; border-bottom: 1px solid #eee; }\n";
  ptr +=".error { color: #e74c3c; }\n";
  ptr +=".success { color: #27ae60; }\n";
  ptr +=".warning { color: #f39c12; }\n";
  ptr +="</style>\n";
  ptr +="<script>\n";
  ptr +="function fetchLog() {\n";
  ptr +="  fetch('/log').then(response => response.text()).then(data => {\n";
  ptr +="    const logDiv = document.getElementById('log');\n";
  ptr +="    logDiv.innerHTML = data.split('\\n').map(line => `<div class='log-entry'>${line}</div>`).join('');\n";
  ptr +="    logDiv.scrollTop = logDiv.scrollHeight;\n";
  ptr +="  });\n";
  ptr +="}\n";
  ptr +="setInterval(fetchLog, 1000);\n";
  ptr +="</script>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div class='container'>\n";
  ptr +="<h1>Veloretti Speed Control</h1>\n";

  // Status Panel
  ptr +="<div class='status-panel'>\n";
  ptr +="<div class='status-item'>\n";
  ptr +="<div class='status-label'>Current Speed</div>\n";
  ptr +="<div class='status-value'>" + String(speed / 100.0) + " km/h</div>\n";
  ptr +="</div>\n";
  ptr +="<div class='status-item'>\n";
  ptr +="<div class='status-label'>Wheel Size</div>\n";
  ptr +="<div class='status-value'>" + String(wheelSize * 10) + " mm</div>\n";
  ptr +="</div>\n";
  ptr +="<div class='status-item'>\n";
  ptr +="<div class='status-label'>Wheel Perimeter</div>\n";
  ptr +="<div class='status-value'>" + String(wheelPerimeter) + " mm</div>\n";
  ptr +="</div>\n";
  ptr +="</div>\n";

  // Control Panel
  ptr +="<div class='control-panel'>\n";
  ptr +="<form action='/readSpeed' method='GET'>\n";
  ptr +="<input type='submit' value='Read from Bike' class='button'>\n";
  ptr +="</form>\n";
  ptr +="<br>\n";
  ptr +="<form action='/setSpeed' method='GET'>\n";
  ptr +="<label for='speed'>Select Speed:</label>\n";
  ptr +="<select id='speed' name='speed'>\n";
  for (int i = 25; i <= 37; i++) {
    ptr += "  <option value='" + String(i) + "'>" + String(i) + " km/h</option>\n";
  }
  ptr +="</select>\n";
  ptr +="<br>\n";
  ptr +="<input type='submit' value='Write to Bike' class='button'>\n";
  ptr +="</form>\n";
  ptr +="<br>\n";
  ptr +="<form action='/toggleLogMode' method='GET'>\n";
  ptr +="<label for='logMode'>Log-Only Mode:</label>\n";
  ptr +="<input type='checkbox' id='logMode' name='logMode' onchange='this.form.submit()'";
  if (logOnlyMode) {
    ptr +=" checked";
  }
  ptr +=">\n";
  ptr +="</form>\n";
  ptr +="</div>\n";

  // Log Display
  ptr +="<h3>Log Output</h3>\n";
  ptr +="<div id='log'>Loading...</div>\n";
  ptr +="</div>\n"; // Close container
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
