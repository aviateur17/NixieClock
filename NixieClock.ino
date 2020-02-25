// Sample code taken from Nixie clock video from GreatScott! and from MD_DS1307_Test

//Todo
// 1. Store variables in RAM
// 1a. develop array to store date/time
// 2. display temperature
// 3. set time zone/time stored in UTC w/ time zone adjustment
// 4. Manual set of daylight savings/standard time via menu
// 5. When hour is midnight (00) display both zeroes on nixie display
// 6. when new minute time scrolls left to right or right to left
// 7. Enable or disable DST feature via menu option
// 8. Menu option to turn nixie outputs off on certain time range or turn CPU off for certain time ranges

// Done
// 1. Event based program
// 2. Easy way to set time via menu
// 3. automatic DST calculation/adjustment

#include <MD_DS1307.h>
#include <Wire.h>
#include <Eventually.h>

#define A1 3
#define B1 4
#define C1 5
#define D1 6
#define A2 7
#define B2 8
#define C2 9
#define D2 10
#define A3 11
#define B3 12
#define C3 13
#define D3 14
#define A4 15
#define B4 16
#define C4 2
#define D4 17
#define CSTCDTADDR 0x10  // RAM memory address 0x10
#define CST 0x53 // ASCII 'S'
#define CDT 0x44 // ASCII 'D'

#define PRINTS(s) Serial.print(F(s));
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }


#define NUMSAMPLES 15
#define DEBUG 1
#define TEMPPIN A7
//#define DISPLAYTIME 3
//#define DISPLAYDATE 3
#define R1VAL 3915
#define TIMEDISPLAY 1000    // Frequency to update the time (in milliseconds)
#define DATEDISPLAY 45000   // Time to display the date (in milliseconds)
#define INPUTUPDATE 100     // Frequency to check for keyboard input (in milliseconds)

char A[4] = {A1, A2, A3, A4};
char B[4] = {B1, B2, B3, B4};
char C[4] = {C1, C2, C3, C4};
char D[4] = {D1, D2, D3, D4};
int nixVal[4] = {0};
int hour;
int minute;
int day;
int month;
int samples[NUMSAMPLES];
float avgtemp = 0;
int adcval = 0;
float A7v = 0;
float resval = 0;
char c;
EvtManager mgr;

