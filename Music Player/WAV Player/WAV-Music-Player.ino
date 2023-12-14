// Main.ino
#include <Arduino.h>
#include <SD.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <vector>
#include <Wire.h>

#include <esp32-hal-dac.h>
#include <driver/i2s.h>

// I2S configuration
#define I2S_SAMPLE_RATE (48000)
#define I2S_SAMPLE_BITS (16)
#define I2S_NUM_CHANNELS (2)
#define I2S_READ_LEN (16 * 1024)
#define I2S_BCK_IO (20)
#define I2S_WS_IO (32)
#define I2S_DO_IO (26)
#define I2S_DI_IO (-1)

// Other constants and pin assignments
const int DAC_PIN = 25; // Internal DAC pin for audio output

// Pin assignments & definitions
const int SD_CS_PIN = 18;    // Chip select pin for the SD card
const int SD_MOSI_PIN = 5;   // MOSI pin for the SD card
const int SD_CLK_PIN = 19;   // CLK pin for the SD card
const int SD_MISO_PIN = 21;  // MISO pin for the SD card

const int OLCD_SDA_Pin = 22; // I2C SDA pin for the OLED
const int OLCD_SCL_Pin = 23; // I2C SCL pin for the OLED

const int BUTTON_PIN = 0;    // Pin for the button (connected to GPIO 0)
const int BUTTON_UP = 13;  // Pin for the button (connected to GPIO 13)
const int BUTTON_OK = 12;    // Pin for the button (connected to GPIO 14)
const int BUTTON_DOWN = 14; // Pin for the button (connected to GPIO 12)

// OLED display definitions
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_ADDR 0x3C // OLED display address

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

std::vector<std::string> fileNames;
int currentFileIndex = 0;
int playingFileIndex = 0;
TaskHandle_t playTaskHandle = NULL;
volatile bool isPlaying = false;
volatile bool isPaused = false;
const char *currentPlayingFile = "";
bool autoplayEnabled = true; // Set to true to enable autoplay on start
String playingMessage = "";


// Forward declarations of functions
void LogDisplay(const char *message);
void LogSerial(const char *message);
void LogBoth(const char *message);
void SplashDisplay(const char *message);
bool playWavFile(const char *filename);
bool initializeSDCard();
void configureI2S();
void loadFileNamesFromSD();
void displayFileName();
bool playWavFile(const char *filename);
void listDir(fs::FS &fs, const char *dirname);
void customLog(const char *message);
bool readDebouncedButton(int buttonPin, int normallyState);
void stopAndPlayCurrentFile();
bool tryPlayWavFile(const char *filename);
void playWavFileTask(void *parameter);

void setup() {
  Serial.begin(115200); 
  
  Wire.begin(OLCD_SDA_Pin, OLCD_SCL_Pin);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) while (true);
  display.display();
  delay(1000);
  display.clearDisplay();

  SplashDisplay("*** Music Player ***");
  delay(500);

  initializeSDCard();
  LogSerial("Listing SD card contents:");
  listDir(SD, "/");

  configureI2S(); // Call the function to configure I2S

  delay(1000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  loadFileNamesFromSD();
  
  // Generate a random index for the initial song
  currentFileIndex = random(fileNames.size()); // Generate a random index
  LogSerial("Random Pick: " + currentFileIndex); // Print the random index to the Serial monitor

  if (autoplayEnabled && !isPlaying && fileNames.size() > 0) {
      stopAndPlayCurrentFile();
  }

  logFreeHeap();
}

void loop() {
    // Navigation with Left and Right buttons
    
    if (digitalRead(BUTTON_UP) == LOW) {
        delay(200); // Debounce delay
        LogSerial("UP button pressed");
        //Navigate Songs up
        currentFileIndex = (currentFileIndex - 1 + fileNames.size()) % fileNames.size();
        displayFileName();
    }

    if (digitalRead(BUTTON_DOWN) == LOW) {
        delay(200); // Debounce delay
        LogSerial("DOWN button pressed");
        //Navigate Songs Down
        currentFileIndex = (currentFileIndex + 1) % fileNames.size();
        displayFileName();
    }

    if (digitalRead(BUTTON_OK) == LOW) {
        delay(1000); // Debounce delay
        LogSerial("OK button pressed");

        // Check if a different file is selected
        if (currentFileIndex != playingFileIndex) {
            stopAndPlayCurrentFile();
            return; // Exit to avoid double handling
        }

        if (isPlaying) {
            if (isPaused) {
                // Resume the paused audio
                i2s_start(I2S_NUM_0);
                isPaused = false;
                LogBoth("- Resumed Playback -");
                delay(2000);
                display.clearDisplay(); 
                LogBoth(playingMessage.c_str());
            } else {
                // Pause the audio if it's playing
                i2s_stop(I2S_NUM_0);
                isPaused = true;
                LogBoth("--- Music Paused ---");
            }
        } else {
             // No audio is playing, so check if a file is selected
            if (currentFileIndex < fileNames.size()) {
                stopAndPlayCurrentFile();
            } else {
                LogBoth("No file selected...");
            }
        }

        delay(1000); // Debounce delay
        //displayFileName();        
    }
}

