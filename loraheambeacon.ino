// LoRaHamBeacon - LoRa for Amateur Radio - a basic terminal
// Matthew Asham 2020 VE7UDP - ve7udp@yahoo.com

//#define DEBUG


#include <SPI.h>
#include <LoRa.h>
#include <avr/wdt.h>
#include <string.h>
#include <EEPROM.h>

#define VERSION "1.00"


// uncomment the section corresponding to your board
// BSFrance 2017 contact@bsrance.fr 

//  //LoR32u4 433MHz V1.0 (white board)
//  #define SCK     15
//  #define MISO    14
//  #define MOSI    16
//  #define SS      1
//  #define RST     4
//  #define DI0     7
//  #define BAND    433E6 
//  #define PABOOST true

//  //LoR32u4 433MHz V1.2 (white board)
//  #define SCK     15
//  #define MISO    14
//  #define MOSI    16
//  #define SS      8
//  #define RST     4
//  #define DI0     7
//  #define BAND    433E6 
//  #define PABOOST true 

//LoR32u4II 868MHz or 915MHz (black board)
#define SCK     15
#define MISO    14
#define MOSI    16
#define SS      8
#define RST     4
#define DI0     7
#define BAND    915E6  // 915E6
#define PABOOST true 

// undefine this if you don't care about terminal echo
#define TERM_ECHO

#ifdef DEBUG
  #define DPRINT(x) Serial.print(x)
  #define DPRINTLN(x) Serial.println(x)
#else
  #define DPRINT(x)
  #define DPRINTLN(x)
#endif

#define CONFIGURED_COOKIE 0xbeef

struct myConfig     // stored in eeprom - i believe the lor32u4ii has 1KB of eeprom
{
  int cookie;
  char callsign[10];
  char beaconText[50];
  bool beaconEnabled;
  bool usePfx;
  long frequency;
  int spreadingFactor;
  long signalBandwidth;
  int codingRate;
  int syncWord;
  long preambleLength;
  long beaconInterval;
} config;


#define NUM_CHANNELS 18  
long channels[NUM_CHANNELS] = { // TODO:  Review for accuracy (maybe make channel sets).  These are LoRaWAN frequencies
  9039E5,
  9041E5,
  9043E5,
  9045E5,
  9047E5,
  9049E5,
  9051E5,
  9053E5,
  9046E5,
  9233E5,
  9239E5,
  9245E5,
  9251E5,
  9257E5,
  9263E5,
  9269E5,
  9275E5,
  9233E5
};

long lastBeacon[NUM_CHANNELS]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int scanEnabled=0;

char output[200];
char inbuffer[100];
int  inbufferidx=0;

char serialBuffer[100] = "";
char serialBuffer2[100]="";
char *serialCommand;
char *commandOption;

void setup() {
  Serial.begin(9600);
  
#ifdef DEBUG
  while (!Serial);
#endif

  EEPROM.get(0, config);

  if(config.cookie != CONFIGURED_COOKIE || config.frequency < channels[0] || config.frequency > channels[NUM_CHANNELS-1]){
#ifdef DEBUG
    Serial.print("Before defaulting config:  \n");
    printStatus();    
#endif
    snprintf(config.beaconText, "CQ CQ CQ - running LoRaHamBeacon %s by ve7udp@yahoo.com ", VERSION, sizeof(config.beaconText));
    sprintf(config.callsign,"N0CALL");
    config.frequency = channels[0];
    config.spreadingFactor = 7;
    config.preambleLength=8;
    config.codingRate=5;
    config.signalBandwidth=125000;
    config.syncWord=69; 
    config.usePfx=true;
    config.beaconEnabled=false;
    Serial.println("** Configuration defaulted");
  }


 
  Serial.print("LoRa Terminal for Amateur Radio Use v");
  Serial.println(VERSION);
  Serial.println("(c) 2020 Matthew Asham - VE7UDP - ve7udp@yahoo.com");
  Serial.println("");
    //
  

  
  Serial.print("Initlalizing LoRa..");
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND,PABOOST )) {
    Serial.println("Starting LoRa failed!");
    while (1);
  } 

  LoRa.setSpreadingFactor(config.spreadingFactor);
  LoRa.setPreambleLength(config.preambleLength);
  LoRa.setCodingRate4(config.codingRate);
  LoRa.setSignalBandwidth(config.signalBandwidth);
  LoRa.setSyncWord(config.syncWord);
  
  Serial.println("done!");

  printStatus();

  printPrompt();
  
}


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}


