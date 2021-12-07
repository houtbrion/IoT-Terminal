
//#define USE_DHCP                 // IPアドレスをDHCPで行う (コメントアウトした場合は固定IP)
#define USE_NTP                  // NTPで時刻を合わせる場合

#include "AusExDHT.h"

#define SENSOR_PIN 2             // センサはデジタルの2番ピンに接続
// センサの種別の選択
//#define DHTTYPE    DHT11       // DHT 11
#define DHTTYPE    DHT22       // DHT 22 (AM2302)
//#define DHTTYPE    DHT21       // DHT 21 (AM2301)


#define WAIT_TIME 1000           // 測定の待ち時間

#include <Ethernet.h>

#ifdef USE_NTP
#include <EthernetUdp.h>
#include <NTPClient.h>
#endif /* USE_NTP */

/*
   ネットワーク関係の設定
   注意 : DHCPでのアドレス取得失敗時の対策や，長時間経過後のアドレス再割当て等は対応していない
*/
//byte mac[] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }; //アドレスは手持ちのarduinoやシールドのモノに変更すること
byte mac[] = { 0x90, 0xa2, 0xda, 0x10, 0x11, 0x51 }; //アドレスは手持ちのarduinoのものに変更すること


#ifdef USE_DHCP
boolean useDhcp = true;   // DHCPでIPアドレスを設定
#else /* USE_DHCP */
boolean useDhcp = false;  // 固定IPアドレス
#endif /* USE_DHCP */

/* 以下は固定IP運用の場合の変数なので適宜変更して使用すること */
IPAddress ip(192, 168, 1, 222);
IPAddress dnsServer(192, 168, 1, 1);
IPAddress gatewayAddress(192, 168, 1, 1);
IPAddress netMask(255, 255, 255, 0);

#ifdef USE_NTP
String currentTime;
#endif /* USE_NTP */

#ifdef USE_NTP
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, 9 * 60 * 60); // JST

#endif /* USE_NTP */

EthernetServer server(80); // サーバオブジェクトの定義． 引数はポート番号
AusExDHT dht = AusExDHT(SENSOR_PIN,DHTTYPE);


//
// 本体をリセットする関数
//
void reboot() {
  asm volatile ("  jmp 0");
}

void setup() {
  Serial.begin(9600) ;    // シリアル通信の初期化
  while (!Serial) {       // シリアルポートが開くのを待つ
    ;
  }
  Serial.println("setup start.");

  // MACアドレスとIPアドレスの設定
  // 参考URL http://arduino.cc/en/Reference/EthernetBegin
  if (useDhcp) {
    if (Ethernet.begin(mac) == 0) {
      Serial.println(F("DHCP fail."));
      reboot();
    }
  } else {
    Ethernet.begin(mac, ip, dnsServer, gatewayAddress, netMask);
    Serial.println(F("ethernet setup done"));
  }
  Serial.print(F("IP address : ")); Serial.println(Ethernet.localIP());
  Serial.println(F("network setup done"));
#ifdef USE_NTP
  timeClient.begin();
#endif /* USE_NTP */

  server.begin();
  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
}

float HeatIndex(float T, float R){
  float c1 = -8.78469475556;
  float c2 = 1.61139411;
  float c3 = 2.33854883889;
  float c4 = -0.14611605;
  float c5 = -0.012308094;
  float c6 = -0.0164248277778;
  float c7 = 0.002211732;
  float c8 = 0.00072546;
  float c9 = -0.000003582;
  return c1+c2*T+c3*R+c4*T*R+c5*T*T+c6*R*R+c7*T*T*R+c8*T*R*R+c9*T*T*R*R;
}

