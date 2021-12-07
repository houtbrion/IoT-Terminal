
#define USE_HARD_SERIAL          // センサデータ出力先にハードウェアシリアルの1番(Serial)を使う

#include "AusExOutputPlugin.h"
#include "AusExAM232X.h"

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "BLEAdvertising.h"
#include "BLEEddystoneURL.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */
#define SENSOR_WAIT_TIME 10000
#define BEACON_DURATION  10000

#define BEACON_DATA_SIZE 25



// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
BLEAdvertising *pAdvertising;
//struct timeval nowTimeStruct;

time_t lastTenth;

#define BEACON_UUID "8ec76ea3-6668-48da-9866-75be8bc86f4d" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)

RTC_DATA_ATTR static time_t last;    // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory

#define AUSEX_OUTPUT_CHANNEL AUSEX_OUTPUT_CHANNEL_SERIAL

AuxExSensorIO outputDevice =  AuxExSensorIO();
AusExAM232X am232x = AusExAM232X(&Wire, AM2321);

OutputChannel channel;


void setBeacon(int beacon_data_length, char beacon_data[])
{
  //char beacon_data[25];
  uint16_t beconUUID = 0xFEAA;
  //uint16_t volt = random(2800, 3700); // 3300mV = 3.3V
  //float tempFloat = random(2000, 3100) / 100.0f;
  //Serial.printf("Random temperature is %.2fC\n", tempFloat);
  //int temp = (int)(tempFloat * 256); //(uint16_t)((float)23.00);
  //Serial.printf("Converted to 8.8 format %0X%0X\n", (temp >> 8), (temp & 0xFF));

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oScanResponseData.setFlags(0x06); // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  oScanResponseData.setCompleteServices(BLEUUID(beconUUID));

  //beacon_data[0] = 0x21;                // Eddystone Frame Type (Unencrypted Eddystone-TLM)
  //beacon_data[1] = 0x00;                // TLM version
  //beacon_data[2] = (volt >> 8);           // Battery voltage, 1 mV/bit i.e. 0xCE4 = 3300mV = 3.3V
  //beacon_data[3] = (volt & 0xFF);           //
  //beacon_data[4] = (temp >> 8);           // Beacon temperature
  //beacon_data[5] = (temp & 0xFF);           //
  //beacon_data[6] = ((bootcount & 0xFF000000) >> 24);  // Advertising PDU count
  //beacon_data[7] = ((bootcount & 0xFF0000) >> 16);  //
  //beacon_data[8] = ((bootcount & 0xFF00) >> 8);   //
  //beacon_data[9] = (bootcount & 0xFF);        //
  //beacon_data[10] = ((lastTenth & 0xFF000000) >> 24); // Time since power-on or reboot as 0.1 second resolution counter
  //beacon_data[11] = ((lastTenth & 0xFF0000) >> 16);   //
  //beacon_data[12] = ((lastTenth & 0xFF00) >> 8);    //
  //beacon_data[13] = (lastTenth & 0xFF);       //

  //oScanResponseData.setServiceData(BLEUUID(beconUUID), std::string(beacon_data, 14));
  oScanResponseData.setServiceData(BLEUUID(beconUUID), std::string(beacon_data, beacon_data_length));
  //oAdvertisementData.setName("TLMBeacon");
  oAdvertisementData.setName("SensorBeacon");
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
}





//
// 本体をリセットする関数
//
void reboot() {
  ESP.restart();
}



void setup() {
  Serial.begin(115200) ;    // シリアル通信の初期化
  while (!Serial) {       // シリアルポートが開くのを待つ
    ;
  }
  Serial.println("setup start.");



#ifdef USE_HARD_SERIAL
  channel.serial= &Serial;
#endif /* USE_HARD_SERIAL*/


  //outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_SYSLOG);
  //outputDevice.SetLogParam(HOSTNAME, APP_NAME);
  outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_PLAIN_TEXT);
  //outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_JSON);







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

  outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_PLAIN_TEXT);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  delay(SENSOR_WAIT_TIME);



  // Create the BLE Device
  //BLEDevice::init("TLMBeacon");
  BLEDevice::init("foo");

  BLEDevice::setPower(ESP_PWR_LVL_N12);

  pAdvertising = BLEDevice::getAdvertising();



}

