# Arduino Morse Trainer

A morse code training device with real-time display feedback. Features an OLED display for immediate character decoding and adjustable timing parameters for customized practice sessions.

## Features

- Real-time morse code decoding with OLED display
- Adjustable speed range: 5-50 WPM
- Configurable tone frequency: 400-1000 Hz
- Support for alphanumeric characters (A-Z, 0-9)
- Compact form factor with single-board design

## Hardware Requirements

- Arduino microcontroller (compatible with Arduino Uno)
- SSD1306 OLED display module (128x64 resolution)
- Rotary encoder with integrated push button
- Dual-contact paddle or separate dit/dah switches
- Piezo buzzer or audio transducer
- Interconnecting wires and breadboard/prototype board

## Pin Configuration

- `ENCODER_PIN_A`: Digital pin 2 (interrupt-capable)
- `ENCODER_PIN_B`: Digital pin 3 (interrupt-capable)
- `DIT_PIN`: Digital pin 4 (dit contact)
- `DAH_PIN`: Digital pin 5 (dah contact)
- `BUZZER_PIN`: Digital pin 6 (audio output)
- `ENCODER_BTN`: Digital pin 7 (menu control)
- OLED Display: I2C interface (SDA/SCL)

## Software Requirements

Required Arduino libraries:
- Wire.h (included with Arduino IDE)
- U8glib (OLED display driver)
- RotaryEncoder (encoder interface)

## Installation

1. Install required libraries via Arduino Library Manager
2. Connect hardware components according to pin configuration
3. Upload firmware to Arduino microcontroller
4. Initialize device and verify display operation

## Operation

### Morse Code Input

1. Device initializes in normal operating mode
2. Input morse code using paddle contacts:
   - Left contact generates dit (.)
   - Right contact generates dah (-)
3. Decoded characters display in real-time

### Configuration Menu

Access settings via encoder button:
1. Primary menu: WPM adjustment (5-50)
   - Encoder rotation modifies transmission speed
2. Secondary menu: Frequency adjustment (400-1000 Hz)
   - Encoder rotation modifies audio frequency
3. Return to normal operation mode

### Timing Parameters

Standard morse code timing ratios:
- Dot duration: Base timing unit
- Dash duration: 3 × dot duration
- Inter-element spacing: 1 × dot duration
- Inter-character spacing: 3 × dot duration
- Inter-word spacing: 7 × dot duration

## Character Set

### Alphabetic Characters
```
A .-    B -...   C -.-.   D -..    E .      F ..-.   G --.    H ....
I ..    J .---   K -.-    L .-..   M --     N -.     O ---    P .--.
Q --.-  R .-.    S ...    T -      U ..-    V ...-   W .--    X -..-
Y -.--  Z --..
```

### Numeric Characters
```
0 -----  1 .----  2 ..---  3 ...--  4 ....-  5 .....
6 -....  7 --...  8 ---..  9 ----.
```

## Configuration Parameters

Adjustable timing constants:
- `ditLength`: Base timing unit (default: 50ms)
- `DAH_MULTIPLY`: Dash duration multiplier (default: 3)
- `ELEMENT_GAP`: Inter-element spacing (default: 1)
- `LETTER_GAP`: Inter-character spacing (default: 3)
- `WORD_GAP`: Inter-word spacing (default: 7)
- `DEBOUNCE_TIME`: Contact debounce period (default: 5ms)

## Troubleshooting

Common issues and solutions:
- Display corruption: Verify I2C connections and address
- Input non-responsiveness: Check contact connections and debounce settings
- Timing inconsistencies: Adjust WPM parameter
- Audio distortion: Modify frequency setting

## License

This project is distributed under the MIT License. See LICENSE file for details. 