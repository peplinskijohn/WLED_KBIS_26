
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Preferences.h>

#include "wled.h"

// Remove Adafruit's generic macros so WLED's can exist without warnings
#ifdef WHITE
#undef WHITE
#endif
#ifdef BLACK
#undef BLACK
#endif
const char LABEL_ACTIVE_TIME[] = "ACT Time:";
const char LABEL_ACTIVE_BRI[] = "ACT Bri :";
const char LABEL_IDLE_BRI[] = "IDL Bri :";
const char LABEL_DEFAULT[] = "DFLT";
class UsermodKBIS26 : public Usermod {
   private:
    const int LED1 = 16;
    const int LED2 = 3;
    const int REED = 12;
    const int TIMEBUTTON = 13;
    const int POT = 36;  // A0
    const int MAXEFFECTTIME = 60000;
    const int MINEFFECTTIME = 1000;

    unsigned long timestamp1 = 0;
    const int effectTime_default = 30000;
    int effectTime = effectTime_default;
    int previousEffectTime = 0;
    int previousBrightnessOut = -1;
    const uint8_t PRESET_ACTIVE = 2;  // bright/effect
    const uint8_t PRESET_IDLE = 1;    // dim/normal
    uint8_t presetState = PRESET_IDLE;
    uint8_t previousPresetState = PRESET_ACTIVE;
    bool presetLaunched = false;
    const uint8_t idleBri_default = 6;
    const uint8_t activeBri_default = 255;
    uint8_t idleBri = idleBri_default;
    uint8_t activeBri = activeBri_default;
    enum CursorStateIndex { EFFECT_TIME = 0, ACTIVE, IDLE, EMPTY_STATE, CURSOR_STATE_COUNT };
    uint8_t cursorState[CURSOR_STATE_COUNT];
    uint8_t currentCursor = EMPTY_STATE;
    bool cfgDirty = false;
    uint32_t cfgDirtySinceMs = 0;
    uint32_t bootUpDelay = 1000;
    uint32_t bootUpTimestamp = 0;
    const uint8_t bootUpBri = 10;

    static const char _name[] PROGMEM;
    Preferences prefs;

    // oled
    static constexpr int OLED_RESET = -1;            // Reset pin # (or -1 if sharing Arduino reset pin)
    static constexpr uint8_t SCREEN_ADDRESS = 0x3C;  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
    static constexpr int SCREEN_WIDTH = 128;         // OLED display width, in pixels
    static constexpr int SCREEN_HEIGHT = 32;         // OLED display height, in pixels
    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    bool displayReady = false;

    void loadFromPrefs() {
        idleBri = prefs.getUChar("idleBri", idleBri_default);
        activeBri = prefs.getUChar("activeBri", activeBri_default);
        effectTime = prefs.getUInt("effTime", effectTime_default);
    }

    void saveToPrefs() {
        prefs.putUChar("idleBri", idleBri);
        prefs.putUChar("activeBri", activeBri);
        prefs.putUInt("effTime", effectTime);
    }

    void resetDisplay() {
        if (!displayReady) return;
        display.clearDisplay();
        display.setTextSize(1);               // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE);  // Draw white text
        display.setCursor(0, 0);
    }
    void setOledContrast(uint8_t c) {
        display.ssd1306_command(SSD1306_SETCONTRAST);  // = 0x81
        display.ssd1306_command(c);                    // 0..255
    }
    void cursorSelectButton() {
        static unsigned long timestamp2 = 0;
        int delay = 250;
        int timeout = 60000;

        unsigned long currentTime = millis();
        if (currentTime - timestamp2 <= delay) {
            return;
        }
        if (!digitalRead(TIMEBUTTON)) {
            setOledContrast(255);
            currentCursor++;
            if (currentCursor >= CURSOR_STATE_COUNT) {
                currentCursor = 0;
            }
            timestamp2 = currentTime;
            // Serial.println(currentCursor);
        }
        if (currentTime >= timestamp2 + timeout) {
            setOledContrast(1);
            currentCursor = EMPTY_STATE;
        }
    }

    uint16_t pollPot() {
        static uint32_t lastSampleMs = 0;
        static float filt = 0;  // filtered ADC (float is fine on ESP32)
        static uint16_t cached = 0;
        const uint32_t SAMPLE_PERIOD_MS = 5;
        uint32_t now = millis();
        if (now - lastSampleMs < SAMPLE_PERIOD_MS) {  // poll pot every 5ms
            return cached;
        }
        lastSampleMs = now;

        // Oversample (average)
        const int N = 8;
        uint32_t sum = 0;
        for (int i = 0; i < N; i++) sum += analogRead(POT);
        float raw = (float)sum / N;  // raw ADC ~0..4095

        // Initialize filter on first run
        if (filt == 0) filt = raw;

        // Adaptive EMA
        float delta = fabsf(raw - filt);
        const float slowAlpha = 0.04f;      // very stable
        const float fastAlpha = 0.25f;      // snappy while moving
        const float fastThreshold = 35.0f;  // ADC counts; when turning, delta exceeds this

        float a = (delta > fastThreshold) ? fastAlpha : slowAlpha;
        filt += a * (raw - filt);

        cached = (uint16_t)constrain((int)lroundf(filt), 0, 4095);
        return cached;
    }

