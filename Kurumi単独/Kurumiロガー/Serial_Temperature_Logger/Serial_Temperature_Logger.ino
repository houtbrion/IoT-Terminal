/*
 * シリアルを用いて，端末のシリアルポートからの出力をSDカードに保存するプログラム
 * 定期的にSHTもしくはDHTの温度・湿度センサの測定結果(温度と湿度)もログと共に，保存する
 */

// 各種機能のON/OFF
#define DEBUG // ONにするとシリアルに実行中のログメッセージが出力される
#define DEBUG_CHIP_SELECT  // SPI通信のデバッグ用
//#define UTC // RTCに入れる時刻はUTCかJSTか?
#define USE_INTERNAL_TIMER // RTCの利用は端末起動時に限定し，以後は内部の時計を利用
#define LED_ON_WRITE // 温度計測の時のみ光る
#define LED_ALWAYS_ON_WRITE // 書き込み時は常時光る

// 機種の定義
//#define MEGA
#define GR_KURUMI // 1.5V駆動で35mA (アイドル時) 1.1V駆動で53mA (アイドル時) 1.0V駆動では動作せず (DC/DCのせい?)

//利用する周辺機器の定義
// Pin 22,23,24 are assigned to RGB LEDs.
int led_red   = 22; // LOW active
int led_green = 23; // LOW active
int led_blue  = 24; // LOW active

//#define USE_SOFT_SERIAL
#ifdef USE_SOFT_SERIAL
#define SOFT_SERIAL_TX 6
#define SOFT_SERIAL_RX 7
#define pinSerialRx SOFT_SERIAL_RX //Rxに接続する (対象の機器Data Out) のピン番号
#define pinSerialTx SOFT_SERIAL_TX //Txに接続する (対象の機器のData In) のピン番号
#endif /* USE_SOFT_SERIAL */
// RTCの種別定義
//#define RTC8564NB
#define DS3234
#ifdef DS3234
#define DS3234_CHIP_SELECT 9
#endif /* DS3234 */
#define SD_CHIP_SELECT 6

// 温度湿度センサの定義
//#define SHT_SENSOR
#define DHT_SENSOR
#ifdef DHT_SENSOR
#define DHT_PIN 4
#endif /* DHT_SENSOR */

// その他一般的な定数の定義
#define BUFF_MAX 256
#define usbSerialBaudrate 9600 //シリアル通信のボーレート
//#define dataSerialBaudrate 115200 //データが入力されるシリアル通信のボーレート
#define dataSerialBaudrate 9600 //データが入力されるシリアル通信のボーレート

#ifdef MEGA
#define AVR
#endif /* MEGA */

#ifdef RTC8564NB
#include <Wire.h>
#include <skRTClib.h>
#endif /* RTC8564NB */
#ifdef DS3234
#include <SPI.h>
#include "ds3234.h"
//#include <RLduino78_RTC.h>
const int cs = DS3234_CHIP_SELECT;              // chip select pin
#endif /* DS3234 */

#ifdef SHT_SENSOR
#include <Wire.h>
#include <SHT2x.h>
#endif /* SHT_SENSOR */

#ifdef DHT_SENSOR
#include "DHT.h"
DHT dht;
#endif /* DHT_SENSOR */

#ifdef RTC8564NB // RTC8564NBは割り込みが使える
// 手持ちのMega2560では，INT0,1が動作しなかったため，大きい番号の割込みを利用
//#define INT_NUMBER 5
//#define PIN_NUMBER 18
// UNOはINT0,1が動作するため，こちらを利用
#define INT_NUMBER 0
#define PIN_NUMBER 2
// M0 pro http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Zero/gaiyo.htm
//#define INT_NUMBER 9
//#define PIN_NUMBER 3
//#define INT_NUMBER 6
//#define PIN_NUMBER 8

skRTClib skRTC = skRTClib() ;             // Preinstantiate Objects
#endif /* RTC8564NB */

/* 時計関係 */
#include <Time.h>
unsigned long bootTime,lastTime;

