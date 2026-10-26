
/*
  Arduino Nano 情緒表達裝置
  共陽 RGB LED + 按鈕

  R = D9
  G = D10
  B = D11
  Button = D2
  按鈕另一端接 GND
*/

// ===== 腳位設定 =====
const int R_PIN = 9;
const int G_PIN = 10;
const int B_PIN = 11;
const int BUTTON_PIN = 2;

// ===== 時間設定 =====
const unsigned long DEBOUNCE_TIME = 30;
const unsigned long CALM_TIME = 3000;
const unsigned long SAD_FADE_TIME = 2000;
const unsigned long SAD_TIME = 3000;
const unsigned long ANGRY_FADE_TIME = 3000;
const unsigned long ANGRY_TIME = 3000;
const unsigned long FLASH_TIME = 1000;
const unsigned long TOTAL_FLASH_TIME = 3000;
const unsigned long SPECIAL_COLOR_TIMEOUT = 5000;

// ===== 情緒狀態 =====
enum EmotionState {
  IDLE,
  CALM,
  SAD_FADE,
  SAD,
  ANGRY_FADE,
  ANGRY,
  FLASH
};

EmotionState state = IDLE;

unsigned long stateStartTime = 0;
unsigned long lastPressTime = 0;

// ===== 按鈕變數 =====
int buttonReading = HIGH;
int stableButtonState = HIGH;
int lastButtonReading = HIGH;

unsigned long debounceStartTime = 0;

bool buttonIsPressed = false;
bool newPress = false;

// ===== 按鈕次數 =====
int pressCount = 0;

// ===== 顏色設定 =====
struct Color {
  int r;
  int g;
  int b;
};

const Color OFF_COLOR    = {0, 0, 0};
const Color GREEN_COLOR  = {0, 255, 0};
const Color BLUE_COLOR   = {0, 0, 255};
const Color RED_COLOR    = {255, 0, 0};
const Color PURPLE_COLOR = {160, 0, 255};
const Color ORANGE_COLOR = {255, 100, 0};
const Color YELLOW_COLOR = {255, 255, 0};
const Color WHITE_COLOR  = {255, 255, 255};

// ===== LED 顏色輸出：共陽 LED =====
void setColor(Color c) {
  analogWrite(R_PIN, 255 - constrain(c.r, 0, 255));
  analogWrite(G_PIN, 255 - constrain(c.g, 0, 255));
  analogWrite(B_PIN, 255 - constrain(c.b, 0, 255));
}

// ===== 顏色漸變 =====
Color fadeColor(
  Color from,
  Color to,
  unsigned long elapsed,
  unsigned long duration
) {
  if (elapsed >= duration) {
    return to;
  }

  float progress = (float)elapsed / duration;

  Color result;
  result.r = from.r + (to.r - from.r) * progress;
  result.g = from.g + (to.g - from.g) * progress;
  result.b = from.b + (to.b - from.b) * progress;

  return result;
}

// ===== 根據按鈕次數取得顏色 =====
Color getDisplayColor(Color moodColor) {
  if (pressCount > 30) {
    return WHITE_COLOR;
  }

  if (pressCount > 20) {
    return PURPLE_COLOR;
  }

  if (pressCount > 10) {
    return YELLOW_COLOR;
  }

  return moodColor;
}

// ===== 判斷是否正在顯示特殊次數顏色 =====
bool isSpecialColorActive() {
  return pressCount > 10;
}

// ===== 切換情緒狀態 =====
void changeState(EmotionState newState) {
  state = newState;
  stateStartTime = millis();
}

// ===== 按鈕按下事件 =====
void handleButtonPress() {
  pressCount++;
  lastPressTime = millis();

  // 每次按下都重新回到平靜狀態
  changeState(CALM);
}

// ===== 按鈕防彈跳 =====
void readButton() {
  newPress = false;

  buttonReading = digitalRead(BUTTON_PIN);

  if (buttonReading != lastButtonReading) {
    debounceStartTime = millis();
  }

  if (millis() - debounceStartTime >= DEBOUNCE_TIME) {
    if (buttonReading != stableButtonState) {
      stableButtonState = buttonReading;

      if (stableButtonState == LOW) {
        buttonIsPressed = true;
        newPress = true;
      } else {
        buttonIsPressed = false;
      }
    }
  }

  lastButtonReading = buttonReading;
}

// ===== 初始化 =====
void setup() {
  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setColor(OFF_COLOR);
}

// ===== 主程式 =====
void loop() {
  unsigned long now = millis();

  // 1. 偵測按鈕
  readButton();

  // 2. 有新按下事件就更新次數與情緒
  if (newPress) {
    handleButtonPress();
  }

  now = millis();

  // 3. 黃、紫、白燈：5 秒沒有新按下事件就回到綠燈
  if (
    isSpecialColorActive() &&
    !buttonIsPressed &&
    now - lastPressTime >= SPECIAL_COLOR_TIMEOUT
  ) {
    pressCount = 0;
    changeState(CALM);
  }

  unsigned long elapsed = now - stateStartTime;

  // 4. 情緒狀態機
  Color emotionColor = OFF_COLOR;

  switch (state) {

    // 初始：熄燈
    case IDLE:
      emotionColor = OFF_COLOR;
      break;

    // 平靜：綠燈 3 秒
    case CALM:
      emotionColor = GREEN_COLOR;

      if (elapsed >= CALM_TIME) {
        changeState(SAD_FADE);
      }
      break;

    // 難過：綠色漸變藍色，2 秒
    case SAD_FADE:
      emotionColor = fadeColor(
        GREEN_COLOR,
        BLUE_COLOR,
        elapsed,
        SAD_FADE_TIME
      );

      if (elapsed >= SAD_FADE_TIME) {
        changeState(SAD);
      }
      break;

    // 藍燈維持 3 秒
    case SAD:
      emotionColor = BLUE_COLOR;

      if (elapsed >= SAD_TIME) {
        changeState(ANGRY_FADE);
      }
      break;

    // 生氣：藍色漸變紅色，3 秒
    case ANGRY_FADE:
      emotionColor = fadeColor(
        BLUE_COLOR,
        RED_COLOR,
        elapsed,
        ANGRY_FADE_TIME
      );

      if (elapsed >= ANGRY_FADE_TIME) {
        changeState(ANGRY);
      }
      break;

    // 紅燈維持 3 秒
    case ANGRY:
      emotionColor = RED_COLOR;

      if (elapsed >= ANGRY_TIME) {
        changeState(FLASH);
      }
      break;

    // 紅、紫交替，共 3 秒
    case FLASH:
      if (elapsed >= TOTAL_FLASH_TIME) {
        pressCount = 0;
        changeState(IDLE);
        emotionColor = OFF_COLOR;
      } else {
        unsigned long phase = (elapsed / FLASH_TIME) % 2;

        if (phase == 0) {
          emotionColor = RED_COLOR;
        } else {
          emotionColor = PURPLE_COLOR;
        }
      }
      break;
  }

  // 5. 顯示顏色
  // 按住按鈕時，橘燈優先
  if (buttonIsPressed && state != IDLE) {
    setColor(ORANGE_COLOR);
  } else if (state == IDLE) {
    setColor(OFF_COLOR);
  } else {
    setColor(getDisplayColor(emotionColor));
  }
}
