# ArduinoProject
Tetris on an 8×8 LED matrix, driven by Arduino Uno and two 74HC595 shift registers.

# Arduino Tetris on an 8×8 LED Matrix

A playable Tetris game running on an Arduino Uno, displayed on an 8×8 LED matrix driven by two 74HC595 shift registers. Includes three control buttons, a buzzer, and a status LED for game-over feedback.

![Demo](docs/demo.gif)

> **Simulation:** [Open the project in Tinkercad](https://www.tinkercad.com/things/54RjZM6j1tC-arduinocircuit?sharecode=nqeSydInjYMYq-jCfSDvliAgFJQ5fIKrk50URqtUgTM)

## Circuit

![Circuit diagram](docs/circuit.png)

## Features

- All 7 standard tetrominoes with 4 rotation states each
- Collision detection against walls and locked pieces
- Rotation with simple wall kicks (the piece is shifted left or right if rotation is blocked)
- Line clearing
- Game over detection: the board resets, the matrix flashes, and the LED and buzzer signal the end of the game
- Non-blocking game loop based on `millis()` / `micros()`, with no `delay()` calls
- Debounced button input with edge detection
- Debug output: the board is printed as ASCII art to the Serial Monitor

## Components

| Component | Quantity |
|---|---|
| Arduino Uno | 1 |
| 74HC595 shift register | 2 |
| 8×8 LED matrix (or 64 individual LEDs) | 1 |
| Resistors for the matrix | 8 |
| Push buttons | 3 |
| Buzzer | 1 |
| Status LED + resistor | 1 |
| Breadboard and jumper wires | – |

## Pin Configuration

| Function | Arduino pin |
|---|---|
| Shift register DATA | 6 |
| Shift register LATCH | 5 |
| Shift register CLOCK | 4 |
| Button: move right | 10 |
| Button: move left | 9 |
| Button: rotate | 8 |
| Status LED | 11 |
| Buzzer | A4 |

Buttons use the internal pull-up resistors (`INPUT_PULLUP`), so each button connects its pin to GND.

## Controls

| Button | Action |
|---|---|
| Left | Move piece left |
| Right | Move piece right |
| Rotate | Rotate piece (with wall kick if needed) |

Pieces fall automatically every 600 ms.

## How It Works

**Display multiplexing.** The matrix is refreshed one row at a time. Two chained 74HC595 shift registers control it: one selects the active row, the other sets which columns are lit. The refresh function runs continuously from the main loop, so the picture appears steady to the eye.

**Board representation.** Each of the 8 board rows is stored as a single `uint8_t`, one bit per cell. Collision checks, locking pieces and clearing lines are done with bitwise operations.

**Pieces.** All tetrominoes and their rotations are stored in a lookup table of relative cell coordinates (`PIECES[type][rotation][cell]`).

**Timing.** Falling, button reading, display refreshing and the game-over animation all run on timers (`millis()` / `micros()`), so none of them block each other.

**Input.** Each button is read on a falling edge (HIGH → LOW) with a 150 ms debounce time.

**Game over.** When a new piece cannot be placed, the board is cleared and the matrix flashes 12 times while the LED and buzzer signal the end of the game. Then a new game starts automatically.

## Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/YOUR_USERNAME/arduino-tetris.git
   ```
2. Build the circuit as shown in `docs/circuit.png`, or open the Tinkercad simulation.
3. Open `tetris_arduino/tetris_arduino.ino` in the Arduino IDE.
4. Select **Arduino Uno** as the board and the correct port.
5. Upload the sketch.
6. (Optional) Open the Serial Monitor at **9600 baud** to see the board printed in real time.

## Project Structure

```
arduino-tetris/
├── README.md
├── LICENSE
├── tetris_arduino/
│   └── tetris_arduino.ino
└── docs/
    ├── circuit.png
    └── demo.gif
```

GitHub: [@YOUR_USERNAME](https://github.com/YOUR_USERNAME)