//#define TIMER_THRESHOLD 86400 // 24時間
//#define TIMER_THRESHOLD 7200 // 2時間
//#define TIMER_THRESHOLD 600 // 10分
//#define TIMER_THRESHOLD 60 // 1分
#define TIMER_THRESHOLD 30 // 30秒
//#define TIMER_THRESHOLD 10 // 10秒

#define REBOOT_THRESHOLD 86400 //一日
//#define REBOOT_THRESHOLD 3600 // 1時間
//#define REBOOT_THRESHOLD 600 // 10分
//#define REBOOT_THRESHOLD 120 // 2分
//#define REBOOT_THRESHOLD 60 // 1分
//#define REBOOT_THRESHOLD 30 // 30秒


#define SERIAL_THRESHOLD 10000 // 10秒
#define COMMAND_TIMER_THRESHOLD 15 // 15秒

#define LINE_FEED 0x0a
#define CR 0x0d

#define LOG_TERMINATE_CHAR CR // TeraTermでのテストはCRが行末
//#define LOG_TERMINATE_CHAR LINE_FEED // 無線ノードは LFが行末??
#define TERMINATE_CHAR  CR //CR
//#define TERMINATE_CHAR LINE_FEED  //LF

#ifdef USE_SOFT_SERIAL
// ソフトシリアル関係の定義
#include <SoftwareSerial.h>
SoftwareSerial sSerial(pinSerialRx, pinSerialTx);  // ソフトウェアシリアルの設定
#endif /* USE_SOFT_SERIAL */

bool lineFlag=true;


// SDシールド関係の定義
#include <SPI.h>
#include <SdFat.h>

const uint8_t chipSelect = SD_CHIP_SELECT;  // sparc funのsdシールドは8番
#define FILE_BASE_NAME "Data"  //データファイルのベース名
SdFat sd;      // File system object
SdFile file;  // Log file.
#define error(msg) sd.errorHalt(F(msg))  // エラーをSDに残すためのマクロ
#define SDCARD_SPEED SPI_HALF_SPEED  // 安全のため，低速モードで動作させる場合
//#define SDCARD_SPEED SPI_FULL_SPEED  // 性能重視の場合


#ifdef GR_KURUMI
//#include <RLduino78.h>
#include <RLduino78_RTC.h>
void alarm_handler();
RTC_TIMETYPE trtc;
int interFlag=0;
void alarm_handler()
{
  //interFlag=1;
  reboot();
}
#endif /* GR_KURUMI */

#ifdef RTC8564NB
/*
 * RTC(CLKOUT)からの外部割込みで実行される関数
 */
void InterRTC()
{
  skRTC.InterFlag = 1 ;
  Serial.println("");
  Serial.println(F("Interupt!"));
  Serial.println("");
}
#endif /* RTC8564NB */


/*******************************************************************************
*  電源起動時とリセットの時だけのみ処理される関数(初期化と設定処理)            *
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
#ifdef RTC8564NB
  int ans = skRTC.sTime((byte)(yearVal-2000),(byte)monthVal,(byte)dayVal,(byte)wDayVal,(byte)hourVal,(byte)minVal,(byte)secVal);
  if (ans == 0) {
    Serial.println("RTC configuration successfull") ;// 初期化成功
    skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
    skRTC.cTime(tm,(byte *)buff) ;             // 日付と時刻を文字列に変換する
    Serial.println(buff) ;                   // シリアルモニターに表示
  } else {
    Serial.print("RTC configuration failure. ans=") ; // 初期化失敗
    Serial.println(ans) ;
    //reboot() ;                         // 処理中断
  }
  setTime(skRTC.bcd2bin(tm[2]),skRTC.bcd2bin(tm[1]),skRTC.bcd2bin(tm[0]),skRTC.bcd2bin(tm[3]),skRTC.bcd2bin(tm[5]),skRTC.bcd2bin(tm[6]));
#endif /* RTC8564NB */
#ifdef DS3234
  struct ts t;
  t.sec = secVal;
  t.min = minVal;
  t.hour = hourVal;
  t.wday = wDayVal;
  t.mday = dayVal;
  t.mon = monthVal;
  t.year = yearVal;
  DS3234_set(cs, t);
  DS3234_get(cs, &t);
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,t.mon, t.mday, t.hour, t.min, t.sec);
  Serial.println(buff);
