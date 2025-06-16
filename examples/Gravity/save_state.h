#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <Arduino.h>
#include <gravity.h>

// Forward-declare AppState to avoid circular dependencies.
struct AppState;

// Define the constants for the current firmware.
const char CURRENT_SKETCH_NAME[] = "Gravity";
const float CURRENT_SKETCH_VERSION = 0.2f;

/**
 * @brief Manages saving and loading of the application state to and from EEPROM.
 */
class StateManager {
   public:
    StateManager();

    bool initialize(AppState& app);
    void reset(AppState& app);
    // Call from main loop, check if state has changed and needs to be saved.
    void update(const AppState& app);
    // Indicate that state has changed and we should save.
    void markDirty();

   private:
    // This struct holds the data that identifies the firmware version.
    struct Metadata {
        char sketchName[16];
        byte version;
    };
    struct ChannelState {
        byte base_clock_mod_index;
        byte base_probability;
        byte base_duty_cycle;
        byte base_offset;
        byte cv_source;       // We'll store the CvSource enum as a byte
        byte cv_destination;  // We'll store the CvDestination enum as a byte
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

    void save(const AppState& app);

    bool isDataValid();
    void _save_worker(const AppState& app);
    void _metadata_worker();

    bool _isDirty;
    unsigned long _lastChangeTime;
    static const unsigned long SAVE_DELAY_MS = 2000;
};

#endif  // SAVE_STATE_H