    void clearCursor() {  // clear cursor column (right side)
        display.fillRect(120, 0, 8, 32, SSD1306_BLACK);
    }

    void setCursor(uint8_t row) {
        row = constrain(row, 0, 3);
        uint8_t y = row * 8;
        display.setCursor(120, y);
        display.print("<");
        display.display();
        // Serial.println("cursor");
    }

    void updateLine(uint8_t row, const char* label, const char* unit) {
        row = constrain(row, 0, 3);
        uint8_t y = row * 8;
        display.fillRect(0, y, 128, 8, SSD1306_BLACK);

        display.setCursor(0, y);
        display.print(label);

        display.print(" ");
        display.print(unit);
        clearCursor();
        setCursor(row);
    }

    void updateLine(uint8_t row, const char* label, int value, const char* unit) {
        uint8_t y = row * 8;
        display.fillRect(0, y, 128, 8, SSD1306_BLACK);

        display.setCursor(0, y);
        display.print(label);
        display.print(" ");

        display.setCursor(60, y);
        display.print(value);

        display.print(" ");
        display.print(unit);
        clearCursor();
        setCursor(row);
    }
    void markConfigDirty() {
        cfgDirty = true;
        cfgDirtySinceMs = millis();
    }
    void cursor() {
        cursorSelectButton();
        static int previousDisplayVal = -1;
        static uint8_t prevCursor = 255;
        static bool armed = false;       // waiting for pot movement?
        static uint16_t armReading = 0;  // pot reading at time of cursor change
        uint16_t reading = pollPot();

        if (currentCursor != prevCursor) {
            prevCursor = currentCursor;
            // previousDisplayVal = -999999;   // force value print
            clearCursor();
            setCursor(currentCursor);
            armed = true;
            armReading = reading;
            if (currentCursor == EMPTY_STATE) {
                setCursor(currentCursor);
            }        // only refresh display once per cursor change
            return;  // need more loops to make armReading differant than reading
        }

        const uint16_t CAPTURE_DELTA = 25;  // ADC counts; tune (20–80 typical)
        if (armed) {
            if ((uint16_t)abs((int)reading - (int)armReading) < CAPTURE_DELTA) {
                return;  // pot hasn't moved enough yet
            }
            armed = false;  // pot moved → now it can control the parameter
        }

        int displayVal = 0;
        const int HYST = 1;
        uint8_t ycoord = 0;

        switch (currentCursor) {
            case EFFECT_TIME: {
                ycoord = 0;
                const int STEP_MS = 5000;

                int t = map(reading, 4095, 0, MINEFFECTTIME, MAXEFFECTTIME);
                t = ((t + STEP_MS / 2) / STEP_MS) * STEP_MS;
                t = constrain(t, MINEFFECTTIME, MAXEFFECTTIME);

                // "DFLT" sentinel -> store default instead of the sentinel
                uint32_t newEffectTime = (t == MINEFFECTTIME) ? effectTime_default : (uint32_t)t;

                // Only act if the stored value actually changes
                if (newEffectTime != (uint32_t)effectTime) {
                    effectTime = newEffectTime;

                    if (t == MINEFFECTTIME) {
                        updateLine(ycoord, LABEL_ACTIVE_TIME, "DFLT");
                    } else {
                        int sec = effectTime / 1000;
                        updateLine(ycoord, LABEL_ACTIVE_TIME, sec, "s");
                    }

                    markConfigDirty();
                }

                previousDisplayVal = t;  // keep this for UI/knob hysteresis if you want
                break;
            }
            case ACTIVE: {
                ycoord = 1;
                int pct = map(reading, 4095, 0, 0, 100);

                if (abs(pct - previousDisplayVal) >= HYST) {
                    uint8_t newActive = (pct == 0) ? activeBri_default : (uint8_t)map(reading, 4095, 0, 0, 255);

                    if (newActive != activeBri) {
                        activeBri = newActive;

                        if (pct == 0)
                            updateLine(ycoord, LABEL_ACTIVE_BRI, LABEL_DEFAULT);
                        else
                            updateLine(ycoord, LABEL_ACTIVE_BRI, pct, "%");

                        markConfigDirty();
                    }

                    previousDisplayVal = pct;
                }
                break;
            }
            case IDLE: {
                ycoord = 2;
                int pct = map(reading, 4095, 0, 0, 100);

                if (abs(pct - previousDisplayVal) >= HYST) {
                    uint8_t newIdle = (pct == 0) ? idleBri_default : (uint8_t)map(reading, 4095, 0, 0, 255);

                    if (newIdle != idleBri) {
                        idleBri = newIdle;

                        if (pct == 0)
                            updateLine(ycoord, LABEL_IDLE_BRI, LABEL_DEFAULT);
                        else
                            updateLine(ycoord, LABEL_IDLE_BRI, pct, "%");

                        markConfigDirty();
                    }

                    previousDisplayVal = pct;
                }
                break;
            }
            case EMPTY_STATE:  // this case is already dealt with in if(currentCursor != prevCursor) guard.
                break;
            default:
                // Serial.println("Faulty cursor state.");
                ;
        }
    }
    void updateDoorLine(bool gate) {
        display.fillRect(0, 24, 60, 8, SSD1306_BLACK);
        if (gate) {
            display.setCursor(0, 24);
            display.print("CLSD");
        }
    }
    void monitorDoor() {
        static bool doorClosed = false;
        bool reedClosed = (digitalRead(REED) == LOW) ? true : false;  // closed = LOW

        if (!reedClosed) {  // door open
            presetState = PRESET_IDLE;
            updateDoorLine(false);  // clear door line
            if (doorClosed) {
                display.display();
                // Serial.println("open");
            }
            doorClosed = false;
            return;
        }
        if (!doorClosed) {  // door closed for first time
            timestamp1 = millis();
            doorClosed = true;
            updateDoorLine(true);  // indicate door closed
            display.display();
            // Serial.println("closed");
        }
        if (millis() - timestamp1 > effectTime) {  // initially false but when true: door closed but time has elapsed so stop effect
            presetState = PRESET_IDLE;
            return;
        }
        presetState = PRESET_ACTIVE;
    }

