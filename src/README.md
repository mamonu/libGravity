# libGravity API Reference

This document provides API documentation for `libGravity`, a library for building custom scripts for the Sitka Instruments Gravity module.

## `Gravity` Class

The `Gravity` class is the main hardware abstraction wrapper for the module. It provides a central point of access to all of the module's hardware components like the display, clock, inputs, and outputs.

A global instance of this class, `gravity`, is created for you to use in your scripts.

```cpp
// Global instance
extern Gravity gravity;
```

### Public Methods

#### `void Init()`

Initializes the Arduino and all the Gravity hardware components. This should be called once in your `setup()` function.

#### `void Process()`

Performs a polling check for state changes on all inputs and outputs. This should be called repeatedly in your main `loop()` function to ensure all components are responsive.

### Public Properties

  * `U8G2_SSD1306_128X64_NONAME_1_HW_I2C display`
      * OLED display object from the `U8g2lib` library. Use this to draw to the screen.
  * `Clock clock`
      * The main clock source wrapper. See the [Clock Class](https://www.google.com/search?q=%23clock-class) documentation for details.
  * `DigitalOutput outputs[OUTPUT_COUNT]`
      * An array of `DigitalOutput` objects, where `OUTPUT_COUNT` is 6. Each element corresponds to one of the six gate/trigger outputs.
  * `DigitalOutput pulse`
      * A `DigitalOutput` object for the MIDI Expander module's pulse output.
  * `Encoder encoder`
      * The rotary encoder with a built-in push button. See the [Encoder Class](https://www.google.com/search?q=%23encoder-class) documentation for details.
  * `Button shift_button`
      * A `Button` object for the 'Shift' button.
  * `Button play_button`
      * A `Button` object for the 'Play' button.
  * `AnalogInput cv1`
      * An `AnalogInput` object for the CV1 input jack.
  * `AnalogInput cv2`
      * An `AnalogInput` object for the CV2 input jack.

## `AnalogInput` Class

This class handles reading and processing the analog CV inputs. It includes features for calibration, offsetting, and attenuation.

### Public Methods

#### `void Init(uint8_t pin)`

Initializes the analog input on a specific pin.

  * **Parameters:**
      * `pin`: The GPIO pin for the analog input.

#### `void Process()`

Reads the raw value from the ADC, applies calibration, offset, and attenuation/inversion. This must be called regularly in the main loop.

#### `void AdjustCalibrationLow(int amount)`

Adjusts the low calibration point to fine-tune the input mapping.

  * **Parameters:**
      * `amount`: The amount to add to the current low calibration value.

#### `void AdjustCalibrationHigh(int amount)`

Adjusts the high calibration point to fine-tune the input mapping.

  * **Parameters:**
      * `amount`: The amount to add to the current high calibration value.

#### `void SetOffset(float percent)`

Sets a DC offset for the input signal.

  * **Parameters:**
      * `percent`: A percentage (e.g., `0.5` for 50%) to shift the signal.

#### `void SetAttenuation(float percent)`

Sets the attenuation (scaling) of the input signal. A negative percentage will also invert the signal.

  * **Parameters:**
      * `percent`: The attenuation level, typically from `0.0` to `1.0`.

#### `int16_t Read()`

Gets the current processed value of the analog input.

  * **Returns:** The read value, scaled to a range of +/-512.

#### `float Voltage()`

Gets the analog read value as a voltage.

  * **Returns:** A `float` representing the calculated voltage (-5.0V to +5.0V).

## `Button` Class

A wrapper class for handling digital inputs like push buttons, including debouncing and long-press detection.

### Enums

#### `enum ButtonChange`

Constants representing a change in the button's state.

  * `CHANGE_UNCHANGED`
  * `CHANGE_PRESSED`
  * `CHANGE_RELEASED` (a normal, short press)
  * `CHANGE_RELEASED_LONG` (a long press)

### Public Methods

#### `void Init(uint8_t pin)`

Initializes the button on a specific GPIO pin.

  * **Parameters:**
      * `pin`: The GPIO pin for the button.

#### `void AttachPressHandler(void (*f)())`

Attaches a callback function to be executed on a short button press.

  * **Parameters:**
      * `f`: The function to call.

#### `void AttachLongPressHandler(void (*f)())`

Attaches a callback function to be executed on a long button press.

  * **Parameters:**
      * `f`: The function to call.

#### `void Process()`

Reads the button's state and handles debouncing and press detection. Call this repeatedly in the main loop.

#### `ButtonChange Change()`

Gets the last state change of the button.

  * **Returns:** A `ButtonChange` enum value indicating the last detected change.

#### `bool On()`

Checks the current physical state of the button.

  * **Returns:** `true` if the button is currently being held down, `false` otherwise.

## `Clock` Class

A wrapper for all clock and timing functions, supporting internal, external, and MIDI clock sources.

### Enums

#### `enum Source`

Defines the possible clock sources.

  * `SOURCE_INTERNAL`
  * `SOURCE_EXTERNAL_PPQN_24` (24 pulses per quarter note)
  * `SOURCE_EXTERNAL_PPQN_4` (4 pulses per quarter note)
  * `SOURCE_EXTERNAL_MIDI`

#### `enum Pulse`

Defines the possible pulse-per-quarter-note rates for the pulse output.

  * `PULSE_NONE`
  * `PULSE_PPQN_1`
  * `PULSE_PPQN_4`
  * `PULSE_PPQN_24`

### Public Methods

#### `void Init()`

Initializes the clock, sets up MIDI serial, and sets default values.

#### `void AttachExtHandler(void (*callback)())`

Attaches a user-defined callback to the external clock input. This is triggered by a rising edge on the external clock pin or by an incoming MIDI clock message.

  * **Parameters:**
      * `callback`: The function to call on an external clock event.

#### `void AttachIntHandler(void (*callback)(uint32_t))`

Sets a callback function that is triggered at the high-resolution internal clock rate (PPQN\_96). This is the main internal timing callback.

  * **Parameters:**
      * `callback`: The function to call on every internal clock tick. It receives the tick count as a `uint32_t` parameter.

#### `void SetSource(Source source)`

Sets the clock's driving source.

  * **Parameters:**
      * `source`: The new clock source from the `Source` enum.

#### `bool ExternalSource()`

Checks if the clock source is external.

  * **Returns:** `true` if the source is external (PPQN or MIDI).

#### `bool InternalSource()`

Checks if the clock source is internal.

  * **Returns:** `true` if the source is the internal master clock.

#### `int Tempo()`

Gets the current tempo.

  * **Returns:** The current tempo in beats per minute (BPM).

#### `void SetTempo(int tempo)`

Sets the clock tempo when in internal mode.

  * **Parameters:**
      * `tempo`: The new tempo in BPM.

#### `void Tick()`

Manually triggers a clock tick. This should be called from your external clock handler to drive the internal timing when in an external clock mode.

#### `void Start()`

Starts the clock.

#### `void Stop()`

Stops (pauses) the clock.

#### `void Reset()`

Resets all clock counters to zero.

#### `bool IsPaused()`

Checks if the clock is currently paused.

  * **Returns:** `true` if the clock is stopped.

## `DigitalOutput` Class

This class is used to control the digital gate/trigger outputs.

### Public Methods

#### `void Init(uint8_t cv_pin)`

Initializes a digital output on a specific pin.

  * **Parameters:**
      * `cv_pin`: The GPIO pin for the CV/Gate output.

#### `void SetTriggerDuration(uint8_t duration_ms)`

Sets the duration for triggers. When `Trigger()` is called, the output will remain high for this duration.

  * **Parameters:**
      * `duration_ms`: The trigger duration in milliseconds.

#### `void Update(uint8_t state)`

Sets the output state directly.

  * **Parameters:**
      * `state`: `HIGH` or `LOW`.

#### `void High()`

Sets the output to HIGH (approx. 5V).

#### `void Low()`

Sets the output to LOW (0V).

#### `void Trigger()`

Begins a trigger. The output goes HIGH and will automatically be set LOW after the configured trigger duration has elapsed (handled by `Process()`).

#### `void Process()`

Handles the timing for triggers. If an output was triggered, this method checks if the duration has elapsed and sets the output LOW if necessary. Call this in the main loop.

#### `bool On()`

Returns the current on/off state of the output.

  * **Returns:** `true` if the output is currently HIGH.

## `Encoder` Class

Handles all interaction with the rotary encoder, including rotation, button presses, and rotation while pressed.

**Header:** `encoder_dir.h`

### Public Methods

#### `void SetReverseDirection(bool reversed)`

Sets the direction of the encoder.

  * **Parameters:**
      * `reversed`: Set to `true` to reverse the direction of rotation.

#### `void AttachPressHandler(void (*f)())`

Attaches a callback for a simple press-and-release of the encoder button.

  * **Parameters:**
      * `f`: The function to call on a button press.

#### `void AttachRotateHandler(void (*f)(int val))`

Attaches a callback for when the encoder is rotated (while the button is not pressed).

  * **Parameters:**
      * `f`: The callback function. It receives an `int` representing the change in position (can be positive or negative).

#### `void AttachPressRotateHandler(void (*f)(int val))`

Attaches a callback for when the encoder is rotated while the button is being held down.

  * **Parameters:**
      * `f`: The callback function. It receives an `int` representing the change in position.

#### `void Process()`

Processes encoder and button events. This method must be called repeatedly in the main loop to check for state changes and dispatch the appropriate callbacks.
