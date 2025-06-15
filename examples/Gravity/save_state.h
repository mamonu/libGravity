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
    bool initialize(AppState& app);
    void save(const AppState& app);
    void reset(AppState& app);

   private:
    // This struct holds the data that identifies the firmware version.
    struct Metadata {
        char sketchName[16];
        float version;
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
        byte selected_param;
        byte selected_channel;
        byte selected_source;
        ChannelState channel_data[Gravity::OUTPUT_COUNT];
    };

    bool isDataValid();
    void writeMetadata();
};

#endif  // SAVE_STATE_H