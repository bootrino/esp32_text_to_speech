# ESP32 text to speech to bluetooth speaker

This project plays text to speech from an esp32 to a bluetooth speaker

ESP32 Version of [SAM](https://github.com/s-macke/SAM)

This is a modifed version of: https://github.com/tuanpmt/esp-adf-sam.git


This example plays audio generated from text `HELLO, MY NAME IS SAM.` using [SAM](https://github.com/s-macke/SAM) - Tiny Speech Synthesizer.

## Hardware

***To run this example you need ESP32 with PSRAM such as a WROVERB esp32.*** Alternatively, you may be smart enough to work out how to reduce the RAM usage of this project so it runs on an ordinary esp32. 
Let me know if you do, and I'll update to include your improvements.

## Software

- IDF
- esp32 ADF - instructions here: https://docs.espressif.com/projects/esp-adf/en/latest/get-started/

This has been tested as of 2022 in the default multiverse.


## Configure

idf.py menuconfig

The default board for this example is `ESP32-Lyrat V4.3`. If you need to run this example on other development boards, select the board in menuconfig, such as `ESP32-Lyrat-Mini V1.1`.

```
menuconfig > Audio HAL > ESP32-Lyrat-Mini V1.1
```

Connect to the Classic Bluetooth sink first. Go to `menuconfig` to configure the device name of the remote sink. The default name is `ESP-ADF-SPEAKER`.
The sync is the device that receives the audio over bluetooth and is typically something that will playback or process.

```
 menuconfig > Example Configuration > (ESP-ADF-SPEAKER) BT remote device name
```
For example: TREKZ Titanium by AfterShokz

TODO: Add configuration for SAM to let you set the voice configurations.


## Compile

```
git clone --recursive https://github.com/bootrino/esp32_text_to_speech.git
cd esp32_text_to_speech
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

```
