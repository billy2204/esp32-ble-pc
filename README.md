# SEND DATA AS JSON TO COMPUTER VIA BLE

## Client 
Customize *ble_receiver.py*: 
Create UUID on ***https://www.uuidgenerator.net***
Open *ble_receiver.py*
Go to line: 
```#define CHARACTERISTIC_UUID ""``` and add the **UUID** generated into ```""```
Install package:
```pip install bleak```
Run  *ble_receiver.py* to view the data

## Hardware
Download **Arduino IDE**
Go to **File→ Preferences**, add the link: *https://dl.espressif.com/dl/package_esp32_index.json* at **Additional Board Manager URLs** 
Search *esp32* in **Tool→Board→Boards Manager**
Choose **ESP32 by Espressif Systems** and press **install**
Open *ble_transmitter.ino*, go to line ```#define CHARACTERISTIC_UUID ""``` and add the **UUID**, create the **second UUID** and add into ```#define SERVICE_UUID        ""```
Save and upload to **esp32**, after reset the **esp32**, open the *Serial Monitor* to view to data sent