void printStatus()
{
    
  Serial.print("Callsign: ");
  Serial.println(config.callsign);
  Serial.print("Frequency: ");
  Serial.print(config.frequency);
  Serial.println(" Hz");

  Serial.print("Beacon: ");
  Serial.print("(");
  Serial.print(config.beaconInterval / 60000 );
  Serial.print(" minutes) ");
  Serial.print(config.beaconText);
  if(config.beaconEnabled){
    Serial.println(" (enabled)");
  } else {
    Serial.println(" (disabled)");
  }

  Serial.print("Spreading Factor: ");
  Serial.println(config.spreadingFactor);

  Serial.print("Sync Word: ");
  Serial.println(config.syncWord);
  
  Serial.print("Bandwidth: ");
  Serial.print(config.signalBandwidth);
  Serial.println(" Hz");

  Serial.print("Coding Rate: ");
  Serial.println(config.codingRate);

  Serial.print("Preamble Length: ");
  Serial.println(config.preambleLength);

  Serial.print("Free memory: ");
  Serial.print(freeMemory());
  Serial.println(" bytes");
  
}

void transmit(long frequency, char *data)
{
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("XMIT ");
  Serial.print(frequency);
  Serial.print( ": '");
  LoRa.setFrequency(frequency);
  
  LoRa.beginPacket();
  if(config.usePfx){
    sprintf(output, "%s: %s", config.callsign, data);
    LoRa.print(output);
    Serial.print(output);
    Serial.println("'");
  } else {
    LoRa.print(data);
    Serial.print(data);
    Serial.println("'");
  }

  LoRa.endPacket();
  
  digitalWrite(LED_BUILTIN, LOW);  
}


void beacon(long frequency)
{
  Serial.print("BEACON ");
  transmit(frequency, config.beaconText);
  lastBeacon[0] = millis();
}

void printPrompt()
{
  Serial.print(config.callsign);
  Serial.print(" ");
  Serial.print(config.frequency);
  Serial.println(">");
}

