// Arduino Mega - Receiver Code
// Connect Uno pin 10 to Mega RX2 (pin 17)
// Connect GND to GND

String inputBuffer = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);   // USB serial for debugging
  Serial2.begin(9600);    // RX2/TX2 serial for receiving from Uno
  
  Serial.println("Mega ready to receive quaternion data on RX2 (pin 17)");
  inputBuffer.reserve(100);
}

void loop() {
  // Read from Serial2 (RX2 pin 17)
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    inputBuffer += inChar;
    
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  
  if (stringComplete) {
    Serial.print("Received: ");
    Serial.print(inputBuffer);
    
    float w, x, y, z;
    if (parseQuaternion(inputBuffer, w, x, y, z)) {
      Serial.print("Parsed - w:");
      Serial.print(w, 3);
      Serial.print(" x:");
      Serial.print(x, 3);
      Serial.print(" y:");
      Serial.print(y, 3);
      Serial.print(" z:");
      Serial.println(z, 3);
      
      // Use quaternion data here for your VR tracking
    }
    
    inputBuffer = "";
    stringComplete = false;
  }
}

bool parseQuaternion(String json, float &w, float &x, float &y, float &z) {
  int wIdx = json.indexOf("\"w\":");
  int xIdx = json.indexOf("\"x\":");
  int yIdx = json.indexOf("\"y\":");
  int zIdx = json.indexOf("\"z\":");
  
  if (wIdx == -1 || xIdx == -1 || yIdx == -1 || zIdx == -1) {
    return false;
  }
  
  w = json.substring(wIdx + 4, json.indexOf(',', wIdx)).toFloat();
  x = json.substring(xIdx + 4, json.indexOf(',', xIdx)).toFloat();
  y = json.substring(yIdx + 4, json.indexOf(',', yIdx)).toFloat();
  z = json.substring(zIdx + 4, json.indexOf('}', zIdx)).toFloat();
  
  return true;
}