//
// Grove - LED
// http://www.seeedstudio.com/wiki/index.php?title=GROVE_-_Starter_Kit_v1.1b#Grove_-_LED
//
// ボタンを使わないように改造
// 1秒間隔でLEDが点滅


#define INTERVAL 1000
#define SHORT_INTERVAL 300

#define SHUTDOWN_PIN 9

//const int ledPin =  6;      // the number of the LED pin
const int ledPin =  3;      // the number of the LED pin

// variables will change:

void setup() {
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);
  for (int i=0;i<10;i++){
      digitalWrite(ledPin, HIGH);
      delay(INTERVAL);
      digitalWrite(ledPin, LOW);
      delay(INTERVAL);
  }
  for (int i=0;i<10;i++){
      digitalWrite(ledPin, HIGH);
      delay(SHORT_INTERVAL);
      digitalWrite(ledPin, LOW);
      delay(SHORT_INTERVAL);
  }
}

void shutdown(){
  digitalWrite(SHUTDOWN_PIN, HIGH);
}

void loop(){
  // read the state of the pushbutton value:
  digitalWrite(ledPin, HIGH);
  shutdown();
}
