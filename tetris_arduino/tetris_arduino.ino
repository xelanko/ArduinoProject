const uint8_t DATA_PIN  = 6;
const uint8_t LATCH_PIN = 5;
const uint8_t CLOCK_PIN = 4;
const uint8_t BTN_RIGHT = 10;
const uint8_t BTN_LEFT  = 9;
const uint8_t BTN_ROT   = 8;
const uint8_t LED_PIN   = 11;
const uint8_t BUZZ_PIN  = A4;

const unsigned long FALL_INTERVAL  = 600;
const unsigned long DISPLAY_ROW_US = 800;
const unsigned long DEBOUNCE_MS    = 150;
const unsigned long FEEDBACK_ON_MS = 100;
const unsigned long FEEDBACK_OFF_MS = 80;
const unsigned long FLASH_INTERVAL = 120;

uint8_t board[8];

struct Point { int8_t r, c; };

const Point PIECES[7][4][4] = {
  {
    { {0,0},{0,1},{0,2},{0,3} },
    { {0,0},{1,0},{2,0},{3,0} },
    { {0,0},{0,1},{0,2},{0,3} },
    { {0,0},{1,0},{2,0},{3,0} },
  },
  {
    { {0,0},{0,1},{1,0},{1,1} },
    { {0,0},{0,1},{1,0},{1,1} },
    { {0,0},{0,1},{1,0},{1,1} },
    { {0,0},{0,1},{1,0},{1,1} },
  },
  {
    { {0,1},{1,0},{1,1},{1,2} },
    { {0,0},{1,0},{2,0},{1,1} },
    { {1,0},{1,1},{1,2},{0,1} },
    { {0,1},{1,1},{2,1},{1,0} },
  },
  {
    { {0,1},{0,2},{1,0},{1,1} },
    { {0,0},{1,0},{1,1},{2,1} },
    { {0,1},{0,2},{1,0},{1,1} },
    { {0,0},{1,0},{1,1},{2,1} },
  },
  {
    { {0,0},{0,1},{1,1},{1,2} },
    { {0,1},{1,0},{1,1},{2,0} },
    { {0,0},{0,1},{1,1},{1,2} },
    { {0,1},{1,0},{1,1},{2,0} },
  },
  {
    { {0,0},{1,0},{2,0},{2,1} },
    { {0,0},{0,1},{0,2},{1,0} },
    { {0,0},{0,1},{1,1},{2,1} },
    { {0,2},{1,0},{1,1},{1,2} },
  },
  {
    { {0,1},{1,1},{2,1},{2,0} },
    { {0,0},{1,0},{1,1},{1,2} },
    { {0,0},{0,1},{1,0},{2,0} },
    { {0,0},{0,1},{0,2},{1,2} },
  },
};

int8_t  pieceType;
int8_t  pieceRot;
int8_t  pieceRow;
int8_t  pieceCol;

unsigned long lastFall       = 0;
unsigned long lastBtnRTime   = 0;
unsigned long lastBtnLTime   = 0;
unsigned long lastBtnRotTime = 0;
bool          lastBtnRRead   = HIGH;
bool          lastBtnLRead   = HIGH;
bool          lastBtnRotRead = HIGH;

uint8_t       displayRow  = 0;
unsigned long lastRowTime = 0;

uint8_t       gameOverFlash = 0;
unsigned long lastFlashTime = 0;
bool          gameOverPhase = false;

uint8_t       feedbackStep    = 0;
unsigned long lastFeedbackTime = 0;
bool          feedbackActive  = false;
bool          feedbackPhase   = false;

