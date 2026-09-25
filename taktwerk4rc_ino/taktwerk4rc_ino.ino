#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <MD_MIDIFile.h>
#include "EspUsbHost.h"

// ----------------------------------------------------
// PIN DEFINITIONEN (ESP32-S3)
// ----------------------------------------------------
#define OLED_SDA        8
#define OLED_SCL        9

#define SD_CS           10
#define SD_MOSI         11
#define SD_SCK          12
#define SD_MISO         13

#define BTN_SONG_NEXT   4
#define BTN_SONG_PREV   5
#define FOOT_START_STOP 6
#define FOOT_FILL       7
#define FOOT_QUIET      15

#define RGB_LED_PIN     48
#define NUM_LEDS        1

// MIDI-Drum-Konfiguration
#define MIDI_CHANNEL_DRUMS 9   // Kanal 10 (0-basierter Index 9)
#define NOTE_BASS_DRUM     36  // C1 / Kick Drum
#define VELOCITY_KICK      100 

// SD-Objekt (SdFat v2)
SdFs SD;

// ----------------------------------------------------
// DISPLAY & METRONOM & USB SETUP
// ----------------------------------------------------
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_NeoPixel rgbLed(NUM_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
EspUsbHost usb;
MD_MIDIFile SMF;



// ----------------------------------------------------
// ENTPRELLUNG & TASTER-STATUS
// ----------------------------------------------------
const uint32_t DEBOUNCE_DELAY = 50; // 50ms Entprellzeit

uint32_t lastDebounceTime_StartStop = 0;
uint32_t lastDebounceTime_Fill      = 0;
uint32_t lastDebounceTime_Next      = 0;
uint32_t lastDebounceTime_Prev      = 0;

bool lastState_StartStop = HIGH;
bool lastState_Fill      = HIGH;
bool lastState_Next      = HIGH;
bool lastState_Prev      = HIGH;

bool btnState_StartStop = HIGH;
bool btnState_Fill      = HIGH;
bool btnState_Next      = HIGH;
bool btnState_Prev      = HIGH;

// ----------------------------------------------------
// STATE MACHINE & AKTUELLE VARIABLEN
// ----------------------------------------------------
enum PlayState { STATE_STOPPED, STATE_PLAYING_LOOP, STATE_PLAYING_FILL };
volatile PlayState currentState = STATE_STOPPED;

// Song-Verwaltung
String songFolders[20];
uint8_t totalSongs = 0;
int currentSongIdx = 0;

int currentLoopNum = 1;
int currentFillNum = 1;

volatile int16_t lastProgramChange = -1;

// Takt-Signatur & Timing
uint8_t timeSignatureNum = 4; 
uint8_t timeSignatureDen = 4; 

volatile uint32_t clockTicks = 0;
volatile bool fillQueued = false;
volatile bool stopQueued = false;

volatile bool isPlaying = false;
volatile bool noteIsOn = false;
volatile bool ledIsOn = false;

// Forward declaration
void updateDisplay();

// ----------------------------------------------------
// HELPER FUNKTIONEN FÜR DRUM OUTPUT
// ----------------------------------------------------
void sendKickOn() {
    usb.midiSendNoteOn(MIDI_CHANNEL_DRUMS, NOTE_BASS_DRUM, VELOCITY_KICK);
}

void sendKickOff() {
    usb.midiSendNoteOff(MIDI_CHANNEL_DRUMS, NOTE_BASS_DRUM, 0);
}

// ----------------------------------------------------
// MIDI CALLBACK HANDLER (Liest Events aus der Datei)
// ----------------------------------------------------
/*
void midiCallback(midi_event *pev) {
    uint8_t type = pev->data[0] & 0xF0;
    uint8_t channel = pev->data[0] & 0x0F;

    if (type == 0x90 || type == 0x80) {
        uint8_t note = pev->data[1];
        uint8_t velocity = pev->data[2];

        // Quiet-Taster Dämpfung
        if (type == 0x90 && digitalRead(FOOT_QUIET) == LOW) {
            velocity = velocity / 2;
        }

        if (type == 0x90 && velocity > 0) {
            usb.midiSendNoteOn(channel, note, velocity);
            Serial.printf("[MIDI OUT] Note ON -> Ch: %d, Note: %d, Vel: %d\n", channel + 1, note, velocity);
        } else {
            usb.midiSendNoteOff(channel, note, 0);
            Serial.printf("[MIDI OUT] Note OFF -> Ch: %d, Note: %d\n", channel + 1, note);
        }
    }
}
*/
//  MIDI-handler, alles auf Channel 10 !
void midiCallback(midi_event *pev) {
    uint8_t type = pev->data[0] & 0xF0;

    if (type == 0x90 || type == 0x80) {
        uint8_t note = pev->data[1];
        uint8_t velocity = pev->data[2];

        // Direct-Routing aller File-Noten auf den Drum-Kanal (Index 9 = Kanal 10)
        uint8_t targetChannel = MIDI_CHANNEL_DRUMS; 

        if (type == 0x90 && digitalRead(FOOT_QUIET) == LOW) {
            velocity = velocity / 2;
        }

        if (type == 0x90 && velocity > 0) {
            usb.midiSendNoteOn(targetChannel, note, velocity);
            Serial.printf("[MIDI OUT] Note ON -> Ch: %d (mapped), Note: %d, Vel: %d\n", targetChannel + 1, note, velocity);
        } else {
            usb.midiSendNoteOff(targetChannel, note, 0);
        }
    }
}

void sysexCallback(sysex_event *pev) {
    if (pev->data[0] == 0x58 && pev->size >= 5) { 
        timeSignatureNum = pev->data[2];
        timeSignatureDen = 1 << pev->data[3];
        Serial.printf("[MIDI Header] Taktsignatur erkannt: %d/%d\n", timeSignatureNum, timeSignatureDen);
    }
}

// ----------------------------------------------------
// SD & DATEI HELPERS
// ----------------------------------------------------
void scanSongFolders() {
    FsFile root;
    FsFile entry;
    
    if (!root.open("/")) {
        Serial.println("Fehler beim Öffnen des SD-Hauptverzeichnisses!");
        return;
    }

    totalSongs = 0;
    while (entry.openNext(&root, O_READ)) {
        if (entry.isDir()) {
            char nameBuf[64];
            entry.getName(nameBuf, sizeof(nameBuf));
            
            if (nameBuf[0] != '.' && nameBuf[0] != '_') {
                songFolders[totalSongs] = String(nameBuf);
                Serial.printf("Song-Ordner [%d] gefunden: %s\n", totalSongs, nameBuf);
                totalSongs++;
                if (totalSongs >= 20) break;
            }
        }
        entry.close();
    }
    root.close();

    Serial.printf("Insgesamt %d Song-Ordner gefunden.\n", totalSongs);
}

void loadMidiFile(String path) {
    SMF.close();
    
    int err = SMF.load(path.c_str());
    if (err == MD_MIDIFile::E_OK) {
        Serial.println("MIDI erfolgreich geladen: " + path);
        SMF.restart(); // Setzt den Reader auf Anfang der Datei
    } else {
        Serial.printf("Fehler beim Laden von %s (Code: %d)\n", path.c_str(), err);
    }
}

// ----------------------------------------------------
// DISPLAY UPDATE
// ----------------------------------------------------
void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.printf("[%d/%d]", currentSongIdx + 1, totalSongs);
    
    display.setCursor(80, 0);
    if (lastProgramChange >= 0) {
        display.printf("PC:%03d", lastProgramChange);
    } else {
        display.print("PC:---");
    }
    
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 18);
    if (totalSongs > 0) {
        display.println(songFolders[currentSongIdx].substring(0, 10));
    } else {
        display.println("Keine Songs");
    }

    display.setTextSize(1);
    display.setCursor(0, 42);
    display.printf("Takt: %d/%d", timeSignatureNum, timeSignatureDen);

    display.setCursor(0, 54);
    if (currentState == STATE_STOPPED) display.print("Status: STOPPED");
    else if (currentState == STATE_PLAYING_LOOP) display.printf("Status: LOOP #%d", currentLoopNum);
    else if (currentState == STATE_PLAYING_FILL) display.printf("Status: FILL #%d", currentFillNum);

    display.display();
}

