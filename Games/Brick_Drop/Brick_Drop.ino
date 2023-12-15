#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
#define OLED_ADDR 0x3C

const int OLCD_SDA_Pin = 22;
const int OLCD_SCL_Pin = 23;

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

const int BUTTON_LEFT = 13;
const int BUTTON_RIGHT = 12;
const int BUTTON_OK = 14;

#define GRID_WIDTH 10
#define GRID_HEIGHT 11
#define BLOCK_SIZE 4

bool gameGrid[GRID_HEIGHT][GRID_WIDTH];
struct Tetromino {
    int x, y;
} currentBlock;

int score = 0;
bool isGameOver = false;
int highScore = 0;
const int DAC_PIN = 25; // DAC pin for audio output

enum GameState { MENU, PLAYING, GAME_OVER };
enum GameSpeed { FAST, NORMAL, SLOW };

GameState gameState = MENU;
GameSpeed gameSpeed = NORMAL;

const int dropSpeeds[] = {80, 400, 800}; // Drop speeds for different game speeds (in milliseconds)

unsigned long lastButtonPress = 0;
unsigned long lastMoveTime = 0;

int noteDurations[] = {
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2,
    1, 2, 1, 1, 1, 1, 2, 1, 2, 2, 1, 2, 1, 1,
    1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 1, 1,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 1, 1, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 1, 1, 2, 2, 1, 2, 2,
    1, 2, 2, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 4, 1, 1, 1, 1, 2, 2, 1, 1
};

int melody[] = {
    659, 494, 523, 587, 523, 494,
    440, 440, 659, 659, 440, 329, 329, 349,
    349, 440, 440, 659, 659, 494, 494, 523,
    587, 659, 523, 494, 440, 329, 349, 392,
    659, 494, 523, 587, 659, 587, 523, 494,
    440, 440, 659, 659, 440, 329, 329, 349,
    349, 440, 440, 659, 659, 494, 494, 523,
    587, 659, 523, 494, 440, 329, 349, 392
};

unsigned long noteStartTime = 0;
int thisNote = 0;
int tempo = 518; // Reduced tempo (10% reduction) for background music

void playBackgroundTune() {
    unsigned long currentTime = millis();

    if (thisNote < sizeof(melody) / sizeof(melody[0])) {
        if (currentTime - noteStartTime >= (60000.0 / tempo) * noteDurations[thisNote]) {
            playTone(melody[thisNote], 0); // Stop the current note
            thisNote++;
            if (thisNote < sizeof(melody) / sizeof(melody[0])) {
                playTone(melody[thisNote], (60000.0 / tempo) * noteDurations[thisNote]);
                noteStartTime = currentTime;
            } else {
                // Reset to the first note
                thisNote = 0;
                playTone(melody[thisNote], (60000.0 / tempo) * noteDurations[thisNote]);
                noteStartTime = currentTime;
            }
        }
    } else {
        // Reset to the first note
        thisNote = 0;
        playTone(melody[thisNote], (60000.0 / tempo) * noteDurations[thisNote]);
        noteStartTime = currentTime;
    }
}

void playFunkyDropSound() {
    // Play a tone or sound effect when a block is placed (you can customize this)
    // Example: Play a beep sound for 100 milliseconds
    int beepFrequency = 1000; // Adjust the frequency as needed
    int beepDuration = 100;   // Adjust the duration as needed
    playTone(beepFrequency, beepDuration);
}

// Function to play a tone
void playTone(int frequency, int duration) {
    if (frequency == 0 || duration == 0) {
        ledcWriteTone(0, 0); // No sound
    } else {
        ledcAttachPin(DAC_PIN, 0);
        ledcWriteTone(0, frequency);
        delay(duration);
        ledcWriteTone(0, 0); // Stop sound
    }
}

