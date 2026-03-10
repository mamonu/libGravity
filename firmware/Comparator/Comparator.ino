#include <EEPROM.h>
#include <libGravity.h>

// EEPROM addrs
const int EEPROM_INIT_ADDR = 0;
const int EEPROM_CV1_LOW = 1;
const int EEPROM_CV1_HIGH = 3;
const int EEPROM_CV1_OFFSET = 5;
const int EEPROM_CV2_LOW = 7;
const int EEPROM_CV2_HIGH = 9;
const int EEPROM_CV2_OFFSET = 11;
const int EEPROM_COMP1_SHIFT = 13;
const int EEPROM_COMP1_SIZE = 15;
const int EEPROM_COMP2_SHIFT = 17;
const int EEPROM_COMP2_SIZE = 19;
const byte EEPROM_INIT_FLAG = 0xAB; // Update flag to re-init

// EEPROM Delay Save
const unsigned long SAVE_DELAY_MS = 5000;
bool eeprom_needs_save = false;
unsigned long last_param_change = 0;

enum AppMode { MODE_COMPARATOR, MODE_CALIBRATION };
AppMode current_mode = MODE_COMPARATOR;
byte cal_selected_param = 0; // 0=CV1 Low, 1=CV1 Offset, 2=CV1 High, 3=CV2 Low,
                             // 4=CV2 Offset, 5=CV2 High

// UI Parameters
enum Parameter { COMP1_SHIFT, COMP1_SIZE, COMP2_SHIFT, COMP2_SIZE };

Parameter selected_param = COMP1_SHIFT;

int comp1_shift = 0;  // Range: -512 to 512
int comp1_size = 512; // Range: 0 to 1024

int comp2_shift = 0;  // Range: -512 to 512
int comp2_size = 512; // Range: 0 to 1024

// State
bool prev_gate1 = false;
bool prev_gate2 = false;
bool ff_state = false;
bool needs_redraw = true;

unsigned long last_redraw = 0;
bool prev_both_buttons = false;

// Calibration Methods
void LoadCalibration() {
  if (EEPROM.read(EEPROM_INIT_ADDR) == EEPROM_INIT_FLAG) {
    int val = 0;
    EEPROM.get(EEPROM_CV1_LOW, val);
    gravity.cv1.SetCalibrationLow(val);
    EEPROM.get(EEPROM_CV1_HIGH, val);
    gravity.cv1.SetCalibrationHigh(val);
    EEPROM.get(EEPROM_CV1_OFFSET, val);
    gravity.cv1.AdjustOffset(val - gravity.cv1.GetOffset());
    EEPROM.get(EEPROM_CV2_LOW, val);
    gravity.cv2.SetCalibrationLow(val);
    EEPROM.get(EEPROM_CV2_HIGH, val);
    gravity.cv2.SetCalibrationHigh(val);
    EEPROM.get(EEPROM_CV2_OFFSET, val);
    gravity.cv2.AdjustOffset(val - gravity.cv2.GetOffset());

    EEPROM.get(EEPROM_COMP1_SHIFT, comp1_shift);
    EEPROM.get(EEPROM_COMP1_SIZE, comp1_size);
    EEPROM.get(EEPROM_COMP2_SHIFT, comp2_shift);
    EEPROM.get(EEPROM_COMP2_SIZE, comp2_size);
  }
}

void SaveCalibration() {
  EEPROM.update(EEPROM_INIT_ADDR, EEPROM_INIT_FLAG);
  int val;
  val = gravity.cv1.GetCalibrationLow();
  EEPROM.put(EEPROM_CV1_LOW, val);
  val = gravity.cv1.GetCalibrationHigh();
  EEPROM.put(EEPROM_CV1_HIGH, val);
  val = gravity.cv1.GetOffset();
  EEPROM.put(EEPROM_CV1_OFFSET, val);
  val = gravity.cv2.GetCalibrationLow();
  EEPROM.put(EEPROM_CV2_LOW, val);
  val = gravity.cv2.GetCalibrationHigh();
  EEPROM.put(EEPROM_CV2_HIGH, val);
  val = gravity.cv2.GetOffset();
  EEPROM.put(EEPROM_CV2_OFFSET, val);

  EEPROM.put(EEPROM_COMP1_SHIFT, comp1_shift);
  EEPROM.put(EEPROM_COMP1_SIZE, comp1_size);
  EEPROM.put(EEPROM_COMP2_SHIFT, comp2_shift);
  EEPROM.put(EEPROM_COMP2_SIZE, comp2_size);
}

// Handlers
void OnPlayPress() {
  if (gravity.shift_button.On())
    return; // ignore if holding both
  if (current_mode == MODE_CALIBRATION) {
    cal_selected_param = (cal_selected_param < 3) ? cal_selected_param + 3
                                                  : cal_selected_param - 3;
    needs_redraw = true;
    return;
  }
  if (selected_param == COMP1_SHIFT)
    selected_param = COMP2_SHIFT;
  else if (selected_param == COMP1_SIZE)
    selected_param = COMP2_SIZE;
  else if (selected_param == COMP2_SHIFT)
    selected_param = COMP1_SHIFT;
  else if (selected_param == COMP2_SIZE)
    selected_param = COMP1_SIZE;
  needs_redraw = true;
}

