

/*******************************************************************************
*  RTCの割り込みで起きて，Xbeeを起こして，リレーをONにした後，
*  外付けのマイコンから計算結果を受け取り，Xbeeで送信するサーバプログラム
*
*  注意事項
*  ・一定周期で起きるだけなので，RTCの時刻合わせ等は行わない．(単なる周期タイマ)
*  ・Xbeeの設定は「デバイスATモード」かつ，スリープモード)は「1」にすること
*******************************************************************************/

/*******************************************************************************
*  機能のON/OFFなど
*******************************************************************************/
#define DEBUG         // 各種メッセージのON/OFF
#define RESET         // リレーをONした場合に，アプリケーションプロセッサにリセットをかけるか否か
#define ARDUINO_SLEEP // 自分はスリープするか否か
#define SERIAL_COM    // アプリケーションプロセッサから処理結果を受け取るか否か
#define SERIAL_UPLOAD // アプリプロセッサの処理結果をXbeeに送信させるか否か
#define XBEE_SLEEP    // Xbeeを寝起きさせるか否か
//#define SHUTDOWN     // アプリプロセッサから終了通知を特定のピンで受信するか否か
#define AVR          // 機種はAVRマイコン (UNO,MEGA等)

/*******************************************************************************
*  ピン番号の指定
*******************************************************************************/
/* アプリプロセッサのシリアル接続端子 */
#define SERIAL_COM_RX 10
#define SERIAL_COM_TX 11
/* アプリプロセッサからの終了信号ピン */
#define SHUTDOWN_SIGNAL 9
/* アプリプロセッサのリセットピン */
#define RESET_SIGNAL 5

/* Xbeeとのシリアル接続端子とXbeeのSLEEP_RQピン*/
#define SERIAL_UPLOAD_RX 12
#define SERIAL_UPLOAD_TX 13
#define XBEE_SLEEP_PIN 4

/* リレーの制御ピン */
#define RELAY_SIGNAL 6
// ASSOC 7
//RSSI 8

/* RTCの割り込みに利用するピンと割り込み番号の指定
  Arduinoの割り込み番号と対応するピン番号
---------+-----------------------------+
         |        割り込み番号         |
 機種名  | 00 | 01 | 02 | 03 | 04 | 05 |
---------+----+----+----+----+----+----+
UNO      |  2 |  3 |    |    |    |    |
---------+----+----+----+----+----+----+
Mega     | 21 | 20 | 19 | 18 |  2 |  3 |
---------+----+----+----+----+----+----+
Leonardo |  3 |  2 |  0 |  1 |    |    |
---------+----+----+----+----+----+----+

参考情報 : M0/M0 Proは以下のURLを参照
・http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/Zero/gaiyo.htm

Pro miniは，UNOと同じ
*/

/* PRO_MINIはUNOと同じ */
#define INT_NUMBER 0
#define INT_PIN_NUMBER 2 // 割り込み番号

/*******************************************************************************
*  定数の指定
*******************************************************************************/
/* アプリプロセッサからの処理結果待ちをする時間 */
#define SERIAL_THRESHOLD 10000 // 10秒

/*　端末(AVR)が眠る場合の眠りの深さの指定 */
//#define STANDBY_MODE SLEEP_MODE_IDLE
//#define STANDBY_MODE SLEEP_MODE_ADC
//#define STANDBY_MODE SLEEP_MODE_PWR_SAVE
//#define STANDBY_MODE SLEEP_MODE_STANDBY
#define STANDBY_MODE SLEEP_MODE_PWR_DOWN

/* 端末が眠る期間の指定 */
#define SLEEP_DURATION 45 //単位の倍数
//#define SLEEP_UNIT 0 // 244.14us単位
//#define SLEEP_UNIT 1 //15.625ms単位
#define SLEEP_UNIT 2 //秒単位
//#define SLEEP_UNIT 3 //分単位

