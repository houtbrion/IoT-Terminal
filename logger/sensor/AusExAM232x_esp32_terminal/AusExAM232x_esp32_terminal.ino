
//#define USE_DHCP                 // IPアドレスをDHCPで行う (コメントアウトした場合は固定IP)
//#define USE_NTP                  // NTPで時刻を合わせる場合
//#define USE_RTC                  // RTCを使うか否か
//#define USE_SOFT_SERIAL          // センサデータ出力先にソフトウェアシリアルを使う
//#define USE_SD                   // SDカードにセンサのログを書き込む場合
#define USE_HARD_SERIAL          // センサデータ出力先にハードウェアシリアルの1番(Serial)を使う

#if defined(USE_NTP) && defined(USE_RTC)
#error "do not define USE_NTP and USE_RTC togather."
#endif /* USE_NTP && USE_RTC */

#include "AusExOutputPlugin.h"
#include "AusExAM232X.h"

#define SSID_STR "foo"
#define WIFI_PASS "bar"

#ifdef USE_SOFT_SERIAL
#include <SoftwareSerial.h>
#endif /* USE_SOFT_SERIAL */

#ifdef USE_RTC
#include "RTC_8564NB_U.h"
#else /* USE_RTC */
#include "RTC_U.h"
#endif /* USE_RTC */

#define LOG_FILE_NAME "log.txt"  // SDカードに書き込むログファイル名
#define SD_CHIP_SELECT 4         // SDのチップセレクトピン番号
#define CLEAR_FILE               // ブート時に古いログファイルを消す設定

#define SOFT_SERIAL_TX 6         // ソフトシリアルの送信ピン番号
#define SOFT_SERIAL_RX 7         // ソフトシリアルの受信ピン番号

#define WAIT_TIME 5000           // 測定の待ち時間　(3秒に一回測定)


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

#define HOSTNAME "esp32"   // syslog形式のログ出力に利用
#define APP_NAME "foo"           // syslog形式のログ出力に利用

// センサ測定結果の出力先選択
#ifdef USE_SD
#define AUSEX_OUTPUT_CHANNEL AUSEX_OUTPUT_CHANNEL_FILE
#endif /* USE_SD */
#ifdef USE_HARD_SERIAL
#define AUSEX_OUTPUT_CHANNEL AUSEX_OUTPUT_CHANNEL_SERIAL
#endif /* USE_HARD_SERIAL */
#ifdef USE_SOFT_SERIAL
#define AUSEX_OUTPUT_CHANNEL AUSEX_OUTPUT_CHANNEL_SOFT_SERIAL
#endif /* USE_SOFT_SERIAL */


#ifdef USE_NTP
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

/*
   ネットワーク関係の設定
   注意 : DHCPでのアドレス取得失敗時の対策や，長時間経過後のアドレス再割当て等は対応していない
*/

#ifdef USE_DHCP
boolean useDhcp = true;   // DHCPでIPアドレスを設定
#else /* USE_DHCP */
boolean useDhcp = false;  // 固定IPアドレス
#endif /* USE_DHCP */

/* 以下は固定IP運用の場合の変数なので適宜変更して使用すること */
IPAddress ip(192, 168, 1, 239);
IPAddress dnsServer(192, 168, 1, 1);
IPAddress gatewayAddress(192, 168, 1, 1);
IPAddress netMask(255, 255, 255, 0);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 9 * 60 * 60); // JST
#endif /* USE_NTP */

#if defined(USE_NTP) || defined(USE_RTC)
unsigned long currentTime;
#endif /* USE_NTP or USE_RTC */

#ifdef USE_SD
File logFile;
#endif /* USE_SD */

#ifdef USE_RTC
RTC_8564NB_U rtc = RTC_8564NB_U(&Wire);
#endif /* USE_RTC */

#ifdef USE_SOFT_SERIAL
SoftwareSerial sSerial(SOFT_SERIAL_RX, SOFT_SERIAL_TX); // RX, TX
#endif /* USE_SOFT_SERIAL */

AuxExSensorIO outputDevice =  AuxExSensorIO();
AusExAM232X am232x = AusExAM232X(&Wire, AM2321);

OutputChannel channel;

//
// 本体をリセットする関数
//
void reboot() {
  ESP.restart();
}

#ifdef USE_SD
bool seekEOF(File * file){
  return file->seek(file->size());
}

