/*******************************************************************************
*  電源が入ると，センサ処理を行い，処理結果をシリアルで外部に送信する
*  プログラム
*******************************************************************************/

/*******************************************************************************
*  機能のON/OFFなど
*******************************************************************************/
#define DEBUG                // デバッグメッセージをコンソールに出力するか否か
//#define STARTUP_LED          // 起動時にLEDを明滅させるか否か
#define SERIAL_OUT           // 処理結果をシリアルでリレーサーバに出力するか否か
//#define SHUTDOWN             // シリアルサーバに処理終了の信号を出すか否か(シリアル出力しない場合)
#define USE_RTC_8564NB       // RTCで現在時刻を測定するか否かと利用するRTCの種類の指定
//#define USE_SD              // 処理結果をSDに保存するか否か UNOだとメモリ不足で使えない．

/*******************************************************************************
*  ピン番号の指定
*******************************************************************************/
/* 接続相手(リレーサーバ or Xbee)との接続端子の指定 */
#define SHUTDOWN_PIN 9     // 処理終了をピンから信号を出して通知する場合のピン番号
#define SERIAL_COM_RX 6    // 処理結果を通知するためのシリアルのRx端子の指定
#define SERIAL_COM_TX 7    // 処理結果を通知するためのシリアルのTx端子の指定

/* シールドで利用する端子の指定 */
#define SD_CHIP_SELECT 8   // マイクロSDシールドのチップセレクト(SPI)は8番ピン
#define RTC_LED_PIN 3          // 処理開始時にLEDを明滅させる場合のLEDのピン番号
#define SD_LED_PIN 5

/* 使わなくても，定義しておかないとRTCをセットアップできないのでやむを得ず定義しておく */
#define RTC_PIN_NUMBER 18   // RTCから割り込みを受ける場合のピン番号の指定
#define INT_NUMBER digitalPinToInterrupt(RTC_PIN_NUMBER)       // RTCの割り込み番号の指定

/* 温度・湿度センサの接続先ピン(センサ処理の例のため) */
#define DHT_PIN 4            // DHTシリーズの温湿度センサを接続するピン番号
/*******************************************************************************
*  定数の指定
*******************************************************************************/
#define MAX_SLEEP 100000            // 処理終了後に一定時間シリアル回線を止めないと，なにが起こるかわからないため，長時間眠らせるための指定
#define LOG_FILE_NAME "log.txt"     // SDカードに処理結果を保存する場合のログファイルの名前
#define TIME_BUFF_MAX 256           // 時刻情報を文字列に変換するためのバッファサイズ
#define BUFF_MAX 512                // 処理結果に対する文字列処理を行うためのバッファのサイズ

/*******************************************************************************
*  センサ関係処理
*  注意事項 :
*    ・本来この部分はユーザが開発する
*    ・DHTシリーズのセンサを用いるこのプログラムはあくまでサンプル
*    ・今回使うのはDHTシリーズセンサ用ライブラリのうち，https://github.com/markruys/arduino-DHT
*******************************************************************************/
/* 定数やオブジェクトの定義 */
#define AVR
#define DHT_SENSOR  // 温度湿度センサを用いるか否かの指定
#define MAX_RETRY 5 // センサからデータが正常に読めなかった場合に，リトライする回数の指定

#ifdef DHT_SENSOR
#include "DHT.h"
DHT dht;
#endif /* DHT_SENSOR */

/* センサの起動時の初期化処理 */
void setupSensor()
{
#ifdef DHT_SENSOR
  dht.setup(DHT_PIN); // DHTをセットアップ
#endif /* DHT_SENSOR */
}

/* センサ処理 */
void getSensorValue(char * buff){
#ifdef DHT_SENSOR
  float temperatureVal, humidityVal;
#ifdef AVR
  char temperature[10],humidity[10];
#endif
  for (int i=0; i < MAX_RETRY ; i++ ){  // 一回ではデータが読み取れないかもしれないため，数回トライする
    delay(dht.getMinimumSamplingPeriod()); // DHTシリーズセンサのデータ読み取り準備のために一定時間待機する
    temperatureVal = dht.getTemperature();
    humidityVal = dht.getHumidity();
    if ((!isnan(temperatureVal)) && (!isnan(humidityVal))) { // 温度湿度両方ちゃんと読めたらループをぬけ出す
      break;
    }
  }
#ifdef AVR
  dtostrf(humidityVal,-1,1,humidity);
  dtostrf(temperatureVal,-1,1,temperature);
  sprintf(buff,"Humidity = %s , Temperature(C) = %s",humidity,temperature);
#endif /* AVR */
#ifdef GR_KURUMI
  sprintf(buff,"Humidity = %f , Temperature(C) = %f",humidityVal,temperatureVal);
#endif /* GR_KURUMI */
#else
  sprintf(buff,"Hello World!");
#endif /* DHT_SENSOR */
}


