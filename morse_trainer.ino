#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RotaryEncoder.h"

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
#define ENCODER_PIN_A 2  // Changed to interrupt-capable pin
#define ENCODER_PIN_B 3  // Changed to interrupt-capable pin
#define DIT_PIN 4       // Moved paddle to regular pins
#define DAH_PIN 5       // Moved paddle to regular pins
#define BUZZER_PIN 6    // Moved buzzer
#define ENCODER_BTN 7   // Kept the same
#define LED_BUILTIN 13   // Added LED_BUILTIN

// Morse code timing (in milliseconds)
unsigned long ditLength = 50;  // Base unit for morse timing, adjusted by WPM
#define DAH_MULTIPLY 3     // Dash is 3x dot length
#define ELEMENT_GAP 1      // Space between elements is 3x dot length (increased from 1)
#define LETTER_GAP 3       // Space between letters is 7x dot length
#define WORD_GAP 7        // Space between words is 7x dot length
#define DEBOUNCE_TIME 5   // Debounce time in milliseconds

// Menu states
enum MenuState {
  NORMAL,
  MENU_WPM,
  MENU_FREQ
};

// Keyer states
enum KeyerState {
  IDLE,
  SENDING_DIT,
  SENDING_DAH,
  DIT_DELAY,  // Space after dit
  DAH_DELAY   // Space after dah
};

// Global variables
RotaryEncoder *encoder = nullptr;  // Changed to pointer for proper ISR handling
MenuState currentState = NORMAL;
KeyerState keyerState = IDLE;
int wpm = 15;  // Words per minute
int frequency = 700;  // Buzzer frequency in Hz
String currentMorseSequence = "";
bool encoderButtonPressed = false;
unsigned long lastEncoderButtonPress = 0;
int lastEncoderPos = 0;  // Track last encoder position

// Timing variables
unsigned long startTime = 0;
unsigned long lastDitTime = 0;  // For debouncingF
unsigned long lastDahTime = 0;  // For debouncing
bool ditPressed = false;
bool dahPressed = false;
bool lastDitState = false;  // For debouncing
bool lastDahState = false;  // For debouncing
bool ditMemory = false;  // For iambic memory
bool dahMemory = false;  // For iambic memory
unsigned long lastLetterTime = 0;  // Time when last letter was completed

// Morse code lookup table
const char* morseTable[] = {
  ".-",   // A
  "-...", // B
  "-.-.", // C
  "-..",  // D
  ".",    // E
  "..-.", // F
  "--.",  // G
  "....", // H
  "..",   // I
  ".---", // J
  "-.-",  // K
  ".-..", // L
  "--",   // M
  "-.",   // N
  "---",  // O
  ".--.", // P
  "--.-", // Q
  ".-.",  // R
  "...",  // S
  "-",    // T
  "..-",  // U
  "...-", // V
  ".--",  // W
  "-..-", // X
  "-.--", // Y
  "--..", // Z
};

// Add these global variables after other global variables
#define CHAR_WIDTH 12    // Width of each character in pixels (for text size 2)
#define CHAR_HEIGHT 16   // Height of each character in pixels (for text size 2)
#define CHARS_PER_LINE 10  // How many characters fit on one line
#define MAX_LINES 3      // Maximum number of lines on screen
int currentX = 0;        // Current X position in characters
int currentY = 0;        // Current Y position in lines
String displayBuffer = ""; // Buffer to store displayed text

// Interrupt Service Routine for encoder
void checkPosition() {
  encoder->tick();  // just call tick() to check the state
}

void setup() {
  // Initialize pins with high impedance pull-up resistors
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);  // Enable pull-up for encoder
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);  // Enable pull-up for encoder
  pinMode(LED_BUILTIN, OUTPUT); // Configure built-in LED
  
  // Initialize encoder with proper pins and interrupts
  encoder = new RotaryEncoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::FOUR3);
  
  // Enable pin change interrupts for encoder pins
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), checkPosition, CHANGE);
  
  // Ensure buzzer is off at startup
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Don't proceed if display initialization fails
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Show welcome message
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(8, 0);
  display.println("MORSE");
  display.setCursor(8, 20);
  display.println("TRAINER");
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("WPM: ");
  display.print(wpm);
  display.print("  Freq: ");
  display.print(frequency);
  display.println("Hz");
  display.setCursor(0, 56);
  display.println("Press encoder for menu");
  display.display();
  delay(2000);  // Show welcome message for 2 seconds
  
  // Initialize serial for debugging
  Serial.begin(9600);
  
  // Calculate initial timing based on WPM
  updateTimings();
  
  // Initialize last states
  lastDitState = digitalRead(DIT_PIN);
  lastDahState = digitalRead(DAH_PIN);
  lastEncoderPos = encoder->getPosition();
}

void updateTimings() {
  // Standard formula: PARIS = 50 units, adjusted for better feel
  ditLength = (1200 / wpm) * 0.8;  // Reduced to 80% of standard timing for better feel
}

void loop() {
  handleEncoder();
  handlePaddles();
  handleDisplay();
}

