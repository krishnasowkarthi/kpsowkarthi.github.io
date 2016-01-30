
int ldrpin=12;
const int led=13;
void setup()
{
  pinMode(ldrpin,INPUT);
  pinMode(led,OUTPUT);
  Serial.begin(9600);
  {
    void loop()
    {
      ldr=digitalRead(ldrpin);
      Serial.println(ldrpin);
      delay(200);
      if(lrpin==HIGH)
      {
        digitalWrite(led,HIGH);
        delay(300);
        
      }
      
         if(lrpin==LOW)
      {
        digitalWrite(led,LOW);
        delay(300);
        
      }
      
      
