# ESP32 RGB LED Controller with HomeSpan

## Introduction
This project is an ESP32-based RGB LED Controller, leveraging HomeSpan to create a smart, HomeKit-compatible lighting solution. 
The controller allows for adjusting the hue, saturation, and brightness of an RGB LED strip, making it an ideal choice for smart home enthusiasts.

## Features
- **HomeKit Integration**: Fully compatible with Apple HomeKit.
- **Customizable Lighting**: Control hue, saturation, and brightness.
- **Serial Logging**: Real-time logging of changes and status via Serial Monitor.
- **Invert Brightness Option**: Ability to invert brightness values for different LED configurations.

## Hardware Requirements
- ESP32 Microcontroller
- RGB LED Strip (compatible with GPIO pins)
- Resistors (appropriate for the LED strip used)
- Breadboard and Connecting Wires
- Power Supply for the ESP32 and LED Strip

## Pin Configuration
- Red LED Pin: GPIO 12
- Green LED Pin: GPIO 13
- Blue LED Pin: GPIO 14

## Setup Instructions
1. **Assemble the Hardware**: Connect the RGB LED Strip to the ESP32 according to the pin configuration.
2. **Install HomeSpan**: Follow the instructions on [HomeSpan's GitHub page](https://github.com/HomeSpan/HomeSpan) to install the HomeSpan library in the Arduino IDE.
3. **Load the Sketch**: Open this sketch in the Arduino IDE.
4. **Modify Configurations**: Adjust pin assignments in the code if different from your setup.
5. **Upload the Code**: Compile and upload the code to your ESP32.

## Usage
- After uploading the sketch, the ESP32 will appear as a "Lighting" accessory in your HomeKit setup.
- You can control the RGB LED Strip using any HomeKit-compatible app or Siri.
- Adjust the hue, saturation, and brightness to your preference.
- Monitor the Serial Monitor for real-time updates and debugging information.

## Additional Notes
- The project uses a mapping technique to ensure smooth LED brightness control.
- Ensure that the power supply can handle the current requirements of the RGB LED Strip.

## Wi-Fi and HomeKit Setup
To connect the ESP32 RGB LED Controller to your Wi-Fi network and integrate it with HomeKit, follow these steps:

1. **Connect the ESP32 to Your Computer**: Use a micro USB cable to connect the ESP32 to your computer.

2. **Open the Arduino IDE**: Launch the Arduino IDE where you uploaded the sketch.

3. **Open the Serial Monitor**: Go to `Tools` > `Serial Monitor` in the Arduino IDE. Make sure the baud rate is set to 115200 (matching the `Serial.begin(115200)` in the sketch).

4. **Wi-Fi Configuration**:
   - When you run the program for the first time, the ESP32 will start in 'Configuration Mode'.
   - The Serial Monitor will display a message with a temporary Access Point (AP) that the ESP32 creates.
   - Use a smartphone or computer to connect to this temporary AP.
   - Once connected, a configuration web page should open automatically where you can enter your Wi-Fi credentials.

5. **Connect to HomeKit**:
   - After entering your Wi-Fi credentials, the ESP32 will restart and connect to your Wi-Fi network.
   - The Serial Monitor will display a HomeKit Setup Code (e.g., `111-22-333`).
   - Open the Home app on your iOS device, tap 'Add Accessory', and use your camera to scan the setup code displayed in the Serial Monitor.

6. **Successful Pairing**:
   - Once successfully paired, you can control the RGB LED Strip through the Home app or using Siri.
   - If you encounter any issues during pairing, check the Serial Monitor for error messages or troubleshooting information.

## Usage
- After completing the setup, the ESP32 will appear as a "Lighting" accessory in your HomeKit setup.
- You can control the RGB LED Strip using any HomeKit-compatible app or Siri.
- Adjust the hue, saturation, and brightness to your preference.
- Monitor the Serial Monitor for real-time updates and debugging information.

## Troubleshooting
- **LEDs Not Lighting Up**: Check the connections and ensure that the GPIO pins are correctly assigned.
- **HomeKit Pairing Issues**: Ensure that the ESP32 is connected to your Wi-Fi network and is within range. Revisit the Wi-Fi and HomeKit setup steps if necessary.


## About Coolshrimp Modz
Developed by Coolshrimp Modz, this project aims to provide an accessible and customizable smart lighting solution. 
For more information or support, visit [Coolshrimp Modz](https://www.coolshrimpmodz.com).

---
Enjoy your smart, colorful lighting with the ESP32 RGB LED Controller!
