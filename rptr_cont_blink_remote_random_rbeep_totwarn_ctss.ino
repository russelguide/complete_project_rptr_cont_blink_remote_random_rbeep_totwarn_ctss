
#include <Timer.h>
#include <EEPROMex.h>  //allows reading and writing nearly all types of variables to/from EEPROM.
#include <EEPROMVar.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "rtc.h"
#include "talk.h"

//-------------------(addr, en,rw,rs,d4,d5,d6,d7,bl,blpol)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address // In my case 0x27

Timer t;               //the main timer. also see onesec().
Timer r;               //timer for blinking led during remote mode

int MorseWPM 					= 20;       		// The CWID Code Speed setting - Change from 20 to your desired WPM.
String MorseString 				= "DY JEM2PH/R";	//Place your Call Sign here. (i.e. "DY JEM2PH/R")
int COSactive 					= 0;              	// Is your COS active High or Low? (1=High, 0=LOW)?
int TPLactive 					= 1;              	// Is your TPL active High or Low? (1=High, 0=LOW)? if your receiver has no CTCSS Decode, leave TPLactive = 1, and connect pin D6 to +5 VCDC.
int TOT 						= 30;             	//timeout timer 30 Seconds - Change to suit your needs. To protect from "STUCK MIKE" conditions.
int totWARN 					= 25;
int CWID_TIM 					= 300;				//cwid timer - 300 Seconds(10 Minutes) Maximum  - Change to suit your needs.
int FANtime 					= 20;				//The FAN run time variable - the time that you want to hold the fan after the transmission ends.

unsigned long int RPTOFF 		= 14757;    //DTMF Command codes to OFF the Repeater.
unsigned long int RPTON 		= 14758;    //DTMF Command codes to ON the Repeater.
unsigned long int CTSSOFF 		= 14759;    //DTMF Command codes to disable the CTSS.
unsigned long int CTSSON 		= 14795;    //DTMF Command codes to enable the CTSS.
unsigned long int ALLOFF 		= 14788;    //Turn OFF all active setting
unsigned long int ALLON 		= 14799;    //Turn ON all setting
const unsigned long PERIOD 		= 120;   //seconds
const unsigned long idle 		= 60;
volatile unsigned long timpPrimImpuls;
volatile unsigned long timpUltimImpuls;
volatile unsigned long numImpuls;

int pttRead 					= 0;	// Reads PTT status from operator (initialize to RX).
int tuneReadPrevious 			= 0; 	// Memorize "tune" signal previous level (initilize to OFF).
int randomBeep 					= 0;
int rogerb 						= 1; 	//(1=ON rogerbeep enable) (0=OFF rogerbeep disable)
int ctcss             = 1;

// PIN ASSIGNMENT
const int COSin 				= 11;	// "Key Up" signal input for the repeater ("squelch" Activity --on the repeater receiver's COS signal).
const int TPLin 				= 6;	// The PL input from receiver with TPL decode capability.  ( "CTCSS decoded" by the receiver)
const int outPTT 				= 4;	// PTT triggering for the transmitter.
const int MorseLEDPin 			= 13;	// flash the "MorseCOCDE CAll Sign" on a LED.
const int MorseTonePin 			= 5;	// Call Sign output in Morse Code -- 800 hz tone.
const int fanpin 				= 12;	//FAN trigger ON/OFF
const int cwidTest 				= 9;	//Ground to send test cwid.
const int RPTRLED 				= 7;	//Repeater Status Indicator
const int CTSSLED 				= 8;	//CTSS enable/disable status indicator
const int RXLED 				= 10;	//RX active indicator
const int PLLED 				= 13;

int FANgate 					= 0;	//the fan timer gating signl.
int MorseMode 					= 2;
int BeaconMode 					= 0;
int timeout;
int fantime;
int fan 						= 0;
int BUTC 						= 0;
int cwidtimer;
int codetime;
float bat_volt 					= 0;

unsigned long cmdcode 			= 0; 	//unsigned int does not work!!

unsigned int dtmf;
int dtmftim;//dtmf entry timeout timer count.
int digit 						= 0;
int DIGIT 						= 0;	//digit counter --set with dtmf interupt, reset with timer or completed action
unsigned long CMD 				= 0;    //unsigned int does not work!!

int RepeaterDebug 				= 1; 	// Debug mode. 1=on, 0=off. Printing some data on Serial Monitor.
int MorseDebug 					= RepeaterDebug;

