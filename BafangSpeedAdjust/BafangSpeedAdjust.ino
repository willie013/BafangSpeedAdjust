#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);

struct can_frame canMsg35;
const int buttonPin = 4;
const int LedPin = 6;
int buttonState = 1;   
void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(LedPin, OUTPUT);
  digitalWrite(LedPin, LOW);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("Setup ok");
  Serial.println();

  //Original read: 82F83203 6 C4 09 C0 2B CE 08 => C4 09 => 09C4 => 2500 => 25km/h
  //Set:           85103203 6 B8 0B 2B CE 08 => AC 0D => 0DAC => 3000 => 30km/h
  //Set:           85103203 6 AC 0D C0 2B CE 08 => AC 0D => 0DAC => 3500 => 35km/h
  
  
  canMsg35.can_id  = 0x85103203 | CAN_EFF_FLAG;
  canMsg35.can_dlc = 6;
  canMsg35.data[0] = 0xC4;
  canMsg35.data[1] = 0x09;
  canMsg35.data[2] = 0xC0;
  canMsg35.data[3] = 0x2B;
  canMsg35.data[4] = 0xCE;
  canMsg35.data[5] = 0x08;
}

void loop() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    putback();
    digitalWrite(LedPin, HIGH);
  }
  }

  //Write speed setting after 10 seconds waiting
  void putback(){
    Serial.println("------ WRITING SPEED ------");
    Serial.println("------ WRITING SPEED ------");
    Serial.println("------ WRITING SPEED ------");
    Serial.println("------ WRITING SPEED ------");
    Serial.println("------ WRITING SPEED ------");
    Serial.println("------ WRITING SPEED ------");
    mcp2515.sendMessage(MCP2515::TXB1, &canMsg35);
    Serial.println("----------- DONE ----------");
    Serial.println("----------- DONE ----------");
    Serial.println("----------- DONE ----------");
    Serial.println("----------- DONE ----------");
    Serial.println("----------- DONE ----------");
    Serial.println("----------- DONE ----------");
  }
