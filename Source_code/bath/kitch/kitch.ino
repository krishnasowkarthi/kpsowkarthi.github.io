//BATHROOM  CODE 

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>
int rx_begin_flag = 0;
int tx_begin_flag = 0;
int rx_msg[1];
RF24 radio(9,10);
const uint64_t pipe = 0xE8E8F0F0E1LL;   // Pipe for communication
String rx_Message = "";
int total_tx_flag = 0;
int warning_tx_flag = 0;
int main_tx_flag = 0;
int tx_msg[1];

const int flow_sensor_pin = 2;
int pulseCount = 0;
int sensor_state = 0;
int last_sensor_state = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
const long interval = 1000;  // Time interval for sending usage details to main unit
int counting_flag = 1;
float calibrationFactor = 4.5;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long totalUsage;
unsigned long oldTime;
unsigned int frac;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setRetries(15,15);
  
  pinMode(flow_sensor_pin, INPUT);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  totalUsage = 0;
  
}

void loop() {
  sensor_state = digitalRead(flow_sensor_pin);
   
  currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    counting_flag = 0;
  }
  if(counting_flag == 0) {
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    if (flowRate > 0) {                //To check if pipe is open
      if (totalMilliLitres <= 2000) {  //to check if used litres exceeds
        flowMilliLitres = (flowRate / 60) * 1000;
        totalMilliLitres += flowMilliLitres;  //to calculate the litres of water flowing through the tap
        totalUsage += flowMilliLitres;  // to calculate the total litres of water used
        totalUsage = (totalUsage/1000);
        frac = (flowRate - int(flowRate)) * 10; 
        pulseCount = 0;
        
        Serial.print("Flow rate: ");
        Serial.print(int(flowRate));
        Serial.print(".");
        Serial.print(frac, DEC) ;
        Serial.print("L/min");
        Serial.print("  Current Liquid Flowing: ");
        Serial.print(flowMilliLitres);
        Serial.print("mL/Sec");
        Serial.print("  Output Liquid Quantity: ");
        Serial.print(totalMilliLitres);
        Serial.println("mL");
        
      }
      else {
        totalMilliLitres = 0; 
        warning_tx_flag = 1;
        tx_begin_flag = 0;
        
        Serial.print("  total usage = ");
        Serial.println(totalUsage);
        
      }
    }
    else if (flowRate == 0) {
      totalMilliLitres = 0;
    }
    counting_flag = 1;
  }
  else {
    if(sensor_state != last_sensor_state) {
      if(sensor_state == HIGH) {
        pulseCount++;
      }
    }
    last_sensor_state = sensor_state;
  }
  //nrf
  if(rx_begin_flag == 0) {
    radio.openReadingPipe(1,pipe);
    radio.startListening(); 
    rx_begin_flag = 1;
  }
  if (radio.available()){
    bool done = false;  
    done = radio.read(rx_msg, 1); 
    char rx_Char = rx_msg[0];
    if(rx_Char == '%') {                     //for reading incoming msg from main unit
      total_tx_flag = 1;
    }
    if (rx_msg[0] != 2){
      rx_Message.concat(rx_Char);
    }
    else {
      Serial.println(rx_Message);
      rx_Message= ""; 
    }
  }
  while(warning_tx_flag == 1) {
    if(tx_begin_flag == 0) {
      radio.openWritingPipe(pipe);
      radio.stopListening();
      tx_begin_flag = 1;
    }
    String tx_Message = "#";
    int tx_messageSize = tx_Message.length();
    for (int i = -1; i < tx_messageSize; i++) {
      int charToSend[1];
      charToSend[0] = tx_Message.charAt(i);
      radio.write(charToSend,1);              //sending msg to main unit
    }   
    tx_msg[0] = 2; 
    radio.write(tx_msg,1);
    delay(1000);
    warning_tx_flag = 0;
    tx_begin_flag = 0;
    rx_begin_flag = 0;
  }
  while(total_tx_flag == 1) {
    delay(3000);
    if(tx_begin_flag == 0) {
      radio.openWritingPipe(pipe);
      radio.stopListening();
      tx_begin_flag = 1;
    }
    String tx_Message = ("@" + String(totalUsage));
    int tx_messageSize = tx_Message.length();
    for (int i = -1; i < tx_messageSize; i++) {
      int charToSend[1];
      charToSend[0] = tx_Message.charAt(i);
      radio.write(charToSend,1);
    }   
    tx_msg[0] = 2; 
    radio.write(tx_msg,1);
    delay(1000);
    total_tx_flag = 0;
    tx_begin_flag = 0;
    rx_begin_flag = 0;
  } 
}



