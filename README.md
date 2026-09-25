# taktwerk4rc
Taktwerk is some kind of a MIDI-File-Player for the Boss RC600. Other Musicians use a Drumcomputer, Drumpads or a Beatbuddy to bring in some variations of the Drum-Sounds.


# Arduino-Code for the ESP32-S3 DEV-Kit (ESP32 S3 N16R8 DevKitC-1 Module)
Check the folder taktwerk4rc_ino, it contains the Code for the Arduino.
To compile the used combination of libraries, You have to change some settings in the Library MD_MIDIFile

Open to edit the file MD_MIDIFile.h unter:/Users/username/Documents/Arduino/libraries/MD_MIDIFile/src/MD_MIDIFile.h
search for Line (452):   #define SD_FAT_TYPE 0
Change 0 into e 3:   #define SD_FAT_TYPE 3
Save it...

used Libraries:
MD_MIDIFile by MajicDesigns
EspUsbHost  Download from >> https://github.com/tanakamasayuki/EspUsbHostSPI.h
SD.h
Wire.h
Adafruit_GFX.h
Adafruit_SSD1306.h
Adafruit_NeoPixel.h

Wiring:
Module            Signal    ESP32-S3 GPIO   Description</br>
I2C OLED (SSD1306)  SDA     GPIO 8          Standard I2C Data</br>
                    SCL     GPIO 9          Standard I2C Clock</br>
SPI         SD-Card CS      GPIO 10         Chip Select</br>
                    MOSI    GPIO 11         SPI Master Out</br>
                    SCK     GPIO 12         SPI Clock</br>
                    MISO    GPIO 13         SPI Master In</br>
Keys (Navi)     Song Next   GPIO 4          Internal Pullup (Button closes to GND)</br>
                Song Prev   GPIO 5          Internal Pullup (Button closes to GND)</br>
Footswitch  Start / Stop    GPIO 6          Internal Pullup (Button closes to GND)</br>
            Fill Trigger    GPIO 7          Internal Pullup (Button closes to GND)
            Quiet (50% Vol) GPIO 15         Internal Pullup (Button closes to GND)
USB Host  D- / D+           GPIO 19 / 20    reserved für RC-600 USB-HOST
NeoPixel LED  DATA          GPIO 48         Onboard LED / Metronom

!! NEEDED Modifications on the ESp32-S3-Hardware !!!
close or bridge the PADS USB-OTG under the board to enable the USB-OTG-Function
close or bridge the PADS 5V In/Out on the top of the board to enable 5 Volt-PIN to create 5 Volts for the SD-Card-Reader

# Taktwerk MIDI-File extractor
Check the file taktwerk_editor.html locally, it contains the Code for the Drum-Editor.
This Editor enabled You to load a MIDI-File, select a MIDI-Channel and some Bars to create a new Drum-Track.
It always exports the track to MIDI-Channel 10!
When selecting a bar, You could edit the drums. 
I tried to make it mostly generic and the webpage uses webaudiofonts to trigger samples.

Save the created or extracted files as fill-*.mid or loop-*.mid in a folder of the SD-Card while * is a number.

Probably this tool is handy for other Drum- or MIDI-Experiments with other tools too.