void OnShiftPress() {
  if (gravity.play_button.On())
    return; // ignore if holding both
  if (current_mode == MODE_CALIBRATION) {
    cal_selected_param =
        (cal_selected_param / 3) * 3 + ((cal_selected_param + 1) % 3);
    needs_redraw = true;
    return;
  }
  if (selected_param == COMP1_SHIFT)
    selected_param = COMP1_SIZE;
  else if (selected_param == COMP1_SIZE)
    selected_param = COMP1_SHIFT;
  else if (selected_param == COMP2_SHIFT)
    selected_param = COMP2_SIZE;
  else if (selected_param == COMP2_SIZE)
    selected_param = COMP2_SHIFT;
  needs_redraw = true;
}

void OnEncoderRotate(int val) {
  if (current_mode == MODE_CALIBRATION) {
    AnalogInput *cv = (cal_selected_param > 2) ? &gravity.cv2 : &gravity.cv1;
    // Scale val up so tuning is practical without excessive encoder interrupts
    int cal_adj = val * 8;
    switch (cal_selected_param % 3) {
    case 0:
      cv->AdjustCalibrationLow(cal_adj);
      break;
    case 1:
      cv->AdjustOffset(cal_adj);
      break;
    case 2:
      cv->AdjustCalibrationHigh(cal_adj);
      break;
    }
    needs_redraw = true;
    return;
  }

  int amount = val * 16;
  switch (selected_param) {
  case COMP1_SHIFT:
    comp1_shift = constrain(comp1_shift + amount, -512, 512);
    break;
  case COMP1_SIZE:
    comp1_size = constrain(comp1_size + amount, 0, 1024);
    break;
  case COMP2_SHIFT:
    comp2_shift = constrain(comp2_shift + amount, -512, 512);
    break;
  case COMP2_SIZE:
    comp2_size = constrain(comp2_size + amount, 0, 1024);
    break;
  }

  eeprom_needs_save = true;
  last_param_change = millis();
  needs_redraw = true;
}

void DisplayCalibrationPoint(AnalogInput *cv, const char *title, int index) {
  int barWidth = 100, barHeight = 10, textHeight = 10;
  int half = barWidth / 2;
  int offsetX = 16, offsetY = (32 * index);

  gravity.display.setDrawColor(1);
  int value = cv->Read();

  gravity.display.setCursor(0, offsetY + textHeight);
  gravity.display.print(title);
  if (value >= 0)
    gravity.display.print(" ");
  gravity.display.print(value);

  gravity.display.setCursor(92, offsetY + textHeight);
  if (cv->Voltage() >= 0)
    gravity.display.print(" ");
  gravity.display.print(cv->Voltage(), 1);
  gravity.display.print(F("V"));

  gravity.display.drawFrame(offsetX, textHeight + offsetY + 2, barWidth,
                            barHeight);
  if (value > 0) {
    int x = constrain(map(value, 0, 512, 0, half), 0, half);
    gravity.display.drawBox(half + offsetX, textHeight + offsetY + 2, x,
                            barHeight);
  } else {
    int x = constrain(map(abs(value), 0, 512, 0, half), 0, half);
    gravity.display.drawBox((half + offsetX) - x, textHeight + offsetY + 2, x,
                            barHeight);
  }

  if (cal_selected_param / 3 == index) {
    int left = offsetX + (half * (cal_selected_param % 3) - 2);
    int top = barHeight + textHeight + offsetY + 12;
    gravity.display.drawStr(left, top, "^");
  }
}

void UpdateCalibrationDisplay() {
  gravity.display.setFontMode(0);
  gravity.display.setDrawColor(1);
  gravity.display.setFont(u8g2_font_profont11_tf);
  DisplayCalibrationPoint(&gravity.cv1, "CV1: ", 0);
  DisplayCalibrationPoint(&gravity.cv2, "CV2: ", 1);
}