int togate;    							//timeout timer's gate signal.
int cwgate;    							//cwid timer.s gate signal.
int cwdel;   							//A delay timer to assure no call sign transmission unless the repeater was inactive before keyup.(COS inactive for 10 seconds).
int dtmfpress;
int COS;
int RPTR;								//holds the status of the repeater.
int TPLR;								//Tone Required
int TPL;								//decision var for active hi/lo CTCSS.

// Define morse alphabet in array
char* MorseCodeAlphabet[] = {
  "A", ".-", "B", "-...", "C", "-.-.", "D", "-..", "E", ".", "F", "..-.", "G", "--.",
  "H", "....", "I", "..", "J", ".---", "K", "-.-", "L", ".-..", "M", "--", "N", "-.",
  "O", "---", "P", ".--.", "Q", "--.-", "R", ".-.", "S", "...", "T", "-", "U", "..-",
  "V", "...-", "W", ".--", "X", "-..-", "Y", "-.--", "Z", "--..",
  "0", "-----", "1", ".----", "2", "..---", "3", "...--", "4", "....-", "5", ".....",
  "6", "-....", "7", "--...", "8", "---..", "9", "----.", ",", "--..--",
  ".", ".-.-.-", "-", "-....-", "=", "..--..", "/", "-..-.", " ", " "
};

byte battery[8] =  //icon for battery
{
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111
};

void setup() {  // SETUP to run ONCE
  Serial.begin(115200);
  attachInterrupt(0, readDTMF, FALLING);	//interrupt 0 read dtmf code if valid tone is detected
  DDRC = 00; // initialize the ANALOG Pins as digital pin inputs.
  pinMode(PLLED, OUTPUT);
  pinMode(COSin, INPUT_PULLUP);
  pinMode(TPLin, INPUT_PULLUP);
  pinMode(cwidTest, INPUT_PULLUP);
  pinMode(outPTT, OUTPUT);
  pinMode(MorseLEDPin, OUTPUT);
  pinMode(MorseTonePin, OUTPUT);
  pinMode(A0, INPUT);// this is declaring port c lower 4 bits as inputs for the DTMF receiver bcd code input.*************************************
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(fanpin, OUTPUT);
  pinMode(RPTRLED, OUTPUT);
  pinMode(CTSSLED, OUTPUT);
  pinMode(RXLED, OUTPUT);
  lcd.begin(20, 4);  // initialize the lcd for 16 chars 2 lines (16,2), turn on backlight
  lcd.backlight(); // finish with backlight on
  lcd.createChar(2, battery);
  lcd.setCursor(0, 0);
  lcd.print("Repeater Controller");
  delay(5000);
  lcd.clear();

  codetime == 0;
  togate == 0;
  cwgate == 0;
  cwdel == 0;
  BUTC == 0;
  rogerb == 1;

  RPTR = EEPROM.readInt(0);//read form EEPROM at setup.
  if (RPTR != 0) {
    RPTR = 1;
    digitalWrite(RPTRLED, HIGH);
  }
  TPLR = EEPROM.readInt(4);//read form EEPROM at setup.
  if (TPLR != 0) {
    TPLR = 1;
    digitalWrite(CTSSLED, HIGH);
  }
  if (RPTR == 1) {
    RPTR_ON();
  }
  EEPROM.updateInt(0, RPTR); //initial default = 1.
  Serial.print("RPTR = "); Serial.println(RPTR);
  EEPROM.updateInt(4, TPLR); //initial default = 1.
  Serial.print("TPLR = "); Serial.println(TPLR);
  t.every(1000, onesec);    //sets the timer to pulse every 1 second. Then calls the "onesec() function".
}

float readFrequency(unsigned int sampleTime)
{
  numImpuls = 0; // start a new reading
  attachInterrupt(1, counter, RISING); // enable the interrupt
  delay(sampleTime);
  detachInterrupt(1);
  return (numImpuls < 3) ? 0 : (996500.0 * (float)(numImpuls - 2)) / (float)(timpUltimImpuls - timpPrimImpuls);
}

void counter()
{
  unsigned long now = micros();
  if (numImpuls == 1)
  {
    timpPrimImpuls = now;
  }
  else
  {
    timpUltimImpuls = now;
  }
  ++numImpuls;
}

// Main program LOOP

void loop() {
  repeater();
  
}