void printBoard() {
  uint8_t frame[8];
  for (uint8_t r = 0; r < 8; r++) frame[r] = board[r];
  for (uint8_t i = 0; i < 4; i++) {
    int8_t pr = pieceRow + PIECES[pieceType][pieceRot][i].r;
    int8_t pc = pieceCol + PIECES[pieceType][pieceRot][i].c;
    if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8)
      frame[pr] |= (0x80 >> pc);
  }
  Serial.println(F("+--------+"));
  for (uint8_t r = 0; r < 8; r++) {
    Serial.print('|');
    for (int8_t c = 7; c >= 0; c--)
      Serial.print((frame[r] & (1 << c)) ? '#' : '.');
    Serial.println('|');
  }
  Serial.println(F("+--------+"));
  Serial.print(F("Piece: ")); Serial.print(pieceType);
  Serial.print(F("  rot: ")); Serial.print(pieceRot);
  Serial.print(F("  row: ")); Serial.print(pieceRow);
  Serial.print(F("  col: ")); Serial.println(pieceCol);
  Serial.println();
}

void sendToMatrix(uint8_t rowBit, uint8_t colMask) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, rowBit);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, colMask);
  digitalWrite(LATCH_PIN, HIGH);
}

void refreshDisplay() {
  unsigned long now = micros();
  if (now - lastRowTime < DISPLAY_ROW_US) return;
  lastRowTime = now;

  uint8_t frame[8];
  for (uint8_t r = 0; r < 8; r++) frame[r] = board[r];
  for (uint8_t i = 0; i < 4; i++) {
    int8_t pr = pieceRow + PIECES[pieceType][pieceRot][i].r;
    int8_t pc = pieceCol + PIECES[pieceType][pieceRot][i].c;
    if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8)
      frame[pr] |= (0x80 >> pc);
  }

  sendToMatrix(0, 0xFF);
  if (frame[displayRow] != 0)
    sendToMatrix(1 << displayRow, ~frame[displayRow]);

  displayRow = (displayRow + 1) % 8;
}

bool isValid(int8_t testRow, int8_t testCol, int8_t testRot) {
  for (uint8_t i = 0; i < 4; i++) {
    int8_t r = testRow + PIECES[pieceType][testRot][i].r;
    int8_t c = testCol + PIECES[pieceType][testRot][i].c;
    if (c < 0 || c > 7) return false;
    if (r > 7)          return false;
    if (r >= 0 && (board[r] & (0x80 >> c))) return false;
  }
  return true;
}

void lockPiece() {
  for (uint8_t i = 0; i < 4; i++) {
    int8_t r = pieceRow + PIECES[pieceType][pieceRot][i].r;
    int8_t c = pieceCol + PIECES[pieceType][pieceRot][i].c;
    if (r >= 0 && r < 8) board[r] |= (0x80 >> c);
  }
}

void clearLines() {
  for (int8_t r = 7; r >= 0; r--) {
    if (board[r] == 0xFF) {
      for (int8_t above = r; above > 0; above--)
        board[above] = board[above - 1];
      board[0] = 0x00;
      r++;
    }
  }
}

void tickFeedback() {
  if (!feedbackActive) return;
  unsigned long now = millis();
  unsigned long interval = feedbackPhase ? FEEDBACK_ON_MS : FEEDBACK_OFF_MS;
  if (now - lastFeedbackTime < interval) return;
  lastFeedbackTime = now;

  if (feedbackPhase) {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZ_PIN);
    feedbackPhase = false;
    feedbackStep++;
    if (feedbackStep >= 3) {
      feedbackActive = false;
      gameOverFlash = 0;
      gameOverPhase = false;
      lastFlashTime = now;
    }
  } else {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZ_PIN, 440);
    feedbackPhase = true;
  }
}

void spawnPiece() {
  pieceType = random(7);
  pieceRot  = 0;
  pieceRow  = 0;
  pieceCol  = 2;

  if (!isValid(pieceRow, pieceCol, pieceRot)) {
    for (uint8_t r = 0; r < 8; r++) board[r] = 0;
    Serial.println(F("=== GAME OVER — board reset ==="));
    feedbackStep    = 0;
    feedbackPhase   = false;
    feedbackActive  = true;
    lastFeedbackTime = millis();
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZ_PIN, 440);
    feedbackPhase = true;
    return;
  }

  Serial.print(F("Spawned piece "));
  Serial.print(pieceType);
  Serial.print(F("  rot: "));
  Serial.println(pieceRot);
  printBoard();
}

