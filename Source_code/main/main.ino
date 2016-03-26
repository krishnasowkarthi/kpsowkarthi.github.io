
//MAIN UNIT 

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>
#include <TESPI.h>

int rx_begin_flag = 0;
int tx_begin_flag = 0;
int rx_msg[1];
RF24 radio(47,49);
const uint64_t pipe = 0xE8E8F0F0E1LL;  //Pipe for communication
String rx_Message = "";
int total_tx_flag = 0;
int tx_msg[1];

const int flow_sensor_pin = 2;
int pulseCount = 0;
int sensor_state = 0;
int last_sensor_state = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long currentMillis = 0;
const long interval = 1000;
const long interval1 = 300000;
int counting_flag = 1;
float calibrationFactor = 4.5;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long totalUsage;
unsigned long oldTime;
unsigned int frac;

const int coagulation_sensor_pin = A0;
const int conductivity_sensor_pin = A1;
int coagulation_value = 0;
int conductivity_value = 0;
int purity = 56;

int sms_bathroom_flag = 0;
int sms_kitchen_flag = 0;

String bathroom = "";
String kitchen = "";

int day_complete = 0;

TESPI esp(Serial3);

//int swstate = 0;
void setup(void){
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial3.begin(9600);
  radio.begin();
  radio.setRetries(15,15);

  pinMode(flow_sensor_pin, INPUT);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  totalUsage = 0;

  pinMode(7, INPUT);
  
  //Wifi Configuration
  esp.setWifiStationConfig("ICFOSS-Workshop", "guest@lbs%", 1);
  esp.loadWifiStationConfig();
  
}
void loop(void){
  //swstate = digitalRead(7);
  //if(swstate == HIGH) {
    //day_complete = 1;
  //}
  currentMillis = millis();
   if(currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    day_complete = 1;
    //String http_request = "/hwm/update.php?mp=201&kp=202&bp=303&purity=1";
    String http_request = "/hwm/update.php?m=" + String(totalUsage) + "&k=" + kitchen + "&b=" + bathroom  + "&p=" + purity;
//    Serial.println("Posted to Server" + String(totalUsage) + "," + kitchen  + "," + bathroom + "," + purity);
    esp.HTTP_GET(http_request, "smartnodes.in", 80);
    
  //  delay(1000);
  }
  //String http_request = "/hwm/update.php?mp=201&kp=202&bp=303&purity=1";
  // esp.HTTP_GET(http_request, "smartnodes.in", 80);
  while(Serial3.available()) Serial.write(Serial3.read());
 // while(Serial.available()) Serial3.write(Serial.read());

  //delay(1000);

  sensor_state = digitalRead(flow_sensor_pin);

  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    counting_flag = 0;
  }
  if(counting_flag == 0) {
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    if (flowRate > 0) {
      if (totalMilliLitres <= 2000) {
        flowMilliLitres = (flowRate / 60) * 1000;
        totalMilliLitres += flowMilliLitres;
        totalUsage += flowMilliLitres;  //Total milliLitres of water used
        totalUsage = (totalUsage/1000);  //Converting to litres
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
        Serial.print("  total usage = ");
        Serial.println(totalUsage);

      }
    }
    else if (flowRate == 0) {
      totalMilliLitres = 0;
    }
    //to check basic purity of water
    coagulation_value = analogRead(coagulation_sensor_pin); //t
    conductivity_value = analogRead(conductivity_sensor_pin);
    if((coagulation_value <= 390) || (conductivity_value <= 300)) { // value to be changed
      purity = 0;
    }
    else {
      purity = 1;
    }
    Serial.print("Coagulation sensor = ");
    Serial.print(coagulation_value);
    Serial.print("    Conductivity sensor = ");
    Serial.println(conductivity_value);

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
    radio.openReadingPipe(1,pipe);  //to receive msg from distribution points
    radio.startListening(); 
    rx_begin_flag = 1;
  }
  if (radio.available()){
    bool done = false;  
    done = radio.read(rx_msg, 1); 
    char rx_Char = rx_msg[0];
    if(rx_Char == '*') {
      sms_kitchen_flag = 1;
    }
    if(rx_Char == '#') {
      sms_bathroom_flag = 1;
    }
    if(rx_Char == 2) {
      int startK = rx_Message.indexOf('!');
      int startB = rx_Message.indexOf('@');
      if (startK >= 0) {
        kitchen = rx_Message.substring(startK+1);
        Serial.print("kitchen usage = ");
        Serial.println(kitchen);
      }
      if (startB >= 0) {
        bathroom = rx_Message.substring(startB+1);
        Serial.print("bathroom usage = ");
        Serial.println(bathroom);
      }
      rx_Message = "";
    }
    if (rx_msg[0] != 2 && rx_msg[0] != 0 ){
      rx_Message.concat(rx_Char);
    }
  }
//message the user when bathroom water usage exceeds
  if(sms_bathroom_flag == 1) {
    Serial1.println("AT+CMGF=1");
    delay(100);
    Serial1.println("AT+CMGS=\"8893589523\"");
    delay(100);
    Serial1.println("BATHROOM USE EXCEEDED");
    delay(100);
    Serial1.write(26);
    delay(100);
    sms_bathroom_flag = 0;
   Serial.println("SMS BATHROOM");
  }
//message the user when kitchen water usage exceeds
  if(sms_kitchen_flag == 1) {
    Serial1.println("AT+CMGF=1");
    delay(100);
    Serial1.println("AT+CMGS=\"8893589523\"");
    delay(100);
    Serial1.println("KITCHEN USE EXCEEDED");
    delay(100);
    Serial1.write(26);
    delay(100);
    sms_kitchen_flag = 0;
    Serial.println("SMS KITCHEN");
  }
  if(day_complete == 1) {
    total_tx_flag = 1;
    day_complete = 0;
  }
  while(total_tx_flag == 1) {
    if(tx_begin_flag == 0) {
      radio.openWritingPipe(pipe);
      radio.stopListening();
      tx_begin_flag = 1;
    }
    String tx_Message = "%";        //to send msg to distribution units when day completes
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






