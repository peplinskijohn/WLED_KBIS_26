
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
    static constexpr char _name[] = "KBIS26";
    const int LED1 = 16;
    const int REED = 12;
    const int TIMEBUTTON = 13;
    const int POT = A0;
    const int MAXEFFECTTIME = 40000;
    const int MINEFFECTTIME = 1000;
    unsigned long timestamp1 = 0;
    int effectTime = (MAXEFFECTTIME - MINEFFECTTIME)/2;
    int previousEffectTime = 0;
    uint8_t brightness = 150;
    int oldvar = 0;
    const uint8_t PRESET_ACTIVE = 2; // bright/effect
    const uint8_t PRESET_IDLE   = 1; // dim/normal
    bool presetLaunched = false;

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

    void pollpot(){
        int var = analogRead(POT);
        uint8_t perc = map(var,1,4095,1,100);
        var = map(var,0,4095,1,255);
        if (abs(var-oldvar)>2){
            brightness = var;
            display.fillRect(0, 8, 128, 8, SSD1306_BLACK);
            display.setCursor(0,8);
            display.print(perc);
            display.display();
            oldvar = var;
        }
    }

    void LEDcontrol() {
        static bool doorClosed = false;

        bool reedClosed = (digitalRead(REED) == LOW)? true: false; // closed = LOW

        if (!reedClosed) { // door open
            doorClosed = false;

            if (presetLaunched) {
            applyPreset(PRESET_IDLE, CALL_MODE_DIRECT_CHANGE);
            presetLaunched = false;
            Serial.println("preset idle.");
            }
            return;
        }
        if (!presetLaunched){
            applyPreset(PRESET_ACTIVE, CALL_MODE_DIRECT_CHANGE);
            presetLaunched = true;
            Serial.println("preset active");
        }
        return;
        
    }

  public:
    void setup() override {
        pinMode(REED, INPUT_PULLUP);
          // Ensure I2C is up on your chosen pins
        Wire.begin(I2CSDAPIN, I2CSCLPIN);   // uses your -D I2CSDAPIN=... -D I2CSCLPIN=...

        // Optional but often helps stability
        Wire.setClock(400000);             // or 100000 if you have long wires

        // Init the OLED
        displayReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println("SSD1306 not found.");
        } else {
            Serial.println("SSD1306 found.");
        }
        display.display();  // SSD splash screen
        //delay(2000);
        resetDisplay();
        display.display();

        Serial.println("KBIS working!!");
    }

    void loop() override {
        LEDcontrol();
        pollTimerButton();
        pollpot();
        if(previousEffectTime != effectTime){
            previousEffectTime = effectTime;
            printStatus(effectTime/1000);
            display.display();
        }
    }
    /*
    void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["active"] = true;
    top["message"] = "KBIS26 usermod is running";
    }
    */
  uint16_t getId() override {
    return USERMOD_ID_UNSPECIFIED;
  }
};

static UsermodKBIS26 kbis26;
REGISTER_USERMOD(kbis26);