// Function to move the current block down until it reaches the bottom or collides
void dropBlockToBottom() {
    while (currentBlock.y < GRID_HEIGHT - 1 && isValidPosition(currentBlock.x, currentBlock.y + 1)) {
        currentBlock.y++;
    }
}

// Function to check for full rows and remove them, then shift the blocks down
void checkAndRemoveFullRows() {
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        bool isFullRow = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!gameGrid[y][x]) {
                isFullRow = false;
                break;
            }
        }
        if (isFullRow) {
            // Remove the full row
            for (int row = y; row > 0; row--) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    gameGrid[row][x] = gameGrid[row - 1][x];
                }
            }
            y++; // Recheck the same row
        }
    }
}

void placeBlock() {
    if (isValidPosition(currentBlock.x, currentBlock.y)) {
        gameGrid[currentBlock.y][currentBlock.x] = true;
        score += 10;
        // Play sound on placing a block
        playTone(523, 100); // C5 note for 100 milliseconds

        // Check for full rows and remove them, then shift the blocks down
        checkAndRemoveFullRows();

        // Play funky drop sound
        playFunkyDropSound();
    } else {
        isGameOver = true;
        if (score > highScore) {
            highScore = score;
        }
        // Play game over tone
        playGameOverTone();
        displayGameOver();
        delay(2000);
        resetGame();
        return;
    }

    currentBlock.x = GRID_WIDTH / 2;
    currentBlock.y = 0;
}

// Function to play game over tone
void playGameOverTone() {
    // Example: a descending tone sequence for game over
    int notes[] = {523, 494, 440, 392};
    int duration = 150;

    for (int i = 0; i < 4; i++) {
        playTone(notes[i], duration);
    }
}

bool isValidPosition(int x, int y) {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT || gameGrid[y][x]) {
        return false;
    }
    return true;
}

void displayGameOver() {
    Serial.println("Game Over");
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 30);
    display.println("Game Over");
    display.display();
}

void resetGame() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            gameGrid[y][x] = false;
        }
    }
    score = 0;
    currentBlock.x = GRID_WIDTH / 2;
    currentBlock.y = 0;
    isGameOver = false;
}

void setup() {
    Serial.begin(115200);
    Wire.begin(OLCD_SDA_Pin, OLCD_SCL_Pin);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            gameGrid[y][x] = false;
        }
    }
    currentBlock.x = GRID_WIDTH / 2;
    currentBlock.y = 0;
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    // Initialize DAC pin for audio
    pinMode(DAC_PIN, OUTPUT);
    ledcSetup(0, 1000, 8); // Channel 0, 1 kHz, 8-bit resolution

    static int selectedMode = NORMAL;
    playBackgroundTune(); // Play background music
    drawMenu(selectedMode);
}

void drawMenu(int selectedMode) {
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(WHITE);

    // Title
    display.setCursor(20, 0);
    display.print("- Brick Drop -");

    display.setTextSize(1);
    display.setCursor(10, 16);
    display.print("Select Game Speed");

    const char* modes[] = {" Fast ", " Normal ", " Slow "};
    for (int i = 0; i < 3; i++) {
        if (i == selectedMode) {
            display.setTextColor(BLACK, WHITE); // Inverted colors for selection
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(35, i * 10 + 30); // Adjust position as needed
        display.println(modes[i]);
    }
    
    display.display();
}

void drawGrid() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Score: ");
    display.println(score);

    int yOffset = 14;
    int framePadding = 2;
    int frameWidth = GRID_WIDTH * BLOCK_SIZE + framePadding * 2;
    int frameHeight = GRID_HEIGHT * BLOCK_SIZE + framePadding * 2;
    display.drawRect(framePadding - 1, yOffset - 1, frameWidth, frameHeight, WHITE);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (gameGrid[y][x]) {
                display.fillRect(x * BLOCK_SIZE + framePadding, y * BLOCK_SIZE + yOffset, BLOCK_SIZE, BLOCK_SIZE, WHITE);
            }
        }
    }

    if (!isGameOver && currentBlock.y < GRID_HEIGHT) {
        display.fillRect(currentBlock.x * BLOCK_SIZE + framePadding, currentBlock.y * BLOCK_SIZE + yOffset, BLOCK_SIZE, BLOCK_SIZE, WHITE);
    }

    // Display high score
    display.setCursor(75, 0); // Adjust position as needed
    display.print("Best:");
    display.println(highScore);

    // Display current game speed
    display.setCursor(55, 55); // Adjust position as needed
    display.print("Speed:");
    switch (gameSpeed) {
        case FAST:
            display.print("Fast");
            break;
        case NORMAL:
            display.print("Normal");
            break;
        case SLOW:
            display.print("Slow");
            break;
    }

    display.display();
}

