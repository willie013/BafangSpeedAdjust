#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

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
String SendHTML(uint8_t led1stat, uint8_t led2stat);

uint8_t LED1pin = 22; // Correct led
bool LED1status = LOW;

uint8_t LED2pin = 5;
bool LED2status = LOW;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet, dns); // Added DNS server configuration
  
  Serial.println("AP started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  server.on("/setSpeed", handle_setSpeed); // New route for speed setting
  server.on("/readSpeed", handle_readSpeed); // New route for reading speed
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
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

    // Confirm writing to device with LED1
    LED1status = LOW;
    Serial.println("GPIO4 Status: ON");
    server.send(200, "text/html", SendHTML(true,LED2status)); 
    
    // Add logic to process the speed value, e.g., send it to the motor controller
  } else {
    Serial.println("No speed value received");
  }
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

void handle_readSpeed() {
  // Logic to read the current speed from the device
  String currentSpeed = "Unknown"; // Replace with actual speed reading logic
  Serial.print("Current Speed: ");
  Serial.println(currentSpeed);
  
  // Send the current speed back to the client
  server.send(200, "text/html", SendHTML(LED1status, LED2status));
}

String SendHTML(uint8_t led1stat,uint8_t led2stat){
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
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Veloretti Speed Settings</h1>\n";

  // Add current speed display and read button
  ptr +="<p>Current Speed: <span id=\"currentSpeed\">Unknown</span></p>\n";
  ptr +="<form action=\"/readSpeed\" method=\"GET\">\n";
  ptr +="<input type=\"submit\" value=\"Read from bike\" class=\"button\">\n";
  ptr +="</form>\n";
  ptr +="<br><br>\n";

  ptr +="<form action=\"/setSpeed\" method=\"GET\">\n";
  ptr +="<label for=\"speed\">Select Speed:</label>\n";
  ptr +="<select id=\"speed\" name=\"speed\">\n";
  ptr +="  <option value=\"25\">25 km/h</option>\n";
  ptr +="  <option value=\"27\">28 km/h</option>\n";
  ptr +="  <option value=\"30\">30 km/h</option>\n";
  ptr +="  <option value=\"32\">32 km/h</option>\n";
  ptr +="  <option value=\"35\">35 km/h</option>\n";
  ptr +="  <option value=\"37\">37 km/h</option>\n";
  ptr +="</select>\n";
  ptr +="<br><br>\n";
  ptr +="<input type=\"submit\" value=\"Write to bike\" class=\"button\">\n";
  ptr +="</form>\n";

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
