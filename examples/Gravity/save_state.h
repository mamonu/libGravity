#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <Arduino.h>
#include <gravity.h>

// Forward-declare AppState to avoid circular dependencies.
struct AppState;

// Define the constants for the current firmware.
const char SKETCH_NAME[] = "Gravity";
const byte SKETCH_VERSION = 4;

// Define the minimum amount of time between EEPROM writes.
static const unsigned long SAVE_DELAY_MS = 2000;

/**
 * @brief Manages saving and loading of the application state to and from EEPROM.
 */
class StateManager {
   public:
    StateManager();

    // Populate the AppState instance with values from EEPROM if they exist.
    bool initialize(AppState& app);
    // Reset AppState instance back to default values.
    void reset(AppState& app);
    // Call from main loop, check if state has changed and needs to be saved.
    void update(const AppState& app);
    // Indicate that state has changed and we should save.
    void markDirty();

   private:
    // This struct holds the data that identifies the firmware version.
    struct Metadata {
        char sketch_name[16];
        byte version;
    };
    struct ChannelState {
        byte base_clock_mod_index;
        byte base_probability;
        byte base_duty_cycle;
        byte base_offset;
        byte base_shuffle;
        byte cv_source;       // Cast the CvSource enum to a byte for storage
        byte cv_destination;  // Cast the CvDestination enum as a byte for storage
    };
    // This struct holds all the parameters we want to save.
    struct EepromData {
        int tempo;
        bool encoder_reversed;
        byte selected_param;
        byte selected_channel;
        byte selected_source;
        ChannelState channel_data[Gravity::OUTPUT_COUNT];
    };

    void _save(const AppState& app);
    bool _isDataValid();
    void _saveState(const AppState& app);
    void _saveMetadata();

    bool _isDirty;
    unsigned long _lastChangeTime;
};

#endif  // SAVE_STATE_H