void setup() {
  pinMode(DATA_PIN,  OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_ROT,   INPUT_PULLUP);
  pinMode(LED_PIN,   OUTPUT);
  pinMode(BUZZ_PIN,  OUTPUT);

  Serial.begin(9600);
  randomSeed(analogRead(A0));

  for (uint8_t r = 0; r < 8; r++) board[r] = 0;
  spawnPiece();
  lastFall    = millis();
  lastRowTime = micros();
}

void loop() {
  unsigned long now = millis();

  tickFeedback();
  if (feedbackActive) return;

  if (gameOverFlash < 12) {
    if (now - lastFlashTime >= FLASH_INTERVAL) {
      lastFlashTime = now;
      if (gameOverPhase) {
        sendToMatrix(0xFF, 0x00);
        digitalWrite(LED_PIN, HIGH);
        tone(BUZZ_PIN, 300);
      } else {
        sendToMatrix(0x00, 0xFF);
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZ_PIN);
      }
      gameOverPhase = !gameOverPhase;
      gameOverFlash++;
      if (gameOverFlash == 12) {
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZ_PIN);
        spawnPiece();
      }
    }
    return;
  }

  refreshDisplay();

  bool btnRRead = digitalRead(BTN_RIGHT);
  if (lastBtnRRead == HIGH && btnRRead == LOW) {
    if (now - lastBtnRTime > DEBOUNCE_MS) {
      lastBtnRTime = now;
      if (isValid(pieceRow, pieceCol + 1, pieceRot)) {
        pieceCol++;
        Serial.print(F("Move right -> col: "));
        Serial.println(pieceCol);
        printBoard();
      } else {
        Serial.println(F("Move right blocked"));
      }
    }
  }
  lastBtnRRead = btnRRead;

  bool btnLRead = digitalRead(BTN_LEFT);
  if (lastBtnLRead == HIGH && btnLRead == LOW) {
    if (now - lastBtnLTime > DEBOUNCE_MS) {
      lastBtnLTime = now;
      if (isValid(pieceRow, pieceCol - 1, pieceRot)) {
        pieceCol--;
        Serial.print(F("Move left -> col: "));
        Serial.println(pieceCol);
        printBoard();
      } else {
        Serial.println(F("Move left blocked"));
      }
    }
  }
  lastBtnLRead = btnLRead;

  bool btnRotRead = digitalRead(BTN_ROT);
  if (lastBtnRotRead == HIGH && btnRotRead == LOW) {
    if (now - lastBtnRotTime > DEBOUNCE_MS) {
      lastBtnRotTime = now;
      int8_t nextRot = (pieceRot + 1) % 4;
      if (isValid(pieceRow, pieceCol, nextRot)) {
        pieceRot = nextRot;
        Serial.print(F("Rotated -> rot: "));
        Serial.println(pieceRot);
        printBoard();
      } else if (isValid(pieceRow, pieceCol - 1, nextRot)) {
        pieceCol--;
        pieceRot = nextRot;
        Serial.println(F("Rotated with left kick"));
        printBoard();
      } else if (isValid(pieceRow, pieceCol + 1, nextRot)) {
        pieceCol++;
        pieceRot = nextRot;
        Serial.println(F("Rotated with right kick"));
        printBoard();
      } else {
        Serial.println(F("Rotation blocked"));
      }
    }
  }
  lastBtnRotRead = btnRotRead;

  if (now - lastFall >= FALL_INTERVAL) {
    lastFall = now;
    if (isValid(pieceRow + 1, pieceCol, pieceRot)) {
      pieceRow++;
    } else {
      lockPiece();
      clearLines();
      spawnPiece();
    }
  }
}
