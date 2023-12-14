# Arduino Music Player

## Introduction
This Arduino-based music player project is designed to play WAV files from an SD card. It features an OLED display for song selection and uses I2S for audio output. The project is built using Arduino libraries and custom functions for ease of use and expandability.

## Hardware Requirements
- Arduino-compatible microcontroller (e.g., ESP32)
- SD card module
- Adafruit SSD1306 OLED display
- Buttons for navigation (UP, DOWN, OK)
- External speakers or headphones
- Appropriate connecting wires

## Setup
1. **Wiring**: Connect the hardware components according to the pin assignments defined in `Main.ino`.
2. **Libraries**: Ensure all required libraries (`Adafruit_SSD1306`, `Adafruit_GFX`, etc.) are installed in your Arduino IDE.
3. **SD Card**: Format the SD card to FAT32 and load it with `.wav` files.

## Usage
- Power on the device.
- The OLED display will show the "**Music Player**" splash screen, followed by a list of `.wav` files from the SD card.
- Use the UP and DOWN buttons to navigate through the list.
- Press the OK button to play the selected file.
- The UP and DOWN buttons can be used to change tracks during playback.
- The OK button can pause and resume playback.

## Additional Notes
- The project includes functions for logging to both the Serial Monitor and the OLED display.
- A debounce mechanism is implemented for button presses.
- The player supports basic navigation and playback controls.
- The code is designed for easy modification and expansion.

## Contributing
Contributions, issues, and feature requests are welcome. Feel free to check [issues page](your-github-repo-link) if you want to contribute.

## License
Distributed under the MIT License. See `LICENSE` for more information.

## Contact
Your Name - your@email.com

Project Link: [https://github.com/your_username/your_project](your-github-repo-link)