bool dumpFile(File * file) {
  if (file->seek(0)){
    while (file->available()) {
      Serial.write(file->read());
      if (file->position() == file->size()) {
        return true;
      }
    }
  } else {
    return false;
  }
}
#endif /* USE_SD */

void setup() {
  Serial.begin(115200) ;    // シリアル通信の初期化
  while (!Serial) {       // シリアルポートが開くのを待つ
    ;
  }
  Serial.println("setup start.");
#ifdef USE_SD
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("initialization failed!");
    reboot();
  }
#ifdef CLEAR_FILE
  if (SD.exists(LOG_FILE_NAME)) {
    if (!SD.remove(LOG_FILE_NAME)) {
      Serial.println("Can not remove log file.");
      reboot();
    }
  }
#endif /* CLEAR_FILE */
  if (!(logFile = SD.open(LOG_FILE_NAME, FILE_READ | FILE_WRITE ))) {
    Serial.println("opening file failed!");
    reboot();
  }
  while (logFile.available()) {
    if (!seekEOF(&logFile)) {
      Serial.println("fail to seek of file.");
      reboot();
    } else {
      break;
    }
    Serial.println("wait to access the log file.");
  }
#endif /* USE_SD */

#ifdef USE_SD
  channel.file= &logFile;
#endif /* USE_SD */
#ifdef USE_HARD_SERIAL
  channel.serial= &Serial;
#endif /* USE_HARD_SERIAL*/
#ifdef USE_SOFT_SERIAL
  channel.sserial= &sSerial;
#endif /* USE_SOFT_SERIAL*/

  //outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_SYSLOG);
  //outputDevice.SetLogParam(HOSTNAME, APP_NAME);
  outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_PLAIN_TEXT);
  //outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_JSON);

#ifdef USE_RTC
  if (rtc.begin()) {
    Serial.println("Successful initialization of the RTC"); // 初期化成功
  } else {
    Serial.print("Failed initialization of the RTC");  // 初期化失敗
    reboot();
  }
#endif /* USE_RTC */

  date_t dateTime;

#ifdef USE_NTP
  // MACアドレスとIPアドレスの設定
  // 参考URL http://arduino.cc/en/Reference/EthernetBegin
  if (useDhcp) {
    WiFi.begin(SSID_STR, WIFI_PASS);
    while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 );
      Serial.print ( "." );
    }
  } else {
    WiFi.config(ip, dnsServer, gatewayAddress, netMask);
    WiFi.begin(SSID_STR, WIFI_PASS);
    while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 );
      Serial.print ( "." );
    }
  }
  Serial.println(WiFi.localIP());
  Serial.println(F("network setup done"));
  timeClient.begin();
#endif /* USE_NTP */

#ifdef USE_RTC
  outputDevice.SetRtc(&rtc);
#endif /* USE_RTC */

  // Initialize device.
  am232x.begin();
  Serial.println(F("am232xxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  am232x.temperature().getSensor(&sensor);
  outputDevice.InfoOutput(sensor);
  // Print humidity sensor details.
  am232x.humidity().getSensor(&sensor);
  outputDevice.InfoOutput(sensor);

  outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_SYSLOG);
  outputDevice.SetLogParam(HOSTNAME, APP_NAME);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  delay(WAIT_TIME);
}

void loop() {
  sensors_event_t event;
  Serial.println("******************");
#ifdef USE_NTP
  timeClient.update();
  currentTime = timeClient.getEpochTime();
#endif /* USE_NTP */
#ifdef USE_RTC
  date_t dateTime;
  rtc.getTime(&dateTime);
  currentTime = rtc.convertDateToEpoch(dateTime);
#endif /* USE_RTC */
  am232x.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
#if defined(USE_NTP) || defined(USE_RTC)
    event.timestamp=currentTime;
#endif /* USE_NTP or USE_RTC */
    outputDevice.EventOutput(event);
  }
  // Get humidity event and print its value.
  am232x.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
#if defined(USE_NTP) || defined(USE_RTC)
    event.timestamp=currentTime;
#endif /* USE_NTP or USE_RTC */
    outputDevice.EventOutput(event);
  }
  Serial.println("Going to sleep now");

#ifdef USE_SD
  logFile.flush();
#endif /* USE_SD */

#ifdef USE_SOFT_SERIAL
  sSerial.flush();
#endif /* USE_SOFT_SERIAL */

  Serial.flush();
  esp_deep_sleep_start();
}
