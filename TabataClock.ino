#include <CountDown.h>
#include <FastLED.h>

#define NUMBER_OF_DIGITS 2
#define LEDS_PER_SEGMENT 5
#define LED_DISPLAY_PIN A1

const int NUM_LEDS = NUMBER_OF_DIGITS * LEDS_PER_SEGMENT * 7;
int blink = 0;
int blink_count = 1;

const int segments[][8] = {
    {1, 1, 1, 1, 1, 1, 0, 0}, // 0
    {1, 0, 0, 0, 0, 1, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1, 0}, // 2
    {1, 1, 0, 0, 1, 1, 1, 0}, // 3
    {1, 0, 1, 0, 0, 1, 1, 0}, // 4
    {0, 1, 1, 0, 1, 1, 1, 0}, // 5
    {0, 1, 1, 1, 1, 1, 1, 0}, // 6
    {1, 1, 0, 0, 0, 1, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1, 0}, // 8
    {1, 1, 1, 0, 1, 1, 1, 0}  // 9

};

// setup buttons
const int BUTTON_COUNT = 4;
int buttons[BUTTON_COUNT] = {PD5, PD6, PD7, PD4};
int buttonState[BUTTON_COUNT] = {LOW, LOW, LOW, LOW};

enum State
{
  DEMO,
  SETUP_UP,
  SETUP_DOWN,
  PAUSED,
  COUNTING
};
State CurrentState;

unsigned long lastUpdateMills = 0;
unsigned int refreshRate = 50;
unsigned long demoLength = 3000;

// display variables
CRGB leds[NUM_LEDS];

CRGB upColor = CRGB::Red;
CRGB downColor = CRGB::Green;
CRGB pauseColor = CRGB::Yellow;
bool isBlinking = false;
unsigned long blinkMills = 0;

class DisplayState
{
private:
  int _previousDisplayValue;
  CRGB _prevoidDisplayColor;
  bool _previousIsEnabled;

public:
  int displayValue;
  CRGB displayColor;
  bool isEnabled;
  void reset()
  {
    _previousDisplayValue = displayValue;
    _prevoidDisplayColor = displayColor;
    _previousIsEnabled = isEnabled;
  }
  bool hasChanged()
  {
    return _previousDisplayValue != displayValue || _prevoidDisplayColor != displayColor || _previousIsEnabled != isEnabled;
  }
};

DisplayState displayState = DisplayState();

// counter variables
unsigned long upTime = 20000;
unsigned long downTime = 10000;
CountDown timer(CountDown::MILLIS);
bool isCountingUp = false;

// beeper
int buzzerPin = A3;
unsigned long beepEnd;
int beepCount = 0;

void setup()
{
  Serial.begin(9600);

  // setup button inputs
  pinMode(buttons[0], INPUT_PULLUP);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(buttons[3], INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);

  FastLED.addLeds<WS2852, LED_DISPLAY_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  displayState.isEnabled = true;
  CurrentState = DEMO;

  delay(500);
  beep(250);
}

void loop()
{
  delay(10);
  handleButtons();
  switch (CurrentState)
  {
  case DEMO:
    if (demoLength > millis())
    {
    }
    else
    {
      CurrentState = SETUP_UP;
    }
    break;
  case SETUP_UP:
    displayState.displayColor = upColor;
    displayMillis(upTime);
    isBlinking = true;
    break;
  case SETUP_DOWN:
    displayState.displayColor = downColor;
    displayMillis(downTime);
    isBlinking = true;
    break;
  case PAUSED:
    isBlinking = false;
    displayState.displayColor = pauseColor;
    break;
  case COUNTING:
    if (timer.remaining() <= 3000 && beepCount == 0)
    {
      beepCount = 4;
    }

    if ((timer.remaining() <= 3000 && beepCount == 4) || (timer.remaining() <= 2000 && beepCount == 3) || (timer.remaining() <= 1000 && beepCount == 2) || (timer.remaining() == 0 && beepCount == 1))
    {
      beepCount -= 1;
      if (beepCount == 0)
      {
        beep(1000);
      }
      else
      {
        beep(250);
      }
    }

    if (timer.remaining() == 0)
    {
      // timer got down to 0
      if (isCountingUp)
      {
        timer.start(downTime);
        isCountingUp = false;
      }
      else
      {
        timer.start(upTime);
        isCountingUp = true;
      }
    }
    if (isCountingUp)
    {
      displayState.displayColor = upColor;
    }
    else
    {
      displayState.displayColor = downColor;
    }

    displayMillis(timer.remaining());
    break;
  }

  handleBeep();
  RefreshView();
}

