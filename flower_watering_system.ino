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
      if (!isPlaying) {
        toneFrequency = 852;
        toneDuration = 600;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
      }
    }

    void twoBeep() {
      if (!isPlaying && beepState == 0) {
        toneFrequency = 852;
        toneDuration = 300;
        toneStartTime = millis();
        tone(pin, toneFrequency);
        isPlaying = true;
        beepState = 1;
      }
    }

    void update() {
      if (isPlaying && (millis() - toneStartTime >= toneDuration)) {
        noTone(pin);
        isPlaying = false;
        if (beepState == 1) {
          beepInterval = millis();
          beepState = 2;
        } else if (beepState == 2 && (millis() - beepInterval >= 200)) {
          toneFrequency = 852;
          toneDuration = 300;
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
    speaker.oneBeep();
  }
};


class Interface {
private:
  unsigned short menuStatus;
  const unsigned short holdTime = 1000;
  const unsigned short holdTime2 = 3000;
  const unsigned long settingsTimeout = 5000;
  unsigned int lastPushButton;
  unsigned long pumpStartTime;
  unsigned long pumpStartWaitingTime;
  unsigned long pumpRunTime;
  unsigned long remainingTime;
  unsigned long pumpWaiting;
  unsigned long pumpWaitingRemain;
  unsigned long lastColonToggle;
  unsigned long startTime;
  bool pumpRunning;
  bool colonState;
  
public:

  Interface() : menuStatus(MAIN_MENU), lastPushButton(0), pumpStartTime(0),
  pumpStartWaitingTime(0), pumpRunTime(10000), remainingTime(0), 
  pumpWaiting(600000), pumpWaitingRemain(0), pumpRunning(false),
  colonState(true), lastColonToggle(0), startTime(0) {}

  const uint8_t ON[] = {
  0, 0,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // O
  SEG_C | SEG_E | SEG_G                         // n
  };
  bool checkButtonsHold();
  bool checkButtonHoldLeft();
  bool checkButtonHoldRight();
  void updateDisplayRunningTime();
  void updateDisplayPumpTime(unsigned long pumpRunTime);
  void updateDisplayWaitingTime(unsigned long displayTimeValue);
};

bool Interface::checkButtonsHold() {
  if (digitalRead(BUTTON_LEFT) == LOW && digitalRead(BUTTON_RIGHT) == LOW) {
    startTime = millis();
    while (millis() - startTime < holdTime) {
      if (digitalRead(BUTTON_LEFT) != LOW || digitalRead(BUTTON_RIGHT) != LOW) {
        return false;
      }
      delay(50);
    }
    return true;
  }
  return false;
};

bool Interface::checkButtonHoldLeft() {
  if (digitalRead(BUTTON_LEFT) == LOW && menuStatus == MAIN_MENU) {
    startTime = millis();
    while (millis() - startTime < holdTime2) {
      if (digitalRead(BUTTON_LEFT) != LOW) {
        return false;
      }
      delay(50);
    }
    return true;
  }
  return false;
};

bool Interface::checkButtonHoldRight() {
  if (digitalRead(BUTTON_RIGHT) == LOW && menuStatus == MAIN_MENU) {
    startTime = millis();
    while (millis() - startTime < holdTime2) {
      if (digitalRead(BUTTON_RIGHT) != LOW) {
        return false;
      }
      delay(50);
    }
    return true;
  }
  return false;
};

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
  if (totalSeconds < 3600) {
    displayTime = minutes * 100 + seconds;
  } else if (totalSeconds < 86400) {
    displayTime = hours * 100 + minutes;
  } else {
    displayTime = (days % 100) * 100 + (hours % 24);
  }
  if (displayTime > 9923 && totalSeconds >= 86400) {
    displayTime = 9923;
  } else if (displayTime > 9959) {
    displayTime = 9959;
  }
  if (menuStatus == INTERVAL_MENU) {
    if (millis() - lastColonToggle >= 500) {
      colonState = !colonState;
      lastColonToggle = millis();
    }
    display.showNumberDecEx(displayTime, colonState ? 0b01000000 : 0, true);
  } else {
    display.showNumberDecEx(displayTime, 0b01000000, true);
  }
};

class SystemManagement {
  public:
    void checkDoubleButtons();
    void checkOneButton();
    void runtimeMenu();
    void intervalMenu();
    void timers();
};