// Function to log free heap memory
void logFreeHeap() {
    String message = "Free heap: " + String(ESP.getFreeHeap());
    LogSerial(message.c_str());
}


bool tryPlayWavFile(const char *filename) {
    if (playWavFile(filename)) {
        return true; // Playback successful
    } else {
        LogBoth(("Error playing file! " + String(filename)).c_str());
        // Remove the problematic file from the list
        fileNames.erase(fileNames.begin() + currentFileIndex);
        // Update file index if needed
        if (currentFileIndex >= fileNames.size()) {
            currentFileIndex = fileNames.size() - 1;
        }
        return false; // Playback failed
    }
}

void playWavFileTask(void *parameter) {
    // Before exiting the task, log the free heap memory
    logFreeHeap();

    isPlaying = true;
    isPaused = false;
    const char *filename = (const char *)parameter;

    if (!tryPlayWavFile(filename)) {
        currentFileIndex = (currentFileIndex + 1) % fileNames.size();
        filename = fileNames[currentFileIndex].c_str();
        currentPlayingFile = filename;
        tryPlayWavFile(filename);
    }

    // Check if there's a next song, and if so, play it
    if ((currentFileIndex + 1) < fileNames.size()) {
        currentFileIndex++;
        filename = fileNames[currentFileIndex].c_str();
        currentPlayingFile = filename;        
        tryPlayWavFile(filename);
    } else {
        // No next song, reset the index
        currentFileIndex = 0;
        currentPlayingFile = "";
        
        // Check if autoplay is enabled and play the first song
        if (autoplayEnabled && !isPlaying) {
            filename = fileNames[currentFileIndex].c_str();
            currentPlayingFile = filename;
            tryPlayWavFile(filename);
        }
    }

    isPlaying = false;
    isPaused = false;
    vTaskDelete(NULL);
}


void stopAndPlayCurrentFile() {
    // Log free heap memory
    logFreeHeap();

    // Stop existing playback task if it's running
    if (playTaskHandle != NULL) {
        vTaskDelete(playTaskHandle);
        playTaskHandle = NULL;
        stopI2S(); // Ensure I2S is properly stopped
        LogBoth("Stopped previous playback.");
    }

    isPaused = false;

    // Update indices and file name for the new song
    playingFileIndex = currentFileIndex;
    currentPlayingFile = fileNames[currentFileIndex].c_str();

    // Remove .wav extension for display and truncate if needed
    String fileNameDisplay = String(currentPlayingFile);
    fileNameDisplay = fileNameDisplay.substring(0, fileNameDisplay.lastIndexOf('.'));
    fileNameDisplay = fileNameDisplay.substring(0, min(static_cast<unsigned int>(18), fileNameDisplay.length()));

    // Display loading message
    LogBoth(("Loading:\n" + fileNameDisplay).c_str());

    // Start a new task for the new song
    BaseType_t taskCreated = xTaskCreate(
        playWavFileTask,      // Task function
        "PlayWAV",            // Task name
        10000,                // Task stack size
        (void *)currentPlayingFile, // Task parameter
        1,                    // Task priority
        &playTaskHandle       // Task handle
    );

    if (taskCreated == pdPASS) {
        String title = fileNames[currentFileIndex].c_str();

        int separatorIndex = title.indexOf('-');
        if (separatorIndex != -1) {
            String artist = title.substring(0, separatorIndex);
            String songTitleWithExtension = title.substring(separatorIndex + 1);

            // Remove ".wav" extension from song title
            int extensionIndex = songTitleWithExtension.lastIndexOf('.');
            if (extensionIndex != -1) {
                String songTitle = songTitleWithExtension.substring(0, extensionIndex);
                songTitle.trim();                

                // Check if artist or song names are too long and break into multiple lines if needed
                if (artist.length() > 18) {
                    artist = artist.substring(0, 18);
                }
                if (songTitle.length() > 40) {
                    songTitle = songTitle.substring(0, 40);
                }

                // Create the playingMessage with the artist and song title
                playingMessage = "  --- Playing ---\n\n" + artist + "\n" + songTitle;
                LogBoth(playingMessage.c_str());
            }
        } else {
            // If there's no hyphen, use the entire title as the song title and display a default message.
            LogBoth(("--- Playing ---\n" + title).c_str());
        }
    } else {
        // Handle task creation failure
        LogBoth("Error: Unable to start playback task.");
    }
}

void stopI2S() {
    i2s_stop(I2S_NUM_0);
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
}

bool initializeSDCard() {
  SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); if (!SD.begin(SD_CS_PIN)) { customLog("Card Mount Failed"); delay(5000); return false; } return true;
}

