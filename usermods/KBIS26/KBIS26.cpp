
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "wled.h"

// Remove Adafruit's generic macros so WLED's can exist without warnings
#ifdef WHITE
  #undef WHITE
#endif
#ifdef BLACK
  #undef BLACK
#endif
class UsermodKBIS26 : public Usermod {
  private:
    
    const int LED1 = 16;
    const int REED = 12;
    const int TIMEBUTTON = 13;
    const int POT = 36;//A0
    const int MAXEFFECTTIME = 40000;
    const int MINEFFECTTIME = 1000;
    unsigned long timestamp1 = 0;
    int effectTime = (MAXEFFECTTIME - MINEFFECTTIME)/2;
    int previousEffectTime = 0;
    uint8_t brightness = 150;
    int previousBrightnessOut = -1;
    const uint8_t PRESET_ACTIVE = 2; // bright/effect
    const uint8_t PRESET_IDLE   = 1; // dim/normal
    uint8_t presetState = PRESET_IDLE;
    uint8_t previousPresetState = PRESET_ACTIVE; 
    bool presetLaunched = false;
    static const char _name[] PROGMEM;

    // oled
    static constexpr int OLED_RESET = -1;        // Reset pin # (or -1 if sharing Arduino reset pin)
    static constexpr uint8_t SCREEN_ADDRESS = 0x3C;  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
    static constexpr int SCREEN_WIDTH = 128;     // OLED display width, in pixels
    static constexpr int SCREEN_HEIGHT = 32;     // OLED display height, in pixels
    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    bool displayReady = false;

    void resetDisplay() {
    if (!displayReady) return;
    display.clearDisplay();
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);
    }

    template<typename T>
    void printStatus(const T& value) {
    resetDisplay();
    display.print(value);
    }

    void pollTimerButton(){
        static unsigned long timestamp2 = 0;
        int delay = 500;
        unsigned long currentTime = millis();
        if (currentTime - timestamp2 <= delay){
            return;
        }
        if (!digitalRead(TIMEBUTTON)){
            effectTime += 1000;
            if (effectTime > MAXEFFECTTIME){
            effectTime = 0;
            }
            timestamp2 = currentTime;
        } 
    }

    void readPotBrightnessFiltered(){
        static uint32_t lastSampleMs = 0;
        static float filt = 0;            // filtered ADC (float is fine on ESP32)
        static int brightnessOut = 1;     // reported brightness 1..255
        // Sample at ~200 Hz
        const uint32_t SAMPLE_PERIOD_MS = 5;
        uint32_t now = millis();
        if (now - lastSampleMs < SAMPLE_PERIOD_MS) {//poll pot every 5ms
            if (brightnessOut == previousBrightnessOut){//do nothing if nothing has changed
                return;
            }
            previousBrightnessOut = brightnessOut;
            disPot(brightnessOut);//send to display when something has changed
            return;
        }
        lastSampleMs = now;

        // Oversample (average)
        const int N = 8;
        uint32_t sum = 0;
        for (int i = 0; i < N; i++) sum += analogRead(POT);
        float raw = (float)sum / N; // raw ADC ~0..4095

        // Initialize filter on first run
        if (filt == 0) filt = raw;

        // Adaptive EMA
        float delta = fabsf(raw - filt);
        const float slowAlpha = 0.04f;  // very stable
        const float fastAlpha = 0.25f;  // snappy while moving
        const float fastThreshold = 35.0f; // ADC counts; when turning, delta exceeds this

        float a = (delta > fastThreshold) ? fastAlpha : slowAlpha;
        filt += a * (raw - filt);

        // Map filtered ADC -> percent 0..100 (rounded)
        int p = (int)lroundf((filt / 4095.0f) * 100.0f);
        if (p < 0) p = 0;
        if (p > 100) p = 100;

        // Output hysteresis (in brightness units)
        const int HYST = 3; 
        if (abs(p - brightnessOut) >= HYST) {
            brightnessOut = p;
        }
        
        if (brightnessOut == previousBrightnessOut){
            return;
        }
        previousBrightnessOut = brightnessOut;
        disPot(brightnessOut);
        return;
        
    }

    void disPot(int perc){
        display.fillRect(0, 8, 128, 8, SSD1306_BLACK);
        display.setCursor(0,8);
        display.print(perc);
        display.display(); 
    }

    void monitorDoor() {
        static bool doorClosed = false;

        bool reedClosed = (digitalRead(REED) == LOW)? true: false; // closed = LOW

        if (!reedClosed) { // door open
            presetState = PRESET_IDLE;
            doorClosed = false;
            return;
        }
        if (!doorClosed){//door closed for first time
            timestamp1 = millis();
            doorClosed = true;
        }
        if (millis() - timestamp1 > effectTime) {//initially false but when true: door closed but time has elapsed so stop effect
            presetState = PRESET_IDLE;
            return;
        }
        presetState = PRESET_ACTIVE;
    }
    void initiatePreset(){
        uint8_t targetBri = (presetState == PRESET_IDLE) ? 6 : 255;

        if (bri != targetBri) {
            bri = targetBri;
            stateUpdated(CALL_MODE_DIRECT_CHANGE);
        }
        if (presetState == previousPresetState){
            return;
        }
        applyPreset(presetState, CALL_MODE_DIRECT_CHANGE);
        previousPresetState = presetState;
    }

  public:
    void setup() override {
        pinMode(REED, INPUT_PULLUP);
        analogReadResolution(12);
        Wire.begin(I2CSDAPIN, I2CSCLPIN);
        Wire.setClock(400000);

        // Init the OLED
        displayReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println("SSD1306 not found.");
        } else {
            Serial.println("SSD1306 found.");
        }
        display.display();  // SSD splash screen
        
        resetDisplay();
        display.display();

        Serial.println("KBIS working!!");
    }

    void loop() override {
        if (strip.isUpdating()) return;
        monitorDoor();
        initiatePreset();
        pollTimerButton();
        readPotBrightnessFiltered();
        if(previousEffectTime != effectTime){
            previousEffectTime = effectTime;
            printStatus(effectTime/1000);
            display.display();
        }
    }
    
    void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["active"] = true;
    top["message"] = "KBIS26 usermod is running";
    }
    
  uint16_t getId() override {
    return USERMOD_ID_UNSPECIFIED;
  }
};

const char UsermodKBIS26::_name[] PROGMEM = "KBIS26";
static UsermodKBIS26 kbis26;

REGISTER_USERMOD(kbis26);