void SystemManagement::checkDoubleButtons() {
  if (interface.checkButtonsHold()) {
    if (!interface.pumpRunning) {
      pump.turnOn();
      speaker.twoBeep();
      interface.pumpRunning = true;
      interface.pumpStartTime = millis();
      interface.pumpStartWaitingTime = millis() + interface.pumpRunTime;
      interface.remainingTime = interface.pumpRunTime;
    } else {
      pump.turnOff();
      speaker.oneBeep();
      interface.pumpRunning = false;
      interface.pumpStartWaitingTime = millis();
    }
    interface.menuStatus = MAIN_MENU;
    delay(200);
  }
};

void SystemManagement::checkOneButton() {
  if (interface.checkButtonHoldLeft() && !interface.pumpRunning) {
    interface.menuStatus = RUNTIME_MENU;
    interface.lastPushButton = millis();
    interface.colonState = true;
    interface.lastColonToggle = millis();
  }

  if (interface.checkButtonHoldRight() && !interface.pumpRunning) {
    interface.menuStatus = INTERVAL_MENU;
    interface.lastPushButton = millis();
    interface.colonState = true;
    interface.lastColonToggle = millis();
  }
};

void SystemManagement::runtimeMenu() {
  if (interface.menuStatus == RUNTIME_MENU) {
    if (millis() - interface.lastPushButton < interface.settingsTimeout) {
      if (digitalRead(BUTTON_LEFT) == LOW && millis() - interface.lastPushButton >= 100) {
        interface.pumpRunTime = (interface.pumpRunTime >= 1000) ? interface.pumpRunTime - 1000 : 5999000;
        interface.lastPushButton = millis();
      }
      if (digitalRead(BUTTON_RIGHT) == LOW && millis() - interface.lastPushButton >= 100) {
        interface.pumpRunTime = (interface.pumpRunTime <= 5999000) ? interface.pumpRunTime + 1000 : 1000;
        interface.lastPushButton = millis();
      }
      interface.updateDisplayPumpTime(interface.pumpRunTime);
    } else {
      interface.menuStatus = MAIN_MENU;
      interface.colonState = true;
      interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
    }
  }
};

void SystemManagement::intervalMenu() {
  if (interface.menuStatus == INTERVAL_MENU) {
    if (millis() - interface.lastPushButton < interface.settingsTimeout) {
      if (digitalRead(BUTTON_LEFT) == LOW && millis() - interface.lastPushButton >= 100) {
        interface.pumpWaiting = (interface.pumpWaiting >= 60000) ? interface.pumpWaiting - 60000 : 8639999000;
        interface.lastPushButton = millis();
      }
      if (digitalRead(BUTTON_RIGHT) == LOW && millis() - interface.lastPushButton >= 100) {
        interface.pumpWaiting = (interface.pumpWaiting <= 8639999000) ? interface.pumpWaiting + 60000 : 60000;
        interface.lastPushButton = millis();
      }
      interface.updateDisplayWaitingTime(interface.pumpWaiting);
    } else {
      interface.menuStatus = MAIN_MENU;
      interface.colonState = true;
      interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
    }
  }
};

void SystemManagement::timers() {
  if (!interface.pumpRunning && millis() - interface.pumpStartWaitingTime >= interface.pumpWaiting) {
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
      speaker.oneBeep();
      interface.pumpRunning = false;
      interface.pumpStartWaitingTime = millis();
    }
  } else if (interface.menuStatus == MAIN_MENU) {
    interface.pumpWaitingRemain = (interface.pumpWaiting > (millis() - interface.pumpStartWaitingTime))
                                  ? (interface.pumpWaiting - (millis() - interface.pumpStartWaitingTime)) : 0;
    interface.updateDisplayWaitingTime(interface.pumpWaitingRemain);
  }
};


TM1637Display display(CLK, DIO);
Speaker speaker(SPEAKER_PIN);
Pump pump(PUMP_PIN);
Interface interface;
SystemManagement systemManager;


void setup() {
  //Display setup
  display.setBrightness(7);
  display.clear();
  display.setSegments(interface.ON);
  //Pins setup
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  speaker.oneBeep();
}


void loop() {
  speaker.update();
  systemManager.checkDoubleButtons();
  systemManager.checkOneButton();
  systemManager.runtimeMenu();
  systemManager.intervalMenu();
  systemManager.timers();
}