void loop() {
  // 接続してきたクライアントを示すオブジェクトを生成
  EthernetClient client = server.available();
  if (client) {
    delay(WAIT_TIME); // DHTは測定の周波数が非常に長いため，連続してアクセスできないように，待ち時間をはさむ
    Serial.println(F("Connection established by a client"));
    // an http request ends with a blank line
    uint8_t endToken = 0;
    float hum,temp;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); //一文字読み取り
        if (c == '\n' && endToken == 3) {
#ifdef USE_NTP
          timeClient.update();
          currentTime = timeClient.getFormattedTime();
#endif /* USE_NTP */
          Serial.println(F("generate output for client."));
          // HTTPのレスポンスのヘッダ生成
          client.println("HTTP/1.1 200 OK");
          //返答する文字コードの定義
          //client.println(F("Content-Type: text/html; charset=iso-2022-jp")); // JIS
          //client.println(F("Content-Type: text/html; charset=shift_jis")); // SJIS
          //client.println(F("Content-Type: text/html; charset=euc-jp")); // EUC
          client.println(F("Content-Type: text/html; charset=UTF-8")); // UTF-8
          //client.println(F("Content-Type: text/html"));  // 定義なし
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println(F("Refresh: 10"));  // refresh the page automatically every 10 sec
          client.println();
          Serial.println("generate header . done.");
          // Get temperature event and print its value.
          sensors_event_t event;
          Serial.println("read temperature");
          dht.temperature().getEvent(&event);
          if (isnan(event.temperature)) {
            Serial.println(F("Error reading temperature!"));
            temp=-1000;
          } else {
            temp=event.temperature;
          }
          Serial.println("read humidity");
          // Get humidity event and print its value.
          dht.humidity().getEvent(&event);
          if (isnan(event.relative_humidity)) {
            Serial.println(F("Error reading humidity!"));
            hum=-1000;
          } else {
            hum=event.relative_humidity;
          }
          client.println(F("<!DOCTYPE html>"));
          client.println(F("<html lang=\"ja\">"));
          client.println(F("<head>"));
          client.println(F("<meta charset=\"UTF-8\">"));
          client.println(F("<title>室内の温度・湿度環境</title>"));
          client.println(F("</head>"));
          client.println(F("<body>"));
          client.println(F("<h1>室内の温度・湿度環境</h1>"));
          client.println(F("<br />"));
          client.println(F("<br />"));
          client.println(F("<style>table {border-collapse: collapse;} td{border: solid 1px;padding: 0.5em;}</style>"));
          client.println(F("<table>"));
          client.println(F("<caption>arduinoの出力</caption>"));
          client.println(F("<tr>"));
          client.println(F("<td colspan=\"2\">DHT11測定結果</td>"));
          client.println(F("<td rowspan=\"2\">体感温度(摂氏)</td>"));
          client.println(F("</tr>"));
          client.println(F("<tr>"));
          client.println(F("<td>温度(摂氏)</td>"));
          client.println(F("<td>湿度(\%)</td>"));
          client.println(F("</tr>"));
          client.println(F("<tr>"));
          if (temp==-1000) {
            client.print(F("<td>"));client.print("未測定");client.println(F("</td>"));
          } else {
            client.print(F("<td>"));client.print(temp);client.println(F("</td>"));
          }
          if (hum==-1000) {
            client.print(F("<td>"));client.print("未測定");client.println(F("</td>"));
          } else {
            client.print(F("<td>"));client.print(hum);client.println(F("</td>"));
          }
          if ((hum==-1000) || (temp==-1000)) {
            client.print(F("<td>"));client.print("計算不可");client.println(F("</td>"));
          } else {
            client.print(F("<td>"));client.print(HeatIndex(temp,hum));client.println(F("</td>"));
          }
          client.println(F("</tr>"));
          client.println(F("</table>"));
          client.println(F("<br />"));
#if defined(USE_NTP) || defined(USE_RTC)
          client.print(F("測定時刻 : "));client.println(currentTime);
#endif /* USE_NTP or USE_RTC */
          client.println(F("</body>"));
          Serial.println("generate response . done.");
          break;
        }
        if (c == '\n') {
          switch (endToken) {
            case 1: endToken++;break;
            default: endToken=0;
          }
        } else if (c == '\r') {
          // CR
          switch (endToken) {
            case 0: endToken++;break;
            case 2: endToken++;break;
            default: endToken=0;
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}