#ifdef UTC
  setTime(t.unixtime+(9*60*60));
#else /* UTC */
  setTime(t.unixtime);
#endif /* UTC */
#endif /* DS3234 */
  bootTime=now();
}

//
// 全体の初期化
//
void setup()
{
#ifdef RTC8564NB
  int ans ;
  byte tm[7] ;
#endif /* RTC8564NB */
  char buff[BUFF_MAX] ;
  int configFlag=0;
  unsigned long currentTime;
#if defined(RTC8564NB) || defined(SHT_SENSOR)
  Wire.begin();
#endif /* RTC8564NB or SHT_SENSOR */
#ifdef DHT_SENSOR
  dht.setup(DHT_PIN); // DHTをセットアップ
#endif /* DHT_SENSOR */
#ifdef DEBUG_CHIP_SELECT
  pinMode(cs,OUTPUT);
  pinMode(chipSelect,OUTPUT);
  digitalWrite(cs,HIGH);
  digitalWrite(chipSelect,HIGH);
#endif /* DEBUG_CHIP_SELECT */
  //pinMode( PIN_NUMBER, INPUT);    // RTCからの割込みを読むのに利用
  Serial.begin(usbSerialBaudrate) ;                    // シリアル通信の初期化
#ifdef USE_SOFT_SERIAL
  sSerial.begin(dataSerialBaudrate);  // ソフトウェアシリアルの初期化
#else /* USE_SOFT_SERIAL */
#ifdef MEGA
  Serial3.begin(dataSerialBaudrate);  // 機器接続用シリアルポート初期化
#endif /* MEGA */
#ifdef GR_KURUMI
  Serial1.begin(dataSerialBaudrate);  // 機器接続用シリアルポート初期化
#endif /* GR_KURUMI */
#endif /* USE_SOFT_SERIAL */
#ifdef GR_KURUMI
  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_blue, OUTPUT);
  //for (int i=0;i<10;i++){
  digitalWrite(led_red, HIGH);
  digitalWrite(led_green, HIGH);
  digitalWrite(led_blue, HIGH);
  for (int i=0;i<10;i++){
    digitalWrite(led_red,LOW);
    delay(200);
    digitalWrite(led_red, HIGH);
    digitalWrite(led_green, LOW);
    delay(200);
    digitalWrite(led_green, HIGH);
    digitalWrite(led_blue, LOW);
    delay(200);
    digitalWrite(led_blue, HIGH);
  }
#endif /* GR_KURUMI */
#ifdef RTC8564NB
  ans = skRTC.begin(PIN_NUMBER,INT_NUMBER,InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  if (ans == 0) {
    Serial.println(F("Successful initialization of the RTC")) ;// 初期化成功
  } else {
    Serial.print(F("Failed initialization of the RTC ans=")) ; // 初期化失敗
    Serial.println(ans) ;
    while(1) ;                         // 処理中断
  }
  skRTC.rTime(tm) ;                        // RTCから現在の日付と時刻を読込む
  skRTC.cTime(tm,(byte *)buf) ;             // 日付と時刻を文字列に変換する
  Serial.println(buf) ;                   // シリアルモニターに表示
  setTime(skRTC.bcd2bin(tm[2]),skRTC.bcd2bin(tm[1]),skRTC.bcd2bin(tm[0]),skRTC.bcd2bin(tm[3]),skRTC.bcd2bin(tm[5]),skRTC.bcd2bin(tm[6]));
#endif /* RTC8564NB  */
#ifdef DS3234
  struct ts t;
  DS3234_init(cs, DS3234_INTCN);
  Serial.println("Start time");
  DS3234_get(cs, &t);
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,t.mon, t.mday, t.hour, t.min, t.sec);
  Serial.println(buff) ;                   // シリアルモニターに表示
