# taktwerk4rc
Taktwerk is some kind of a MIDI-File-Player for the Boss RC600. Other Musicians use a Drumcomputer, Drumpads or a Beatbuddy to bring in some variations of the Drum-Sounds.

<h2>Why?</h2>
I wanted to get a "FILL"-Function in my RC600 to create a Fill-Action to get differences or variations into the drum-loop.
But I was not able to build this with the RC600.
<br>

While holding a ESp32-S3 in my hand, I was thinking if it could act as a USB-Host (yes) and receive MIDI-Clock via USB (yes) and could send MIDI-NoteOn to a Drum-Sound of the RC600 (yes).... all 3 Key-Points were working fine, so I started to create a MIDI-File-Player very specialised to my needs and to the RC600.
... it works as a Sidekick for the RC600.

<h2>Features:</h2>
<ul>
<li>USB-C-Host to drive the RC600 ( not Power, only MIDI-Data ), no MIDI or Audio-Cables needed… Clean Connections
<li>Receive Clock via USB-C, send MIDI-NoteOn/Off for Drums via USB-C
<li>Taktwerk4RC will loop all „loop“-files and play „fill“-files only once
<li>Seamless switching MIDI-Files on the "1": If you press the "Fill" footswitch in the middle of a bar, the song remains synchronized. The device waits until the start of the next bar (tick 0) to switch directly to the next `fill-n.mid` file.
<li>Dynamic time signatures: When the MIDI file is first opened, the meta-event parser reads the time signature specification (e.g., 3/4 or 6/8). The beat ticks and the NeoPixel metronome automatically adjust their counting accordingly.
<li>Real-time volume reduction: Using the optional `FOOT-QUIET` footswitch, the velocity byte of all NoteOn commands is halved within the `midiCallback`, allowing for an instantaneous switch to a quieter passage without latency.
</ul>

<h2>Does everything work???</h2>
I did not test all features, because I was missing usable MIDI-Drum-Grooves on my PC to test all the things. 
Therefore I started to create an editor to extract or create working Fill- or Loop-Files.

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

<img src="https://github.com/ErichHeinemann/taktwerk4rc/blob/main/pictures/Dokumentation.004.png">

!! NEEDED Modifications on the ESp32-S3-Hardware !!!
close or bridge the PADS USB-OTG under the board to enable the USB-OTG-Function<br>
close or bridge the PADS 5V In/Out on the top of the board to enable 5 Volt-PIN to create 5 Volts for the SD-Card-Reader<br>
<img src="https://github.com/ErichHeinemann/taktwerk4rc/blob/main/pictures/Dokumentation.006.png">

# Taktwerk MIDI-File extractor
Check the file taktwerk_editor.html locally, it contains the Code for the Drum-Editor.
This Editor enabled You to load a MIDI-File, select a MIDI-Channel and some Bars to create a new Drum-Track.
It always exports the track to MIDI-Channel 10!
When selecting a bar, You could edit the drums. 
I tried to make it mostly generic and the webpage uses webaudiofonts to trigger samples.

Save the created or extracted files as fill-*.mid or loop-*.mid in a folder of the SD-Card while * is a number.

Probably this tool is handy for other Drum- or MIDI-Experiments with other tools too.

