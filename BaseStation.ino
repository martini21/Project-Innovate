#include <UIPEthernet.h>      // UIPEthernet - Version: Latest
#include <utility/logging.h>  // logging utility
#include <SPI.h>              // SPI
#include <SoftwareSerial.h>   // Software serial library - IMPORTANT BECAUSE WE ONLY HAVE ONE RX AND ONE TX SERIAL PIN ON THE ARDUINO UNO

//-----------
//GLOBAL VARS
//-----------

//ANALOG PINS
const int pinMQ = A0;         // MQ2 Sensor in KITCHEN

//DIGITAL PINS
const int pinPIR = 2;         // PIR Sensor in the LIVING ROOM

char roomNumber = '0';        //Number (char) of the room where the alarm was triggered

//---------
//INSTANCES
//---------

// Variables for PIR 
long interval = 2000000;          //Amount of time needed for the person to be considered idle (in milliseconds) 2MM = 0,56 hour
unsigned long millisAtZero;       //Sets the milliseconds of the start timer
unsigned long millisNow;          //Sets the milliseconds of the moment right now
boolean timerSet = false;         //Indicates if the timer is activated or not
boolean inRoom = false;           //Indicates if the person is in the room right now
boolean itsPIR = false;           //Indicates if the PIR triggered the alarm or the MQ2 sensor

//Creating BT instance
SoftwareSerial BTserial(5, 6);    //TX --> 5 | RX --> 6
char response;                    //Response of the Bluetooth module from the wristband


//EthernetVars
//Ethernet PIN 10 = CS (ChipSelect)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //The MAC-Address of the ENC28J60 Microcontroller
char server[] = "stenden.protonbytez.com";            //URL of the server with the php processing file
EthernetClient client;                                //EthernetClient Instance
const String apiKey = "EmmendDevHouseGuard";          //Password for the php file to prevent spam request
const String sendNumber = "4915120559108";            //Number to send the SMS too

void setup()
{  
  Serial.begin(9600);
 
  pinMode(pinPIR, INPUT);               // Setting port for PIR
  SetupEthernet();                      // Setting up Ethernet Connection
  SetupBluetooth();                     // Setting up Bluetooth (Pairs automatically)
}
 
void loop()
{
  SmokeGasSensor();
  PIRMotionSensor();
  reciveEvent();
}

/*
----------------------------------
SENSORS
----------------------------------
*/

//Smoke or gas detection (ANALOG)
void SmokeGasSensor()         
{
  if (analogRead(pinMQ) > 400)
  {
    roomNumber = '1';
    triggerEvent('f');      //sends alarmcode and roomnumber to the wristband
    delay(5000);            //Delay to give the mq2 sensor some time to go back to the start value
  }
}

//PIR Motion detection sensor (DIGITAL)
void PIRMotionSensor()
{
  int sensorValue = digitalRead(pinPIR);  //Get the value of the PIR sensor (0 = NO MOTION or 1 = MOTION DETECTED)
  
  //True if the person is inside the room and idle - resets the millisAtZero and starts the timer
  if (sensorValue == 0 && timerSet == false && inRoom == true)         
  {
    millisAtZero = millis();
    timerSet = true;
  }
  //True if there is motion detected
  if(sensorValue == 1)
  {
    inRoom == true;
  }
  //Cancels idelness and resets the timer
  if (sensorValue == 1 && timerSet == true)
  {
    millisAtZero = millis();
    timerSet = false;
  }
  
  millisNow = millis();

  //Timer interval reached
  if(millisNow - millisAtZero > interval)
  {
    Serial.println("Idelness");
    roomNumber = '0';
    triggerEvent('p');    //p = trigger the problem alarm on the wristband
    timerSet = false;     //stop the timer
    itsPIR = true;        //let the basestation sms part know which sensor triggered the event
    delay(5000);
  }
}

/*
------------------------------------------------------
THIS SECTION IS ABOUT ALARMING THE USER AND RELATIVES.
------------------------------------------------------
*/
//method for triggering the alarm on the wristband
void triggerEvent(char alarmCode)
{
  BTserial.write(alarmCode);
  BTserial.write(roomNumber);
}
//Checks from a reponse from the wristband to know the user's state
void reciveEvent()
{
  if (BTserial.available() > 0) 
  {
    response = BTserial.read();
    Serial.println(response);
    if(response == 'k')
    {
      if(itsPIR == true)
      {
        millisAtZero = millis();
        itsPIR = false;
      }
    }
    else if(response == 'b')
    {
      Serial.println(response);
      Serial.println(itsPIR);
      if(itsPIR)
      {
        sendSMS(sendNumber, URLEncode("Idelness has been detected at your relatives house."));
        itsPIR = false;
      }
      else
      {
        sendSMS(sendNumber, URLEncode("Fire or gas has been detected at your relatives house"));
      }
    }
  }
  
}

//--------------------
//SMS SPECIFIC METHODS
//--------------------
void sendSMS(String number,String message)
{
  // Make a TCP connection to remote host
  if (client.connect(server, 80))
  {

    //should look like this...
    //api.thingspeak.com/apps/thinghttp/send_request?api_key={api key}&number={send to number}&message={text body}

    client.print("GET /sendSMS.php?auth=");
    client.print(apiKey);
    client.print("&number=");
    client.print(number);
    client.print("&body=");
    client.print(message);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  }
  else
  {
    Serial.println(F("Connection failed"));
  } 

  // Check for a response from the server, and route it
  // out the serial port.
  while (client.connected())
  {
    if ( client.available() )
    {
      char c = client.read();
      Serial.print(c);
    }      
  }
  Serial.println();
  client.stop();
}

//Encode URL
String URLEncode(const char* msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg!='\0'){
    if( ('a' <= *msg && *msg <= 'z')
      || ('A' <= *msg && *msg <= 'Z')
      || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    } 
    else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

/*
THIS SECTION IS ABOUT SETING UP BLUETOOTH AND ETHERNET
*/
//ETHERNET
void SetupEthernet() 
{  
  Serial.println("Setting up Ethernet...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    //Ethernet.begin(mac, ip);
  }
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

void SetupBluetooth() 
{
  BTserial.begin(38400);
}