/*******************************************************************************
*  ここからプログラム本体
*******************************************************************************/
/*******************************************************************************
*  各種定義
*******************************************************************************/
#ifdef SERIAL_OUT
#include <SoftwareSerial.h>
#endif /* SERIAL_OUT */

//#ifdef STARTUP_LED
//#define INTERVAL 1000
//#define SHORT_INTERVAL 300
//const int ledPin =  LED_PIN;      // the number of the LED pin
//#endif /* STARTUP_LED */

#ifdef SERIAL_OUT
SoftwareSerial serialCom(SERIAL_COM_RX, SERIAL_COM_TX); // RX, TX
#endif /* SERIAL_OUT */

#ifdef USE_RTC_8564NB
#include <Wire.h>
#include <skRTClib.h>
skRTClib skRTC = skRTClib() ;             // objectの作成
#endif /* USE_RTC_8564NB */

#ifdef USE_SD
// SDシールド関係の定義
#include <SPI.h>
#include <SdFat.h>
SdFat sd;      // File system object
SdFile file;  // Log file.
#define error(msg) sd.errorHalt(F(msg))  // エラーをSDに残すためのマクロ
#define SDCARD_SPEED SPI_HALF_SPEED  // 安全のため，低速モードで動作させる場合
//#define SDCARD_SPEED SPI_FULL_SPEED  // 性能重視の場合
#endif /* USE_SD */

/*******************************************************************************
*  初期化処理
*******************************************************************************/
void setup() {
#ifdef STARTUP_LED
  // initialize the LED pin as an output:
  pinMode(RTC_LED_PIN, OUTPUT);
  pinMode(SD_LED_PIN, OUTPUT);
#endif /*  STARTUP_LED */
  setupSensor();
#ifdef SHUTDOWN // シャットダウン信号を使う場合のピン設定，LOWは活動中，HIGHは処理終了を示す．
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
#endif /* SHUTDOWN */
#ifdef DEBUG
  Serial.begin(9600);
#endif /* DEBUG */
#ifdef SERIAL_OUT
  serialCom.begin(9600); // 処理結果を返信するためのシリアルポートの設定
#endif /* SERIAL_OUT */
#ifdef USE_RTC_8564NB
  int ans;
  ans = skRTC.begin(RTC_PIN_NUMBER,INT_NUMBER,NULL,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化するが，すでに時刻を設定済みの場合，元々設定していた値が活かされる．
  if (ans != 0) {
#ifdef DEBUG
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
#endif /* DEBUG */
    //serialCom.println("Failed initialization of the RTC");
    digitalWrite(RTC_LED_PIN,HIGH);
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
  //if (!sd.cardBegin(SD_CHIP_SELECT, SPI_HALF_SPEED)) {
  if (!sd.begin(SD_CHIP_SELECT, SDCARD_SPEED)) {
#ifdef DEBUG
    Serial.println("Failed initialization of SD card") ; // 初期化失敗
#endif /* DEBUG */
    //serialCom.println("Failed initialization of SD card");
    digitalWrite(SD_LED_PIN,HIGH);
    delay(MAX_SLEEP);
    sd.initErrorHalt();
  }
  if (!file.open(LOG_FILE_NAME, FILE_WRITE)) {
#ifdef DEBUG
    Serial.println("file.open error");
#endif /* DEBUG */
    //serialCom.println("file.open error");
    delay(MAX_SLEEP);
  }
  file.flush();
#endif /* USE_SD */
#ifdef DEBUG
  Serial.println("start!");
#endif /* DEBUG */
}
/*******************************************************************************
*  処理終了通知をピンで実施する場合
*******************************************************************************/
#ifdef SHUTDOWN
void shutdown(){
  digitalWrite(SHUTDOWN_PIN, HIGH);
}
#endif /* SHUTDOWN */
/*******************************************************************************
*  実際の処理
*******************************************************************************/
void doWork(){
  char buff[BUFF_MAX];
  char sensorVal[BUFF_MAX];
  getSensorValue(sensorVal);
#ifdef USE_RTC_8564NB
  byte tm[7];
  char tbuff[TIME_BUFF_MAX];
  skRTC.rTime(tm);
  skRTC.cTime(tm,(byte *)tbuff);
  sprintf(buff,"%s : %s",tbuff,sensorVal);
#else /* USE_RTC_8564NB */
  sprintf(buff,"unkown : %s",sensorVal);
#endif /* USE_RTC_8564NB */
#ifdef USE_SD
  file.println(buff);
  file.flush();
#endif /* USE_SD */
#ifdef SERIAL_OUT
  serialCom.println(buff);
  serialCom.flush();
#ifdef DEBUG
  Serial.println(buff);
  Serial.flush();
#endif /* DEBUG */
#endif /* SERIAL_OUT */
}
/*******************************************************************************
*  ループ
*******************************************************************************/
void loop(){
  // read the state of the pushbutton value:
  doWork();
#ifdef SHUTDOWN
  shutdown();
#else /* SHUTDOWN */
  delay(30000);
#endif /* SHUTDOWN */
}