// ----------------------------------------------------
// MIDI CLOCK HANDLER (Eingehend vom Looper / Host)
// ----------------------------------------------------
void handleMidiMessage(const EspUsbHostMidiMessage &msg) {
    uint8_t status = msg.status;

    // 1. MIDI CLOCK (0xF8)
    if (status == 0xF8) {
        if (!isPlaying) {
            isPlaying = true; 
        }

        uint32_t beatTick = clockTicks % 96; // 96 Ticks = 1 ganzer 4/4 Takt

        // Alle 24 Ticks (Viertelnote / Beat)
        if (beatTick % 24 == 0) {
            uint8_t beatNumber = (beatTick / 24) + 1; // Beat 1, 2, 3 oder 4

            // --- TAKTGESTEUERTE UMSCHALT-LOGIK (AUF DIE "1") ---
            if (beatNumber == 1) {
                if (stopQueued) {
                    currentState = STATE_STOPPED;
                    SMF.close();
                    stopQueued = false;
                    isPlaying = false;
                    updateDisplay();
                } 
                else if (fillQueued && currentState == STATE_PLAYING_LOOP) {
                    fillQueued = false;
                    currentState = STATE_PLAYING_FILL;
                    
                    String fillPath = "/" + songFolders[currentSongIdx] + "/fill-" + String(currentFillNum) + ".mid";
                    if (!SD.exists(fillPath.c_str())) {
                        fillPath = "/" + songFolders[currentSongIdx] + "/fill-1.mid";
                    }
                    
                    Serial.println("Starte Fill: " + fillPath);
                    loadMidiFile(fillPath);
                    updateDisplay();
                }
            }

            // Metronom Visualisierung
            if (beatNumber == 1) {
                rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0)); // ROT
                // sendKickOn();
                // noteIsOn = true;
            } 
            else if (beatNumber == 3) {
                rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0)); // GRÜN
                // sendKickOn();
                // noteIsOn = true;
            } 
            else {
                rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0)); // GRÜN
            }

            rgbLed.show();
            ledIsOn = true;
        }

        // Nach 12 Ticks (1/8-Note) LED & Test-Kick aus
        if ((beatTick % 24) == 12) {
            if (ledIsOn) {
                rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 0));
                rgbLed.show();
                ledIsOn = false;
            }
            if (noteIsOn) {
                // sendKickOff();
                // noteIsOn = false;
            }
        }

        clockTicks++;
    } 
    // 2. MIDI START (0xFA) / CONTINUE (0xFB)
    else if (status == 0xFA || status == 0xFB) {
        clockTicks = 0;
        isPlaying = true;
        if (currentState != STATE_STOPPED) {
            SMF.restart();
        }
    } 
    // 3. MIDI STOP (0xFC)
    else if (status == 0xFC) {
        isPlaying = false;
        currentState = STATE_STOPPED;
        SMF.close();
        clockTicks = 0;
        
        sendKickOff();
        noteIsOn = false;
        
        rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 0));
        rgbLed.show();
        ledIsOn = false;
        updateDisplay();
    }
}

