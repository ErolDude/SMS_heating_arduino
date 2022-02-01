 /*
(Copy and paste)

Rotary encoder decoding using two interrupt lines.
Most Arduino boards have two external interrupts,
numbers 0 (on digital pin 2) and 1 (on digital pin 3).

Program sketch is for SparkFun Rotary Encoder sku: COM-09117
Connect the middle pin of the three to ground.
The outside two pins of the three are connected to
digital pins 2 and 3

http://w264779-www.fnweb.no/rotary_encoder/
https://community.createlabz.com/knowledgebase/wireless-lamp-control-using-sim800c-gsm-module-robotdyn-uno-and-dc-relay-module/
https://www.jameco.com/jameco/workshop/techtip/working-with-seven-segment-displays.html
*/
#include <Servo.h>
#include <SoftwareSerial.h>

//******SETUP******

//General
byte MINTEMP=5;
byte MAXTEMP=30;
byte ServoPin=4;

double FunkcijaZaUgao(int temp){
  return (6*temp);              //it isn't important to set the exact angle since you can move the servo horn around. 
                                //Only the difference in angle matters i.e. old equation was 6*temp-30, but the difference
                                //of angle between two temperatures in that and in this equation are the same.
}

//Display setup
int segPins[] = {5, 11, 9, 8, 7, 6, 10, 0 }; //A,B,C,D,E,F,G,DP
int displayPins[] = {A1, A0};   // pin 10 controls D0, pin 11 controls D1
int displayBuf[2];               // The display buffer contains the digits to be displayed.
                                 // displayBuf[0] contains the LSD, displayBuf[1] contains the MSD
//GSM setup
SoftwareSerial mySerial(12,13);  //RX and TX ports on the arduino, connect GSM TX to RX and GSM RX to TX

//Whitelisted numbers (country code required!! ex. +387xxxxxxxx)
String whitelist[3]={"+387603322723","+38761132573","+38761208354"};
byte whitelist_length = 3;

//*****END OF SETUP*****



//7seg stuff
byte segCode[10][8] = {
// 7 segment code table
//  a  b  c  d  e  f  g  .
  { 1, 1, 1, 1, 1, 1, 0, 0},  // 0
  { 0, 1, 1, 0, 0, 0, 0, 0},  // 1
  { 1, 1, 0, 1, 1, 0, 1, 0},  // 2
  { 1, 1, 1, 1, 0, 0, 1, 0},  // 3
  { 0, 1, 1, 0, 0, 1, 1, 0},  // 4
  { 1, 0, 1, 1, 0, 1, 1, 0},  // 5
  { 1, 0, 1, 1, 1, 1, 1, 0},  // 6
  { 1, 1, 1, 0, 0, 0, 0, 0},  // 7
  { 1, 1, 1, 1, 1, 1, 1, 0},  // 8
  { 1, 1, 1, 1, 0, 1, 1, 0}   // 9
};


//encoder stuff
volatile int number = MINTEMP;          // Testnumber, print it when it changes value,
                                        // used in loop and both interrupt routines
byte oldnumber = number;

volatile boolean halfleft = false;      // Used in both interrupt routines
volatile boolean halfright = false;


//GSM stuff
char incomingByte; 
String incomingData;
bool atCommand = true;


byte index = 0;
String caller = "";
String message = "";


Servo myservo;                         //create servo object


void setup(){
  
 Serial.begin(250000);

 
   //7seg stuff
    for (byte i=0; i < 8; i++)
  {
    pinMode(segPins[i], OUTPUT);
  }
  pinMode(displayPins[0], OUTPUT);
  pinMode(displayPins[1], OUTPUT);

  byte temp=MINTEMP;  // initializes the display
    displayBuf[0]=temp%10;
    temp/=10;
    displayBuf[1]=temp%10;


  //GSM setup
    mySerial.begin(9600);
    // Check if you're currently connected to SIM800C 
    while(!mySerial.available()){
      mySerial.println("AT");
      delay(1000); 
      Serial.println("connecting....");
    }
    
    bool network=false; //Connection check!
    while(!network){
      mySerial.println("AT+CGREG?");  
    while(mySerial.available()){
        incomingByte = mySerial.read();
        incomingData += incomingByte;
       }
        incomingData.trim();
        //Serial.println(incomingData);
    if(incomingData.indexOf("0,1")>=0){
    network=true;
    } else if(incomingData.indexOf("0,5")>=0){
      network=true;
    }
      blinkMinus();
      incomingData = "";
    }
     
     
   // Serial.println("Connected..");  
    mySerial.println("AT+CMGF=1");  //Set SMS Text Mode 
    delay(1000);  
    mySerial.println("AT+CNMI=1,2,0,0,0");  //procedure, how to receive messages from the network
    delay(1000);
    mySerial.println("AT+CMGL=\"REC UNREAD\""); // Read unread messages
    //Serial.println("Ready to receive Commands..");  */
  
  //servo initialization
  myservo.attach(ServoPin);
  myservo.write(FunkcijaZaUgao(MINTEMP));
  //encoder setup
  pinMode(2, INPUT);
  digitalWrite(2, INPUT_PULLUP);                // Turn on internal pullup resistor
  pinMode(3, INPUT);
  digitalWrite(3, INPUT_PULLUP);                // Turn on internal pullup resistor
  attachInterrupt(0, isr_2, FALLING);   // Call isr_2 when digital pin 2 goes LOW
  attachInterrupt(1, isr_3, FALLING);   // Call isr_3 when digital pin 3 goes LOW

}
void loop(){


 if(mySerial.available()){
      digitalWrite(displayPins[0], LOW);  //turns off display to avoid bugs
      digitalWrite(displayPins[1],LOW );
      myservo.detach();
      delay(100);
      // Serial buffer
      while(mySerial.available()){
        incomingByte = mySerial.read();
        incomingData += incomingByte;
       }
        delay(10); 
        if(atCommand == false){
          receivedMessage(incomingData);
        }
        else{
          atCommand = false;
        }        
        //delete messages to save memory
        if (incomingData.indexOf("OK") == -1){
          mySerial.println("AT+CMGDA=\"DEL ALL\"");
          delay(1000);
          atCommand = true;
        }        
        incomingData = "";
        myservo.attach(ServoPin);
  }                                               

  if(number != oldnumber){                      // Change in value ?
    //Serial.println(number);                     // Yes, print it (or whatever)
    myservo.write(FunkcijaZaUgao(number));
    byte temp=number;
    displayBuf[0]=temp%10;
    temp/=10;
    displayBuf[1]=temp%10;
    oldnumber = number;
  }
  refreshDisplay(displayBuf[1],displayBuf[0]);  // Refreshes the display with the contents of displayBuf
                                                // each iteration of the main loop.
}
//Display functions
void setSegments(byte n)
{
  for (byte i=0; i < 8; i++)
  {
    digitalWrite(segPins[i], segCode[n][i]);
  }
}