void setup() {
  pinMode(A1, OUTPUT);
  pinMode(B1, OUTPUT);
  pinMode(C1, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(B2, OUTPUT);
  pinMode(C2, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(B3, OUTPUT);
  pinMode(C3, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(B4, OUTPUT);
  pinMode(C4, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(TEMPPIN, INPUT);
  randomSeed(TEMPPIN);

  allOff();
  
  if (!RTC.isRunning())
    RTC.control(DS1307_CLOCK_HALT, DS1307_OFF);
  
  Serial.begin(57600);

  // Cycle all digits on nixies
  cycleUpToZero();
  
  //Turn off
  allOff();
  delay(2000);

  mgr.addListener(new EvtTimeListener(TIMEDISPLAY, true, (EvtAction)displayTime));
  mgr.addListener(new EvtTimeListener(DATEDISPLAY, true, (EvtAction)displayDate));
  mgr.addListener(new EvtTimeListener(INPUTUPDATE, true, (EvtAction)readInput));
  printTime();
  if(DEBUG) usage();
  
}

USE_EVENTUALLY_LOOP(mgr)

bool start() {
  mgr.resetContext();
  mgr.addListener(new EvtTimeListener(TIMEDISPLAY, true, (EvtAction)displayTime));
  mgr.addListener(new EvtTimeListener(DATEDISPLAY, true, (EvtAction)displayDate));
  mgr.addListener(new EvtTimeListener(INPUTUPDATE, true, (EvtAction)readInput));

  return true;
}

bool displayTime() {

  RTC.readTime();
  hour = RTC.h;
  minute = RTC.m;
  // if(DEBUG) printTime();
  nixVal[0] = (hour / 10) % 10;
  nixVal[1] = hour % 10;
  nixVal[2] =  (minute / 10) % 10;
  nixVal[3] = minute % 10;

  // randomize output on 15, 30, 45 minutes after the hour
  if(minute % 15 == 0 && RTC.s <= 10 && minute != 0)
    randomValues();

  // blink output on the hour
  if(minute == 0 && RTC.s <= 10) {
    for(int i=0; i<5; i++) {
      allOff();
      delay(500);
      if(nixVal[0] == 0 && nixVal[1] != 0) off(0);
      else writenumber(0,nixVal[0]);
      for(int i=1; i<4; i++)
        writenumber(i,nixVal[i]);
      delay(500);
    }
  }
  else {
    if(nixVal[0] == 0 && nixVal[1] != 0) off(0);
    else writenumber(0,nixVal[0]);
    for(int i=1; i<4; i++) {
      writenumber(i,nixVal[i]);
    }
  }
  return true;  
}

bool displayDate() {

  mgr.resetContext();
  mgr.addListener(new EvtTimeListener(7000, true, (EvtAction)start));
  mgr.addListener(new EvtTimeListener(INPUTUPDATE, true, (EvtAction)readInput));
  
  RTC.readTime();
  day = RTC.dd;
  month = RTC.mm;
 
  nixVal[0] = (month / 10) % 10;
  nixVal[1] = month % 10;
  nixVal[2] =  (day / 10) % 10;
  nixVal[3] = day % 10;
   for(int i=0; i<4; i++) {
    writenumber(i,nixVal[i]);
  }
  checkDST();

  return true;
}


////    avgtemp=0;
////    for(int j=0; j<NUMSAMPLES; j++) {
////      samples[j] = analogRead(TEMPPIN);
////      adcval += samples[j];
////      delay(20);
////    }
////    adcval /= NUMSAMPLES;
////    A7v = (float)adcval * 5 / 1023;
////    resval = (float)R1VAL * ((5 - A7v)/A7v);
////    if(DEBUG) printOutSerial();
//
//  // Cycle nixie tube numbers to 0
//  //cycleUpToZero();
//  
//  // display random values and then display time
//  //randomValues();


void randomValues() {
  
  for(int i=3; i>=0; i--) {
    for(int j=0; j<15; j++) {
      for(int k=0; k<=i; k++) {
        writenumber(k,random(0,9));
      }
      delay(75);
    }
    writenumber(i,nixVal[i]);
    delay(50);
  }
  
}

void printTime()
{
  uint8_t buf[1] = {0};
  RTC.readRAM(CSTCDTADDR, buf, 1);
  RTC.readTime();
  
  PRINTS("\n");
  PRINT("Current date/time: ", RTC.yyyy);
  PRINT("-", p2dig(RTC.mm, DEC));
  PRINT("-", p2dig(RTC.dd, DEC));
  PRINT(" ", p2dig(RTC.h, DEC));
  PRINT(":", p2dig(RTC.m, DEC));
  PRINT(":", p2dig(RTC.s, DEC));
  if (RTC.status(DS1307_12H) == DS1307_ON)
    PRINT(" ", RTC.pm ? "pm" : "am");
  if(buf[0] == CDT)
    PRINTS(" CDT ");
  if(buf[0] == CST)
    PRINTS(" CST ");
  if(buf[0] != CDT && buf[0] != CST)
    PRINTS(" !!Unknown DST!! ");
  PRINT(" ", dow2String(RTC.dow));
}

char readInput() {
// Read the next character from the serial input stream, skip whitespace.
// Busy loop with a delay.

  char  c;

  if(Serial.available()) {
    mgr.resetContext();
  
    c = Serial.read();
      
    switch(toupper(c)) {
      case '?': usage(); break;
      case 'C': writeControl(); break;
      case 'D': showDoW(); break;
      case 'T': 
        c = ReadNext();
        switch(toupper(c)) {
          case 'R': printTime(); break;
          case 'W': writeTime(); break;
          default: goto no_good;
        } break;
      case 'R':
        c = ReadNext();
        switch(toupper(c)) {
          case 'R': showRAM(); break;
          case 'W': writeRAM(); break;
          default: goto no_good;
        } break;
      case 'S': showStatus(); break;
      default: 
      no_good: {
        PRINT("\nNot Implemented '", c);
        PRINTS("'\n");
      }
      break;
    }
    // Clear the input buffer
    while(Serial.available())
      Serial.read();
    mgr.resetContext();
    mgr.addListener(new EvtTimeListener(TIMEDISPLAY, true, (EvtAction)displayTime));
    mgr.addListener(new EvtTimeListener(DATEDISPLAY, true, (EvtAction)displayDate));
    mgr.addListener(new EvtTimeListener(INPUTUPDATE, true, (EvtAction)readInput));
  }
  return true;
}

char ReadNext() {

  char c;
  do {
    while(!Serial.available())
      delay(50);
    c = Serial.read();
  } while(isspace(c));
   
  return(c); 
}

void usage() {
  PRINTS("\n---------------------------------------------------------");
  PRINTS("\n?\thelp - this message");
  PRINTS("\n\ntr\tread the current time");
  PRINTS("\ntw yyyymmdd hhmmss dw\twrite the current date, time and day of week (1-7)");
  PRINTS("\n\nrr\tread the contents of RAM buffer");
  PRINTS("\nrw aa nn vv [vv...]\twrite RAM address hex aa with nn hex values vv");
  PRINTS("\n\ns\tstatus of the RTC");
  PRINTS("\nd\tcalculate day of week from current date");
  PRINTS("\n\nc n v\twrite the value v to status n, where n is");
  PRINTS("\n\t0 - Clock Halt (n 0=run, 1=halt)");
  PRINTS("\n\t1 - SQW Enable(n 0=halt, 1=run)");
  PRINTS("\n\t2 - SQW Type (on) (n 1=1Hz, 2=4Mhz, 3=8Mhz, 4=32MHz)");
  PRINTS("\n\t3 - SQW Type (off) (n 0=low, 1=high)");
  PRINTS("\n\t4 - 12 hour mode (n 0=24h, 1=12h)\n");
  PRINTS("\n---------------------------------------------------------");
}

void showStatus() {
  PRINTS("\n---------------------------------------------------------");
  PRINT("\nClock Halt:\t", sts2String(RTC.status(DS1307_CLOCK_HALT)));
  PRINT("\nIs running:\t", RTC.isRunning());
  PRINT("\nSQW Output:\t", sts2String(RTC.status(DS1307_SQW_RUN)));
  PRINT("\nSQW Type (on):\t", sts2String(RTC.status(DS1307_SQW_TYPE_ON)));
  PRINT("\nSQW Type (off):\t", sts2String(RTC.status(DS1307_SQW_TYPE_OFF)));
  PRINT("\n12h mode:\t\t", sts2String(RTC.status(DS1307_12H)));
  PRINTS("\n---------------------------------------------------------");
}

void showDoW(void)
{
  RTC.readTime();
  PRINT("\nCalculated DoW is ", dow2String(RTC.calcDoW(RTC.yyyy, RTC.mm, RTC.dd)));
}

void printOutSerial() {
  Serial.print(nixVal[0]);
  Serial.print(":");
  Serial.print(nixVal[1]);
  Serial.print(":");
  Serial.print(nixVal[2]);
  Serial.print(":");
  Serial.print(nixVal[3]);
  Serial.print("  ADCVal: ");
  Serial.print(adcval);
  Serial.print(" A7v: ");
  Serial.print(A7v);
  Serial.print(" R: ");
  Serial.println(resval);
  //Serial.print(" Temp: ");
  //Serial.println(avgtemp);
  
}

void writeTime()
{
  int dow, day, month, hour;
  uint8_t buf[1] = {0};
    
  RTC.yyyy = i2dig(DEC)*100 + i2dig(DEC);
  RTC.mm = i2dig(DEC);
  RTC.dd = i2dig(DEC);
  
  RTC.h = i2dig(DEC);
  RTC.m = i2dig(DEC);
  RTC.s = i2dig(DEC);
  
  RTC.dow = i2dig(DEC);

  day = RTC.dd;
  month = RTC.mm;
  hour = RTC.h;
  dow = RTC.dow;
  
  // "Automatically" set daylight savings time or standard time
  if(day > 7 && month >= 3 && hour >= 2 && buf[0] == CST) {
    buf[0] = CDT;
    RTC.writeRAM(CSTCDTADDR, buf, 1);
    if(DEBUG) PRINTS("\nSet clock for daylight savings time!\n");
  }
  if(day < 8 && month == 11 && hour >= 2 && buf[0] == CDT) {
    buf[0] = CST;
    RTC.writeRAM(CSTCDTADDR, buf, 1);  
    if(DEBUG) PRINTS("\nSet clock for standard time!\n");
  }

  RTC.writeTime();
  PRINTS("\nWriting ");
  printTime(); 
}

void writeControl()
{
  char  c = ReadNext();
  uint8_t  item, value;
  
  switch (c)
  {
    case '0':  // halt
      item = DS1307_CLOCK_HALT;
      c = ReadNext();
      switch (c)
      {
        case '0': value = DS1307_OFF;  break;
        case '1': value = DS1307_ON;  break;
        default: goto error;
      }
      break;
      
    case '1':  // enable
      item = DS1307_SQW_RUN;
      c = ReadNext();
      switch (c)
      {
        case '0': value = DS1307_OFF;  break;
        case '1': value = DS1307_ON;   break;
        default: goto error;
      }
      break;
      
    case '2':  // type on
      item = DS1307_SQW_TYPE_ON;
      c = ReadNext();
      switch (c)
      {
        case '1': value = DS1307_SQW_1HZ;    break;
        case '2': value = DS1307_SQW_4KHZ;   break;
        case '3': value = DS1307_SQW_8KHZ;   break;
        case '4': value = DS1307_SQW_32KHZ;  break;
        default: goto error;
      }
      break;
      
    case '3':  // type off
      item = DS1307_SQW_TYPE_OFF;
      c = ReadNext();
      switch (c)
      {
        case '0': value = DS1307_SQW_LOW;   break;
        case '1': value = DS1307_SQW_HIGH;  break;
        default: goto error;
      }
      break;
      
    case '4':  // 12 h mode
      item = DS1307_12H;
      c = ReadNext();
      switch (c)
      {
        case '0': value = DS1307_OFF;  break;
        case '1': value = DS1307_ON;   break;
        default: goto error;
      }
      break;
      
    default:
 error:
      PRINTS("\nBad control element or parameter");
      return;
  }
  
  // do it
  PRINT("\nControlling ", ctl2String(item));
  PRINT(" value ", sts2String(value));
  
  RTC.control(item, value);
  
  return;
}

void writenumber(int a, int b) {
  switch (b) {
    case 0:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 9:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 8:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 7:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 6:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 5:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 4:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 3:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 2:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], HIGH);
      break;
    case 1:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], HIGH);
      break;
  }
}