void beep(int mill)
{
  beepEnd = millis() + mill;
  tone(buzzerPin, 4000);
}

void handleBeep()
{
  if (beepEnd != 0)
  {
    if (millis() > beepEnd)
    {
      noTone(buzzerPin);
      beepEnd = 0;
      digitalWrite(buzzerPin, HIGH);
    }
  }
}

void displayMillis(unsigned long milis)
{
  displayState.displayValue = milis / 1000 + !!(milis % 1000);
}

void RefreshView()
{
  if (lastUpdateMills + refreshRate < millis() || lastUpdateMills > millis())
  {
    lastUpdateMills = millis();

    if (displayState.hasChanged())
    {
      Serial.println(displayState.displayValue);

      for (int i = 0; i < NUM_LEDS; i++)
      {
        if (displayState.isEnabled)
        {
          int digitIndex = i / (7 * LEDS_PER_SEGMENT);
          int segmentIndex = (i / LEDS_PER_SEGMENT) % 7;
          int digitValue = (displayState.displayValue % (int)pow(10, digitIndex + 1)) / pow(10, digitIndex);
          if (segments[digitValue][segmentIndex] == 1)
          {
            leds[i] = displayState.displayColor;
          }
          else
          {
            leds[i] = CRGB::Black;
          }
        }
        else
        {
          leds[i] = CRGB::Black;
        }
      }
      FastLED.show();
      displayState.reset();
    }
  }
}

void handleButtons()
{
  if (handleButtonAction(0))
  {
    Serial.println("Button UP pressed...");
    beep(100);
    switch (CurrentState)
    {
    case SETUP_UP:
      if (upTime < 95000)
      {
        upTime = upTime + 5000;
      }
      break;
    case SETUP_DOWN:
      if (downTime < 95000)
      {
        downTime = downTime + 5000;
      }
      break;
    case DEMO:
    case PAUSED:
    case COUNTING:
      break;
    }
  }
  if (handleButtonAction(1))
  {
    Serial.println("Button DOWN pressed...");
    beep(100);
    switch (CurrentState)
    {
    case SETUP_UP:
      if (upTime > 5000)
      {
        upTime = upTime - 5000;
      }
      break;
    case SETUP_DOWN:
      if (downTime > 5000)
      {
        downTime = downTime - 5000;
      }
      break;
    case DEMO:
    case PAUSED:
    case COUNTING:
      break;
    }
  }
  if (handleButtonAction(2))
  {
    Serial.println("Button START pressed...");
    beep(100);
    switch (CurrentState)
    {
    case SETUP_UP:
      CurrentState = SETUP_DOWN;
      break;
    case SETUP_DOWN:
      CurrentState = PAUSED;
      isCountingUp = false;
      timer.start(downTime);
      timer.stop();
      break;
    case PAUSED:
      CurrentState = COUNTING;
      timer.cont();
      break;
    case COUNTING:
      Serial.println("pause");
      CurrentState = PAUSED;
      //    isBeepCountActive = false;
      timer.stop();
      break;
    case DEMO:
      break;
    }
  }
  if (handleButtonAction(3))
  {
    Serial.println("Button RESET pressed...");
    beep(100);
    timer.stop();
    CurrentState = DEMO;
  }
}

bool handleButtonAction(int buttonIndex)
{
  if (buttonIndex < 0 || buttonIndex >= BUTTON_COUNT)
  {
    // Invalid button index
    return false;
  }

  int buttonPin = buttons[buttonIndex];

  // Read the button state
  int reading = digitalRead(buttonPin);

  // Check for debouncing
  if (reading != buttonState[buttonIndex])
  {
    delay(5);                         // Adjust the delay based on your requirements
    reading = digitalRead(buttonPin); // Read again

    // If still different, update the state
    if (reading != buttonState[buttonIndex])
    {
      buttonState[buttonIndex] = reading;
      return (buttonState[buttonIndex] == LOW);
    }
  }

  return false;
}
