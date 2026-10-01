#include "display_controller.h"

#include "device_config.h"

#include <U8g2lib.h>
#include <Wire.h>

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

bool DisplayController::begin() {
    Wire.begin(4, 5);
    if (!u8g2.begin()) {
        return false;
    }
    u8g2.setContrast(255);
    lastState_ = DeviceState::Boot;
    lastLevel_ = 255;
    lastLang_ = SessionLanguage::English;
    dirty_ = true;
    return true;
}

void DisplayController::show(DeviceState state, uint8_t panicLevel, SessionLanguage language) {
    if (!dirty_ && state == lastState_ && panicLevel == lastLevel_ && language == lastLang_) {
        return;
    }
    lastState_ = state;
    lastLevel_ = panicLevel;
    lastLang_ = language;
    dirty_ = false;

    u8g2.clearBuffer();
    if (language == SessionLanguage::MandarinGlitch) {
        drawMandarin(state, panicLevel);
    } else {
        drawEnglish(state, panicLevel);
    }
    u8g2.sendBuffer();
}

void DisplayController::drawEnglish(DeviceState state, uint8_t panicLevel) {
    u8g2.setFont(u8g2_font_helvB14_tr);
    if (state == DeviceState::Boot) {
        u8g2.drawStr(0, 20, "HELLO WORLD");
        u8g2.drawStr(0, 44, DEVICE_NAME);
        return;
    }
    if (state == DeviceState::Lockout) {
        u8g2.drawStr(0, 24, "SYSTEM");
        u8g2.drawStr(0, 48, "OFFLINE");
        return;
    }
    if (state == DeviceState::Level5Sequence || panicLevel >= 5) {
        u8g2.drawStr(0, 24, "PANIC");
        u8g2.drawStr(0, 48, "OVERLOAD");
        return;
    }
    if (panicLevel == 0) {
        u8g2.drawStr(0, 32, "ALL CLEAR");
        return;
    }
    char line[12] = "PANIC";
    size_t len = 5;
    const uint8_t marks = (panicLevel <= 1) ? 0 : ((panicLevel > 4) ? 3 : (panicLevel - 1));
    for (uint8_t i = 0; i < marks; ++i) {
        line[len++] = '!';
    }
    line[len] = '\0';
    u8g2.drawStr(0, 32, line);
}

void DisplayController::drawMandarin(DeviceState state, uint8_t panicLevel) {
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    if (state == DeviceState::Boot) {
        u8g2.drawUTF8(0, 20, "系统启动");
        u8g2.setFont(u8g2_font_helvB14_tr);
        u8g2.drawStr(0, 44, DEVICE_NAME);
        return;
    }
    if (state == DeviceState::Lockout) {
        u8g2.drawUTF8(0, 32, "系统离线");
        return;
    }
    if (state == DeviceState::Level5Sequence || panicLevel >= 5) {
        u8g2.drawUTF8(0, 32, "恐慌过载");
        return;
    }
    if (panicLevel == 0) {
        u8g2.drawUTF8(0, 32, "一切正常");
        return;
    }
    if (panicLevel == 1) {
        u8g2.drawUTF8(0, 32, "恐慌");
    } else if (panicLevel == 2) {
        u8g2.drawUTF8(0, 32, "恐慌!");
    } else if (panicLevel == 3) {
        u8g2.drawUTF8(0, 32, "恐慌!!");
    } else {
        u8g2.drawUTF8(0, 32, "恐慌!!!");
    }
}
