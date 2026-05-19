#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define LCD_ADDR 0x27

// BASE CLASS (ABSTRACTION)
class Device {
public:
    virtual void begin() = 0;
    virtual void update() = 0;
};

// SENSOR CLASS (ENCAPSULATION)
class Sensor : public Device {

private:
    int pin;
    int value;

public:
    Sensor(int p) {
        pin = p;
        value = 0;
    }

    void begin() override {
        pinMode(pin, INPUT);
    }

    void update() override {
        value = analogRead(pin);
    }

    int getValue() {
        return value;
    }
};


class Relay : public Device {

private:
    int pin;

public:
    Relay(int p) {
        pin = p;
    }

    void begin() override {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void on() {
        digitalWrite(pin, HIGH);
    }

    void off() {
        digitalWrite(pin, LOW);
    }

    void update() override {
  
    }
};

// SOLAR TRACKER (POLYMORPHISM FIXED)
class SolarTracker : public Device {

private:
    Servo servo;
    int pin;
    int angle;
    int error;

public:
    SolarTracker(int p) {
        pin = p;
        angle = 90;
        error = 0;
    }

    void begin() override {
        servo.attach(pin);
        servo.write(angle);
    }

    void setError(int e) {
        error = e;
    }

    void update() override {
        angle += error * 0.01;

        if (angle > 180) angle = 180;
        if (angle < 0) angle = 0;

        servo.write(angle);
    }
};

class Display : public Device {

private:
    LiquidCrystal_I2C lcd;

public:
    Display() : lcd(LCD_ADDR, 20, 4) {}

    void begin() override {
        lcd.init();
        lcd.backlight();
    }

    void show(float solar, float load, int battery, String source) {
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Solar: ");
        lcd.print(solar);

        lcd.setCursor(0, 1);
        lcd.print("Load: ");
        lcd.print(load);

        lcd.setCursor(0, 2);
        lcd.print("Battery: ");
        lcd.print(battery);
        lcd.print("%");

        lcd.setCursor(0, 3);
        lcd.print("Source: ");
        lcd.print(source);
    }

    void update() override {
       
    }
};

Sensor solarPot(15);
Sensor loadPot(16);
Sensor ldrEast(17);
Sensor ldrWest(14);

Relay relaySolar(4);
Relay relayBattery(2);
Relay relayGrid(27);

// ACTUATORS
SolarTracker tracker(13);
Display display;

float battery = 70.0;
String source = "SOLAR";

void setup() {

    Serial.begin(115200);

    solarPot.begin();
    loadPot.begin();
    ldrEast.begin();
    ldrWest.begin();

    relaySolar.begin();
    relayBattery.begin();
    relayGrid.begin();

    tracker.begin();
    display.begin();
}

void loop() {

    // updating sensors
    solarPot.update();
    loadPot.update();
    ldrEast.update();
    ldrWest.update();

    float solar = solarPot.getValue();
    float load = loadPot.getValue();

    int error = ldrEast.getValue() - ldrWest.getValue();

    // solar tracking 
    tracker.setError(error);
    tracker.update();

    // ENERGY MANAGEMENT LOGIC
    if (solar > load) {

        battery += 0.5;
        source = "SOLAR";

        relaySolar.on();
        relayBattery.off();
        relayGrid.off();
    }
    else if (battery > 20) {

        battery -= 0.3;
        source = "BATTERY";

        relaySolar.off();
        relayBattery.on();
        relayGrid.off();
    }
    else {

        source = "GRID";

        relaySolar.off();
        relayBattery.off();
        relayGrid.on();
    }

    if (battery > 100) battery = 100;
    if (battery < 0) battery = 0;

    // display output
    display.show(solar, load, (int)battery, source);

    Serial.print("Solar: ");
    Serial.print(solar);
    Serial.print(" | Load: ");
    Serial.print(load);
    Serial.print(" | Battery: ");
    Serial.print(battery);
    Serial.print("% | Source: ");
    Serial.println(source);

    delay(500);
}