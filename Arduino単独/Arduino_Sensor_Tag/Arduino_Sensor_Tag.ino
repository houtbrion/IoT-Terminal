/**********************************
 *
 * 周期的に起きて，センサを読み取る
 * 端末プログラムのテンプレート
 *
 **********************************/

 /**********************************
 * どの機能を使うかの選択
 *   注意事項 : 
 *     ・全てONにすると，メモリ不足でUnoの動作がおかしくなる．
 **********************************/
#define DEBUG              // 動作ログをコンソールに出力
#define MIN_LOG            // 動作ログのうち，最小限度のものだけを出力
#define RESET              // 夜中に再起動
#define ARDUINO_SLEEP      // 待機中は低電力状態に設定
#define SERIAL_COM         // センサ処理の結果をシリアルで外部に出力
#define XBEE_SLEEP         // Xbeeを使う場合に，寝ているXbeeを寝起きさせる処理を行う場合
#define USE_SD             // センサ処理の結果をSDカードに保存する

/**********************************
 * ここから，センサ関係処理の定義
 *  注意事項 :
 *    ・本来この部分はユーザが開発する
 *    ・DHTシリーズのセンサを用いるこのプログラムはあくまでサンプル
 *    ・今回使うのはDHTシリーズセンサ用ライブラリのうち，https://github.com/markruys/arduino-DHT
 **********************************/

#define DHT_SENSOR  // 温度湿度センサを用いるか否かの指定


#ifdef DHT_SENSOR
#define DHT_PIN 3
#include "DHT.h"
DHT dht;
#endif /* DHT_SENSOR */

void setupSensor()
{
#ifdef DHT_SENSOR
  dht.setup(DHT_PIN); // DHTをセットアップ
#endif /* DHT_SENSOR */
}

#define MAX_RETRY 5 // センサからデータが正常に読めなかった場合に，リトライする回数の指定

void getSensorValue(char * buff){
#ifdef DHT_SENSOR
  float temperatureVal, humidityVal;
  char temperature[10],humidity[10];
  for (int i=0; i < MAX_RETRY ; i++ ){  // 一回ではデータが読み取れないかもしれないため，数回トライする
    delay(dht.getMinimumSamplingPeriod()); // DHTシリーズセンサのデータ読み取り準備のために一定時間待機する
    temperatureVal = dht.getTemperature();
    humidityVal = dht.getHumidity();
    if ((!isnan(temperatureVal)) && (!isnan(humidityVal))) { // 温度湿度両方ちゃんと読めたらループをぬけ出す
      break;
    }
  }
  dtostrf(humidityVal,-1,1,humidity);
  dtostrf(temperatureVal,-1,1,temperature);
  sprintf(buff,"Humidity = %s , Temperature(C) = %s",humidity,temperature);
#else
  sprintf(buff,"Hello World!");
#endif /* DHT_SENSOR */
}

/**********************************
 * 端末が眠る期間の定義
 **********************************/
#define SLEEP_DURATION 30     //単位の倍数
//#define SLEEP_UNIT 0          // 244.14us単位
//#define SLEEP_UNIT 1          //15.625ms単位
#define SLEEP_UNIT 2          //秒単位
//#define SLEEP_UNIT 3          //分単位

 /**********************************
 * ピン配置の設定
 **********************************/
 
#define SD_CHIP_SELECT 8     // sparcfunのマイクロSDシールドのチップセレクト用のピンは8番ピン

#ifdef SERIAL_COM            // Xbee等をシリアルに接続してセンサの処理結果を出力する場合のシリアル接続の端子番号
#define SERIAL_COM_RX 5
#define SERIAL_COM_TX 6
#endif /* SERIAL_COM */

#ifdef XBEE_SLEEP            // Xbeeを使う場合のXbeeのスリープ端子の指定
#define XBEE_SLEEP_PIN 4
#endif /* XBEE_SLEEP */

/* RTCからの割り込みを受ける端子と割り込み番号の指定                  */
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


/**********************************
 *
 * ここまで
 *
 **********************************/

#define AVR  /* AVR搭載機対応 */

#define XBEE_DELAY 2000

#include <Wire.h>
#include <skRTClib.h>


/* 時計関係 */
#include <Time.h>
unsigned long bootTime;
#define TIME_BUFF_MAX 256

#define MAX_SLEEP 10000

#ifdef SERIAL_COM
#include <SoftwareSerial.h>
#endif

#ifdef ARDUINO_SLEEP
#include <avr/sleep.h>
#endif /* ARDUINO_SLEEP */


#ifdef SERIAL_COM
SoftwareSerial serialCom(SERIAL_COM_RX, SERIAL_COM_TX); // RX, TX
#endif

#ifdef USE_SD
// SDシールド関係の定義
#include <SPI.h>
#include <SdFat.h>

