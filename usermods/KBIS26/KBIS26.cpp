#include "wled.h"
//#warning "KBIS26 USERMOD COMPILED"
#pragma message WLED_RELEASE_NAME

struct message_structure_t {
  uint8_t seq[4];
  uint8_t button;
};

class UsermodKBIS26 : public Usermod {
  private:
    uint32_t last_seq = 0;
    static constexpr const char* _name = "KBIS26";

    bool UMhandleWiZdata(const uint8_t* incomingData, size_t len) {
        if (len != sizeof(message_structure_t)) {
            DEBUG_PRINTF_P(PSTR("Unknown ESP-NOW msg length: %u\n"), len);
            return false;
        }

        const message_structure_t* incoming =
            reinterpret_cast<const message_structure_t*>(incomingData);

        uint32_t cur_seq =
            incoming->seq[0] |
            (incoming->seq[1] << 8) |
            (incoming->seq[2] << 16) |
            (incoming->seq[3] << 24);

        if (cur_seq == last_seq) return true;

        last_seq = cur_seq;
        uint8_t button = incoming->button;

        if (button == 1) {
            DEBUG_PRINT(F("WiZ ON button pressed (seq="));
            DEBUG_PRINT(cur_seq);
            DEBUG_PRINTLN(F(")"));
            return true;
        }

        return false;
    }

  public:
    void setup() override {
        DEBUG_PRINTLN(F("KBIS_26 usermod setup"));
        delay(2000);
        Serial.println("KBIS working!!");
    }

    void loop() override {
        static uint32_t t = 0;
        if (millis() - t > 2000) {
            t = millis();
            Serial.println(F("KBIS: loop alive"));
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
    /*
    bool onEspNowMessage(uint8_t* mac, uint8_t* data, int len) {
        if (data[0] == 0x91 || data[0] == 0x81 || data[0] == 0x80) {
            return UMhandleWiZdata(data, len);
        }
        return false;
    }
        */
};

static UsermodKBIS26 kbis26;
REGISTER_USERMOD(kbis26);