void off(int a) {
  digitalWrite(A[a], HIGH);
  digitalWrite(B[a], HIGH);
  digitalWrite(C[a], HIGH);
  digitalWrite(D[a], HIGH);
}

void allOff() {
  for(int i=0;i<4;i++) {
    off(i);
  }
}

void cycleUpToZero() {
  for(int i=0; i<4; i++) {
    for(nixVal[i]; nixVal[i]<=10; nixVal[i]++) { writenumber(i,nixVal[i] % 10); delay(200); }
    delay(50);
  }
  delay(500);
}

uint8_t htoi(char c)
{
  c = toupper(c);
  
  if (c >= '0' && c <= '9')
      return(c - '0');
  else if (c >= 'A' && c <= 'F')
      return(c - 'A' + 10);
  else
      return(0);
}
        
uint8_t i2dig(uint8_t mode)
// input 2 digits in the specified base
{
  uint8_t  v = 0;
  char    c[3] = { "00" };

  c[0] = ReadNext();
  c[1] = ReadNext();

  switch (mode)
  {
    case DEC: v = atoi(c);  break;
    case HEX: v = (htoi(c[0]) << 4) + htoi(c[1]); ;  break;
  }
  
  return(v);
}

const char *p2dig(uint8_t v, uint8_t mode)
// print 2 digits leading zero
{
  static char c[3] = { "00" };

  switch(mode)
  {
    case HEX:
    {
      c[0] = htoa((v >> 4) & 0xf);
      c[1] = htoa(v & 0xf);
    }
    break;
  
    case DEC:
    {
      c[0] = ((v / 10) % 10) + '0';
      c[1] = (v % 10) + '0';
    }
    break;
  }

  return(c);
}

