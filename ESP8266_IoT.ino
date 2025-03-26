#include <MQTT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Servo.h>

// ESP8266 PINOUT
#define temperature_sensor_pin 2  // D4
#define motion_sensor_pin 4       // D2
#define gas_sensor_pin 12         // D6
#define brightness_sensor_pin 16  // D0
#define water_sensor_pin 1        // TX

#define light_pin 14        // D5
#define outside_light_pin 3 // RX
#define window_pin 5        // D1
#define airing_pin 13       // D7
#define AC_pin 15           // D8


const char *ssid = "WiFi Name"; // Имя вайфай точки доступа
const char *pass = "Password";     // Пароль от точки доступа

const char *mqtt_server = "server.wqtt.ru";   // Имя сервера MQTT
const int   mqtt_port = 12345;            // Порт для подключения к серверу MQTT
const char *mqtt_user = "Login"; // Логин от сервер
const char *mqtt_pass = "Password";       // Пароль от сервера

const int sending_period = 2;  // Задержка таймера 
int tmr1 = 0;                  // Таймер


byte temperature_sensor_value = 0;  // Датчик температуры
byte humidity_sensor_value = 0;     // Датчик влажности
byte motion_sensor_value = 0;       // Датчик движения
int  gas_sensor_value;              // Датчик газа   
int  brightness_sensor_value = 0;   // Датчик света
byte water_sensor_value = 0;        // Датчик протечки

byte light_web_value = 0;          // Состояние света
byte outside_light_web_value = 0;  // Состояние уличного света 
byte window_web_value = 0;         // Состояние окна
byte airing_web_value = 0;         // Состояние вытяжки
byte AC_web_value = 0;             // Температура кондишки
byte AC_web_value2 = 0;            // Состояние кондишки
bool security_web_value = 0;       // Состояние охраны


// Даем названия объектам
WiFiClient wclient;
PubSubClient client(mqtt_server, mqtt_port, wclient);
DHT dht(temperature_sensor_pin, DHT11);
Servo Window_Control;


void setup() {

  pinInitialize();
  
  client.setCallback(callback);

  Serial.begin(115200);
  delay(1500);

  WiFi_Connecting();

}

void loop() {

  if (!client.connected()) {
    MQTT_Connecting();  // Подключение к серверу и подписка на топики
  }

  client.loop();

  Read_Sensors();

  if (millis() - tmr1 >= (sending_period * 1000)) {
    tmr1 = millis();
    Publish_To_Server(); // Отправка данных на сервер
  }

  Lights_Logic();

  Outside_Light_Logic();
  
  Window_Logic();

  AC_Logic();

  Airing_Logic();

  Fire_Logic();

  Water_Logic();

  Security_Logic();

}


void pinInitialize(){  

  pinMode(motion_sensor_pin, INPUT);
  pinMode(gas_sensor_pin, INPUT);
  pinMode(brightness_sensor_pin, INPUT);
  pinMode(water_sensor_pin, INPUT);
  dht.begin();

  
  pinMode(light_pin, OUTPUT);
  pinMode(outside_light_pin, OUTPUT);
  pinMode(airing_pin, OUTPUT);
  pinMode(AC_pin, OUTPUT);
  Window_Control.attach(window_pin);

}

void callback(const char *topic, byte *payload, unsigned int length){
  String message;
  for (int i = 0; i < length; i++){
    message = message + (char)payload[i];
  }

  if (String(topic) == "light_web_value_topic"){\

    if (message.toInt() > 1){
      light_web_value = message.toInt() * 2.55;
    }

    else{
      light_web_value = 0;
    }
  } 

  if (String(topic) == "window_web_value_topic"){

    if (message.toInt() > 1){
      window_web_value = message.toInt() * 1.8;
    }

    else{
      window_web_value = 0;
    }
  }

  if (String(topic) == "AC_web_value_topic"){
    if (message.toInt() > 1){
      AC_web_value = message.toInt();
    }

    else{
      AC_web_value = 0;
    }
  }

  if (String(topic) == "AC_web_value_topic2"){
    if (message.toInt() == 1){
      AC_web_value2 = message.toInt();
    }

    else{
      AC_web_value2 = 0;
    }
  }
  
  if (String(topic) == "airing_web_value_topic"){
    if (message.toInt() == 1){
      airing_web_value = message.toInt();
    }

    else{
      airing_web_value = 0;
    }
  }

  if (String(topic) == "outside_light_web_value_topic"){ 

    if (message.toInt() > 1){
      outside_light_web_value = message.toInt() * 2.55;
    }

    else{
      outside_light_web_value = 0;
    }
  } 

  if (String(topic) == "security_web_value_topic"){
    if (message.toInt() == 1){
      security_web_value = message.toInt();
    }

    else{
      security_web_value = 0;
    }
  }
  
} 