void configureI2S() {
  i2s_config_t i2s_config = { .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN), .sample_rate = I2S_SAMPLE_RATE, .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS, .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, .communication_format = I2S_COMM_FORMAT_STAND_MSB,.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,.dma_buf_count = 8, .dma_buf_len = 64, .use_apll = false, .tx_desc_auto_clear = true, .fixed_mclk = 0};
  i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCK_IO, .ws_io_num = I2S_WS_IO, .data_out_num = I2S_DO_IO, .data_in_num = I2S_DI_IO };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, I2S_SAMPLE_RATE, (i2s_bits_per_sample_t)I2S_SAMPLE_BITS, (i2s_channel_t)I2S_NUM_CHANNELS);
}

struct WavHeader { char chunkID[4]; uint32_t chunkSize; char format[4]; char subchunk1ID[4]; uint32_t subchunk1Size; uint16_t audioFormat; uint16_t numChannels; uint32_t sampleRate; uint32_t byteRate; uint16_t blockAlign; uint16_t bitsPerSample; char subchunk2ID[4]; uint32_t subchunk2Size; };

void listDir(fs::FS &fs, const char *dirname) { File root = fs.open(dirname); if (!root) return; if (!root.isDirectory()) return; File file = root.openNextFile(); while (file) { if (file.isDirectory()) Serial.print("DIR : "); else { Serial.print("FILE: "); Serial.print(file.name()); Serial.print("  SIZE: "); Serial.println(file.size()); } file = root.openNextFile(); } }

void customLog(const char *message) { Serial.println(message); display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0, 0); display.println(message); display.display(); }

void loadFileNamesFromSD() { File root = SD.open("/"); while (File file = root.openNextFile()) { if (!file.isDirectory()) { std::string fileName = file.name(); if (fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".wav") { fileNames.push_back(fileName); } } file.close(); } root.close(); displayFileName(); }

// Global variable to keep track of the scrolling position
int scrollPosition = 0;

void displayFileName() {
    display.clearDisplay();  
    display.setTextSize(1);

    for (int i = -1; i <= 1; i++) {
        int index = (currentFileIndex + i + fileNames.size()) % fileNames.size();
        String displayName = " " + String(fileNames[index].c_str()); // Add space at the beginning
        
        int y = 30 + (i * 10);
        int x = 0; // Left align all items

        if (i == 0) {
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Invert colors for selected item
        } else {
            display.setTextColor(SSD1306_WHITE); // Normal color for other items
        }

        // Truncate the display name to 18 characters plus space
        displayName = displayName.substring(0, min(static_cast<unsigned int>(19), displayName.length()));

        display.setCursor(x, y);
        display.println(displayName);
    }

    display.display();
}


bool playWavFile(const char *filename) {
    // Ensure that filename starts with '/'
    String filenameWithSlash = filename;
    if (filenameWithSlash[0] != '/') {
        filenameWithSlash = '/' + filenameWithSlash;
    }

    // Open the file
    File file = SD.open(filenameWithSlash.c_str());
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    WavHeader wavHeader;

    // Read the WAV file header
    if (file.read((uint8_t *)&wavHeader, sizeof(WavHeader)) != sizeof(WavHeader)) {
        Serial.println("Failed to read WAV header");
        file.close();
        return false;
    }

    // Check the WAV file format
    if (wavHeader.audioFormat != 1 || (wavHeader.bitsPerSample != 16 && wavHeader.bitsPerSample != 8)) {
        Serial.println("Invalid WAV file format");
        file.close();
        return false;
    }

    // Reset I2S configuration for the new file
    i2s_bits_per_sample_t i2s_bits = (wavHeader.bitsPerSample == 16) ? I2S_BITS_PER_SAMPLE_16BIT : I2S_BITS_PER_SAMPLE_8BIT;
    i2s_set_clk(I2S_NUM_0, wavHeader.sampleRate, i2s_bits, (i2s_channel_t)wavHeader.numChannels);
    
    configureI2S();

    // Buffer for reading audio data
    const int bufferSize = 1024;
    uint8_t buffer[bufferSize];
    size_t bytesRead;

    // Read and write WAV file data
    while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
        size_t bytesWritten;
        if (wavHeader.bitsPerSample == 16) {
            i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
        } else {
            uint16_t outBuffer[bufferSize];
            for (int i = 0; i < bytesRead; i++) {
                outBuffer[i] = ((uint16_t)buffer[i]) << 8;
            }
            i2s_write(I2S_NUM_0, outBuffer, bytesRead * 2, &bytesWritten, portMAX_DELAY);
        }
    }

    // Close the file and clear the I2S buffer
    file.close();
    i2s_zero_dma_buffer(I2S_NUM_0);

    return true;
}

void LogDisplay(const char *message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // Use SSD1306_WHITE constant
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void LogSerial(const char *message) {
  // Print message to the Serial monitor
  Serial.println(message);
}

void LogBoth(const char *message) {
    LogSerial(message);
    LogDisplay(message);
}

void SplashDisplay(const char *message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // Use SSD1306_WHITE constant
  display.setCursor(0, 0);
  display.println(message);
  display.setCursor(0, 20);
  display.println("Coolshrimp Modz");
  display.display();
}