void refreshDisplay(byte digit1, byte digit0)
{
  digitalWrite(displayPins[0], HIGH);  // displays digit 0 (least significant)
  digitalWrite(displayPins[1],LOW );
  setSegments(digit0);
  delay(5);
  digitalWrite(displayPins[0],LOW);    // then displays digit 1
  digitalWrite(displayPins[1], HIGH);
  setSegments(digit1);
  delay(5);
}

void blinkMinus(){  //status update message, g segment blinks
      digitalWrite(displayPins[0], HIGH);  
      digitalWrite(displayPins[1],HIGH);
      digitalWrite(segPins[6],HIGH);
      delay(1000);
      digitalWrite(segPins[6],LOW);
      digitalWrite(displayPins[0], LOW);  
      digitalWrite(displayPins[1],LOW);
      delay(1000);
}
//encoder interrupt functions
void isr_2(){                                              // Pin2 went LOW
  delay(1);                                                // Debounce time
  if(digitalRead(2) == LOW){                               // Pin2 still LOW ?
    if(digitalRead(3) == HIGH && halfright == false){      // -->
      halfright = true;                                    // One half click clockwise
    }  
    if(digitalRead(3) == LOW && halfleft == true){         // <--
      halfleft = false;                                    // One whole click counter-
      if(number!=MINTEMP){                                 // clockwise
      number--;           
      }
    }
  }
}
void isr_3(){                                             // Pin3 went LOW
  delay(1);                                               // Debounce time
  if(digitalRead(3) == LOW){                              // Pin3 still LOW ?
    if(digitalRead(2) == HIGH && halfleft == false){      // <--
      halfleft = true;                                    // One half  click counter-
    }                                                     // clockwise
    if(digitalRead(2) == LOW && halfright == true){       // -->
      halfright = false;                                  // One whole click clockwise
      if(number!=MAXTEMP){
      number++;
      }
    }
  }
}

//GSM functions
void receivedMessage(String inputString){
   
  //Get The number of the sender
  index = inputString.indexOf('"')+1;
  inputString = inputString.substring(index);
  index = inputString.indexOf('"');
  caller = inputString.substring(0,index);
  //Serial.println("Number: " + caller);
 
  //Get The Message of the sender
  index = inputString.indexOf("\n")+1;
  message = inputString.substring(index);
  message.trim();
  //Serial.println("Message: " + message);
        
  message.toUpperCase(); // uppercase the message received
  bool approved=false; //flag for number comparing in whitelist
  
  for(byte i=0;i<whitelist_length-1;i++){ //checking caller number against whitelist
    if(caller==whitelist[i]){
    approved=true;
    break;
    }
  }
  
  byte val;
  if(approved){
  //Heating control
    if (message=="OFF"){
        number=MINTEMP;
   }
    else if (message.length()==1){
      val=message[0]-'0'; //must convert to int
    if(val>=MINTEMP&&val<=MAXTEMP){
    number=val;
    }
   } else if (message.length()==2){
    val=(message[0]-'0')*10+(message[1]-'0'); //must convert to int
    if(val>=MINTEMP&&val<=MAXTEMP){
    number=val;
    }
   
    /*} else if (message=="KREDIT"){
      Serial.println("uslo");
        incomingData="";
        mySerial.println("AT+CMGF=0"); //set messages to PDU mode
        delay(1000);
        mySerial.println("AT+CUSD=1,\"*100#\",15");
        
         while(mySerial.available()){
        incomingByte = mySerial.read();
        incomingData += incomingByte;
       }
       Serial.println(incomingData);
    mySerial.println("AT+CMGF=1");
    
    mySerial.println("AT+CMGS=\"" + caller + "\"");
    mySerial.print(incomingData);
    mySerial.println((char)26);
       
    }else if (message=="HELLO"){
      Serial.println("ello");
      mySerial.println("AT+CMGF=1");
      mySerial.println("AT+CMGS=\"" + caller + "\"");
      mySerial.print(incomingData);
      mySerial.println((char)26);
    }*/

  } else {
   // Serial.println("Unauthorised number");
  }
  }
}
