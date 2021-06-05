/*
 제목    : I2C LCD에 문자 출력하기
 내용   : I2C LCD에 원하는 문자를 표시해 봅니다.
 */

// LCD를 쉽게 제어하기 위한 라이브러리를 추가합니다.
#include <LiquidCrystal_I2C.h>
#include <SimpleDHT.h>
#include <Wire.h>


const int pinTemperature = A0;
const int pinDHT11 = 11;
const int btnIn[]={9,8};
const int ledOut[]={3,4,5};

class Humid {
   public:
    SimpleDHT11 *dht11;

   public:
    Humid(int port) { this->dht11 = new SimpleDHT11(port); }
    int get() {
        byte temperature = 0;
        byte humidity = 0;
        int err = SimpleDHTErrSuccess;
        if ((err = this->dht11->read(&temperature, &humidity, NULL)) !=
            SimpleDHTErrSuccess) {
            Serial.print("Read DHT11 failed, err=");
            Serial.print(SimpleDHTErrCode(err));
            Serial.print(",");
            Serial.println(SimpleDHTErrDuration(err));
            delay(10);
            return;
        }
        Serial.println(humidity);
        return humidity;
    }
};
Humid humid(pinDHT11);

class Temperature {
   public:
    int port;

   public:
    Temperature(int port) { this->port = port; }
    int get() {
        return map(analogRead(this->port),20,358, -40, 125);  //온드 가공 필요
    }
};
Temperature temperature(pinTemperature);

class Fan {
   public:
    int port;
    bool on;
    int time_left;

   public:
    Fan(int port) {
        this->port = port;
        this->on = false;
        this->time_left = 0;

        pinMode(this->port, OUTPUT);
    }
    void setActive() {
        this->on = true;
        this->time_left = 100;
        digitalWrite(pinFan,HIGH);
    }
    void setInactive() {
        this->on = false;
        digitalWrite(this->port, LOW);
    }
    void act() {
        if (this->on) {
            this->time_left--;
            if (this->time_left <= 0) {
                setInactive();
            }
        }
    }
};
Fan fan(pinFan);

// 0x3F I2C 주소를 가지고 있는 16x2 LCD객체를 생성합니다.(I2C 주소는 LCD에 맞게
// 수정해야 합니다.)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
    pinMode(pinFan, OUTPUT);
    
    for(int i:btnIn){
        pinMode(i, INPUT);
    }
    for(int i:ledOut){
        pinMode(i, OUTPUT);
    }

    // I2C LCD를 초기화 합니다..
    lcd.init();
    // I2C LCD의 백라이트를 켜줍니다.
    lcd.backlight();
}

void loop() {

    int vTemp = temperature.get(), vHumid = humid.get();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T: ");    lcd.print(vTemp);
    lcd.setCursor(0, 1);
    lcd.print("H: ");    lcd.print(vHumid);    lcd.print("  fan: ");    lcd.print(fan.on);

    if(50 < vHumid){
        fan.setActive();
    }

    fan.act();

    delay(100);
    
}