void loop() {
  // try to parse packet

  
  int readByte;
  
  // command parser to take serial console commands and do stuff
  while(Serial.peek()>0){

    readByte = Serial.read();
    
#ifdef TERM_ECHO
    Serial.print(char(readByte));
#endif

    if(readByte==10||readByte==13){
      
      // oooh hacky
      
      DPRINT("Read command '");
      DPRINT(serialBuffer);
      DPRINTLN("'");
      
      if(serialBuffer[0]=='/'){
        char *p;
        strncpy(serialBuffer2, serialBuffer, sizeof(serialBuffer2));
  
        if(strchr(serialBuffer, ' ') != NULL){
          DPRINTLN("tokens");
          serialCommand = strtok_r(serialBuffer2, " ", &p);
          commandOption = strtok_r(NULL, " ", &p);
        } else {
          serialCommand = serialBuffer2;
          commandOption=NULL;
        }
          
        DPRINT("serialCommand = '");
        DPRINT(serialCommand);
        DPRINTLN("'");
  
        
        DPRINT("commandOption = '");
        DPRINT(commandOption);
        DPRINTLN("'");

        // TODO:  Move strcasecmp command processor to a struct->processFunc pattern.  Use _F() to minimize sram usage
        if(!strcasecmp(serialCommand, "/help")){
          
          //Serial.println("There is no help for you..");
          Serial.println("/beacon /bival /btext /dfl /callsign /channel /frequency /reg /scan /save /reboot /mesh");

        } else if(!strcasecmp(serialCommand, "/bival")){
          if(commandOption){
            config.beaconInterval = atoi(commandOption) * 60000;
          }

          Serial.print("Beacon Interval set to: ");
          Serial.print(config.beaconInterval);
          Serial.println(" ms");
          
        } else if(!strcasecmp(serialCommand, "/btext")){
          
          if(commandOption){
            strncpy(config.beaconText, serialBuffer+strlen("/btext "), sizeof(config.beaconText));
            //strcpy(config.beaconText, commandOption);
          }
          
          Serial.print("Beacon is: ");
          Serial.println(config.beaconText);
          
        } else if(!strcasecmp(serialCommand, "/callsign")){
          
          if(commandOption){
            strncpy(config.callsign, commandOption, sizeof(config.callsign));
          }
          
          Serial.print("Callsign is: ");
          Serial.println(config.callsign);
          
        } else if(!strcasecmp(serialCommand, "/channel") || !strcasecmp(serialCommand, "/qsy")){
        
          if(commandOption){
            
            int newChannel = atoi(commandOption);
            if(newChannel < 0 || newChannel > NUM_CHANNELS){
              Serial.println("Illegal channel #");
              continue;
            }
            config.frequency = channels[newChannel];
            
          } else {
            int i;
            for(i=0;i<NUM_CHANNELS; i++){
              Serial.print("Channel #");
              Serial.print(i);
              Serial.print(" ");
              Serial.print(channels[i]);
              Serial.println(" Hz");
            }
            Serial.println("Use /channel <channelid> to quickly set a new frequency by channel #");
          
          }
          
          Serial.print("Current frequency is: ");
          Serial.print(config.frequency);

          Serial.println(" Hz");

        } else if(!strcasecmp(serialCommand, "/frequency")){
          
          if(commandOption){
            config.frequency=atol(commandOption);
          }
          Serial.print("Current frequency is: ");
          Serial.print(config.frequency);
          Serial.println(" Hz");

        } else if(!strcasecmp(serialCommand, "/pfx")){
          
          config.usePfx = !config.usePfx;
          if(config.usePfx){
            Serial.println("Transmissions are prefixed with callsign");
          } else {
            Serial.println("Transmissions are NOT prefixed with callsign");
          }
          
        } else if(!strcasecmp(serialCommand, "/reg")){
          
          LoRa.dumpRegisters(Serial);
          
        } else if(!strcasecmp(serialCommand, "/scan")){
          
          scanEnabled = !scanEnabled;
          if(scanEnabled){
            Serial.println("Scan enabled");
          } else {
            Serial.println("Scan disabled");
          }
          
        } else if(!strcasecmp(serialCommand, "/beacon")){
          
          config.beaconEnabled = !config.beaconEnabled;
          if(config.beaconEnabled){
            Serial.println("Beacon enabled");
          } else {
            Serial.println("Beacon disabled");
          }
          
        } else if(!strcasecmp(serialCommand,"/mesh")){
          
          sprintf(config.beaconText, "MESH<%s> CQCQCQ", config.callsign);
          Serial.print("Beacon set to: ");
          Serial.print(config.beaconText);
          Serial.println("");
          
        } else if(!strcasecmp(serialCommand,"/save")){

          config.cookie = CONFIGURED_COOKIE;
          EEPROM.put(0, config);
          Serial.println("config structure written to 0x00");
          
        } else if(!strcasecmp(serialCommand, "/reboot")){
            
            Serial.println("Rebooting... (hopefully)\n");
            wdt_disable();
            wdt_enable(WDTO_15MS);
            while (1) {}
            
        } else if(!strcasecmp(serialCommand, "/status")){
          
          printStatus();
          
        } else if(!strcasecmp(serialCommand, "/dfl")){
          
          config.cookie = 0;
          EEPROM.put(0, config);
          Serial.println("reset cookie");
          Serial.println("Rebooting... (hopefully)\n");
          wdt_disable();
          wdt_enable(WDTO_15MS);
          while (1) {}
          
        } else {
          Serial.println("Command unknown");
        } 


        
      } else {  // this is not a command (buffer[0] != '/')

        
        if(strcmp(serialBuffer, "")){

          DPRINT("In adhoc handler serialBuffer='");
          DPRINT(serialBuffer);
          DPRINTLN("'");
          
          if(scanEnabled){
            Serial.println("Scanning disabled for adhoc comm");
          }
          scanEnabled=0;
          
          LoRa.setFrequency(config.frequency);
  
          Serial.print("LoRa transmit: ");
          
          Serial.print(config.frequency);
          Serial.print(" : ");
          
          if(1){
            
            LoRa.beginPacket();
            
            if(config.usePfx){
              sprintf(output,"%s: %s", config.callsign, serialBuffer);
              LoRa.print(output);
              Serial.println(output);
            } else {
              LoRa.print(serialBuffer);
              Serial.println(serialBuffer);
            }
              
            LoRa.endPacket();
            
          } else {
            Serial.println("xmit disabled");
          }
        } // we have something in serialBuffer
        
      } // End of command processing (new line/cr was received)

      
      serialBuffer[0]=0;
      printPrompt();
      
      
    } else {  // serialBuffer management
      serialBuffer[strlen(serialBuffer)+1]=0;
      serialBuffer[strlen(serialBuffer)] = readByte;
      
      //strncat(serialBuffer, char(readByte), sizeof(serialBuffer)); //char(readByte));
      
      DPRINT("Read '");
      DPRINT(char(readByte));
      DPRINTLN("'");
      DPRINT("Serial buffer is now '");
      DPRINT(serialBuffer);
      DPRINTLN("'");
    }
  }

   // TODO:  Fix this so that lastBeacon gets used properly again.  This used to contain lastBeacon[channel] so that we could work on multiple channels. but now we're just using index 0 since we might use any abirtrarey frequency.  
  if(config.beaconEnabled && config.beaconInterval > 0 && ( lastBeacon[0] == 0 || millis() - lastBeacon[0] > config.beaconInterval)){  // Don't allow beacons to occur more than 60,000 seconds
      beacon(config.frequency);
  }
      
  if(scanEnabled){
    int i=0;
    for(i=1; i<18;i++){

      // lazy coders lora stumbler
      
      LoRa.setFrequency(channels[i]);

      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        // received a packet
        Serial.print("Scanner received : ");
        Serial.print(channels[i]);
        Serial.print("  '");
        // read packet
        while (LoRa.available()) {
          Serial.print((char)LoRa.read());
        }
        // print RSSI of packet
        Serial.print("' with RSSI ");
        Serial.print(LoRa.packetRssi());
        Serial.print(" SNR ");
        Serial.print(LoRa.packetSnr());
        
        Serial.println("");
      }
     
    }
  } else {
      
      char *inptr;
      char readbyte;
      
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        // received a packet

        // read packet
        while (LoRa.available()) {
          
          readbyte = (char)LoRa.read();
          
          if(readbyte == 10 || readbyte == 13 ){ 
 
            continue;
          } else {
            // Just populate the buffer
            inbuffer[inbufferidx++]=readbyte;
            inbuffer[inbufferidx]=0;
          }

        }
    
        Serial.print("RCVD ");
        Serial.print(config.frequency);
        Serial.print(": '");
        Serial.print(inbuffer);
          
        // print RSSI of packet
        Serial.print("' ");
        Serial.print(inbufferidx);
        Serial.print(" with RSSI ");
        Serial.print(LoRa.packetRssi());
        Serial.print(" SNR ");
        Serial.print(LoRa.packetSnr());
        
        Serial.println("");

        if(!strcasecmp(inbuffer, "ping")){
           transmit(config.frequency, "pong");
        }

        // reset the buffer
        inbufferidx=0;
        inbuffer[0]=0;

      }
  }
   
}