/* 起動時の時刻設定のコマンド入力待ち時間 */
//#define COMMAND_TIMER_THRESHOLD 15 // 15秒
//#define SERIAL_THRESHOLD 10000 // 10秒

/* アプリプロセッサの処理に必要な最長の時間 */
#define WORK_TIMER_THRESHOLD 15 // 30秒

/* 改行文字の定義と処理結果の文字列の最終文字の指定 */
#define LINE_FEED 0x0a            // ラインフィードのアスキーコード
#define CR 0x0d                   // キャリッジリターンのアスキーコード
#define TERMINATE_CHAR  CR        // 最終文字はキャリッジリターン
//#define TERMINATE_CHAR LINE_FEED  //最終文字はラインフィード

/*******************************************************************************
*  ここからプログラム本体
*******************************************************************************/
#include <Wire.h>
#include <skRTClib.h>

/* 時計関係 */
#include <Time.h>
unsigned long bootTime;

#if defined(SERIAL_COM) || defined(SERIAL_UPLOAD)
#include <SoftwareSerial.h>
#endif

#ifdef ARDUINO_SLEEP
#include <avr/sleep.h>
#endif /* ARDUINO_SLEEP */

#ifdef SERIAL_COM
//#define MAX_BUFFER_LENGTH 256
SoftwareSerial serialCom(SERIAL_COM_RX, SERIAL_COM_TX); // RX, TX
#ifdef SERIAL_UPLOAD
SoftwareSerial serialUpload(SERIAL_UPLOAD_RX, SERIAL_UPLOAD_TX); // RX, TX
#endif
#endif

bool relayFlag=false;

skRTClib skRTC = skRTClib() ;             // Preinstantiate Objects

 /*******************************************************************************
*  RTC(CLKOUT)からの外部割込みで実行される関数
*******************************************************************************/
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
  //byte tm[7] ;
  int configFlag=0;
  //unsigned long currentTime;
  pinMode(INT_PIN_NUMBER, INPUT);    // RTCからの割込みを読むのに利用
#ifdef SHUTDOWN
  pinMode(SHUTDOWN_SIGNAL,INPUT);
#endif /* SHUTDOWN */
#ifdef RESET
  pinMode(RESET_SIGNAL,OUTPUT);
#endif /* RESET */
#ifdef DEBUG
  Serial.begin(9600) ;                    // シリアル通信の初期化
#endif /* DEBUG */
  //ans = skRTC.begin(0,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  ans = skRTC.begin(INT_PIN_NUMBER,INT_NUMBER,InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  if (ans != 0) {
#ifdef DEBUG
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
#endif /* DEBUG */
    reboot() ;                         // 処理中断
  }
#ifdef DEBUG
    else {
    Serial.println("Successful initialization of the RTC") ;// 初期化成功
  }
#endif /* DEBUG */
#ifdef SERIAL_COM
  serialCom.begin(9600);
#ifdef SERIAL_UPLOAD
  serialUpload.begin(9600);
#endif /* SERIAL_UPLOAD */
#endif /* SERIAL_COM */
  pinMode(RELAY_SIGNAL,OUTPUT);
  digitalWrite(RELAY_SIGNAL,LOW);
#ifdef RESET
  digitalWrite(RESET_SIGNAL,HIGH);
