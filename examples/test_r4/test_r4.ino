
#include "peripherials.h"

#include "encoder.h"


#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R2, SCL, SDA, U8X8_PIN_NONE);


Encoder encoder;

const int OUTPUT_COUNT = 6;
int outputs[OUTPUT_COUNT] = {
    OUT_CH1,
    OUT_CH2,
    OUT_CH3,
    OUT_CH4,
    OUT_CH5,
    OUT_CH6,
};

volatile int idx = 0;

// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        pinMode(outputs[i], OUTPUT);
    }

    encoder.AttachRotateHandler(rotateEncoder);
    encoder.AttachPressHandler(press);

    display.begin();

}

void rotateEncoder(int val) {
    idx = (val > 0) 
            ? constrain(idx + 1, 0 , OUTPUT_COUNT)
            : constrain(idx - 1, 0 , OUTPUT_COUNT);
}

// the loop function runs over and over again forever
void loop() {
    encoder.Process();
    UpdateDisplay();

    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    digitalWrite(outputs[idx], HIGH);          // turn the LED on (HIGH is the voltage level)
    delay(500);                          // wait for a second
    digitalWrite(LED_BUILTIN, LOW);       // turn the LED off by making the voltage LOW
    digitalWrite(outputs[idx], LOW);           // turn the LED on (LOW is the voltage level)
    delay(500);                          // wait for a second
}

void press() {
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
        digitalWrite(outputs[i], HIGH);      // turn the LED on (HIGH is the voltage level)
        delay(50);                        // wait for a second
        digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
        digitalWrite(outputs[i], LOW);       // turn the LED on (LOW is the voltage level)
        delay(50);
    }
}


void UpdateDisplay() {
    display.firstPage();
    do {
        display.drawStr(0, 0, "Hello");
    } while (display.nextPage());
}