#define FILE_BASE_NAME "Data"  //データファイルのベース名
SdFat sd;      // File system object
SdFile file;  // Log file.
#define error(msg) sd.errorHalt(F(msg))  // エラーをSDに残すためのマクロ
#define SDCARD_SPEED SPI_HALF_SPEED  // 安全のため，低速モードで動作させる場合
//#define SDCARD_SPEED SPI_FULL_SPEED  // 性能重視の場合
#endif /* USE_SD */
#define BUFF_MAX 512


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



// 起動時の時刻設定のコマンド入力待ち時間
#define COMMAND_TIMER_THRESHOLD 15 // 15秒
#define SERIAL_THRESHOLD 10000 // 10秒

// 改行文字の定義
#define LINE_FEED 0x0a
#define CR 0x0d

#define TERMINATE_CHAR  CR //CR
//#define TERMINATE_CHAR LINE_FEED  //LF

#define REBOOT_THRESHOLD 86400 //一日
//#define REBOOT_THRESHOLD 3600 // 1時間
//#define REBOOT_THRESHOLD 600 // 10分
//#define REBOOT_THRESHOLD 120 // 2分
//#define REBOOT_THRESHOLD 60 // 1分
//#define REBOOT_THRESHOLD 30 // 30秒


skRTClib skRTC = skRTClib() ;             // Preinstantiate Objects

/*
 * RTC(CLKOUT)からの外部割込みで実行される関数
 */
void InterRTC()
{
  skRTC.InterFlag = 1 ;
}


/*******************************************************************************
*  reboot                                                                      *
*******************************************************************************/
void reboot() {
#ifdef AVR
  asm volatile ("  jmp 0");
#endif /* AVR */
}

#ifdef ARDUINO_SLEEP
void goodNight(int i) {
#ifdef DEBUG
  Serial.println(F("  Good Night"));
#endif /* DEBUG */
  delay(100);
  noInterrupts();
  set_sleep_mode(i);
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
}
#endif
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
  Serial.println(F("Please input current time."));
yearRetry:
  Serial.print(F("Year (2000 - 2100): "));
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
  Serial.print(F("Month (1 - 12): "));
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
  Serial.print(F("Day (1 - 31): "));
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
  Serial.print(F("week Day (sun(0), mon(1), tue(2) , wen(3), tur(4), fri(5), sat(6): "));
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
  Serial.print(F("Hour (0 - 23): "));
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
  Serial.print(F("Min (0 - 59): "));
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
  Serial.print(F("Sec (0 - 59): "));
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
  Serial.print(F("The time is correct?  [Y/n] : "));
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
  Serial.println(F("start!"));
  //
  char tbuff[TIME_BUFF_MAX] ;
  int ans = skRTC.sTime((byte)(yearVal-2000),(byte)monthVal,(byte)dayVal,(byte)wDayVal,(byte)hourVal,(byte)minVal,(byte)secVal);
  if (ans == 0) {
    Serial.println(F("RTC configuration successfull")) ;// 初期化成功
    skRTC.rTime(tm) ;                         // RTCから現在の日付と時刻を読込む
    skRTC.cTime(tm,(byte *)tbuff) ;             // 日付と時刻を文字列に変換する
    Serial.println(tbuff) ;                   // シリアルモニターに表示
  } else {
    Serial.print(F("RTC configuration failure. ans=")) ; // 初期化失敗
    Serial.println(ans) ;
    reboot() ;                         // 処理中断
  }
  setTime(skRTC.bcd2bin(tm[2]),skRTC.bcd2bin(tm[1]),skRTC.bcd2bin(tm[0]),skRTC.bcd2bin(tm[3]),skRTC.bcd2bin(tm[5]),skRTC.bcd2bin(tm[6]));
  bootTime=now();
}


#ifdef USE_SD
// Log a data record.
void logData(int data) {
  file.write(data);
  if (!file.sync() || file.getWriteError()) {
    error("write error");
  }
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
      Serial.print(F("lastFileNumber = "));
      Serial.println(lastFileNumber);
#endif
      return(100);
    }
    lastFileNumber++;
  }
#ifdef DEBUG
  Serial.print(F("lastFileNumber = "));
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
  Serial.print(F("logRotation(num); num = "));
  Serial.println(num);
#endif
  if (counter==0) {
#ifdef DEBUG
    Serial.println(F("do nothing"));
#endif
    return;
  } else if (counter==100) {
#ifdef DEBUG
    Serial.println(F("remove oldest file"));
#endif
    sd.remove(eraseFileName);
    counter--;
  }
