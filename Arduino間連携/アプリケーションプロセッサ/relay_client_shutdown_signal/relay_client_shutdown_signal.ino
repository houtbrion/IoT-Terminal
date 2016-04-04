
#define DEBUG

#define STARTUP_LED
#define SERIAL_OUT
//#define SHUTDOWN
//#define USE_RTC_8564NB
#define USE_SD

#define MAX_SLEEP 100000

#ifdef SHUTDOWN
#define SHUTDOWN_PIN 9
#endif /* SHUTDOWN */

#ifdef SERIAL_OUT
#include <SoftwareSerial.h>
#define SERIAL_COM_RX 6
#define SERIAL_COM_TX 7
#endif /* SERIAL_OUT */

#ifdef STARTUP_LED
#define INTERVAL 1000
#define SHORT_INTERVAL 300
//const int ledPin =  6;      // the number of the LED pin
const int ledPin =  3;      // the number of the LED pin
#endif /* STARTUP_LED */


#ifdef SERIAL_OUT
SoftwareSerial serialCom(SERIAL_COM_RX, SERIAL_COM_TX); // RX, TX
#endif /* SERIAL_OUT */

#ifdef USE_RTC_8564NB
#include <Wire.h>
#include <skRTClib.h>
skRTClib skRTC = skRTClib() ;             // objectの作成
#define INT_NUMBER 0  // 使わなくても，定義しておかないとRTCをセットアップできないのでやむを得ず
#define PIN_NUMBER 2
#define TIME_BUFF_MAX 256
#endif /* USE_RTC_8564NB */

#ifdef USE_SD
// SDシールド関係の定義
#include <SPI.h>
#include <SdFat.h>
#define SD_CHIP_SELECT 8 // sparcfunのマイクロSDシールドは8番ピン
#define LOG_FILE_NAME "log.txt"
SdFat sd;      // File system object
SdFile file;  // Log file.
#define error(msg) sd.errorHalt(F(msg))  // エラーをSDに残すためのマクロ
#define SDCARD_SPEED SPI_HALF_SPEED  // 安全のため，低速モードで動作させる場合
//#define SDCARD_SPEED SPI_FULL_SPEED  // 性能重視の場合
#define BUFF_MAX 512
#endif /* USE_SD */

// variables will change:
void setup() {
#ifdef SHUTDOWN
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
#endif /* SHUTDOWN */
#ifdef DEBUG
  Serial.begin(9600);
#endif /* DEBUG */
  serialCom.begin(9600);
#ifdef USE_RTC_8564NB
  int ans;
  ans = skRTC.begin(PIN_NUMBER,INT_NUMBER,NULL,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化するが，すでに時刻を設定済みの場合，元々設定していた値が活かされる．
  if (ans != 0) {
#ifdef DEBUG
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
#endif /* DEBUG */
    serialCom.println("Failed initialization of the RTC");
    delay(MAX_SLEEP);
#ifndef DEBUG
  }
#else /* DEBUG */
  } else {
     Serial.println("Successful initialization of the RTC") ;// 初期化成功
  }
#endif /* DEBUG */
#endif /* USE_RTC_8564NB */
#ifdef USE_SD
  if (!sd.begin(SD_CHIP_SELECT, SDCARD_SPEED)) {
#ifdef DEBUG
    Serial.println("Failed initialization of SD card") ; // 初期化失敗
#endif /* DEBUG */
    serialCom.println("Failed initialization of SD card");
    delay(MAX_SLEEP);
    sd.initErrorHalt();
  }
  if (!file.open(LOG_FILE_NAME, FILE_WRITE)) {
 #ifdef DEBUG
    Serial.println("file.open error");
#endif /* DEBUG */
    serialCom.println("file.open error");
    delay(MAX_SLEEP);
  }
  file.flush();
#endif /* USE_SD */
#ifdef STARTUP_LED
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(INTERVAL);
  digitalWrite(ledPin, LOW);
#endif /*  STARTUP_LED */
#ifdef DEBUG
  Serial.println("start!");
#endif /* DEBUG */
}

#ifdef SHUTDOWN
void shutdown(){
  digitalWrite(SHUTDOWN_PIN, HIGH);
}
#endif /* SHUTDOWN */

void doWork(){
  char buff[BUFF_MAX];
#ifdef USE_RTC_8564NB
  byte tm[7];
  char tbuff[TIME_BUFF_MAX];
  skRTC.rTime(tm);
  skRTC.cTime(tm,(byte *)tbuff);
  sprintf(buff,"%s HelloWorld!",tbuff);
#else /* USE_RTC_8564NB */
  sprintf(buff,"HelloWorld!");
#endif /* USE_RTC_8564NB */
#ifdef USE_SD
  file.println(buff);
  file.flush();
#endif /* USE_SD */
#ifdef SERIAL_OUT
  for (int i=0;i<5;i++){
    serialCom.println(buff);
#ifdef DEBUG
    Serial.println(buff);
#endif /* DEBUG */
    delay(1000);
  }
#else /* SERIAL_OUT */
  delay(30000);
#endif /* SERIAL_OUT */
}

void loop(){
  // read the state of the pushbutton value:
  doWork();
#ifdef SHUTDOWN
  shutdown();
#endif /* SHUTDOWN */
}
