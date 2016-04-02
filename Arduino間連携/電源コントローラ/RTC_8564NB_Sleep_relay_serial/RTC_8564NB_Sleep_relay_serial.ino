/*
 *
 */

#include <Wire.h>
#include <skRTClib.h>

/* 時計関係 */
#include <Time.h>
unsigned long bootTime;

#define DEBUG

#define ARDUINO_SLEEP
#define AVR
#define SERIAL_COM
//#define SERIAL_UPLOAD

#ifdef SERIAL_COM
#define SERIAL_COM_RX 10
#define SERIAL_COM_TX 11
#ifdef SERIAL_UPLOAD
#define SERIAL_UPLOAD_RX 12
#define SERIAL_UPLOAD_TX 13
#endif
#endif

#if defined(SERIAL_COM) || defined(SERIAL_UPLOAD)
#include <SoftwareSerial.h>
#endif

#ifdef ARDUINO_SLEEP
#include <avr/sleep.h>
#endif /* ARDUINO_SLEEP */

//
// Mega2560の割込み
//
//#define INT_NUMBER 0
//#define PIN_NUMBER 21
//#define INT_NUMBER 1
//#define PIN_NUMBER 20
//#define INT_NUMBER 2
//#define PIN_NUMBER 19
//#define INT_NUMBER 3
//#define PIN_NUMBER 18
//#define INT_NUMBER 4
//#define PIN_NUMBER 2
//#define INT_NUMBER 5
//#define PIN_NUMBER 3
//
// UNOの割り込み
//
#define INT_NUMBER 0
#define PIN_NUMBER 2
//#define INT_NUMBER 1
//#define PIN_NUMBER 3
//
// Leonardoのピン配置
//
//#define INT_NUMBER 0
//#define PIN_NUMBER 3
//#define INT_NUMBER 1
//#define PIN_NUMBER 2
//#define INT_NUMBER 2
//#define PIN_NUMBER 0
//#define INT_NUMBER 3
//#define PIN_NUMBER 1
//
// M0 pro http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Zero/gaiyo.htm
//
//#define INT_NUMBER 9
//#define PIN_NUMBER 3
//#define INT_NUMBER 6
//#define PIN_NUMBER 8

#ifdef SERIAL_COM
//#define MAX_BUFFER_LENGTH 256
SoftwareSerial serialCom(SERIAL_COM_RX, SERIAL_COM_TX); // RX, TX
#ifdef SERIAL_UPLOAD
SoftwareSerial serialUpload(SERIAL_UPLOAD_RX, SERIAL_UPLOAD_TX); // RX, TX
#endif
#endif


#ifdef ARDUINO_SLEEP
/*
 *　端末が眠る場合の眠りの深さの指定
 */
//#define STANDBY_MODE SLEEP_MODE_IDLE
//#define STANDBY_MODE SLEEP_MODE_ADC
//#define STANDBY_MODE SLEEP_MODE_PWR_SAVE
//#define STANDBY_MODE SLEEP_MODE_STANDBY
#define STANDBY_MODE SLEEP_MODE_PWR_DOWN
#endif

#define SHUTDOWN_SIGNAL 9
#define RELAY_SIGNAL 4
bool relayFlag=false;
/*
 * 端末が眠る期間の指定
 */
#define SLEEP_DURATION 30 //単位の倍数
//#define SLEEP_UNIT 0 // 244.14us単位
//#define SLEEP_UNIT 1 //15.625ms単位
#define SLEEP_UNIT 2 //秒単位
//#define SLEEP_UNIT 3 //分単位

// 起動時の時刻設定のコマンド入力待ち時間
#define COMMAND_TIMER_THRESHOLD 15 // 15秒
#define SERIAL_THRESHOLD 10000 // 10秒

// アプリプロセッサの処理に必要な最長の時間
#define WORK_TIMER_THRESHOLD 30 // 30秒

// 改行文字の定義
#define LINE_FEED 0x0a
#define CR 0x0d

#define TERMINATE_CHAR  CR //CR
//#define TERMINATE_CHAR LINE_FEED  //LF

// その他一般的な定数の定義
#define BUFF_MAX 256

skRTClib skRTC = skRTClib() ;             // Preinstantiate Objects

/*
 * RTC(CLKOUT)からの外部割込みで実行される関数
 */
void InterRTC()
{
  skRTC.InterFlag = 1 ;
}