#ifdef DEBUG
  Serial.println(F("log rotation"));
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
    // counter-1のファイルをcounterのファイルにrename
    sd.rename(srcfileName,dstfileName);
    // counterの減算
    counter--;
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
#endif /* USE_SD */
/*******************************************************************************
*  電源起動時とリセットの時だけのみ処理される関数(初期化と設定処理)            *
*******************************************************************************/
void setup()
{
  int ans ;
  byte tm[7] ;
  int configFlag=0;
  unsigned long currentTime;
  pinMode(PIN_NUMBER, INPUT);    // RTCからの割込みを読むのに利用

  Serial.begin(9600) ;                    // シリアル通信の初期化
  setupSensor();


  ans = skRTC.begin(PIN_NUMBER,INT_NUMBER,InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
#ifdef DEBUG
  if (ans == 0) {
    Serial.println(F("Successful initialization of the RTC")) ;// 初期化成功
  } else {
    Serial.print(F("Failed initialization of the RTC ans=")) ; // 初期化失敗
    Serial.println(ans) ;
    reboot() ;                         // 処理中断
  }
#else /* DEBUG */
  if (ans != 0) {
    Serial.print(F("Failed initialization of the RTC"));
    reboot();
  }
#endif /* DEBUG */
#ifdef USE_SD
  if (!sd.begin(SD_CHIP_SELECT, SDCARD_SPEED)) {
    Serial.println(F("Failed initialization of SD card")) ; // 初期化失敗
    delay(MAX_SLEEP);
    sd.initErrorHalt();
  }
  char fileName[13] = FILE_BASE_NAME "00.txt";
  openLogFile();
  
#ifdef DEBUG
  Serial.print(F("Logging to: "));
  Serial.println(fileName);
#endif /* DEBUG */
#endif /* USE_SD */
#ifdef SERIAL_COM
  serialCom.begin(9600);
#endif
  skRTC.rTime(tm) ;                        // RTCから現在の日付と時刻を読込む
#ifdef DEBUG
  char tbuff[TIME_BUFF_MAX] ;
  skRTC.cTime(tm,(byte *)tbuff) ;             // 日付と時刻を文字列に変換する
  Serial.println(tbuff) ;                   // シリアルモニターに表示
#endif /* DEBUG */
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
}
/*******************************************************************************
* Xbee関連の処理 *
*******************************************************************************/
#ifdef XBEE_SLEEP
void wakeupXbee(){
  pinMode(XBEE_SLEEP_PIN, OUTPUT);
  digitalWrite(XBEE_SLEEP_PIN, LOW);
  delay(XBEE_DELAY);
}
void sleepXbee(){
  delay(XBEE_DELAY);
  pinMode(XBEE_SLEEP_PIN, INPUT); // put pin in a high impedence state
  digitalWrite(XBEE_SLEEP_PIN, HIGH);
}
#endif /* XBEE_SLEEP */
/*******************************************************************************
* 本来の仕事                                                                   *
*******************************************************************************/
void doWork(){
  byte tm[7] ; 
  char tbuff[TIME_BUFF_MAX] ;
  char buff[BUFF_MAX];
  char sensorVal[BUFF_MAX];
  skRTC.rTime(tm);
  skRTC.cTime(tm,(byte *)tbuff);
  getSensorValue(sensorVal);
  sprintf(buff,"%s : %s",tbuff,sensorVal);
#if defined(DEBUG) || defined (MIN_LOG)
  Serial.println(buff);
#endif /* DEBUG || MIN_LOG */
#ifdef SERIAL_COM
#ifdef XBEE_SLEEP
  wakeupXbee();
#endif /* XBEE_SLEEP */
  serialCom.println(buff);
#ifdef XBEE_SLEEP
  sleepXbee();
#endif /* XBEE_SLEEP */
#endif /* SERIAL_COM */
#ifdef USE_SD
  file.println(buff);
  file.flush();
#endif /* USE_SD */
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
  byte tm[7] ; 
  char tbuff[TIME_BUFF_MAX] ;
  //char buff[BUFF_MAX];

#ifdef RESET
if ((1==hour())&&(REBOOT_THRESHOLD < (now() - bootTime))){ // 稼働時間が1時間以上で，夜中一時だったらリブート
#ifdef DEBUG
    Serial.println(F("reboot"));
#endif /* DEBUG */
    reboot();
  }
#endif /* RESET */
#ifdef ARDUINO_SLEEP
  skRTC.SetTimer(SLEEP_UNIT,SLEEP_DURATION) ;
  goodNight(STANDBY_MODE);// 端末を眠らせる
#else
  delay(5000);
#endif
  if (skRTC.InterFlag == 1) {// 割込みが発生したか？
    skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
#ifdef DEBUG
    Serial.println(F("Interupt and wake up!"));
#endif /* DEBUG */
    skRTC.StopTimer();
  }
  doWork();
}

