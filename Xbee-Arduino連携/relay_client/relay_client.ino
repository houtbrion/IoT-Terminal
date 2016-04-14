
#define DEBUG

#define STARTUP_LED
#define SERIAL_OUT
//#define SHUTDOWN
#define USE_RTC_8564NB
//#define USE_SD

#define DHT_PIN 4            // DHTシリーズの温湿度センサを接続するピン番号

#define AVR

#define MAX_SLEEP 10000
#define BUFF_MAX 64
#define TIME_BUFF_MAX 32

//char buff[BUFF_MAX];
char sensorVal[BUFF_MAX];
char tbuff[TIME_BUFF_MAX];
/**********************************
 * ここから，センサ関係処理の定義
 *  注意事項 :
 *    ・本来この部分はユーザが開発する
 *    ・DHTシリーズのセンサを用いるこのプログラムはあくまでサンプル
 *    ・今回使うのはDHTシリーズセンサ用ライブラリのうち，https://github.com/markruys/arduino-DHT
 **********************************/

#define DHT_SENSOR  // 温度湿度センサを用いるか否かの指定

#include <math.h>

#ifdef DHT_SENSOR
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

void getSensorValue(char * buff) {
#ifdef DHT_SENSOR
  float temperatureVal, humidityVal;
#ifdef AVR
  //char temperature[10],humidity[10];
#endif
  for (int i = 0; i < MAX_RETRY ; i++ ) { // 一回ではデータが読み取れないかもしれないため，数回トライする
    delay(dht.getMinimumSamplingPeriod()); // DHTシリーズセンサのデータ読み取り準備のために一定時間待機する
    temperatureVal = dht.getTemperature();
    humidityVal = dht.getHumidity();
    if ((!isnan(temperatureVal)) && (!isnan(humidityVal))) { // 温度湿度両方ちゃんと読めたらループをぬけ出す
      break;
    }
  }
#ifdef AVR
  //dtostrf(humidityVal,-1,1,humidity);
  //dtostrf(temperatureVal,-1,1,temperature);
  //sprintf(buff,"H = %s , T(C) = %s",humidity,temperature);
  sprintf(buff, "H = %d , T(C) = %d", round(humidityVal), round(temperatureVal));
#endif /* AVR */
#ifdef GR_KURUMI
  sprintf(buff, "H= %f , T(C) = %f", humidityVal, temperatureVal);
#endif /* GR_KURUMI */
#else
  sprintf(buff, "Hello World!");
#endif /* DHT_SENSOR */
#ifdef DEBUG
  //Serial.println(buff);
#endif /* DEBUG */
}


/**********************************
 *
 * ここまで
 *
 **********************************/




#ifdef SHUTDOWN
#define SHUTDOWN_PIN 10
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
const int ledPin =  13;      // the number of the LED pin
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
#endif /* USE_SD */

// variables will change:
void setup() {
  setupSensor();

#ifdef SHUTDOWN
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
#endif /* SHUTDOWN */
#ifdef DEBUG
  Serial.begin(9600);
#endif /* DEBUG */
#ifdef SERIAL_OUT
  serialCom.begin(9600);
#endif /* SERIAL_OUT */
#ifdef DEBUG
  Serial.println("start!");
#ifdef SERIAL_OUT
  serialCom.println("start!");
#endif /* SERIAL_OUT */
#endif /* DEBUG */
#ifdef USE_RTC_8564NB
  int ans;
  ans = skRTC.begin(PIN_NUMBER, INT_NUMBER, NULL, 12, 1, 10, 2, 15, 30, 0) ; // 2012/01/10 火 15:30:00 でRTCを初期化するが，すでに時刻を設定済みの場合，元々設定していた値が活かされる．
  if (ans != 0) {
#ifdef DEBUG
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
#ifdef SERIAL_OUT
    serialCom.println("Failed initialization of the RTC");
#endif /* SERIAL_OUT */
#endif /* DEBUG */
    delay(MAX_SLEEP);
#ifndef DEBUG
  }
#else /* DEBUG */
  } else {
    Serial.println(F("Successful initialization of the RTC")) ;// 初期化成功
  }
#endif /* DEBUG */
#endif /* USE_RTC_8564NB */
#ifdef USE_SD
  if (!sd.begin(SD_CHIP_SELECT, SDCARD_SPEED)) {
#ifdef DEBUG
    Serial.println("Failed initialization of SD card") ; // 初期化失敗

#ifdef SERIAL_OUT
    serialCom.println("Failed initialization of SD card");
#endif /* SERIAL_OUT */
#endif /* DEBUG */
    delay(MAX_SLEEP);
    sd.initErrorHalt();
  }
  if (!file.open(LOG_FILE_NAME, FILE_WRITE)) {
#ifdef DEBUG
    Serial.println("file.open error");
#ifdef SERIAL_OUT
    serialCom.println("file.open error");
#endif /* SERIAL_OUT */
#endif /* DEBUG */
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
}

#ifdef SHUTDOWN
void shutdown() {
  digitalWrite(SHUTDOWN_PIN, HIGH);
}
#endif /* SHUTDOWN */

void doWork() {
  getSensorValue(sensorVal);
#ifdef USE_RTC_8564NB
  byte tm[7];
  char *buff;
  skRTC.rTime(tm);
  skRTC.cTime(tm, (byte *)tbuff);
  buff = tbuff + strlen(tbuff);
  //sprintf(buff,"%s : %s",tbuff,sensorVal);
  sprintf(buff, " : %s", sensorVal);
  //sprintf(buff,"%s : Hello World!",tbuff);
#else /* USE_RTC_8564NB */
  sprintf(buff, "%s", sensorVal);
  //sprintf(buff,"Hello World!");
#endif /* USE_RTC_8564NB */
  buff=tbuff;
#ifdef USE_SD
  file.println(buff);
  file.flush();
#endif /* USE_SD */
#ifdef SERIAL_OUT
  for (int i = 0; i < 5; i++) {
    serialCom.println(buff);
    Serial.println(buff);
#ifdef DEBUG
    Serial.println(buff);
#endif /* DEBUG */
    delay(1000);
  }
  delay(MAX_SLEEP);
#else /* SERIAL_OUT */
  Serial.println(buff);
  //delay(30000);
  delay(MAX_SLEEP);
#endif /* SERIAL_OUT */
}

void loop() {
  // read the state of the pushbutton value:
  doWork();
#ifdef SHUTDOWN
  shutdown();
#endif /* SHUTDOWN */
}