/*******************************************************************************
*  時刻を設定する対話処理                                                      *
*******************************************************************************/
void shell(){
  byte tm[7] ;
  unsigned long currentTime;
  unsigned int yearVal=-1,monthVal=-1,dayVal=-1,wDayVal=-1,hourVal=-1,minVal=-1,secVal=-1;
retry:
  bootTime=now();
  Serial.setTimeout(SERIAL_THRESHOLD);
  Serial.println("Please input current time.");
yearRetry:
  Serial.print("Year (2000 - 2100): ");
  String yearString = Serial.readStringUntil(TERMINATE_CHAR);
  yearVal=yearString.toInt();
  Serial.println(yearVal);
  if ((yearVal < 2000)||(yearVal>2100)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    yearVal=-1;
    goto yearRetry;
  }
  bootTime=now();
monthRetry:
  Serial.print("Month (1 - 12): ");
  String monthString = Serial.readStringUntil(TERMINATE_CHAR);
  monthVal=monthString.toInt();
  Serial.println(monthVal);
  if ((monthVal < 1)||(monthVal>12)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    monthVal=-1;
    goto monthRetry;
  }
  bootTime=now();
dayRetry:
  Serial.print("Day (1 - 31): ");
  String dayString = Serial.readStringUntil(TERMINATE_CHAR);
  dayVal=dayString.toInt();
  Serial.println(dayVal);
  if ((dayVal < 1)||(dayVal>31)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    dayVal=-1;
    goto dayRetry;
  }
  bootTime=now();
wDayRetry:
  Serial.print("week Day (sun(0), mon(1), tue(2) , wen(3), tur(4), fri(5), sat(6): ");
  String wDayString = Serial.readStringUntil(TERMINATE_CHAR);
  wDayVal=wDayString.toInt();
  Serial.println(wDayVal);
  if ((wDayVal < 0)||(wDayVal>6)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    wDayVal=-1;
    goto wDayRetry;
  }
  bootTime=now();
hourRetry:
  Serial.print("Hour (0 - 23): ");
  String hourString = Serial.readStringUntil(TERMINATE_CHAR);
  hourVal=hourString.toInt();
  Serial.println(hourVal);
  if ((hourVal < 0)||(hourVal>23)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    hourVal=-1;
    goto hourRetry;
  }
  bootTime=now();
minRetry:
  Serial.print("Min (0 - 59): ");
  String minString = Serial.readStringUntil(TERMINATE_CHAR);
  minVal=minString.toInt();
  Serial.println(minVal);
  if ((minVal < 0)||(minVal>59)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    minVal=-1;
    goto minRetry;
  }
  bootTime=now();
secRetry:
  Serial.print("Sec (0 - 59): ");
  String secString = Serial.readStringUntil(TERMINATE_CHAR);
  secVal=secString.toInt();
  Serial.println(secVal);
  if ((secVal < 0)||(secVal>59)) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    secVal=-1;
    goto secRetry;
  }
  bootTime=now();
again:
  Serial.print(yearVal);Serial.print("/");Serial.print(monthVal);Serial.print("/");Serial.print(dayVal);Serial.print(" ");Serial.print(hourVal);Serial.print(":");Serial.print(minVal);Serial.print(":");Serial.println(secVal);
  Serial.print("The time is correct?  [Y/n] : ");
  String yesno = Serial.readStringUntil(TERMINATE_CHAR);
  Serial.println(yesno);
  if ((!(yesno.equals("Y")))&&(!(yesno.equals("N")))&&(!(yesno.equals("y")))&&(!(yesno.equals("n")))) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD){
      return;
    }
    goto again;
  }
  bootTime=now();
  if (yesno.equals("N")||(yesno.equals("n"))) {
    yearVal=-1,monthVal=-1,dayVal=-1,hourVal=-1,minVal=-1,secVal=-1;
    goto retry;
  }
  Serial.println("Let us go");
  //
  char buff[BUFF_MAX] ;
  int ans = skRTC.sTime((byte)(yearVal-2000),(byte)monthVal,(byte)dayVal,(byte)wDayVal,(byte)hourVal,(byte)minVal,(byte)secVal);
  if (ans == 0) {
    Serial.println("RTC configuration successfull") ;// 初期化成功
    skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
    skRTC.cTime(tm,(byte *)buff) ;             // 日付と時刻を文字列に変換する
    Serial.println(buff) ;                   // シリアルモニターに表示
  } else {
    Serial.print("RTC configuration failure. ans=") ; // 初期化失敗
    Serial.println(ans) ;
    reboot() ;                         // 処理中断
  }
  setTime(skRTC.bcd2bin(tm[2]),skRTC.bcd2bin(tm[1]),skRTC.bcd2bin(tm[0]),skRTC.bcd2bin(tm[3]),skRTC.bcd2bin(tm[5]),skRTC.bcd2bin(tm[6]));
  bootTime=now();
}
/*******************************************************************************
*  reboot                                                                      *
*******************************************************************************/
void reboot() {
#ifdef AVR
  asm volatile ("  jmp 0");
#endif /* AVR */
#ifdef GR_KURUMI
  softwareReset();
#endif /* GR_KURUMI */
}
/*******************************************************************************
*  電源起動時とリセットの時だけのみ処理される関数(初期化と設定処理)            *
*******************************************************************************/
void setup()
{
  int ans ;
  byte tm[7] ;
  int configFlag=0;
  unsigned long currentTime;
  pinMode( PIN_NUMBER, INPUT);    // RTCからの割込みを読むのに利用
  pinMode(SHUTDOWN_SIGNAL,INPUT);
  Serial.begin(9600) ;                    // シリアル通信の初期化
  //ans = skRTC.begin(0,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  ans = skRTC.begin(PIN_NUMBER,INT_NUMBER,InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  if (ans == 0) {
    Serial.println("Successful initialization of the RTC") ;// 初期化成功
  } else {
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
    reboot() ;                         // 処理中断
  }
