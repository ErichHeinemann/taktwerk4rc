# taktwerk4rc
Taktwerk is some kind of a MIDI-File-Player for the Boss RC600. Other Musicians use a Drumcomputer, Drumpads or a Beatbuddy to bring in some variations of the Drum-Sounds.


# Arduino-Code for the ESP32-S3 DEV-Kit (ESP32 S3 N16R8 DevKitC-1 Module)
Check the folder taktwerk4rc_ino, it contains the Code for the Arduino.
To compile the used combination of libraries, You have to change some settings in the Library MD_MIDIFile

Open to edit the file MD_MIDIFile.h unter:/Users/username/Documents/Arduino/libraries/MD_MIDIFile/src/MD_MIDIFile.h
search for Line (452):   #define SD_FAT_TYPE 0
Change 0 into e 3:   #define SD_FAT_TYPE 3
Save it...

used Libraries:<br>
MD_MIDIFile by MajicDesigns<br>
EspUsbHost  Download from >> https://github.com/tanakamasayuki/EspUsbHost<br>
SPI.h <br>
SD.h<br>
Wire.h<br>
Adafruit_GFX.h<br>
Adafruit_SSD1306.h<br>
Adafruit_NeoPixel.h<br>

Wiring:
<table>
<tr><th>Module    </th><th>       Signal  </th><th>  ESP32-S3 GPIO  </th><th> Description </th></tr>
<tr><td> I2C OLED (SSD1306) </td><td> SDA  </td><td>   GPIO 8   </td><td>       Standard I2C Data </td></tr>
<tr><td> I2C OLED (SSD1306) </td><td>  SCL  </td><td>   GPIO 9   </td><td>       Standard I2C Clock </td></tr>
<tr><td> SPI SD-Card        </td><td> CS    </td><td>    GPIO 10    </td><td>      Chip Select </td></tr>
<tr><td> SPI SD-Card        </td><td> MOSI  </td><td>   GPIO 11     </td><td>     SPI Master Out </td></tr>
<tr><td> SPI SD-Card        </td><td> SCK   </td><td>   GPIO 12      </td><td>    SPI Clock </td></tr>
<tr><td> SPI SD-Card        </td><td>  MISO  </td><td>   GPIO 13      </td><td>    SPI Master In </td></tr>
<tr><td> Keys (Navi)        </td><td> Song Next </td><td>  GPIO 4   </td><td>        Internal Pullup (Button closes to GND) </td></tr>
<tr><td> Keys (NAVI)        </td><td> Song Prev </td><td>   GPIO 5    </td><td>       Internal Pullup (Button closes to GND)</td></tr>
<tr><td> Footswitch         </td><td> Start / Stop  </td><td>   GPIO 6      </td><td>     Internal Pullup (Button closes to GND)</td></tr>
<tr><td> Footswitch         </td><td> Fill Trigger   </td><td>  GPIO 7    </td><td>       Internal Pullup (Button closes to GND)</td></tr>
<tr><td> Footswitch         </td><td> Quiet (50% Vol) </td><td> GPIO 15    </td><td>      Internal Pullup (Button closes to GND)</td></tr>
<tr><td> USB Host           </td><td> D- / D+    </td><td>       GPIO 19 / 20  </td><td>  reserved für RC-600 USB-HOST</td></tr>
<tr><td> NeoPixel           </td><td> LED  DATA   </td><td>       GPIO 48      </td><td>   Onboard LED / Metronom</td></tr>
</table>

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