#ifdef UTC
  setTime(t.unixtime+(9*60*60));
#else /* UTC */
  setTime(t.unixtime);
#endif /* UTC */
#endif /* DS3234 */
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
#ifdef GR_KURUMI
  int err = rtc_init();

  trtc.year    = year()-2000;
  trtc.mon     = month();
  trtc.day     = day();
  trtc.weekday = weekday()+1;
  trtc.hour    = hour();
  trtc.min     = minute();
  trtc.second  = second();

  err = rtc_set_time(&trtc);
  rtc_attach_alarm_handler(alarm_handler);
  if (trtc.hour!=1){
    err = rtc_set_alarm_time(1, 0);
  }
  //digitalWrite(led_blue, LOW);
  //digitalWrite(led_green, HIGH);
#endif /* GR_KURUMI */
#ifdef GR_KURUMI
#endif /* GR_KURUMI */
  Serial.println(F("Start"));
  char fileName[13] = FILE_BASE_NAME "00.txt";

  if (!sd.begin(chipSelect, SDCARD_SPEED)) {
#ifdef GR_KURUMI
    digitalWrite(led_blue, HIGH);
    digitalWrite(led_green, HIGH);
    digitalWrite(led_red, LOW);
#endif /* GR_KURUMI */
    sd.initErrorHalt();
  }
  openLogFile();
  
  Serial.print(F("Logging to: "));
  Serial.println(fileName);
  
  lastTime=now();
#ifdef GR_KURUMI
    digitalWrite(led_blue, LOW);
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, LOW);
    delay(1000);
    digitalWrite(led_blue, HIGH);
    digitalWrite(led_green, HIGH);
    digitalWrite(led_red, HIGH);
#endif /* GR_KURUMI */
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
#ifndef USE_INTERNAL_TIMER
#ifdef RTC8564NB
  byte tm[7] ;
#endif /* RTC8564NB */
#ifdef DS3234
  struct ts t;
#endif /* DS3234 */
  char buff[BUFF_MAX] ;
#endif /* USE_INTERNAL_TIMER */
//#ifndef GR_KURUMI
  if ((1==hour())&&(REBOOT_THRESHOLD < (now() - bootTime))){ // 稼働時間が1時間以上で，夜中一時だったらリブート
#ifdef DEBUG
    Serial.println(F("reboot"));
#endif /* DEBUG */
    reboot();
  }
//#endif /* GR_KURUMI */
  if ((TIMER_THRESHOLD < (now()-lastTime))&& lineFlag) {
#ifdef LED_ON_WRITE
    digitalWrite(led_red,LOW);
    digitalWrite(led_green,LOW);
    digitalWrite(led_blue,LOW);
#endif /* LED_ON_WRITE */
    // 温度を測る
#ifndef USE_INTERNAL_TIMER
    memset(buff,0,BUFF_MAX-1);
#endif /* USE_INTERNAL_TIMER */
#ifndef USE_INTERNAL_TIMER
#ifdef RTC8564NB
    skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
    skRTC.cTime(tm,(byte *)buff) ;             // 日付と時刻を文字列に変換する
#ifdef DEBUG
    Serial.print(buff) ;                   // シリアルモニターに表示
#endif /* DEBUG */
#endif /* RTC8564NB */
#ifdef DS3234
#ifdef DEBUG_CHIP_SELECT
    digitalWrite(chipSelect,HIGH);
#endif /* DEBUG_CHIP_SELECT */
    DS3234_get(cs, &t);
#ifdef DEBUG
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,t.mon, t.mday, t.hour, t.min, t.sec);
    Serial.print(buff) ;                   // シリアルモニターに表示
#endif /* DEBUG */
#endif /* DS3234 */
#else /* USE_INTERNAL_TIMER */
    Serial.print(year());Serial.print("/");Serial.print(month());Serial.print("/");Serial.print(day());Serial.print(" ");Serial.print(hour());Serial.print(":");Serial.print(minute());Serial.print(":");Serial.print(second());
