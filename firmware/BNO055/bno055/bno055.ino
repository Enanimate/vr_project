#include <DFRobot_BNO055.h>
#include "Wire.h"
#include <SoftwareSerial.h>

// SoftwareSerial for output to Mega
// TX pin 10 will send data to Mega RX1
SoftwareSerial outputSerial(11, 10); // RX, TX (only using TX pin 10)

int ledStatus = 0;
const int buttonPin = 22;
const int ledPin = 40;
int buttonNew;
int buttonOld = 1;

typedef DFRobot_BNO055_IIC    BNO;
BNO   bno(&Wire, 0x28);

void printLastOperateStatus(BNO::eStatus_t eStatus)
{
  switch(eStatus) {
  case BNO::eStatusOK:    Serial.println("everything ok"); break;
  case BNO::eStatusErr:   Serial.println("unknow error"); break;
  case BNO::eStatusErrDeviceNotDetect:    Serial.println("device not detected"); break;
  case BNO::eStatusErrDeviceReadyTimeOut: Serial.println("device ready time out"); break;
  case BNO::eStatusErrDeviceStatus:       Serial.println("device internal status error"); break;
  default: Serial.println("unknow status"); break;
  }
}

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.begin(115200);      // Keep for debugging
  outputSerial.begin(9600);  // Lower baud for SoftwareSerial reliability
  
  bno.reset();
  while(bno.begin() != BNO::eStatusOK) {
    Serial.println("bno begin faild");
    printLastOperateStatus(bno.lastOperateStatus);
    delay(2000);
  }
  //bno.setAxisMapConfig(BNO::eMapConfig_P7);

  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("bno begin success");
}

void loop()
{
  buttonNew = digitalRead(buttonPin);

  if (buttonOld == 0 && buttonNew == 1) {
    if (ledStatus == 0) {
      digitalWrite(ledPin, HIGH);
      ledStatus = 1;
    } else {
      digitalWrite(ledPin, LOW);
      ledStatus = 0;
    }
  }

  buttonOld = buttonNew;
  delay(20);

  BNO::sQuaAnalog_t   sQua;
  BNO::sEulAnalog_t   sEul;

  sQua = bno.getQua();
  sEul = bno.getEul(); 

  digitalWrite(LED_BUILTIN, HIGH);
  
  // Send to Mega via SoftwareSerial (pin 10)
  outputSerial.print("{\"w\":");
  outputSerial.print(sQua.w, 3);
  outputSerial.print(",\"x\":");
  outputSerial.print(-sQua.y, 3);
  outputSerial.print(",\"y\":");
  outputSerial.print(-sQua.x, 3);
  outputSerial.print(",\"z\":");
  outputSerial.print(-sQua.z, 3);
  outputSerial.print(",\"button_m\":");
  outputSerial.print(buttonNew == 0 ? "true" : "false");  // LOW (0) = pressed = true
  outputSerial.println("}");
  
  // Optional: also print to USB Serial for debugging
  Serial.print("{\"w\":");
  Serial.print(sQua.w, 3);
  Serial.print(",\"x\":");
  Serial.print(-sQua.y, 3);
  Serial.print(",\"y\":");
  Serial.print(-sQua.x, 3);
  Serial.print(",\"z\":");
  Serial.print(-sQua.z, 3);
  Serial.print(",\"button_m\":");
  Serial.print(buttonNew == 0 ? "true" : "false");  // LOW (0) = pressed = true
  Serial.println("}");
  
  digitalWrite(LED_BUILTIN, LOW);
  delay(80);
}