void repeater() {
  detCTCSS();
  if (DIGIT > digit) {
    if ((dtmf == 15) and (BUTC == 0)) { //Waiting of Button C is press to accept codes
      resetDATA();
      BUTC = 1;
      r.oscillate(RPTRLED, PERIOD, HIGH);
      codetime = 1;
    }
  }

  if (BUTC == 1) {
    r.update();
    if (dtmfpress >= 10) { //Wait timer for Code about 10 seconds, if no code wass press it will reset the routine.
      digitalWrite(outPTT, HIGH);
      delay(400);
      TransmitMorse(2, "ERR", MorseWPM, MorseLEDPin, 5, 800, 0);
      resetDATA();
    }
  }

  if ((BUTC == 1) and (DIGIT > digit)) {
    remote();
  }

  t.update();
  if (cwidtimer >= CWID_TIM) {
    if (RPTR == 1) {
      cwid();
    }
  }
  if (digitalRead(cwidTest) ==  LOW) {//Ground to send test cwid.
    if (RPTR == 1) {
      cwid();
    }
  }

  if (((digitalRead(COSin) == HIGH) and COSactive == 1)  or ((digitalRead(COSin) == LOW) and COSactive == 0)) {
    COS = 1;

  }
  else {
    COS = 0;

  }

  if (((digitalRead(TPLin) == HIGH) and TPLactive == 1)  or ((digitalRead(TPLin) == LOW) and TPLactive == 0)) {
    TPL = 1;
  }
  else {
    TPL = 0;
  }

  if ((COS == 1) and (TPL == 1)) {

    digitalWrite(RXLED, HIGH);

    detCOSin();
  }

  if ((COS == 1) and (TPL == 0)) {
    detCOSin();
  }

  if (COS == 0) { // If no RX audio received, reset the the time out timer.
    timeout = 0;
  }


  if ((COS == 1) and (timeout < TOT)) {
    if ((((RPTR == 1) and (TPL == 1) && TPLR == 1)) || ((RPTR == 1) && (TPLR == 0))) {
      detCOSin();
      digitalWrite(outPTT, HIGH);
      FANgate = 0;
      cwgate = 1;
      togate = 1;

      if (timeout >= 5) {
        digitalWrite(fanpin, HIGH);
        fantime = 0;
        FANgate = 0;

      }

      if (timeout >= totWARN) {
        TransmitMorse(2, "T", MorseWPM, MorseLEDPin, 5, 800, 0);
      }


    }
    else {
      digitalWrite(outPTT, LOW);
      FANgate = 1;
      togate = 0;

    }
    if (timeout >= 10) {
      digitalWrite(fanpin, HIGH);
      fantime = 0;
      FANgate = 0;
    }
  }
  else {
    if ((togate == 0) && (COS == 1)) {
      digitalWrite(RXLED, HIGH);
    }
    if (COS == 0) {
      digitalWrite(RXLED, LOW);
    }
    delay(50);
    digitalWrite(outPTT, LOW);
    FANgate = 1;
    togate = 0;
  }
  if (fantime >= FANtime) {
    FANgate = 0;
    fantime = 0;
    digitalWrite(fanpin, LOW);
  }
  //path for first CWID  ---delays 1st CWID until 10 secs repeater idle- prevents 2nd kerchunk CWID
  if (cwdel >= idle && timeout >= 1) {//Sends CWID when RX in active at first time
    timeout = timeout + 5;
    cwdel = 0;
    cwid();
  }

  if (timeout >= TOT) {
    togate = 0;
    digitalWrite(outPTT, LOW);
    delay(50);
  }
}

void detCTCSS()
{
  float freq = readFrequency(100);
  lcd.setCursor(0, 0);//1st line
  lcd.print("Freq:");
  lcd.print (freq);
  lcd.setCursor (0, 1);//2nd line
  // Too low but over 10 Hz
  if ((freq > 10) && (freq < 65.8) || (freq > 256.80))
  {
    digitalWrite(PLLED, LOW);  // visual indication FRIEND CTCS
  }
  else if ((freq > 231.70) && (freq < 235.00))
  {
    lcd.print("PL:233.6");
    digitalWrite(PLLED, HIGH);
  }
  else
  {
    lcd.print("PL:-----");
    digitalWrite(PLLED, LOW);  // visual indication FRIEND CTCSS
    lcd.print("        ");
  }
}