#endif /* USE_INTERNAL_TIMER */
#ifdef DEBUG
#ifdef SHT_SENSOR
    Serial.print(F(" : Humidity(%RH) = "));
    Serial.print(SHT2x.GetHumidity());
    Serial.print(F(" , Temperature(C) = "));
    Serial.println(SHT2x.GetTemperature());
#endif /* SHT_SENSOR */
#ifdef DHT_SENSOR
    Serial.print(F(" : Humidity(%RH) = "));
    Serial.print(dht.getHumidity(), 1);
    Serial.print(F(" , Temperature(C) = "));
    Serial.println(dht.getTemperature(), 1);
#endif /* DHT_SENSOR */
#endif /* DEBUG */
#ifndef USE_INTERNAL_TIMER
    file.print(buff) ;                   // ファイルに書き込み
#else /* USE_INTERNAL_TIMER */
    file.print(year());file.print("/");file.print(month());file.print("/");file.print(day());file.print(" ");file.print(hour());file.print(":");file.print(minute());file.print(":");file.print(second());
#endif /* USE_INTERNAL_TIMER */
#ifdef SHT_SENSOR
    file.print(F(" : Humidity(%RH) = "));
    file.print(SHT2x.GetHumidity());
    file.print(F(" , Temperature(C) = "));
    file.println(SHT2x.GetTemperature());
#endif /* SHT_SENSOR */
#ifdef DHT_SENSOR
    file.print(F(" : Humidity(%RH) = "));
    file.print(dht.getHumidity(), 1);
    file.print(F(" , Temperature(C) = "));
    file.println(dht.getTemperature(), 1);
#endif /* DHT_SENSOR */
    file.flush();
    lastTime=now();
#ifdef LED_ON_WRITE
    digitalWrite(led_red,HIGH);
    digitalWrite(led_green,HIGH);
    digitalWrite(led_blue,HIGH);
#endif /* LED_ON_WRITE */
  }
#ifdef USE_SOFT_SERIAL
  if (sSerial.available()) {
    int data=sSerial.read();
#endif /* USE_SOFT_SERIAL */
#ifdef MEGA
  if (Serial3.available()) {
    int data=Serial3.read();
#endif /* MEGA */
#ifdef DATA_CONSOLE
  if (Serial.available()) {
    int data=Serial.read();
#endif /* DATA_CONSOLE */
#ifdef GR_KURUMI
  if (Serial1.available()) {
    int data=Serial1.read();
#endif /* GR_KURUMI  */
#ifdef LED_ALWAYS_ON_WRITE
    digitalWrite(led_red,LOW);
    digitalWrite(led_green,LOW);
    digitalWrite(led_blue,LOW);
#endif /* LED_ALWAYS_ON_WRITE */
    if (lineFlag) {
#ifndef USE_INTERNAL_TIMER
      memset(buff,0,BUFF_MAX-1);
#ifdef RTC8564NB
      skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
      skRTC.cTime(tm,(byte *)buff) ;             // 日付と時刻を文字列に変換する
#endif /* RTC8564NB */
#ifdef DS3234
#ifdef DEBUG_CHIP_SELECT
      digitalWrite(chipSelect,HIGH);
#endif /* DEBUG_CHIP_SELECT */
      DS3234_get(cs, &t);
      snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,t.mon, t.mday, t.hour, t.min, t.sec);
#endif /* DS3234 */
#endif /* USE_INTERNAL_TIMER */
#ifdef DEBUG
#ifndef USE_INTERNAL_TIMER
      Serial.print(buff) ;                   // シリアルモニターに表示
#else /* USE_INTERNAL_TIMER */
      Serial.print(year());Serial.print("/");Serial.print(month());Serial.print("/");Serial.print(day());Serial.print(" ");Serial.print(hour());Serial.print(":");Serial.print(minute());Serial.print(":");Serial.print(second());
#endif /* USE_INTERNAL_TIMER */
      Serial.print(F(" : "));
#endif /* DEBUG */
#ifndef USE_INTERNAL_TIMER
      file.print(buff) ;
#else /* USE_INTERNAL_TIMER */
      file.print(year());file.print("/");file.print(month());file.print("/");file.print(day());file.print(" ");file.print(hour());file.print(":");file.print(minute());file.print(":");file.print(second());
#endif /* USE_INTERNAL_TIMER */
      file.print(F(" : "));
      lineFlag=false;
    }
    if ((data==CR)||(data==LINE_FEED)) {
      if (data==LOG_TERMINATE_CHAR) {
        lineFlag=true;
#ifdef DEBUG
        Serial.println("");
#endif /* DEBUG */
        file.println("");
        file.flush();
#ifdef LED_ALWAYS_ON_WRITE
        digitalWrite(led_red,LOW);
        digitalWrite(led_green,LOW);
        digitalWrite(led_blue,LOW);
#endif /* LED_ALWAYS_ON_WRITE */
      }
    } else {
#ifdef DEBUG
      Serial.write(data);
#endif /* DEBUG */
      logData(data);
    }
  }
}
//
// 本体をリセットする関数
//
void reboot() {
#ifdef AVR
  asm volatile ("  jmp 0");
#endif /* AVR */
#ifdef GR_KURUMI
  softwareReset();
#endif /* GR_KURUMI */
}
// Log a data record.
void logData(int data) {
  file.write(data);
  if (!file.sync() || file.getWriteError()) {
    error("write error");
  }
}

// ログファイルのオープン
void openLogFile(){
  int lastNumber=0;
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char logFileName[13] = FILE_BASE_NAME "00.txt";
  lastNumber=searchLogFile();
  logRotation(lastNumber);
  // ログファイルをオープン
#ifdef DEBUG
  Serial.print(F("open file : name = "));
  Serial.println(logFileName);
#endif
  if (!file.open(logFileName, FILE_WRITE)) {
    error("file.open");
  }
  file.flush();
}
// ログファイルの数のチェック
int searchLogFile(){
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.txt";
  int lastFileNumber=0;
  // ログファイルのうち，番号が一番大きい物を探し，lastFileNumberに番号を代入
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
#ifdef DEBUG
      Serial.print("lastFileNumber = ");
      Serial.println(lastFileNumber);
#endif
      return(100);
    }
    lastFileNumber++;
  }