int generateBeaconData(char beacon_data[], sensors_event_t *event_list[]){
  Serial.println("beacon data process function");
  int num=2;
  //char beacon_data[BEACON_DATA_SIZE];
  int dataPointer=2;
  for (int i=0; i<BEACON_DATA_SIZE;i++){
    beacon_data[i]=0;
  }
  beacon_data[0] = 0x21;                // sensor beacon Frame Type
  beacon_data[1] = 0x00;                // sensor beacon format version
  int16_t temp16_t;
  uint8_t type;
  int counter=2;
  for (int i=0;i<num;i++){
    if (event_list[i] != NULL) {
      switch (event_list[i]->type) {
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
          if ((counter+3) > (BEACON_DATA_SIZE-2)) break;
          type=(uint8_t) event_list[i]->type;
          beacon_data[counter]=(char) type;
          counter++;
          temp16_t=(int) (event_list[i]->relative_humidity)*10;
          beacon_data[counter+1]=(char) temp16_t;
          beacon_data[counter]=(char) (temp16_t >> 8);
          counter+=2;
          Serial.print("Humidity=");Serial.print(temp16_t);Serial.println("(%)");
          break;
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
          if ((counter+3) > (BEACON_DATA_SIZE-2)) break;
          type=(uint8_t) event_list[i]->type;
          beacon_data[counter]=(char) type;
          counter++;
          temp16_t=(int) (event_list[i]->temperature)*10;
          beacon_data[counter+1]=(char) temp16_t;
          beacon_data[counter]=(char) (temp16_t >> 8);
          counter+=2;
          Serial.print("Temperature=");Serial.print(temp16_t);Serial.println("(C)");
          break;
      }
    }
  }
  beacon_data[counter]=0;
  return counter;
}

void loop() {
  sensors_event_t temperature_event, humidity_event;
  sensors_event_t *events[3];
  events[0]=NULL;
  events[1]=NULL;
  events[2]=NULL;
  int counter=0;
  char beacon_data[BEACON_DATA_SIZE];
  Serial.println("******************");


  am232x.temperature().getEvent(&temperature_event);
  if (isnan(temperature_event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    //outputDevice.EventOutput(temperature_event);
    //Serial.print("****Temperature=");Serial.print(temperature_event.temperature);Serial.println("(C)****");
    events[counter]=&temperature_event;
    counter++;
  }
  // Get humidity event and print its value.
  am232x.humidity().getEvent(&humidity_event);
  if (isnan(humidity_event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    //outputDevice.EventOutput(humidity_event);
    //Serial.print("****Humidity=");Serial.print(humidity_event.relative_humidity);Serial.println("(%)****");
    events[counter]=&humidity_event;
    counter++;
  }
  int beacon_data_length = generateBeaconData(beacon_data, events);
  Serial.print("beacon_data=");
  for (int i=0;i<BEACON_DATA_SIZE;i++){
    Serial.print("0x");Serial.print((uint8_t) beacon_data[i], HEX);Serial.print(" , ");
  }
  Serial.println("");
  Serial.print("counter=");Serial.println(beacon_data_length);
  Serial.flush();

  //setBeacon(beacon_data_length, beacon_data);
  setBeacon(beacon_data_length+1, beacon_data);
  pAdvertising->start();
  Serial.println("Advertizing started for 10s ...");
  delay(BEACON_DURATION);
  pAdvertising->stop();

  Serial.println("Going to sleep now");

  Serial.flush();
  esp_deep_sleep_start();
}
