# Pico DFRobot MAX30102 BLE Server

A Bluetooth Low Energy (BLE) peripheral application for the Raspberry Pi Pico W that streams health data from the [DFRobot Gravity MAX30102 PPG Heart Rate and Oximeter Sensor](https://www.dfrobot.com/product-2529.html). 

This project uses the Pico C SDK and BTstack to broadcast real-time Heart Rate and SPO2 data. The sensor features an integrated MCU that handles the complex PPG algorithms internally, so the Pico simply acts as an I2C master and BLE bridge. To conserve power, the sensor can be turned on and off remotely via a BLE Write Command.

## Hardware Connections

The module uses the standard I2C interface with a default address of `0x57`.

| Pico W Pin | DFRobot MAX30102 Pin | Notes |
| :--- | :--- | :--- |
| **3V3 (Out)** | VCC  | Requires 3.3V logic. |
| **GND** | GND  | |
| **GP4 (I2C0 SDA)** | SDA | I2C Data |
| **GP5 (I2C0 SCL)** | SCL | I2C Clock |

*Note: The sensor updates its internal algorithmic data every 4 seconds. The BLE server matches this interval for notifications.*

## BLE GATT Profile

The device advertises as **"Pico-Health"**.

**Primary Service UUID:** `0000AA10-0000-1000-8000-00805F9B34FB`

| Characteristic | UUID | Properties | Format | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Heart Rate** | `...AA11...` | READ, NOTIFY | `uint32` (Little Endian) | Heart rate in beats per minute (bpm). |
| **SPO2** | `...AA12...` | READ, NOTIFY | `uint8` | Blood oxygen saturation percentage (%). |
| **Control** | `...AA13...` | WRITE | `uint8` | Write `0x01` to Start collection, `0x00` to Stop. |

## Build and Flash

1. Use VSCode with the official [Pico extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) for easier building and flashing.

2. `Ctlr+Shift+P` -> `CMake: Configure`

3. Click `Compile` in bottom bar.

4. Put your Pico 2 W into BOOTSEL mode (hold the BOOTSEL button while plugging it in).

## LICENSE
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