void loop() {
    unsigned long currentTime = millis();

    if (gameState == MENU) {
        static int selectedMode = NORMAL;

        if (!digitalRead(BUTTON_LEFT) && selectedMode > FAST && currentTime - lastButtonPress > 200) {
            selectedMode--;
            drawMenu(selectedMode);
            lastButtonPress = currentTime;
        }
        if (!digitalRead(BUTTON_RIGHT) && selectedMode < SLOW && currentTime - lastButtonPress > 200) {
            selectedMode++;
            drawMenu(selectedMode);
            lastButtonPress = currentTime;
        }
        if (!digitalRead(BUTTON_OK) && currentTime - lastButtonPress > 200) {
            gameSpeed = static_cast<GameSpeed>(selectedMode);
            gameState = PLAYING;
            resetGame();
        }
        // Continue playing the background tune
        playBackgroundTune();
        return;
    }

    if (gameState == PLAYING) {
        if (!digitalRead(BUTTON_LEFT) && currentBlock.x > 0 && isValidPosition(currentBlock.x - 1, currentBlock.y) && currentTime - lastButtonPress > 200) {
            currentBlock.x--;
            lastButtonPress = currentTime;
            playTone(440, 50); // A4 note for 50 milliseconds
        }

        if (!digitalRead(BUTTON_RIGHT) && currentBlock.x < GRID_WIDTH - 1 && isValidPosition(currentBlock.x + 1, currentBlock.y) && currentTime - lastButtonPress > 200) {
            currentBlock.x++;
            lastButtonPress = currentTime;
            playTone(440, 50); // A4 note for 50 milliseconds
        }

        int dropSpeed = dropSpeeds[gameSpeed];

        // Check if the OK button is pressed to drop the block faster
        if (!digitalRead(BUTTON_OK)) {
            dropSpeed = 30; // Adjust this value for the desired drop speed when OK is pressed
        }

        if (currentTime - lastMoveTime > dropSpeed) {
            lastMoveTime = currentTime;
            if (currentBlock.y < GRID_HEIGHT - 1 && isValidPosition(currentBlock.x, currentBlock.y + 1)) {
                currentBlock.y++;
            } else {
                placeBlock();
                //playFunkyDropSound(); // Play funky drop sound
            }
        }

        if (isGameOver) {
            gameState = GAME_OVER;
        }
    }

    if (gameState == GAME_OVER) {
        // Allow selecting a new game speed and starting a new game
        if (!digitalRead(BUTTON_LEFT) && gameSpeed > FAST && currentTime - lastButtonPress > 200) {
            gameSpeed = static_cast<GameSpeed>(gameSpeed - 1);
            resetGame();
            lastButtonPress = currentTime;
        }
        if (!digitalRead(BUTTON_RIGHT) && gameSpeed < SLOW && currentTime - lastButtonPress > 200) {
            gameSpeed = static_cast<GameSpeed>(gameSpeed + 1);
            resetGame();
            lastButtonPress = currentTime;
        }
        if (!digitalRead(BUTTON_OK) && currentTime - lastButtonPress > 200) {
            gameState = PLAYING;
            resetGame();
        }
    }

    drawGrid();
}