void TransmitMorse(int MorseMode, String MorseString, int MorseWPM, int MorseLEDPin, int MorseTonePin, int MorseToneFreq, int BeaconMode) {
  int CWdot = 1200 / MorseWPM;
  int CWdash = (1200 / MorseWPM) * 3;
  int istr;
  for (istr = 0; istr < MorseString.length(); istr++) {
    String currentchar = MorseString.substring(istr, istr + 1);
    int ichar = 0;
    while (ichar < sizeof(MorseCodeAlphabet)) {
      String currentletter = MorseCodeAlphabet[ichar];   // letter
      String currentcode = MorseCodeAlphabet[ichar + 1]; // corresponding morse code
      if (currentletter.equalsIgnoreCase(currentchar)) {
        int icp = 0;
        for (icp = 0; icp < currentcode.length(); icp++) {

          if (currentcode.substring(icp, icp + 1).equalsIgnoreCase(".")) {
            if (MorseMode <= 2) {
              digitalWrite(MorseLEDPin, HIGH);
            }
            if (MorseMode >= 2) {
              tone(MorseTonePin, MorseToneFreq);
            }
            delay(CWdot);
            if (MorseMode <= 2) {
              digitalWrite(MorseLEDPin, LOW);
            }
            if (MorseMode >= 2) {
              noTone(MorseTonePin);
            }
            delay(CWdot);
          }

          else if (currentcode.substring(icp, icp + 1).equalsIgnoreCase("-")) {
            // Depending om Mode, what to transmit?
            if (MorseMode <= 2) {
              digitalWrite(MorseLEDPin, HIGH);
            }
            if (MorseMode >= 2) {
              tone(MorseTonePin, MorseToneFreq);
            }
            delay(CWdash);
            if (MorseMode <= 2) {
              digitalWrite(MorseLEDPin, LOW);
            }
            if (MorseMode >= 2) {
              noTone(MorseTonePin);
            }
            delay(CWdot);
          }

          else if (currentcode.substring(icp, icp + 1).equalsIgnoreCase(" ")) {
            delay(CWdot * 4);
          }
        }
      }
      ichar = ichar + 2; // Increase by two (every characters has one index for character and one for the code).
    }
    delay(CWdot * 3); // Delay after each character is transmitted
  }

  delay(CWdot * 7);
  // Beacon mode? Then send long tone.
}


void cwid() {
  togate = 1;   //delay timer to preclude double CWIDs
  digitalWrite(outPTT, HIGH);
  delay(200);
  TransmitMorse(2, MorseString, MorseWPM, MorseLEDPin, 5, 800, 0);   //send call sign
  cwidtimer = 0;
  cwgate = 0;
  if (timeout == TOT) {
    Time_Out();
  }
}

void detCOSin() { // Main function, generates roger beep (executed endlessly).
  pttRead = digitalRead(COSin); // Read PTT input from operator on analog pin 3.
  if (pttRead == 0) {  // If operator presses on PTT (beginning of a transmission).
    tuneReadPrevious = 1;
	  if (ctcss == 1) {
	  //detCTCSS();
	  }
  }
  if ((COS == 1) && (TPL == 0) && (TPLR == 1)) {
    tuneReadPrevious = 0;
  }
  if ((pttRead == 1) && (tuneReadPrevious == 1) && (rogerb == 1) && (RPTR == 1)) { // If operator releases PTT (end of transmission).
    delay(300);
    rogerbeep();
    tuneReadPrevious = 0;
    randomBeep = randomBeep + 1;
  }
}
void rogerbeep() {
  if (randomBeep == 3) {
    randomBeep = 0;
  }
  if (randomBeep == 0) {
    rogerbeep2();
  }
  else if (randomBeep == 1) {
    rogerbeep1();
  }
  else if (randomBeep == 2) {
    rogerbeep3();
  }
}
void rogerbeep1() {
  tone(MorseTonePin, 523.25, 200); delay(100);
  tone(MorseTonePin, 349.23, 200); delay(100);
  tone(MorseTonePin, 261.63, 200); delay(100);
  tone(MorseTonePin, 261.63, 200); delay(100);
}
void rogerbeep2() {
  tone(MorseTonePin, 261.63, 200); delay(100);
  tone(MorseTonePin, 329.63, 200); delay(100);
  tone(MorseTonePin, 523.25, 200); delay(100);
  tone(MorseTonePin, 523.25, 200); delay(100);
}
void rogerbeep3() {
  tone(MorseTonePin, 293.66, 200); delay(100);
  tone(MorseTonePin, 493.88, 200); delay(100);
  tone(MorseTonePin, 261.63, 200); delay(100);
  tone(MorseTonePin, 523.25, 200); delay(100);
}


