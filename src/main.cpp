#include <Arduino.h>

#include "hal.h"
#include "ui.h"

void hal_init();
void hal_init_display(bool flipped);

static App app;

void setup() {
    Serial.begin(115200);
    hal_init();
    Settings probe;
    settingsLoad(probe);                 // the panel orientation must be known before the first frame
    hal_init_display(probe.flipped);
    app.begin();
    hal_backlight(app.s.backlight);
}

void loop() {
    app.tick();
    delay(5);
}
