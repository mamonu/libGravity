/**
 * Analog Input Attenuate and Offset Script
 *
 * This script provides a demonstration of representing cv input on a 270
 * degree arc (similar to the range of a potentiometer) with 12 o'clock
 * representing 0v, ~5 o'clock representing 5v and ~7 o'clock representing 0v.
 * 
 * The input voltage can be attenuated/attenuverted and offset. The arc will
 * shrink and rotate according to the attenuate/offset values and the input cv
 * will be displayed within the configured boundaries.
 * 
 * Note: drawing an arc is expensive and there are a lot of arcs in this
 * sketch, so the refresh rate is pretty slow.
 *
 */

#include "libGravity.h"

#define TEXT_FONT u8g2_font_profont11_tf

byte selected_param = 0;
int offset = 0;
int attenuate = 100;

// Initialize the gravity library and attach your handlers in the setup method.
void setup() {
    gravity.Init();
    gravity.encoder.AttachRotateHandler(CalibrateCV);
    gravity.encoder.AttachPressHandler(NextCalibrationPoint);
}

// The loop method must always call `gravity.Process()` to read any peripherial state changes.
void loop() {
    gravity.Process();
    UpdateDisplay();
}

void NextCalibrationPoint() {
    selected_param = (selected_param + 1) % 2;
}

void CalibrateCV(int val) {
    // AnalogInput* cv = (selected_param > 2) ? &gravity.cv2 : &gravity.cv1;
    AnalogInput* cv = &gravity.cv1;
    switch (selected_param % 2) {
        case 0:
            attenuate = constrain(attenuate + val, -100, 100);
            cv->SetAttenuation(float(attenuate) / 100.0f);
            break;
        case 1:
            offset = constrain(offset + val, -100, 100);
            cv->SetOffset(float(offset) / 100.0f);
            break;
    }
}

void UpdateDisplay() {
    gravity.display.firstPage();
    do {
        // Set default font mode and color for each page draw
        gravity.display.setFontMode(0);   // Transparent font background
        gravity.display.setDrawColor(1);  // Draw with color 1 (ON)
        gravity.display.setFont(TEXT_FONT);

        DisplayCalibrationArc(&gravity.cv1, 0);
    } while (gravity.display.nextPage());
}

void DisplayCalibrationArc(AnalogInput* cv, int index) {
    gravity.display.setFont(TEXT_FONT);
    int text_ascent = gravity.display.getAscent();

    int inputValue = cv->Read();

    // Print param label, param value, and internal value.
    gravity.display.setCursor(0, 64);
    switch (selected_param) {
        case 0:
            gravity.display.print("attenuate: ");
            if (attenuate >= 0) gravity.display.print(" ");
            gravity.display.print(attenuate);
            gravity.display.print("%");
            break;
        case 1:
            gravity.display.print("offset: ");
            if (offset >= 0) gravity.display.print(" ");
            gravity.display.print(offset);
            gravity.display.print("%");
            break;
    }
    gravity.display.setCursor(100, 64);
    if (inputValue >= 0) gravity.display.print(" ");
    gravity.display.print(inputValue);

    // If attenuate is 0, do not draw arc.
    if (attenuate == 0) {
        return;
    }

    // Arc drawing parameters.
    const int arc_cx = 64;
    const int arc_cy = 32;
    const int outter_radius = 28;
    const int inner_radius = 12;
    const int arc_dist = outter_radius - inner_radius;  // (28 - 12) = 16
    const int arc_north = 64;                           // Approx (90.0 / 360.0) * 255.0
    const int half_arc = 96;                            // Approx (135.0 / 360.0) * 255.0
    const int max_start = 223;  // map(360-45, 0, 360, 0, 255);  // 315 -> 223
    const int max_end = 159;    // map(270-45, 0, 360, 0, 255);    // 225 -> 159
    int start = max_start;
    int end = max_end;

    // Modify the cv arc frame start/end according to the attenuate/offset values.
    if (attenuate != 100) {
        float attenuation_factor = abs(float(attenuate) / 100.0f);
        int attenuate_amount = round((float)half_arc * (1.0f - attenuation_factor));
        start += attenuate_amount;
        end -= attenuate_amount;
    }
    if (offset != 0) {
        float offset_factor = float(offset) / 100.0f;
        int offset_amount = round((float)(half_arc) * (offset_factor));
        // check attenuation if the offset should be flipped.
        if (attenuate > 0) {
            start = max(start - offset_amount, max_start);
            end = min(end - offset_amount, max_end);
        } else {
            start = max(start + offset_amount, max_start);
            end = min(end + offset_amount, max_end);
        }
    }

    // Draw the cv arc frame and end cap lines.
    gravity.display.drawArc(arc_cx, arc_cy, outter_radius, start, end);
    gravity.display.drawArc(arc_cx, arc_cy, inner_radius, start, end);
    // Use drawArc to draw lines connecting the ends of the arc to close the frame.
    for (int i = 0; i < arc_dist; i++) {
        gravity.display.drawArc(arc_cx, arc_cy, inner_radius + i, start, start + 1);
        gravity.display.drawArc(arc_cx, arc_cy, inner_radius + i, end, end + 1);
    }

    int fill_arc_start;
    int fill_arc_end;

    if (inputValue >= 0) {
        // For positive values (0 to 512), fill clockwise from North
        // map inputValue (0 to 512) to angle_offset_units (0 to half_arc)
        long mapped_angle_offset = map(inputValue, 0, 512, 0, half_arc);

        fill_arc_start = (arc_north - mapped_angle_offset + 256) % 256;
        fill_arc_end = arc_north + 1;
    } else {  // Negative values (-512 to -1)
        // For negative values, fill counter-clockwise from North
        long mapped_angle_offset = map(abs(inputValue), 0, 512, 0, half_arc);  // abs(inputValue) is 1 to 512

        fill_arc_start = arc_north - 1;
        fill_arc_end = (arc_north + mapped_angle_offset) % 256;
    }

    // Draw the filled portion of the arc by drawing multiple concentric arcs
    // The step for 'i' determines the density of the fill.
    // i+=1 for solid fill, i+=4 for a coarser, faster fill.
    int fill_step = 4;

    // Only draw if there's an actual arc segment to fill (inputValue != 0)
    if (inputValue != 0) {
        for (int i = fill_step; i < arc_dist - 1; i += fill_step) {
            gravity.display.drawArc(arc_cx, arc_cy, inner_radius + i, fill_arc_start, fill_arc_end);
        }
    }
}