void WiFi_Connecting() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  
  }

  Serial.println("");
  Serial.println("WiFi connected to " + String(ssid));

}

void MQTT_Connecting() {
  while (!client.connected()) {

    String clientId = "ESP8266-" + WiFi.macAddress();

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass) ) {

      Serial.println("Connected as " + clientId);
      
      client.subscribe("light_web_value_topic");
      client.subscribe("outside_light_web_value_topic");
      client.subscribe("window_web_value_topic");
      client.subscribe("airing_web_value_topic");
      client.subscribe("AC_web_value_topic");
      client.subscribe("AC_web_value_topic2");
      client.subscribe("security_web_value_topic");
      

    } 

    else {

      Serial.println("Connection to server failure. Error code: " + String(client.state()));

    }
  }
}


void Read_Sensors(){

  humidity_sensor_value = dht.readHumidity();
  temperature_sensor_value = dht.readTemperature();

  motion_sensor_value = digitalRead(motion_sensor_pin);

  gas_sensor_value = analogRead(gas_sensor_pin);
  
  brightness_sensor_value = analogRead(brightness_sensor_pin);

  water_sensor_value = analogRead(water_sensor_pin);

}

void Publish_To_Server(){

  client.publish("temperature_sensor_value_topic", String(temperature_sensor_value).c_str(), false);
  client.publish("humidity_sensor_value_topic", String(humidity_sensor_value).c_str(), false);
  
}


void Lights_Logic(){
  if (security_web_value == 0){
    client.publish("security_sensor_value_topic", "0");
    if (motion_sensor_value == 1 or light_web_value > 1){
      if (light_web_value > 1){
        analogWrite(light_pin, light_web_value);
      }

      else{
        digitalWrite(light_pin, HIGH);
      }
    }

    if (light_web_value == 0 and motion_sensor_value == 0){
      digitalWrite(light_pin, LOW);
    }
  }

  if (security_web_value == 1){
      analogWrite(light_pin, light_web_value);
  }
}

void Outside_Light_Logic(){
  if (brightness_sensor_value > 1 or outside_light_web_value > 1){
    if (outside_light_web_value > 1){
      analogWrite(outside_light_pin, outside_light_web_value);
    }

    else{
      analogWrite(outside_light_pin, brightness_sensor_value / 4);
    }
  }

  if (outside_light_web_value == 0 and brightness_sensor_value == 0){
    digitalWrite(outside_light_pin, LOW);
  }
}

void Window_Logic(){
  if (window_web_value > 1 or humidity_sensor_value > 60){
    if (window_web_value > 1){
      Window_Control.write(window_web_value);
    }

    else{
       Window_Control.write(90);
    }
  }

  if (window_web_value == 0 and humidity_sensor_value < 60){
    Window_Control.write(0);
  }
}

void Airing_Logic(){
  
  if (humidity_sensor_value > 60 or airing_web_value == 1){
    digitalWrite(airing_pin, HIGH);
  }
  if (humidity_sensor_value < 60 and airing_web_value == 0){
    digitalWrite(airing_pin, LOW);
  }
}

void AC_Logic(){
  if (temperature_sensor_value > 29 or AC_web_value2 == 1){
    if (AC_web_value2 == 1){
      analogWrite(AC_pin, AC_web_value);
    }

    else{
      analogWrite(AC_pin, 25);
    }
  }

  if (temperature_sensor_value < 29 and AC_web_value2 == 0){
    digitalWrite(AC_pin, LOW);
  }
}

void Fire_Logic(){
  if (gas_sensor_value > 400){
    client.publish("gas_web_value_topic", "1");
  }
  else{
    client.publish("gas_web_value_topic", "0");
  }
}

void Water_Logic(){
  if (water_sensor_value > 100){
    client.publish("water_sensor_value_topic", "1");
  }

  else{
    client.publish("water_sensor_value_topic", "0");
  }
}

void Security_Logic(){
  if (security_web_value == 1){
    if (motion_sensor_value == 1){
      client.publish("security_sensor_value_topic", "1");
    }
    else{
      client.publish("security_sensor_value_topic", "0");
    }
  }
}