#ifdef SERIAL_COM
  serialCom.begin(9600);
#ifdef SERIAL_UPLOAD
  serialUpload.begin(9600);
#endif
#endif
  skRTC.rTime(tm) ;                        // RTCから現在の日付と時刻を読込む
  char buff[BUFF_MAX] ;
  skRTC.cTime(tm,(byte *)buff) ;             // 日付と時刻を文字列に変換する
  Serial.println(buff) ;                   // シリアルモニターに表示
  setTime(skRTC.bcd2bin(tm[2]),skRTC.bcd2bin(tm[1]),skRTC.bcd2bin(tm[0]),skRTC.bcd2bin(tm[3]),skRTC.bcd2bin(tm[5]),skRTC.bcd2bin(tm[6]));
  bootTime=now();
  Serial.print(F("Current system time : ")) ;
  Serial.print(year());Serial.print("/");Serial.print(month());Serial.print("/");Serial.print(day());Serial.print(" ");Serial.print(hour());Serial.print(":");Serial.print(minute());Serial.print(":");Serial.println(second());
  Serial.println(F("Hit any key to configure RTC.")) ;
  while(configFlag==0) {
    currentTime=now();
    if ((currentTime - bootTime) > COMMAND_TIMER_THRESHOLD) {
      configFlag=1;
    }
    if (Serial.available()) {
      configFlag=2;
      int data=Serial.read();
      shell();
    }
  }
  pinMode(RELAY_SIGNAL,OUTPUT);
  digitalWrite(RELAY_SIGNAL,LOW);
}
/*******************************************************************************
* 本来の仕事                                                                   *
*******************************************************************************/
bool doWork(){
  unsigned long currentTime;
  unsigned long startTime;
  //char buff[MAX_BUFFER_LENGTH];
  int configFlag=0;
  //int counter=0;
  //memset(buff,0,MAX_BUFFER_LENGTH);
  startTime=now();
  serialCom.setTimeout(SERIAL_THRESHOLD);
  while(configFlag==0) {
    currentTime=now();
    if ((currentTime - startTime) > WORK_TIMER_THRESHOLD) {
      return false;
    }
    if (serialCom.available()) {
      String processResult = serialCom.readStringUntil(TERMINATE_CHAR);
#ifdef DEBUG
      Serial.println(processResult);
#endif /* DEBUG */
#ifdef SERIAL_UPLOAD
      serialUpload.println(processResult);
#endif /* SERIAL_UPLOAD */
      configFlag=2;
    }
  }
  return true;
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
  byte tm[7] ; 
  char buf[24] ;
  int relayCounter=0;
#ifdef ARDUINO_SLEEP
  skRTC.SetTimer(SLEEP_UNIT,SLEEP_DURATION) ;
  goodNight(STANDBY_MODE);// 端末を眠らせる
#else
  delay(5000);
#endif
  if (skRTC.InterFlag == 1) {// 割込みが発生したか？
    skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
    Serial.println("Interupt and wake up!");
    skRTC.StopTimer();
    Serial.println("Relay ON");
    digitalWrite(RELAY_SIGNAL,HIGH);
    relayFlag = true;
  }
#ifdef SERIAL_COM
  if (!doWork()) {
    digitalWrite(RELAY_SIGNAL,LOW);
    return;
  }
#else
  skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
  skRTC.cTime(tm,(byte *)buf) ;             // 日付と時刻を文字列に変換する
  Serial.println(buf) ;                   // シリアルモニターに表示
#endif
  int loopCounter=0;
CheckRelay:
  loopCounter++;
  if (loopCounter > 10000){
    digitalWrite(RELAY_SIGNAL,LOW);
    return;
  }
  if (relayFlag) {
    if (digitalRead(SHUTDOWN_SIGNAL)==HIGH){
      if (relayCounter< 100){
        //Serial.println("counting to shutdown");
        relayCounter++;
        goto CheckRelay;
      } else {
        relayCounter=0;
        Serial.println(" shutdown");
        digitalWrite(RELAY_SIGNAL,LOW);
        relayFlag=false;
      }
    } else {
      //Serial.println(" shutdown count reset");
      relayCounter=0;
      goto CheckRelay;
    }
  }
}

#ifdef ARDUINO_SLEEP
void goodNight(int i) {
  Serial.println(F("  Good Night"));
  delay(100);
  noInterrupts();
  set_sleep_mode(i);
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
}
#endif
