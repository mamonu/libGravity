// File: save_state.cpp

#include <EEPROM.h>
#include "save_state.h"
#include "app_state.h" // Includes AppState and Channel definitions

bool StateManager::initialize(AppState& app) {
    if (isDataValid()) {
        EepromData loadedState;
        EEPROM.get(sizeof(Metadata), loadedState);
        
        // Restore main app state
        app.selected_param = loadedState.selected_param;
        app.selected_channel = loadedState.selected_channel;
        app.selected_source = static_cast<Source>(loadedState.selected_source);
        
        // --- NEW: Loop through and restore each channel's state ---
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            auto& ch = app.channel[i]; // Get a reference to the channel object
            const auto& saved_ch_state = loadedState.channel_data[i]; // Get a const reference to the saved data

            ch.setClockMod(saved_ch_state.base_clock_mod_index);
            ch.setProbability(saved_ch_state.base_probability);
            ch.setDutyCycle(saved_ch_state.base_duty_cycle);
            ch.setOffset(saved_ch_state.base_offset);
            ch.setCvSource(static_cast<CvSource>(saved_ch_state.cv_source));
            ch.setCvDestination(static_cast<CvDestination>(saved_ch_state.cv_destination));
        }
        
        return true;
    } else {
        writeMetadata();
        save(app); // Save the initial default state
        return false;
    }
}

void StateManager::save(const AppState& app) {
    EepromData stateToSave;
    
    // Populate main app state
    stateToSave.selected_param = app.selected_param;
    stateToSave.selected_channel = app.selected_channel;
    stateToSave.selected_source = static_cast<byte>(app.selected_source);

    // --- NEW: Loop through and populate each channel's state ---
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        const auto& ch = app.channel[i]; // Get a const reference to the channel object
        auto& saved_ch_state = stateToSave.channel_data[i]; // Get a reference to the struct we're saving to

        // Use the getters with 'withCvMod = false' to get the base values
        saved_ch_state.base_clock_mod_index = ch.getClockModIndex(false);
        saved_ch_state.base_probability = ch.getProbability(false);
        saved_ch_state.base_duty_cycle = ch.getDutyCycle(false);
        saved_ch_state.base_offset = ch.getOffset(false);
        saved_ch_state.cv_source = static_cast<byte>(ch.getCvSource());
        saved_ch_state.cv_destination = static_cast<byte>(ch.getCvDestination());
    }

    // Write the entire state struct to EEPROM
    EEPROM.put(sizeof(Metadata), stateToSave);
}

void StateManager::reset(AppState& app) {
    AppState defaultState;
    app = defaultState;
    writeMetadata();
    save(app);
}

// isDataValid() and writeMetadata() remain unchanged
bool StateManager::isDataValid() {
    Metadata storedMeta;
    EEPROM.get(0, storedMeta);
    bool nameMatch = (strcmp(storedMeta.sketchName, CURRENT_SKETCH_NAME) == 0);
    bool versionMatch = (storedMeta.version == CURRENT_SKETCH_VERSION);
    return nameMatch && versionMatch;
}

void StateManager::writeMetadata() {
    Metadata currentMeta;
    strcpy(currentMeta.sketchName, CURRENT_SKETCH_NAME);
    currentMeta.version = CURRENT_SKETCH_VERSION;
    EEPROM.put(0, currentMeta);
}