    void updateEffectLine(bool gate) {
        display.fillRect(60, 24, 60, 8, SSD1306_BLACK);
        if (gate) {
            display.setCursor(60, 24);
            display.print("EFCT");
        }
    }

    void initiatePreset() {
        if (presetState == previousPresetState) {
            return;
        }
        bool gateOpen =((int32_t)(millis() - bootUpTimestamp) >= 0);
        uint8_t targetBri = gateOpen ? ((presetState == PRESET_IDLE) ? idleBri : activeBri) : bootUpBri;  
        if (bri != targetBri) {
            bri = targetBri;
            stateUpdated(CALL_MODE_DIRECT_CHANGE);
        }
        applyPreset(presetState, CALL_MODE_DIRECT_CHANGE);

        previousPresetState = presetState;
    }
    uint32_t macDelayMs(uint32_t maxDelayMs) {
        uint64_t mac = ESP.getEfuseMac();
        uint32_t x = (uint32_t)(mac ^ (mac >> 32));
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x % maxDelayMs;
    }

   public:
    void setup() override {
        Wire.begin(I2CSDAPIN, I2CSCLPIN);  // not in usermod??
        Wire.setClock(400000);
        Wire.setTimeOut(50);
        pinMode(REED, INPUT_PULLUP);
        analogReadResolution(12);
        // pinMode(LED1, OUTPUT);
        // pinMode(LED2, OUTPUT);
        pinMode(REED, INPUT_PULLUP);
        pinMode(TIMEBUTTON, INPUT_PULLUP);
        // Serial.begin(9600);
        // delay(2000);
        // Serial.println("Serial active");
        prefs.begin("kbis26", false);
        loadFromPrefs();  // should NOT call begin/end anymore

        bootUpDelay = macDelayMs(8000);

        // Init the OLED
        displayReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            // Serial.println("SSD1306 not found.");
        } else {
            // Serial.println("SSD1306 found.");
        }
        display.display();  // SSD splash screen
        resetDisplay();

        // explicitly print first lines of display
        // ACTIVE TIME
        if (effectTime == effectTime_default) {
            updateLine(0, LABEL_ACTIVE_TIME, LABEL_DEFAULT);
        } else {
            updateLine(0, LABEL_ACTIVE_TIME, (int)(effectTime / 1000), "s");
        }

        // ACTIVE BRIGHTNESS
        if (activeBri == activeBri_default) {
            updateLine(1, LABEL_ACTIVE_BRI, LABEL_DEFAULT);
        } else {
            updateLine(1, LABEL_ACTIVE_BRI, (int)map(activeBri, 0, 255, 0, 100), "%");
        }

        // IDLE BRIGHTNESS
        if (idleBri == idleBri_default) {
            updateLine(2, LABEL_IDLE_BRI, LABEL_DEFAULT);
        } else {
            updateLine(2, LABEL_IDLE_BRI, (int)map(idleBri, 0, 255, 0, 100), "%");
        }
        clearCursor();
        setCursor(currentCursor);
    }

    void loop() override {
        static bool applied = false;
        static uint32_t bootMs = 0;

        // wait 2s while wled comes up
        if (bootMs == 0) bootMs = millis();
        if (!applied && millis() - bootMs > 2000) {
            applied = true;
            applyPreset(presetState, CALL_MODE_DIRECT_CHANGE);
            bootUpTimestamp = millis() + bootUpDelay;
        }

        if (strip.isUpdating()) return;
        monitorDoor();
        if (applied) initiatePreset();
        cursor();

        if (!strip.isUpdating() && cfgDirty && millis() - cfgDirtySinceMs > 10000) {
            cfgDirty = false;
            saveToPrefs();
        }
    }
    
    void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["active"] = true;
    top["message"] = "KBIS26 usermod is running";
    }

    uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }
};

const char UsermodKBIS26::_name[] PROGMEM = "KBIS26";
static UsermodKBIS26 kbis26;

REGISTER_USERMOD(kbis26);
