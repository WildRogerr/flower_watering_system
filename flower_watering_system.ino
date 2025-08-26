#include <TM1637Display.h>


#define CLK 2             // TM1637 CLK
#define DIO 3             // TM1637 DIO
#define SPEAKER_PIN 8     // Speaker
#define PUMP_PIN 9        // Pump (BC337-40, NPN)
#define BUTTON_LEFT 4     // Left Button
#define BUTTON_RIGHT 5    // Right Button
#define MAIN_MENU 100     // Main menu
#define RUNTIME_MENU 200  // Menu of pump runtime
#define INTERVAL_MENU 300 // Interval menu between the launch of the pump


//Classes:
TM1637Display display(CLK, DIO);


class Speaker {
  private:
    int pin;
    unsigned long toneStartTime;
    bool isPlaying;
    unsigned long toneDuration;
    int toneFrequency;
    int beepState;
    unsigned long beepInterval;

  public:
    Speaker(int speakerPin) : pin(speakerPin), toneStartTime(0), isPlaying(false), toneDuration(0), 
                             toneFrequency(0), beepState(0), beepInterval(0) {
      pinMode(pin, OUTPUT);
    }

    void oneBeep() {
      if (!isPlaying && beepState == 0) {
        toneFrequency = 880;
        toneDuration = 200;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
        beepState = 3;
      }
    }

    void twoBeep() {
      if (!isPlaying && beepState == 0) {
        toneFrequency = 880;
        toneDuration = 150;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
        beepState = 1;
      }
    }

    void runtimeBeep() {
      if (!isPlaying && beepState == 0) {
        toneFrequency = 1319;
        toneDuration = 200;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
        beepState = 3;
      }
    }

    void intervalBeep() {
      if (!isPlaying && beepState == 0) {
        toneFrequency = 587;
        toneDuration = 200;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
        beepState = 3;
      }
    }

    void update() {
      if (beepState > 0 && (millis() - toneStartTime >= toneDuration)) {
        noTone(pin);
        isPlaying = false;
        if (beepState == 1) {
          beepInterval = millis();
          beepState = 2;
        } else if (beepState == 2 && (millis() - beepInterval >= 150)) {
          toneFrequency = 880;
          toneDuration = 150;
          toneStartTime = millis();
          tone(pin, toneFrequency);
          isPlaying = true;
          beepState = 3;
        } else if (beepState == 3) {
          beepState = 0;
        }
      }
    }
};

Speaker speaker(SPEAKER_PIN);


class Pump {
private:
  int pin;
public:
  Pump(int pumpPin) : pin(pumpPin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  void turnOn() {
    digitalWrite(pin, HIGH);
  }
  void turnOff() {
    digitalWrite(pin, LOW);
  }
};

Pump pump(PUMP_PIN);


class Interface {
private:
  unsigned short menuStatus;
  const unsigned short holdTime = 2000;
  const unsigned short settingsTimeout = 3000;
  unsigned long lastPushButton;
  unsigned long pumpStartTime;
  unsigned long pumpStartWaitingTime;
  unsigned long pumpRunTime;
  unsigned long remainingTime;
  unsigned long pumpWaiting;
  unsigned long pumpWaitingRemain;
  unsigned long lastColonToggle;
  unsigned long startTime;
  unsigned long startTimeLeft;
  unsigned long startTimeRight;
  bool pumpRunning;
  bool colonState;
  
public:

  Interface() : menuStatus(MAIN_MENU), lastPushButton(0), pumpStartTime(0),
  pumpStartWaitingTime(0), pumpRunTime(10000), remainingTime(0), 
  pumpWaiting(600000), pumpWaitingRemain(0), pumpRunning(false),
  colonState(true), lastColonToggle(0), startTime(0),
  startTimeLeft(0), startTimeRight(0) {}

  const uint8_t ON[4] = {
  0, 0,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // O
  SEG_C | SEG_E | SEG_G                         // n
  };

  friend class SystemManagement;

