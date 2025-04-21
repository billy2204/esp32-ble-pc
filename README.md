## Sending JSON Data to Computer via BLE

This is instruction to transmit JSON data from an ESP32 to a computer using Bluetooth Low Energy (BLE).
### Generate UUIDs
Go to [https://www.uuidgenerator.net](https://www.uuidgenerator.net) to generate a UUID for the BLE characteristic. Note that you will need a second UUID for the service. Refresh the page to generate additional UUIDs as needed.

### Hardware (ESP32) Setup

1. **Install Arduino IDE:**
    * Download and install the Arduino IDE from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).
2. **Add ESP32 Board Support:**
    * Open the Arduino IDE.
    * Go to `File` \> `Preferences`.
    * In the `Additional Board Manager URLs` field, add the following link:

```
https://dl.espressif.com/dl/package_esp32_index.json
```

    * Click `OK`.
3. **Install ESP32 Board:**
    * Go to `Tools` \> `Board` \> `Boards Manager...`.
    * Search for `esp32`.
    * Select `ESP32 by Espressif Systems` and click `Install`.
    * Wait for the installation to complete.
    ***Notice: you must install the esp32 version 2.0.17 to use the ledc library***
4. **Customize `ble_transmitter.ino`:**
    * Open the `ble_transmitter.ino` file in the Arduino IDE.
    * Locate the line `#define CHARACTERISTIC_UUID ""`
    * Replace the empty string `""` with the **characteristic UUID** that you created.
    * Locate the line `#define SERVICE_UUID        ""`
    * Replace the empty string `""` with the **service UUID** that you created.
5. **Upload Code to ESP32:**
    * Connect your ESP32 board to your computer using a USB cable.
    * In the Arduino IDE, go to `Tools` \> `Board` and select your ESP32 board (e.g., "ESP32 Dev Module").
    * Go to `Tools` \> `Port` and select the COM port that your ESP32 is connected to.
    * Click the `Upload` button (the right arrow) to compile and upload the code to your ESP32.
    * Wait for the upload to complete.
6. **Connect to PC**
    * Connect to PC normally through bluetooth
    * Choose the device name `ESP32_Vibration` and connect
7. **View Serial Monitor Output:**
    * After the upload is complete, open the Serial Monitor by going to `Tools` \> `Serial Monitor`.
    * Set the baud rate to 115200.
    * Press the reset button on your ESP32.
    * The Serial Monitor will display the JSON data being sent by the ESP32.

### Client (Computer) Setup

1. **Customize `ble_receiver.py`:**
    * Open the `ble_receiver.py` file in a text editor.
    * Locate the line `#define CHARACTERISTIC_UUID ""`
    * Replace the empty string `""` with the **characteristic UUID** generated in step 1.
2. **Install Dependencies:**
    * Open a terminal or command prompt.
    * Run the following command to install the `bleak` package:

```bash
pip install bleak
```

3. **Run `ble_receiver.py`:**
    * In the terminal, navigate to the directory containing `ble_receiver.py`.
    * Execute the following command to start the receiver script:

```bash
python ble_receiver.py
```

    * The script will start scanning for BLE devices and print the received data to the console.




