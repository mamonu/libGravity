/**
 * @file save_state.h
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Alt firmware version of Gravity by Sitka Instruments.
 * @version 2.0.1
 * @date 2025-07-04
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <Arduino.h>
#include <gravity.h>

// Forward-declare AppState to avoid circular dependencies.
struct AppState;

// Define the constants for the current firmware.
const char SKETCH_NAME[] = "ALT GRAVITY";
const char SEMANTIC_VERSION[] = "V2.0.0BETA1";

// Number of available save slots.
const byte MAX_SAVE_SLOTS = 10;  // Count of save slots 0 - 9 to save/load  presets.
const byte TRANSIENT_SLOT = 10;  // Transient slot index to persist state when powered off.

// Define the minimum amount of time between EEPROM writes.
static const unsigned long SAVE_DELAY_MS = 2000;

/**
 * @brief Manages saving and loading of the application state to and from EEPROM.
 * The number of user slots is defined by MAX_SAVE_SLOTS, and one additional slot
 * is reseved for transient state to persist state between power cycles before
 * state is explicitly saved to a user slot. Metadata is stored in the beginning
 * of the memory space which stores firmware version information to validate that
 * the data can be loaded into the current version of AppState.
 */
class StateManager {
   public:
    StateManager();

    // Populate the AppState instance with values from EEPROM if they exist.
    bool initialize(AppState& app);
    // Load data from specified slot.
    bool loadData(AppState& app, byte slot_index);
    // Save data to specified slot.
    void saveData(const AppState& app);
    // Reset AppState instance back to default values.
    void reset(AppState& app);
    // Call from main loop, check if state has changed and needs to be saved.
    void update(const AppState& app);
    // Indicate that state has changed and we should save.
    void markDirty();
    // Erase all data stored in the EEPROM.
    void factoryReset();

    // This struct holds the data that identifies the firmware version.
    struct Metadata {
        char sketch_name[16];
        char version[16];
        // Additional global/hardware settings
        byte selected_save_slot;
        bool encoder_reversed;
    };
    struct ChannelState {
        byte base_clock_mod_index;
        byte base_probability;
        byte base_duty_cycle;
        byte base_offset;
        byte base_swing;
        byte base_euc_steps;
        byte base_euc_hits;
        byte cv1_dest;  // Cast the CvDestination enum as a byte for storage
        byte cv2_dest;  // Cast the CvDestination enum as a byte for storage
    };
    // This struct holds all the parameters we want to save.
    struct EepromData {
        int tempo;
        bool encoder_reversed;
        byte selected_param;
        byte selected_channel;
        byte selected_source;
        byte selected_pulse;
        ChannelState channel_data[Gravity::OUTPUT_COUNT];
    };

   private:
    bool _isDataValid();
    void _saveMetadata(const AppState& app);
    void _loadMetadata(AppState& app);
    void _saveState(const AppState& app, byte slot_index);
    void _loadState(AppState& app, byte slot_index);

    bool _isDirty;
    unsigned long _lastChangeTime;
};

#endif  // SAVE_STATE_H