void onesec() { //this is the main timer function.  The timers will count up each second, when their respective gate is open.
  if (DIGIT >= 1) {
    dtmftim = dtmftim + 1;
  }
  if (dtmftim == 7) { //7 seconds is max code input time! otherwise codes are invalid.
    dtmftim = 0;//Reset the dtmf entry timeout timer.
    DIGIT = 0;//Reset the command code digit counter.
    cmdcode = 0;
    digit = 0;
  }
  if (togate == 1) {
    timeout = timeout + 1;
  }
  if (cwgate == 1) {
    cwidtimer = cwidtimer + 1;
  }
  if (COS == 0) {
    cwdel = cwdel + 1;// timer for repeater idle of 120 seconds..
  }
  if (FANgate == 1) {
    fantime = fantime + 1;
  }
  if (codetime == 1) {
    dtmfpress = dtmfpress + 1;
  }
}

void remote() {
  digit = DIGIT;
  cmdcode = cmdcode + (dtmf);

  if (DIGIT < 5) {
    cmdcode = cmdcode * 10, DEC;
  }
  if (DIGIT >= 5) {
    DIGIT = 0;
    CMD = cmdcode;
    if (CMD == RPTOFF) {
      RPTR = 0;//repeater off
      EEPROM.updateInt(0, RPTR);
      codeAccepted();
      RPTR_OFF();
    }
    if (CMD == RPTON) {
      RPTR = 1; //repeater on
      EEPROM.updateInt(0, RPTR);
      codeAccepted();
      RPTR_ON();
    }
    if (CMD == CTSSOFF) {
      TPLR = 0; //TPL not required
      EEPROM.updateInt(4, TPLR);
      codeAccepted();
    }
    if (CMD == CTSSON) {
      TPLR = 1;//on - TPL Required
      EEPROM.updateInt(4, TPLR);
      codeAccepted();
    }
    if (CMD == ALLOFF) {
      TPLR = 0;
      RPTR = 0;
      EEPROM.updateInt(4, TPLR);
      EEPROM.updateInt(0, RPTR);
      codeAccepted();
      RPTR_OFF();
    }
    if (CMD == ALLON) {
      TPLR = 1;
      RPTR = 1;
      EEPROM.updateInt(4, TPLR);
      EEPROM.updateInt(0, RPTR);
      codeAccepted();
      RPTR_ON();
    }
    if ((CMD != RPTON) && (CMD != RPTOFF) && (CMD != CTSSOFF) && (CMD != CTSSON) && (CMD != ALLOFF) && (CMD != ALLON)) {
      if (RPTR == 0) {
      }
      if (RPTR == 1) {
        INVALID();
      }
    }
    resetDATA();
  }
}

void resetDATA() {
  cmdcode = 00;
  digit = 0;
  DIGIT = 0;
  dtmftim = 0;
  dtmfpress = 0;
  CMD = 00;
  BUTC = 0;
  codetime = 0;
  dtmfpress = 0;
  r.stop(RPTRLED);
  Status();
}

void Status() {
  if (RPTR != 0) {
    digitalWrite(RPTRLED, HIGH);
  }
  if (RPTR == 0) {
    digitalWrite(RPTRLED, LOW);
  }
  if (TPLR != 0) {
    digitalWrite(CTSSLED, HIGH);
  }
  if (TPLR == 0) {
    digitalWrite(CTSSLED, LOW);
  }
}

void codeAccepted() {
  Status();
  digitalWrite(outPTT, HIGH);
  delay(400);
  TransmitMorse(2, "I", MorseWPM, MorseLEDPin, 5, 800, 0);
}

void readDTMF() {  //"interrupt service routine".
  dtmf = PINC & 0x0F;    //Get 4 l.s. bits port c from dtmf receiver.  port C; C0=A0,C1=A1,C2=A2, & C3= A3....
  DIGIT = DIGIT + 1;
  delay(50);

}

void Time_Out() {
  digitalWrite(outPTT, HIGH);
  delay(400);
  TransmitMorse(2, "TOT", MorseWPM, MorseLEDPin, 5, 800, 0);
  delay(100);
}

void RPTR_OFF() {
  digitalWrite(outPTT, HIGH);
  delay(400);
  TransmitMorse(2, "DEACTIVATED", MorseWPM, MorseLEDPin, 5, 800, 0);
  delay(100);
}

void RPTR_ON() {
  digitalWrite(outPTT, HIGH);
  delay(400);
  TransmitMorse(2, "ACTIVATED", MorseWPM, MorseLEDPin, 5, 800, 0);
  delay(100);
  rogerbeep();
}

void INVALID() {
  Status();
  digitalWrite(outPTT, HIGH);
  delay(400);
  TransmitMorse(2, "H", MorseWPM, MorseLEDPin, 5, 800, 0);
  delay(100);
}
