/*
 제목    : I2C LCD에 문자 출력하기
 내용   : I2C LCD에 원하는 문자를 표시해 봅니다.
 */

// LCD를 쉽게 제어하기 위한 라이브러리를 추가합니다.
#include <LiquidCrystal_I2C.h>
#include <SimpleDHT.h>
#include <Wire.h>

const int MAX_PIN = 10;
const int MIN_TEMP = 1, MAX_TEMP = 70, TEMP_BIAS = 2;

const int pinTemperature = A0;
const int pinDHT11 = 11;
const int pinFan = 13;
int btnIn[] = {8};
int ledOut[] = {7, 6, 3};

inline int len(int *arr) { return sizeof(arr) / sizeof(arr[0]); }

class BtnLed {
   public:
    int port_btn[MAX_PIN] = {0};
    int port_led[MAX_PIN] = {0};

   public:
    BtnLed(int *btn_arr, int nBtn, int *led_arr, int nLed) {
        for (int i = 0; i < nBtn; i++) {
            port_btn[i] = btn_arr[i];
        }
        for (int i = 0; i < nLed; i++) {
            port_led[i] = led_arr[i];
        }
    }
    int get(int n) { return digitalRead(port_btn[n]); }
    void lit(int n) {
        digitalWrite(ledOut[n], HIGH);
    }  //임시로 전역변수 사용함 - 멤버 속성으로 바꿔야 함
    void extinguish(int n) { digitalWrite(ledOut[n], LOW); }
    void extinguishAll() {
        for (int i = 0; i < len(this->port_led); i++) {
            this->extinguish(i);
        }
    }
};
BtnLed btnled(btnIn, len(btnIn), ledOut, len(ledOut));

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
            return -1;
        }
        Serial.println(humidity);
        return humidity;
    }
};
Humid humid(pinDHT11);

class Temperature {
   public:
    int port;
    int temp_now;
    int temp_mem;

   public:
    Temperature(int port) {
        this->port = port;
        temp_now = -1;
        temp_mem = -1;
    }
    int get() {
        this->temp_now = map(analogRead(this->port), 20, 358, -40, 125);
        return this->temp_now;
    }
    int getMem() { return this->temp_mem; }
    int setTempMem() { temp_mem = this->get(); }
    bool isInRange(int temp, int min, int max) {
        return (min <= temp && temp < max);
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
        digitalWrite(pinFan, HIGH);
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

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
    Serial.begin(9600);

    pinMode(pinFan, OUTPUT);

    for (int i : btnIn) {
        pinMode(i, INPUT);
    }
    for (int i : ledOut) {
        pinMode(i, OUTPUT);
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < len(ledOut); j++) {
            btnled.lit(j);
        }
        delay(100);
        btnled.extinguish(0);
        btnled.extinguish(1);
        btnled.extinguish(2);
        delay(100);
    }

    lcd.init();
    lcd.backlight();
}

void loop() {
    int vTemp = temperature.get(), vHumid = humid.get();

    static int count = 0;
    if (count++ % 2 == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("T: ");
        lcd.print(vTemp);
        lcd.print(" MEM:");
        lcd.print(temperature.getMem());

        lcd.setCursor(0, 1);
        lcd.print("H: ");
        lcd.print(vHumid);
        lcd.print(" fan: ");
        lcd.print(fan.on);
    }

    if (btnled.get(0) == HIGH &&
        temperature.isInRange(temperature.get(), MIN_TEMP, MAX_TEMP)) {
        temperature.setTempMem();
    }

    if (temperature.isInRange(temperature.get(), MIN_TEMP, MAX_TEMP)) {
        if (temperature.get() < temperature.getMem() - TEMP_BIAS) {
            btnled.lit(0);
            btnled.extinguish(1);
            btnled.extinguish(2);
        } else if (temperature.getMem() + TEMP_BIAS < temperature.get()) {
            btnled.extinguish(0);
            btnled.extinguish(1);
            btnled.lit(2);
        } else {
            btnled.extinguish(0);
            btnled.lit(1);
            btnled.extinguish(2);
        }
    } else {
        btnled.extinguishAll();
    }

    if (50 < vHumid) {
        fan.setActive();
    }

    fan.act();

    delay(100);
}