void UpdateDisplay() {
  // Comp 1 graphics (Left)
  int c1_h = max((comp1_size * 3) / 64, 1);
  int c1_center = 26 - ((comp1_shift * 3) / 64);
  int c1_y = c1_center - (c1_h / 2);
  gravity.display.drawBox(20, c1_y, 44, c1_h);

  // Comp 2 graphics (Right)
  int c2_h = max((comp2_size * 3) / 64, 1);
  int c2_center = 26 - ((comp2_shift * 3) / 64);
  int c2_y = c2_center - (c2_h / 2);
  gravity.display.drawBox(74, c2_y, 44, c2_h);

  // Restore solid drawing for labels
  gravity.display.setDrawColor(1);
  gravity.display.setFont(u8g2_font_5x7_tf);
  gravity.display.setCursor(0, 7);
  gravity.display.print("+5V");
  gravity.display.setCursor(6, 29);
  gravity.display.print("0V");
  gravity.display.setCursor(0, 51);
  gravity.display.print("-5V");

  gravity.display.setDrawColor(2); // XOR mode

  // Draw center divider and dotted lines in XOR
  for (int x = 20; x < 128; x += 4) {
    gravity.display.drawPixel(x, 2);  // +5V
    gravity.display.drawPixel(x, 26); // 0V
    gravity.display.drawPixel(x, 50); // -5V
  }
  for (int y = 0; y <= 50; y += 4) {
    gravity.display.drawPixel(69, y); // Center divider
  }

  // Restore draw color to default (solid)
  gravity.display.setDrawColor(1);

  // Bottom text area
  gravity.display.setDrawColor(0);
  gravity.display.drawBox(0, 52, 128, 12);
  gravity.display.setDrawColor(1);
  gravity.display.drawHLine(0, 52, 128);

  gravity.display.setFont(u8g2_font_6x10_tf);
  gravity.display.setCursor(2, 62);

  char text[32];
  switch (selected_param) {
  case COMP1_SHIFT:
    snprintf(text, sizeof(text), "> Comp 1 Shift: %d", comp1_shift);
    break;
  case COMP1_SIZE:
    snprintf(text, sizeof(text), "> Comp 1 Size: %d", comp1_size);
    break;
  case COMP2_SHIFT:
    snprintf(text, sizeof(text), "> Comp 2 Shift: %d", comp2_shift);
    break;
  case COMP2_SIZE:
    snprintf(text, sizeof(text), "> Comp 2 Size: %d", comp2_size);
    break;
  }
  gravity.display.print(text);
}

void setup() {
  gravity.Init();
  LoadCalibration();

  // Speed up ADC conversions
  ADCSRA &= ~(bit(ADPS2) | bit(ADPS1) | bit(ADPS0));
  ADCSRA |= bit(ADPS2);

  gravity.play_button.AttachPressHandler(OnPlayPress);
  gravity.shift_button.AttachPressHandler(OnShiftPress);
  gravity.encoder.AttachRotateHandler(OnEncoderRotate);
}

void loop() {
  gravity.Process();

  bool both_pressed = gravity.play_button.On() && gravity.shift_button.On();
  if (both_pressed && !prev_both_buttons) {
    if (current_mode == MODE_COMPARATOR) {
      current_mode = MODE_CALIBRATION;
      cal_selected_param = 0;
    } else {
      SaveCalibration();
      current_mode = MODE_COMPARATOR;
    }
    needs_redraw = true;
  }
  prev_both_buttons = both_pressed;

  int cv1_val = gravity.cv1.Read();
  int cv2_val = gravity.cv2.Read();

  if (current_mode == MODE_COMPARATOR) {
    int c1_lower = comp1_shift - (comp1_size / 2);
    int c1_upper = comp1_shift + (comp1_size / 2);
    
    int c2_lower = comp2_shift - (comp2_size / 2);
    int c2_upper = comp2_shift + (comp2_size / 2);

    const int HYSTERESIS = 4; // Margin to prevent noise bouncing at threshold

    bool gate1 = prev_gate1;
    if (gate1) {
      if (cv1_val < c1_lower - HYSTERESIS || cv1_val > c1_upper + HYSTERESIS) {
        gate1 = false;
      }
    } else {
      if (cv1_val >= c1_lower + HYSTERESIS && cv1_val <= c1_upper - HYSTERESIS) {
        gate1 = true;
      }
    }

    bool gate2 = prev_gate2;
    if (gate2) {
      if (cv2_val < c2_lower - HYSTERESIS || cv2_val > c2_upper + HYSTERESIS) {
        gate2 = false;
      }
    } else {
      if (cv2_val >= c2_lower + HYSTERESIS && cv2_val <= c2_upper - HYSTERESIS) {
        gate2 = true;
      }
    }

    bool logic_and = gate1 && gate2;
    bool logic_or = gate1 || gate2;
    bool logic_xor = gate1 ^ gate2;

    static bool prev_logic_xor = false;
    if (logic_xor && !prev_logic_xor) {
      ff_state = !ff_state;
    }
    prev_logic_xor = logic_xor;

    gravity.outputs[0].Update(gate1 ? HIGH : LOW);
    gravity.outputs[1].Update(gate2 ? HIGH : LOW);
    gravity.outputs[2].Update(logic_and ? HIGH : LOW);
    gravity.outputs[3].Update(logic_or ? HIGH : LOW);
    gravity.outputs[4].Update(logic_xor ? HIGH : LOW);
    gravity.outputs[5].Update(ff_state ? HIGH : LOW);

    prev_gate1 = gate1;
    prev_gate2 = gate2;
  }

  if (eeprom_needs_save && (millis() - last_param_change > SAVE_DELAY_MS)) {
    SaveCalibration();
    eeprom_needs_save = false;
  }

  // Force frequent redraws in calibration mode for immediate feedback
  if (current_mode == MODE_CALIBRATION && (millis() - last_redraw >= 30)) {
    needs_redraw = true;
    last_redraw = millis();
  }

  if (needs_redraw) {
    needs_redraw = false;
    gravity.display.firstPage();
    do {
      if (current_mode == MODE_COMPARATOR) {
        UpdateDisplay();
      } else {
        UpdateCalibrationDisplay();
      }
    } while (gravity.display.nextPage());
  }
}