// ----------------------------------------------------
// SETUP & LOOP
// ----------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(BTN_SONG_NEXT, INPUT_PULLUP);
    pinMode(BTN_SONG_PREV, INPUT_PULLUP);
    pinMode(FOOT_START_STOP, INPUT_PULLUP);
    pinMode(FOOT_FILL, INPUT_PULLUP);
    pinMode(FOOT_QUIET, INPUT_PULLUP);

    rgbLed.begin();
    rgbLed.setBrightness(50);

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED Display nicht gefunden!");
    }

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(24)))) {
        Serial.println("SD-Karten Initialisierung fehlgeschlagen!");
    } else {
        Serial.println("SD-Karte erfolgreich initialisiert.");
        scanSongFolders();
    }

    // MIDI-File Player Setup
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);

    usb.onMidiMessage(handleMidiMessage);
    usb.begin();

    updateDisplay();
}

// ----------------------------------------------------
// MAIN LOOP
// ----------------------------------------------------
// ----------------------------------------------------
// MAIN LOOP
// ----------------------------------------------------
void loop() {
    uint32_t currentMillis = millis();

    // ----------------------------------------------------
    // 1. TASTER LESEN & ENTPRELLEN
    // ----------------------------------------------------
    bool readingStartStop = digitalRead(FOOT_START_STOP);
    bool readingFill      = digitalRead(FOOT_FILL);
    bool readingNext      = digitalRead(BTN_SONG_NEXT);
    bool readingPrev      = digitalRead(BTN_SONG_PREV);

    // Debounce Start/Stop
    if (readingStartStop != lastState_StartStop) {
        lastDebounceTime_StartStop = currentMillis;
    }
    if ((currentMillis - lastDebounceTime_StartStop) > DEBOUNCE_DELAY) {
        btnState_StartStop = readingStartStop;
    }
    lastState_StartStop = readingStartStop;

    // Debounce Fill
    if (readingFill != lastState_Fill) {
        lastDebounceTime_Fill = currentMillis;
    }
    if ((currentMillis - lastDebounceTime_Fill) > DEBOUNCE_DELAY) {
        btnState_Fill = readingFill;
    }
    lastState_Fill = readingFill;

    // Debounce Next
    if (readingNext != lastState_Next) {
        lastDebounceTime_Next = currentMillis;
    }
    if ((currentMillis - lastDebounceTime_Next) > DEBOUNCE_DELAY) {
        btnState_Next = readingNext;
    }
    lastState_Next = readingNext;

    // Debounce Prev
    if (readingPrev != lastState_Prev) {
        lastDebounceTime_Prev = currentMillis;
    }
    if ((currentMillis - lastDebounceTime_Prev) > DEBOUNCE_DELAY) {
        btnState_Prev = readingPrev;
    }
    lastState_Prev = readingPrev;


    // ----------------------------------------------------
    // 2. NOT-AUS CHECK (BEIDE FUßTASTER GEDRÜCKT)
    // ----------------------------------------------------
    if (btnState_StartStop == LOW && btnState_Fill == LOW) {
        if (currentState != STATE_STOPPED) {
            Serial.println("!!! NOT-AUS REVOLVIERT !!! Sofortiger Stopp.");
            currentState = STATE_STOPPED;
            SMF.close();
            stopQueued = false;
            fillQueued = false;
            isPlaying = false;
            
            // Metronom / Kick ausschalten & LED resetten
            sendKickOff();
            noteIsOn = false;
            rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 0));
            rgbLed.show();
            ledIsOn = false;

            updateDisplay();
            
            // Warte kurz, damit Not-Aus nicht mehrfach getriggert wird
            delay(500); 
        }
    } 
    else {
        // ----------------------------------------------------
        // 3. EINZELNE TASTER LOGIK (FLANKENERKENNUNG)
        // ----------------------------------------------------
        
        // --- START / STOP TASTATUR ---
        static bool prevBtnStartStop = HIGH;
        if (btnState_StartStop == LOW && prevBtnStartStop == HIGH) { // Taste wurde GERADE gedrückt
            if (currentState == STATE_STOPPED) {
                currentState = STATE_PLAYING_LOOP;
                currentLoopNum = 1;
                stopQueued = false;
                fillQueued = false;
                clockTicks = 0;

                String loopPath = "/" + songFolders[currentSongIdx] + "/loop-1.mid";
                loadMidiFile(loopPath);
                updateDisplay();
            } else {
                stopQueued = true;
                Serial.println("Stop für nächste '1' vorgemerkt!");
            }
        }
        prevBtnStartStop = btnState_StartStop;

        // --- FILL TASTATUR ---
        static bool prevBtnFill = HIGH;
        if (btnState_Fill == LOW && prevBtnFill == HIGH) { // Taste wurde GERADE gedrückt
            if (currentState == STATE_PLAYING_LOOP && !fillQueued) {
                fillQueued = true;
                Serial.println("Fill für nächste '1' vorgemerkt!");
            }
        }
        prevBtnFill = btnState_Fill;

        // --- SONG NEXT ---
        static bool prevBtnNext = HIGH;
        if (btnState_Next == LOW && prevBtnNext == HIGH) {
            if (totalSongs > 0) {
                currentSongIdx = (currentSongIdx + 1) % totalSongs;
                updateDisplay();

                if (currentState != STATE_STOPPED) {
                    currentLoopNum = 1;
                    String loopPath = "/" + songFolders[currentSongIdx] + "/loop-1.mid";
                    loadMidiFile(loopPath);
                    currentState = STATE_PLAYING_LOOP;
                }
            }
        }
        prevBtnNext = btnState_Next;

        // --- SONG PREV ---
        static bool prevBtnPrev = HIGH;
        if (btnState_Prev == LOW && prevBtnPrev == HIGH) {
            if (totalSongs > 0) {
                currentSongIdx = (currentSongIdx - 1 + totalSongs) % totalSongs;
                updateDisplay();

                if (currentState != STATE_STOPPED) {
                    currentLoopNum = 1;
                    String loopPath = "/" + songFolders[currentSongIdx] + "/loop-1.mid";
                    loadMidiFile(loopPath);
                    currentState = STATE_PLAYING_LOOP;
                }
            }
        }
        prevBtnPrev = btnState_Prev;
    }


    // ----------------------------------------------------
    // 4. MIDI FILE EVENT VERARBEITUNG
    // ----------------------------------------------------
    if (currentState != STATE_STOPPED) {
        if (!SMF.isEOF()) {
            SMF.getNextEvent(); 
        } else {
            if (currentState == STATE_PLAYING_LOOP) {
                SMF.restart();
            } 
            else if (currentState == STATE_PLAYING_FILL) {
                currentState = STATE_PLAYING_LOOP;
                String loopPath = "/" + songFolders[currentSongIdx] + "/loop-" + String(currentLoopNum) + ".mid";
                if (!SD.exists(loopPath.c_str())) {
                    loopPath = "/" + songFolders[currentSongIdx] + "/loop-1.mid";
                }
                loadMidiFile(loopPath);
                updateDisplay();
            }
        }
    }

    delay(1);
}
