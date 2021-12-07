
#define USE_HARD_SERIAL          // センサデータ出力先にハードウェアシリアルの1番(Serial)を使う
#define MKR

#include "AusExOutputPlugin.h"
#include "AusExAM232X.h"

#include "arduinoHardwareHelper.h"

#ifdef ESP32
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "BLEAdvertising.h"
#include "BLEEddystoneURL.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */

#endif /* ESP32 */

#ifdef MKR
#include <ArduinoBLE.h>
#endif /* MKR */

#ifdef MKR
#define BEACON_DATA_TYPE byte
#endif /* MKR */

#ifdef ESP32
#define BEACON_DATA_TYPE char
#endif /* ESP32 */

#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */
#define SENSOR_WAIT_TIME 10000
#define BEACON_DURATION  10000

#define BEACON_DATA_SIZE 25



// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#ifdef ESP32
BLEAdvertising *pAdvertising;
#endif /* ESP32 */


//time_t lastTenth;

#ifdef ESP32
#define BEACON_UUID "8ec76ea3-6668-48da-9866-75be8bc86f4d" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#endif /* ESP32 */

#ifdef ESP32
RTC_DATA_ATTR static time_t last;    // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory
#endif /* ESP32 */

#define AUSEX_OUTPUT_CHANNEL AUSEX_OUTPUT_CHANNEL_SERIAL

AuxExSensorIO outputDevice =  AuxExSensorIO();
AusExAM232X am232x = AusExAM232X(&Wire, AM2321);

OutputChannel channel;
HardwareHelper hwHelper;



void setBeacon(int beacon_data_length, BEACON_DATA_TYPE beacon_data[])
{

#ifdef ESP32
  uint16_t beconUUID = 0xFEAA;

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oScanResponseData.setFlags(0x06); // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  oScanResponseData.setCompleteServices(BLEUUID(beconUUID));



  oScanResponseData.setServiceData(BLEUUID(beconUUID), std::string(beacon_data, beacon_data_length));

  oAdvertisementData.setName("SensorBeacon");
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
#endif /* ESP32 */
#ifdef MKR
  BLE.setManufacturerData(beacon_data, beacon_data_length);
#endif /* MKR */
}





//
// 本体をリセットする関数
//
void reboot() {
  Serial.println("reboot");
  hwHelper.SoftwareReset();
}



void setup() {
  Serial.begin(115200) ;    // シリアル通信の初期化
  hwHelper.SerialWait();    // シリアルポートが開くのを待つ

  Serial.println("setup start.");

#ifdef MKR
  if (!BLE.begin()) {
      while (1);
  }
#endif /* MKR */

#ifdef USE_HARD_SERIAL
  channel.serial= &Serial;
#endif /* USE_HARD_SERIAL*/



  outputDevice.SetIO(AUSEX_OUTPUT_CHANNEL, channel, FORMAT_TYPE_PLAIN_TEXT);








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
#ifdef ESP32
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
#endif /* ESP32 */
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  delay(SENSOR_WAIT_TIME);


#ifdef ESP32
  // Create the BLE Device
  BLEDevice::init("foo");

  BLEDevice::setPower(ESP_PWR_LVL_N12);

  pAdvertising = BLEDevice::getAdvertising();
#endif /* ESP32 */


}

int generateBeaconData(BEACON_DATA_TYPE beacon_data[], sensors_event_t *event_list[]){
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
          beacon_data[counter]=(BEACON_DATA_TYPE) type;
          counter++;
          temp16_t=(int) (event_list[i]->relative_humidity)*10;
          beacon_data[counter+1]=(BEACON_DATA_TYPE) temp16_t;
          beacon_data[counter]=(BEACON_DATA_TYPE) (temp16_t >> 8);
          counter+=2;
          Serial.print("Humidity=");Serial.print(temp16_t);Serial.println("(%)");
          break;
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
          if ((counter+3) > (BEACON_DATA_SIZE-2)) break;
          type=(uint8_t) event_list[i]->type;
          beacon_data[counter]=(BEACON_DATA_TYPE) type;
          counter++;
          temp16_t=(int) (event_list[i]->temperature)*10;
          beacon_data[counter+1]=(BEACON_DATA_TYPE) temp16_t;
          beacon_data[counter]=(BEACON_DATA_TYPE) (temp16_t >> 8);
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
  BEACON_DATA_TYPE beacon_data[BEACON_DATA_SIZE];
  Serial.println("******************");


  am232x.temperature().getEvent(&temperature_event);
  if (isnan(temperature_event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    events[counter]=&temperature_event;
    counter++;
  }
  // Get humidity event and print its value.
  am232x.humidity().getEvent(&humidity_event);
  if (isnan(humidity_event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
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


  setBeacon(beacon_data_length+1, beacon_data);
#ifdef ESP32
  pAdvertising->start();
  Serial.println("Advertizing started for 10s ...");
  delay(BEACON_DURATION);
  pAdvertising->stop();
#endif /* ESP32 */
  BLE.advertise();
  Serial.println("Advertizing started for 10s ...");
  delay(BEACON_DURATION);
  BLE.stopAdvertise();
#ifdef MKR
  
#endif /* MKR */

  Serial.println("Going to sleep now");

  Serial.flush();

#ifdef ESP32
  esp_deep_sleep_start();
#endif /* ESP32 */
  delay(10000);
}