#endif /* RESET */
  skRTC.SetTimer(SLEEP_UNIT,SLEEP_DURATION) ;
}
/*******************************************************************************
* Xbee関連の処理 *
*******************************************************************************/
#ifdef XBEE_SLEEP
void wakeupXbee(){
  pinMode(XBEE_SLEEP_PIN, OUTPUT);
  digitalWrite(XBEE_SLEEP_PIN, LOW);
}
void sleepXbee(){
  pinMode(XBEE_SLEEP_PIN, INPUT); // put pin in a high impedence state
  digitalWrite(XBEE_SLEEP_PIN, HIGH);
}
#endif /* XBEE_SLEEP */
/*******************************************************************************
* 本来の仕事                                                                   *
*******************************************************************************/
bool doWork(){
  unsigned long currentTime;
  unsigned long startTime;
  int configFlag=0;
  startTime=now();
  serialCom.setTimeout(SERIAL_THRESHOLD);
  while(configFlag==0) {
    currentTime=now();
    if ((currentTime - startTime) > WORK_TIMER_THRESHOLD) {
#ifdef XBEE_SLEEP
      sleepXbee();
#endif /* XBEE_SLEEP */
      return false;
    }
    if (serialCom.available()) {
      String processResult = serialCom.readStringUntil(TERMINATE_CHAR);
#ifdef DEBUG
      Serial.println(processResult);
      Serial.flush();
#endif /* DEBUG */
#ifdef SERIAL_UPLOAD
      serialUpload.println(processResult);
      serialUpload.flush();
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
  int relayCounter=0;
  goodNight(STANDBY_MODE);// 端末を眠らせる
  if (skRTC.InterFlag == 1) {// 割込みが発生したか？
    skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
#ifdef DEBUG
    Serial.println("Interupt and wake up!");
#endif /* DEBUG */
#ifdef DEBUG
    Serial.println("Relay ON");
#endif /* DEBUG */
    digitalWrite(RELAY_SIGNAL,HIGH);
#ifdef RESET
    delay(100);
    digitalWrite(RESET_SIGNAL,LOW);
    delay(100);
    digitalWrite(RESET_SIGNAL,HIGH);
#endif /* RESET */
    relayFlag = true;
  }
#ifdef XBEE_SLEEP
  wakeupXbee();
#endif /* XBEE_SLEEP */
#ifdef SERIAL_COM
  if (!doWork()) {
#ifdef DEBUG
    Serial.println("client work result timeout!");
#endif /* DEBUG */
    digitalWrite(RELAY_SIGNAL,LOW);
#ifdef XBEE_SLEEP
    sleepXbee();
#endif /* XBEE_SLEEP */
    return;
  }else{
    digitalWrite(RELAY_SIGNAL,LOW);
#ifdef XBEE_SLEEP
    sleepXbee();
#endif /* XBEE_SLEEP */
    return;
  }
#else /* SERIAL_COM */
  int loopCounter=0;
  delay(100);
CheckRelay:
  loopCounter++;
  if (loopCounter > 100000){
#ifdef DEBUG
    Serial.println("shutdown signal timeout!");
#endif /* DEBUG */
    digitalWrite(RELAY_SIGNAL,LOW);
    return;
  }
  if (relayFlag) {
    if (digitalRead(SHUTDOWN_SIGNAL)==HIGH){
      if (relayCounter< 100){
#ifdef DEBUG
        Serial.println("counting to shutdown");
#endif /* DEBUG */
        relayCounter++;
        goto CheckRelay;
      } else {
        relayCounter=0;
#ifdef DEBUG
        Serial.println(" shutdown");
#endif /* DEBUG */
        digitalWrite(RELAY_SIGNAL,LOW);
        relayFlag=false;
        //return;
      }
    } else {
#ifdef DEBUG
      Serial.println(" shutdown count reset");
#endif /* DEBUG */
      relayCounter=0;
      goto CheckRelay;
    }
  }
#ifdef XBEE_SLEEP
  sleepXbee();
#endif /* XBEE_SLEEP */
#endif /* SERIAL_COM */
}


void goodNight(int i) {
  skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
  digitalWrite(RELAY_SIGNAL,LOW);
#ifdef DEBUG
  Serial.println(F("  Good Night"));
#endif /* DEBUG */
#ifdef ARDUINO_SLEEP
  delay(100);
  noInterrupts();
  set_sleep_mode(i);
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
#else /* ARDUINO_SLEEP */
  delay(30000);
#endif /* ARDUINO_SLEEP */
}

