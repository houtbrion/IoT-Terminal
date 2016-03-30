#include <Wire.h>
#include <skRTClib.h>

/* 時計関係 */
#include <Time.h>
unsigned long bootTime;

#define ARDUINO_SLEEP
#define AVR
#define USE_RTC
#define USE_XBEE


#ifdef ARDUINO_SLEEP
#include <avr/sleep.h>
#endif /* ARDUINO_SLEEP */

//
// Mega2560の割込み
//
//#define RTC_PIN_NUMBER 21
//#define RTC_PIN_NUMBER 20
//#define RTC_PIN_NUMBER 19
//#define RTC_PIN_NUMBER 18
//#define RTC_PIN_NUMBER 2
//#define RTC_PIN_NUMBER 3
//
// UNOの割り込み
//
#define RTC_PIN_NUMBER 2
//#define RTC_PIN_NUMBER 3
//
// Leonardoのピン配置
//
//#define RTC_INT_NUMBER 0
//#define RTC_PIN_NUMBER 3
//#define RTC_INT_NUMBER 1
//#define RTC_PIN_NUMBER 2
//#define RTC_INT_NUMBER 2
//#define RTC_PIN_NUMBER 0
//#define RTC_INT_NUMBER 3
//#define RTC_PIN_NUMBER 1
//
// M0 pro http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Zero/gaiyo.htm
//
//#define RTC_INT_NUMBER 9
//#define RTC_PIN_NUMBER 3
//#define RTC_INT_NUMBER 6
//#define RTC_PIN_NUMBER 8

#define XBEE_PIN_NUMBER 3


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

/*
 * 端末が眠る期間の指定
 */
#define SLEEP_DURATION 10 //単位の倍数
//#define SLEEP_UNIT 0 // 244.14us単位
//#define SLEEP_UNIT 1 //15.625ms単位
#define SLEEP_UNIT 2 //秒単位
//#define SLEEP_UNIT 3 //分単位

// 起動時の時刻設定のコマンド入力待ち時間
#define COMMAND_TIMER_THRESHOLD 15 // 15秒
#define SERIAL_THRESHOLD 10000 // 10秒

// 改行文字の定義
#define LINE_FEED 0x0a
#define CR 0x0d

#define TERMINATE_CHAR  CR //CR
//#define TERMINATE_CHAR LINE_FEED  //LF

// その他一般的な定数の定義
#define BUFF_MAX 256

skRTClib skRTC = skRTClib() ;             // Preinstantiate Objects

int interXbeeFlag=0;
/*
 * RTC(CLKOUT)からの外部割込みで実行される関数
 */
void InterRTC()
{
  skRTC.InterFlag = 1 ;
}

void InterXbee()
{
  interXbeeFlag=1;
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
#ifdef ARDUINO_SLEEP
/*
 * Arduinoを寝かせる関数
 */

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
 
/*
 * 寝ているArduinoを起こすための割り込みを設定する
 */
 
void setupInterapt()
{
  int ans;
  pinMode(RTC_PIN_NUMBER, INPUT);    // RTCからの割込みを読むのに利用
  ans = skRTC.begin(RTC_PIN_NUMBER,digitalPinToInterrupt(RTC_PIN_NUMBER),InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  if (ans == 0) {
    Serial.println("Successful initialization of the RTC") ;// 初期化成功
  } else {
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
    reboot() ;                         // 処理中断
  }
  pinMode(XBEE_PIN_NUMBER, INPUT);
  attachInterrupt(digitalPinToInterrupt(XBEE_PIN_NUMBER),InterXbee,LOW);
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

  Serial.begin(9600) ;                    // シリアル通信の初期化
  setupInterapt();
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
  skRTC.SetTimer(SLEEP_UNIT,SLEEP_DURATION) ;
}
/*
 * 起こされた場合に行う処理
 */
void doWork()
{
  Serial.println("Interupt and wake up!");
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
  byte tm[7] ; 
  char buf[24] ;
  skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
  skRTC.cTime(tm,(byte *)buf) ;             // 日付と時刻を文字列に変換する
  Serial.println(buf) ;                   // シリアルモニターに表示
#ifdef ARDUINO_SLEEP
  goodNight(STANDBY_MODE);// 端末を眠らせる
#else
  delay(5000);
#endif
  if ((skRTC.InterFlag == 1)||(interXbeeFlag==1)) {// 割込みが発生したか？
    skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
    interXbeeFlag = 0;
    doWork();
  }
}


