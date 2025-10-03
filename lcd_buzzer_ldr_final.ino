#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int ledPins[] = {2, 3, 4, 5, 6, 7, 8, 9}; 
const int numLEDs = 8;

const int irSensorUpPin = A0;
const int irSensorDownPin = A1;

const int potPin = A2;
const int ldrPin = A3;
const int buttonPin = 10;
const int buzzerPin = 11;

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool theftAlertActive = false;

bool theftMode = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  // Initialize LED pins
  for (int pin : ledPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  pinMode(irSensorUpPin, INPUT);
  pinMode(irSensorDownPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
}

void displayStatus(int ldrValue) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(theftMode ? "Theft Mode" : "Normal Mode");

  lcd.setCursor(0, 1);
  lcd.print("LDR: ");
  lcd.print(ldrValue);
}

void playBuzzerPattern() {
  for (int i = 1700; i <= 1950; i += 20) {
    tone(buzzerPin, i);
    delay(5);
  }
  for (int i = 1950; i >= 1700; i -= 20) {
    tone(buzzerPin, i);
    delay(5);
  }
  noTone(buzzerPin);
}

void lightUpSequentially(int start, int end, int step, int ledDelay) {
  for (int i = start; i != end; i += step) {
    digitalWrite(ledPins[i], HIGH);
    delay(ledDelay);
  }
}

void lightDownSequentially(int start, int end, int step, int ledDelay, int stopSensor) {
  unsigned long startTime = millis();
  while (millis() - startTime < 7000) {
    if (digitalRead(stopSensor) == LOW) break;
    delay(50);
  }

  for (int i = start; i != end; i += step) {
    digitalWrite(ledPins[i], LOW);
    delay(ledDelay);
  }
}

void checkButton() {
  static bool lastState = HIGH;
  static unsigned long lastPressTime = 0;

  bool currentState = digitalRead(buttonPin);

  if (lastState == HIGH && currentState == LOW) {
    // Button just pressed
    if (millis() - lastPressTime > 200) {  // debounce delay
      theftMode = !theftMode;
      Serial.println(theftMode ? "Switched to Theft Mode" : "Switched to Normal Mode");
      lastPressTime = millis();
    }
  }

  lastState = currentState;
}


void loop() {
  checkButton();

  int potValue = analogRead(potPin);
  int ledDelay = map(potValue, 0, 1023, 100, 800);
  int irSensorUpState = digitalRead(irSensorUpPin);
  int irSensorDownState = digitalRead(irSensorDownPin);
  int ldrValue = analogRead(ldrPin);

  lcd.setCursor(0, 1);
  lcd.print("LDR:");
  lcd.print(ldrValue);
  lcd.print("   "); // clear trailing digits

  if (theftMode) {
    lcd.setCursor(0, 0);
    lcd.print("Mode: THEFT       ");

    // Trigger alert if either IR sensor is LOW
    if (irSensorUpState == LOW || irSensorDownState == LOW) {
      theftAlertActive = true;
    }

    // If alert is active, play buzzer continuously
    if (theftAlertActive) {
      for (int i = 1700; i <= 1950; i += 20) {
        tone(buzzerPin, i);
        delay(5);
      }
      for (int i = 1950; i >= 1700; i -= 20) {
        tone(buzzerPin, i);
        delay(5);
      }
    }

  } else {
    lcd.setCursor(0, 0);
    lcd.print("Mode: NORMAL      ");

    theftAlertActive = false;    // Reset alert
    noTone(buzzerPin);           // Turn off buzzer

    if (ldrValue < 350) {
      if (irSensorUpState == LOW) {
        lightUpSequentially(0, numLEDs, 1, ledDelay);
        lightDownSequentially(0, numLEDs, 1, ledDelay, irSensorDownPin);
      }

      if (irSensorDownState == LOW) {
        lightUpSequentially(numLEDs - 1, -1, -1, ledDelay);
        lightDownSequentially(numLEDs - 1, -1, -1, ledDelay, irSensorUpPin);
      }
    }
  }

  delay(50);
}