void handleEncoder() {
  // Handle encoder button with debouncing
  bool btnState = digitalRead(ENCODER_BTN);
  if (!btnState && !encoderButtonPressed) {
    if (millis() - lastEncoderButtonPress > 500) { // Debounce
      encoderButtonPressed = true;
      lastEncoderButtonPress = millis();
      
      // Cycle through menu states
      switch(currentState) {
        case NORMAL:
          currentState = MENU_WPM;
          break;
        case MENU_WPM:
          currentState = MENU_FREQ;
          break;
        case MENU_FREQ:
          currentState = NORMAL;
          break;
      }
      
      // Update display immediately when state changes
      handleDisplay();
    }
  } else if (btnState) {
    encoderButtonPressed = false;
  }
  
  // Handle rotation with improved position tracking
  int newPos = encoder->getPosition();
  if (newPos != lastEncoderPos) {
    int change = newPos - lastEncoderPos;
    
    switch(currentState) {
      case MENU_WPM:
        wpm = constrain(wpm + (change > 0 ? 1 : -1), 5, 50);
        updateTimings();
        break;
      case MENU_FREQ:
        frequency = constrain(frequency + (change > 0 ? 10 : -10), 400, 1000);
        break;
    }
    
    lastEncoderPos = newPos;
    // Update display immediately when value changes
    handleDisplay();
  }
}

void handlePaddles() {
  unsigned long currentTime = millis();
  
  // Read current states with debouncing
  bool rawDitState = !digitalRead(DIT_PIN);  // Inverted due to pull-up
  bool rawDahState = !digitalRead(DAH_PIN);  // Inverted due to pull-up
  
  // Check for word gap first (when no keys are pressed)
  if (!rawDitState && !rawDahState && currentMorseSequence.length() == 0) {
    if ((currentTime - lastLetterTime) >= (ditLength * WORD_GAP) && lastLetterTime > 0) {
      // Add space character
      displayCharacter(' ');
      lastLetterTime = 0;  // Reset timer
      return;
    }
  }
  
  // Debounce dit paddle
  if (rawDitState != lastDitState) {
    lastDitTime = currentTime;
  }
  if ((currentTime - lastDitTime) > DEBOUNCE_TIME) {
    ditPressed = rawDitState;
  }
  lastDitState = rawDitState;
  
  // Debounce dah paddle
  if (rawDahState != lastDahState) {
    lastDahTime = currentTime;
  }
  if ((currentTime - lastDahTime) > DEBOUNCE_TIME) {
    dahPressed = rawDahState;
  }
  lastDahState = rawDahState;
  
  switch(keyerState) {
    case IDLE:
      if (ditPressed || ditMemory) {
        startTime = currentTime;
        keyerState = SENDING_DIT;
        tone(BUZZER_PIN, frequency);
        currentMorseSequence += ".";
        ditMemory = false;
      }
      else if (dahPressed || dahMemory) {
        startTime = currentTime;
        keyerState = SENDING_DAH;
        tone(BUZZER_PIN, frequency);
        currentMorseSequence += "-";
        dahMemory = false;
      }
      break;
      
    case SENDING_DIT:
      if (currentTime - startTime >= ditLength) {
        noTone(BUZZER_PIN);
        startTime = currentTime;
        keyerState = DIT_DELAY;
        // Store opposite paddle state in memory
        if (dahPressed) dahMemory = true;
      }
      break;
      
    case SENDING_DAH:
      if (currentTime - startTime >= ditLength * DAH_MULTIPLY) {
        noTone(BUZZER_PIN);
        startTime = currentTime;
        keyerState = DAH_DELAY;
        // Store opposite paddle state in memory
        if (ditPressed) ditMemory = true;
      }
      break;
      
    case DIT_DELAY:
    case DAH_DELAY:
      if (currentTime - startTime >= ditLength * ELEMENT_GAP) {  // Use ELEMENT_GAP multiplier
        keyerState = IDLE;
        // Check for letter gap
        if (!ditPressed && !dahPressed && !ditMemory && !dahMemory) {
          if (currentMorseSequence.length() > 0) {
            processSequence();
            currentMorseSequence = "";
            lastLetterTime = currentTime;  // Record when we finished the letter
          }
        }
      }
      break;
  }
}


void processSequence() {
  // Convert morse sequence to character
  for (int i = 0; i < 26; i++) {  // 26 letters + 10 numbers = 36 total
    if (currentMorseSequence == morseTable[i]) {
        char letter = 'A' + i;  // For letters A-Z
      displayCharacter(letter);
      return;
    }
  }
  // If no match found, ignore the sequence
}

void displayCharacter(char c) {
  if (currentState == NORMAL) {
    // If we're at the end of a line, move to next line
    if (currentX >= CHARS_PER_LINE) {
      currentX = 0;
      currentY++;
    }
    
    // If we're at the bottom of the screen, clear it
    if (currentY >= MAX_LINES) {
      display.clearDisplay();
      currentX = 0;
      currentY = 0;
      displayBuffer = "";
    }
    
    // If this is the start of a new display, clear it
    if (currentX == 0 && currentY == 0) {
      display.clearDisplay();
    }
    
    // Calculate pixel positions
    int pixelX = currentX * CHAR_WIDTH;
    int pixelY = currentY * CHAR_HEIGHT;
    
    // Set text properties
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    
    // Position cursor and print character
    display.setCursor(pixelX, pixelY);
    if (c == ' ') {
      // For space, just move the cursor
      display.print(' ');
    } else {
      display.print(c);
    }
    display.display();
    
    // Add character to buffer and increment position
    displayBuffer += c;
    currentX++;
  } else {
    // For menu states, just show the character in the center (except spaces)
    if (c != ' ') {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(48, 24);
      display.print(c);
      display.display();
    }
  }
}

void handleDisplay() {
  if (currentState != NORMAL) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    
    switch(currentState) {
      case MENU_WPM:
        display.println("WPM Setting");
        display.setTextSize(2);
        display.setCursor(32, 24);
        display.print(wpm);
        display.print(" WPM");
        break;
      case MENU_FREQ:
        display.println("Frequency");
        display.setTextSize(2);
        display.setCursor(24, 24);
        display.print(frequency);
        display.println("Hz");
        break;
    }
    display.display();
  }
} 