char htoa(uint8_t i)
{
  if (i < 10)
  {
    return(i + '0');
  }
  else if (i < 16)
  {
    return(i - 10 + 'a');
  }
  return('?');
}

const char *sts2String(uint8_t code)
{
  static const char *str[] = 
  {
    "ERROR",
    "ON",
    "OFF",
    "1Hz",
    "4KHz",
    "8KHz",
    "32KHz",
    "HIGH",
    "LOW"
  };

  return(str[code]);
}

const char *ctl2String(uint8_t code)
{
  static const char *str[] = 
  {
    "CLOCK_HALT",
    "SQW_RUN",
    "SQW_TYPE_ON",
    "SQW_TYPE_OFF",
    "12H MODE"
  };

  return(str[code]);
}

const char *dow2String(uint8_t code)
{
  static const char *str[] = { "---", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

  if (code > 7) code = 0;
  return(str[code]);
}

void showRAM()
{
  #define  MAX_READ_BUF  (DS1307_RAM_MAX / 8)  // do 8 lines
 
  uint8_t  buf[MAX_READ_BUF];

  for (int i=0; i<DS1307_RAM_MAX; i+=MAX_READ_BUF)
  {
    RTC.readRAM(i, buf, MAX_READ_BUF);
    
    PRINT("\n", p2dig(i, HEX));
    PRINTS(":");
    for (int j = 0; j < MAX_READ_BUF; j++)
      PRINT(" ", p2dig(buf[j], HEX));
    PRINTS("  ");
    for (int j=0; j<MAX_READ_BUF; j++)
    {
      if (isalnum(buf[j]) || ispunct(buf[j]))
      {
        PRINT(" ", (char)buf[j]);
      }
      else
        PRINTS(" .");
    } 
  }
}

void writeRAM()
{
  uint8_t  addr = i2dig(HEX);
  uint8_t  len = i2dig(HEX);
  uint8_t  val[DS1307_RAM_MAX];

  if ((len == 0) || (len > DS1307_RAM_MAX))
  {
    PRINTS("\nInvalid data length");
    return;
  }
  
  for (int i=0; i<len; i++)
    val[i] = i2dig(HEX);
  
  PRINT("\nAddress 0x", p2dig(addr, HEX));
  PRINTS(" write value");
  for (int i=0; i<len; i++)
    PRINT(" ", p2dig(val[i], HEX));
  
  PRINT("\n", RTC.writeRAM(addr, val, len));
  PRINTS(" bytes written");
}

void checkDST() {
  int dow, day, month, hour;
  uint8_t buf[1] = {0};
    
  RTC.readRAM(CSTCDTADDR, buf, 1);
  
  RTC.readTime();
  day = RTC.dd;
  month = RTC.mm;
  hour = RTC.h;
  dow = RTC.dow;

  // Savings time begins 2nd Sunday in March at 2AM (spring forward: +1)
  // Standard time begins 1st Sunday in November at 2AM (fall back: -1)
  // check if we are in daylight savings and date/time is after standard time ends
  // check if we are in standard time and date/time is after daylight savings time ends
  // if necessary, change the time and set the DST flag in RAM
  // CST = UTC minus 6 
  // CDT = UTC minus 5
  if(dow == 1 && day > 7 && day < 15 && month == 3 && hour >= 2 && buf[0] == CST) {
    buf[0] = CDT;
    RTC.h += 1;
    RTC.writeTime();
    RTC.writeRAM(CSTCDTADDR, buf, 1);
    if(DEBUG) {
      PRINTS("\nAdjusted clock for daylight savings time!\n");
      printTime();
    }
  }
  if(dow == 1 && day < 8 && month == 11 && hour >= 2 && buf[0] == CDT) {
    buf[0] = CST;
    RTC.h -= 1;
    RTC.writeTime();
    RTC.writeRAM(CSTCDTADDR, buf, 1);  
    if(DEBUG) {
      PRINTS("\nAdjusted clock for end of dalight savings time!\n");
      printTime();
    }
  }
}