  bool checkButtonsHold();
  bool checkButtonHoldLeft();
  bool checkButtonHoldRight();
  void updateDisplayRunningTime();
  void updateDisplayPumpTime(unsigned long pumpRunTime);
  void updateDisplayWaitingTime(unsigned long displayTimeValue);
};

bool Interface::checkButtonsHold() {
  static unsigned long lastDebounceTime = 0;
  if (millis() < 50 && lastDebounceTime == 0) {
  } else if (millis() - lastDebounceTime < 50) {
    return false;
  }
  if (digitalRead(BUTTON_LEFT) == LOW && digitalRead(BUTTON_RIGHT) == LOW) {
    if (startTime == 0) {
      startTime = millis();
    }
    if (millis() - startTime >= holdTime) {
      lastDebounceTime = millis();
      startTime = 0;
      return true;
    }
  } else {
    startTime = 0;
    lastDebounceTime = millis();
  }
  return false;
}

bool Interface::checkButtonHoldLeft() {
  static unsigned long lastDebounceTime = 0;
  if (millis() < 50 && lastDebounceTime == 0) {
  } else if (millis() - lastDebounceTime < 50) {
      return false;
  }
  if (digitalRead(BUTTON_LEFT) == LOW && digitalRead(BUTTON_RIGHT) == !LOW && menuStatus == MAIN_MENU) {
    if (startTimeLeft == 0) {
      startTimeLeft = millis();
    }
    if (millis() - startTimeLeft >= holdTime) {
      lastDebounceTime = millis();
      startTimeLeft = 0;
      return true;
    }
  } else {
    startTimeLeft = 0;
    lastDebounceTime = millis();
  }
  return false;
}

bool Interface::checkButtonHoldRight() {
  static unsigned long lastDebounceTime = 0;
  if (millis() < 50 && lastDebounceTime == 0) {
  } else if (millis() - lastDebounceTime < 50) {
      return false;
  }
  if (digitalRead(BUTTON_RIGHT) == LOW && digitalRead(BUTTON_LEFT) == !LOW && menuStatus == MAIN_MENU) {
    if (startTimeRight == 0) {
      startTimeRight = millis();
    }
    if (millis() - startTimeRight >= holdTime) {
      lastDebounceTime = millis();
      startTimeRight = 0;
      return true;
    }
  } else {
    startTimeRight = 0;
    lastDebounceTime = millis();
  }
  return false;
}

void Interface::updateDisplayRunningTime() {
  unsigned long totalSeconds = remainingTime / 1000;
  unsigned long minutes = totalSeconds / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long displayTime = minutes * 100 + seconds;
  if (displayTime > 9959) {
    displayTime = 9959;
  }
  display.showNumberDecEx(displayTime, 0b01000000, true);
};

void Interface::updateDisplayPumpTime(unsigned long pumpRunTime) {
  unsigned long totalSeconds = pumpRunTime / 1000;
  unsigned long minutes = totalSeconds / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long displayTime = minutes * 100 + seconds;
  if (displayTime > 9959) {
    displayTime = 9959;
  }
  if (menuStatus == RUNTIME_MENU) {
    if (millis() - lastColonToggle >= 500) {
      colonState = !colonState;
      lastColonToggle = millis();
    }
    display.showNumberDecEx(displayTime, colonState ? 0b01000000 : 0, true);
  } else {
    display.showNumberDecEx(displayTime, 0b01000000, true);
  }
};

void Interface::updateDisplayWaitingTime(unsigned long displayTimeValue) {
  unsigned long totalSeconds = displayTimeValue / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long displayTime;
  if (totalSeconds < 3600 && menuStatus == INTERVAL_MENU) {
    displayTime = 00+minutes;
  } else if (totalSeconds < 86400 && totalSeconds > 3600) {
    displayTime = hours * 100 + minutes;
  } else if (totalSeconds > 86400) {
    displayTime = (days % 100) * 100 + (hours % 24);
  } else if (totalSeconds < 3600 && menuStatus == RUNTIME_MENU || menuStatus == MAIN_MENU) {
    displayTime = minutes * 100 + seconds;
  if (displayTime > 9923 && totalSeconds >= 86400) {
    displayTime = 9923;
  } else if (displayTime > 9959) {
    displayTime = 9959;
  }
  if (menuStatus == INTERVAL_MENU || menuStatus == RUNTIME_MENU) {
    if (millis() - lastColonToggle >= 500) {
      colonState = !colonState;
      lastColonToggle = millis();
    }
    display.showNumberDecEx(displayTime, colonState ? 0b01000000 : 0, true);
  } else {
    display.showNumberDecEx(displayTime, 0b01000000, true);
  }
};

Interface interface;


class SystemManagement {
  public:
    void checkDoubleButtons();
    void checkOneButton();
    void runtimeMenu();
    void intervalMenu();
    void timers();
};

void SystemManagement::checkDoubleButtons() {
  static unsigned long lastButtonPress = 0;
  if (interface.checkButtonsHold() && (millis() - lastButtonPress >= 200)) {
    if (!interface.pumpRunning) {
      pump.turnOn();
      speaker.twoBeep();
      interface.pumpRunning = true;
      interface.pumpStartTime = millis();
      interface.pumpStartWaitingTime = millis() + interface.pumpRunTime;
      interface.remainingTime = interface.pumpRunTime;
    } else {
      pump.turnOff();
      interface.pumpRunning = false;
      interface.pumpStartWaitingTime = millis();
      speaker.oneBeep();
    }
    interface.menuStatus = MAIN_MENU;
    lastButtonPress = millis();
  }
};

void SystemManagement::checkOneButton() {
  static unsigned long lastDebounceTime = 0;
  if (millis() - lastDebounceTime < 50) return;
  
  if (interface.checkButtonHoldLeft() && !interface.pumpRunning) {
    interface.menuStatus = RUNTIME_MENU;
    speaker.runtimeBeep();
    interface.lastPushButton = millis();
    interface.colonState = true;
    interface.lastColonToggle = millis();
    lastDebounceTime = millis();
  }

  if (interface.checkButtonHoldRight() && !interface.pumpRunning) {
    interface.menuStatus = INTERVAL_MENU;
    speaker.intervalBeep();
    interface.lastPushButton = millis();
    interface.colonState = true;
    interface.lastColonToggle = millis();
    lastDebounceTime = millis();
  }
};

void SystemManagement::runtimeMenu() {
  static unsigned long lastDebounceTime = 0;
  if (millis() - lastDebounceTime < 50) return;
  if (interface.menuStatus == RUNTIME_MENU) {
    if (millis() - interface.lastPushButton < interface.settingsTimeout) {
      if (digitalRead(BUTTON_LEFT) == LOW && (millis() - interface.lastPushButton >= 100)) {
        interface.pumpRunTime = (interface.pumpRunTime >= 1000) ? interface.pumpRunTime - 1000 : 5999000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      }
      if (digitalRead(BUTTON_RIGHT) == LOW && (millis() - interface.lastPushButton >= 100)) {
        interface.pumpRunTime = (interface.pumpRunTime <= 5999000) ? interface.pumpRunTime + 1000 : 1000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      }
      interface.updateDisplayPumpTime(interface.pumpRunTime);
    } else if (digitalRead(BUTTON_LEFT) == HIGH && digitalRead(BUTTON_RIGHT) == HIGH) {
      interface.menuStatus = MAIN_MENU;
      speaker.oneBeep();
      interface.colonState = true;
      interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
    }
  }
};

void SystemManagement::intervalMenu() {
  static unsigned long lastDebounceTime = 0;
  if (millis() - lastDebounceTime < 50) return;
  if (interface.menuStatus == INTERVAL_MENU) {
    if (millis() - interface.lastPushButton < interface.settingsTimeout) {
      if (digitalRead(BUTTON_LEFT) == LOW && millis() - interface.lastPushButton >= 100 && interface.pumpWaiting <= 86400000) {
        interface.pumpWaiting = (interface.pumpWaiting >= 60000) ? interface.pumpWaiting - 60000 : 2678400000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      } else if (digitalRead(BUTTON_LEFT) == LOW && millis() - interface.lastPushButton >= 100 && interface.pumpWaiting <= 2678400000) {
        interface.pumpWaiting = (interface.pumpWaiting >= 60000) ? interface.pumpWaiting - 3600000 : 2678400000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      }
      if (digitalRead(BUTTON_RIGHT) == LOW && millis() - interface.lastPushButton >= 100 && interface.pumpWaiting >= 86400000) {
        interface.pumpWaiting = (interface.pumpWaiting <= 2678400000) ? interface.pumpWaiting + 3600000 : 60000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      } else if (digitalRead(BUTTON_RIGHT) == LOW && millis() - interface.lastPushButton >= 100 && interface.pumpWaiting >= 60000) {
        interface.pumpWaiting = (interface.pumpWaiting <= 2678400000) ? interface.pumpWaiting + 60000 : 60000;
        interface.lastPushButton = millis();
        lastDebounceTime = millis();
      }
      interface.updateDisplayWaitingTime(interface.pumpWaiting);
    } else if (digitalRead(BUTTON_LEFT) == HIGH && digitalRead(BUTTON_RIGHT) == HIGH) {
      interface.menuStatus = MAIN_MENU;
      speaker.oneBeep();
      interface.colonState = true;
      interface.pumpStartWaitingTime = millis();
      interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
    }
  }
};

void SystemManagement::timers() {
  if (!interface.pumpRunning && millis() - interface.pumpStartWaitingTime >= interface.pumpWaiting && interface.menuStatus == MAIN_MENU) {
    pump.turnOn();
    speaker.twoBeep();
    interface.pumpRunning = true;
    interface.pumpStartTime = millis();
    interface.pumpStartWaitingTime = millis() + interface.pumpRunTime;
    interface.remainingTime = interface.pumpRunTime;
  }
  
  if (interface.pumpRunning) {
    interface.remainingTime = (interface.pumpRunTime > (millis() - interface.pumpStartTime))
                              ? (interface.pumpRunTime - (millis() - interface.pumpStartTime)) : 0;
    interface.updateDisplayRunningTime();
    if (interface.remainingTime == 0) {
      pump.turnOff();
      interface.pumpRunning = false;
      interface.pumpStartWaitingTime = millis();
      speaker.oneBeep();
    }
  } else if (interface.menuStatus == MAIN_MENU) {
    interface.pumpWaitingRemain = (interface.pumpWaiting > (millis() - interface.pumpStartWaitingTime))
                                  ? (interface.pumpWaiting - (millis() - interface.pumpStartWaitingTime)) : 0;
    interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
  }
};

SystemManagement systemManager;


void setup() {
  //Display setup
  display.setBrightness(5);
  display.clear();
  display.setSegments(interface.ON);
  delay(2000);
  //Pins setup
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  speaker.oneBeep();
}


void loop() {
  speaker.update();
  systemManager.timers();
  systemManager.checkDoubleButtons();
  systemManager.checkOneButton();
  systemManager.runtimeMenu();
  systemManager.intervalMenu();
}