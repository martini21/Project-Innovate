// TMRpcm - Version: Latest 
#include <TMRpcm.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#define SD_ChipSelectPin 4  //CS Pin of SDCard Module
#define BUTTON_PRESSED LOW   
#define BUTTON_PRESS_TIME 1000      //~5 seconds at 5V

SoftwareSerial BTserial(5,6); //10,11 TX | RX

//-----------
//GLOBAL VARS
//-----------
bool sleepMode;
TMRpcm tmrpcm;


//ANALOG PINS

//DIGITAL PINS
const int pinButton = 3;

char tempChar;
char alarmCode;
char roomNumber;
bool waitRoom;

bool buttonPressed;
int buttonPressCount;
int songQueue;
bool notResponding;

unsigned long millisStart;
unsigned long millisNow;
unsigned long interval;

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT); //Setting up internal led as sleep mode indicator
  digitalWrite(LED_BUILTIN, LOW);
  waitRoom = false;
  notResponding = false;
  interval = 10000;
  
  pinMode(pinButton, INPUT);
  digitalWrite(pinButton, HIGH);
  buttonPressCount = -1;
  songQueue = -1;
  
  sleepMode = false;
  Serial.begin(9600);
  
  Serial.println("Started!");
  setupSDCard();
  setupSpeaker();
  setupBluetooth();   
}

void loop() {
    
  checkButton();  
    
  if(BTserial.available()){   
    
    tempChar = BTserial.read();
    
    if (waitRoom) {
      roomNumber = tempChar;
      songQueue = 0;
      waitRoom = false;
      millisStart = millis();
    }
    
    if (((tempChar == 'f' || tempChar == 'p') && !sleepMode) || (sleepMode && tempChar == 'f')) {
      alarmCode = tempChar;
      waitRoom = true;
    }
  }
  
  if (songQueue >= 0 && !notResponding) {
    millisNow = millis();
    
    if ((millisNow - millisStart) > interval) {
      BTserial.write("b");
      notResponding = true;
    }
  }
  
  if (!tmrpcm.isPlaying() && songQueue >= 0) {  
    
    if (songQueue == 0) {
      
      if (alarmCode == 'f') {
        tmrpcm.play("f.wav");
        songQueue = 1;
      }
      else if (alarmCode == 'p') {
        tmrpcm.play("p.wav");
      }
    }
    else if (songQueue == 1) {
      if (roomNumber == '1') {
        tmrpcm.play("1.wav");
      }
      else if (roomNumber == '2') {
        tmrpcm.play("2.wav");
      }
      else if (roomNumber == '3') {
        tmrpcm.play("3.wav");
      }

      songQueue = 0;
    }
  }
}

/*
* SETUP METHODS
*/
//Method for setting up Bluetooth
void setupBluetooth() 
{
  BTserial.begin(38400);
}

//Method for setting up Audio
void setupSpeaker() {
  tmrpcm.speakerPin = 9;
  tmrpcm.setVolume(5);
  tmrpcm.quality(1);
  tmrpcm.loop(0);
}

//Method for setting up SDCard
void setupSDCard() 
{
  if (!SD.begin(SD_ChipSelectPin)) 
  {
    Serial.println("SD fail");
    return;
  }
}

//Start vibration
void setVibration(bool start) 
{
  //The vibe board unfortunatly never arrived.
}

//Take action after pushing the button
void checkButton()
{
  int buttonState = digitalRead(pinButton);
  
  if (buttonState == LOW) {
    buttonPressed = true;
  }
  else {
    buttonPressed = false;
    if (buttonPressCount == -1) {
      buttonPressCount = -1;
    }
    else if (buttonPressCount == -2 && buttonState == HIGH) {
      buttonPressCount = -1;
    }
  }
  
  if (!buttonPressed && buttonPressCount >= 0 && buttonPressCount < BUTTON_PRESS_TIME) {
    BTserial.write("k");
    tmrpcm.disable();
    songQueue = -1;
    buttonPressCount = -2;
  }  
  
  if (buttonState == LOW && buttonPressed && buttonPressCount >= -1) {
    buttonPressCount++;
    Serial.println(buttonPressCount);
    
    //Activate / Deactivate sleep mode
    if (buttonPressCount > BUTTON_PRESS_TIME) {
      sleepMode = !sleepMode;
      
      if (sleepMode) 
      {
        if (tmrpcm.isPlaying()) 
        {
          tmrpcm.disable();
        }
        
        tmrpcm.play("sleepon.wav");
      }
      else 
      {
        if (tmrpcm.isPlaying()) 
        {
          tmrpcm.disable();
        }
        
        tmrpcm.play("sleepoff.wav");
      }
      
      buttonPressCount = -2;
    }
  }  
}