#ifdef DEBUG
  Serial.print("lastFileNumber = ");
  Serial.println(lastFileNumber);
#endif
  return(lastFileNumber);
}
// logローテーション
void logRotation(int num){
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char srcfileName[13] = FILE_BASE_NAME "00.txt";
  char dstfileName[13] = FILE_BASE_NAME "00.txt";
  char eraseFileName[13] = FILE_BASE_NAME "99.txt";
  int counter=num;
  int src1Num,src2Num,dst1Num,dst2Num;
#ifdef DEBUG
  Serial.print("logRotation(num); num = ");
  Serial.println(num);
#endif
  if (counter==0) {
#ifdef DEBUG
    Serial.println(" do nothing");
#endif
    return;
  } else if (counter==100) {
#ifdef DEBUG
    Serial.println("remove oldest file");
#endif
    sd.remove(eraseFileName);
    counter--;
  }
#ifdef DEBUG
  Serial.println("log rotation");
#endif
  while (counter>0) {
    dst1Num=counter%10;
    dst2Num=int(counter/10);
    src1Num=(counter-1)%10;
    src2Num=int((counter-1)/10);
    //ファイル名の作成
    srcfileName[BASE_NAME_SIZE + 1]=0x30 + src1Num;
    srcfileName[BASE_NAME_SIZE    ]=0x30 + src2Num;
    dstfileName[BASE_NAME_SIZE + 1]=0x30 + dst1Num;
    dstfileName[BASE_NAME_SIZE    ]=0x30 + dst2Num;
#ifdef DEBUG
    Serial.print("src file name = ");
    Serial.println(srcfileName);
    Serial.print("dst file name = ");
    Serial.println(dstfileName);
#endif
    // counter-1のファイルをcounterのファイルにrename
    sd.rename(srcfileName,dstfileName);
    // counterの減算
    counter--;
  }
}


