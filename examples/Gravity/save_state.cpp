#include "save_state.h"

#include <EEPROM.h>

#include "app_state.h"

bool StateManager::initialize(AppState& app) {
    if (isDataValid()) {
        EepromData load_data;
        EEPROM.get(sizeof(Metadata), load_data);

        // Restore main app state
        app.tempo = load_data.tempo;
        app.selected_param = load_data.selected_param;
        app.selected_channel = load_data.selected_channel;
        app.selected_source = static_cast<Clock::Source>(load_data.selected_source);

        // Loop through and restore each channel's state.
        for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
            auto& ch = app.channel[i];
            const auto& saved_ch_state = load_data.channel_data[i];

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
        save(app);  // Save the initial default state
        return false;
    }
}

void StateManager::save(const AppState& app) {
    EepromData save_data;

    // Populate main app state
    save_data.tempo = app.tempo;
    save_data.selected_param = app.selected_param;
    save_data.selected_channel = app.selected_channel;
    save_data.selected_source = static_cast<byte>(app.selected_source);

    // Loop through and populate each channel's state
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        const auto& ch = app.channel[i];
        auto& saved_ch_state = save_data.channel_data[i];

        // Use the getters with 'withCvMod = false' to get the base values
        saved_ch_state.base_clock_mod_index = ch.getClockModIndex(false);
        saved_ch_state.base_probability = ch.getProbability(false);
        saved_ch_state.base_duty_cycle = ch.getDutyCycle(false);
        saved_ch_state.base_offset = ch.getOffset(false);
        saved_ch_state.cv_source = static_cast<byte>(ch.getCvSource());
        saved_ch_state.cv_destination = static_cast<byte>(ch.getCvDestination());
    }

    // Write the entire state struct to EEPROM
    EEPROM.put(sizeof(Metadata), save_data);
}

void StateManager::reset(AppState& app) {
    app.tempo = Clock::DEFAULT_TEMPO;
    app.selected_param = 0;
    app.selected_channel = 0;
    app.selected_source = Clock::SOURCE_INTERNAL;

    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].Init();
    }

    writeMetadata();
    save(app);
}

bool StateManager::isDataValid() {
    Metadata load_meta;
    EEPROM.get(0, load_meta);
    bool nameMatch = (strcmp(load_meta.sketchName, CURRENT_SKETCH_NAME) == 0);
    bool versionMatch = (load_meta.version == CURRENT_SKETCH_VERSION);
    return nameMatch && versionMatch;
}

void StateManager::writeMetadata() {
    Metadata save_meta;
    strcpy(save_meta.sketchName, CURRENT_SKETCH_NAME);
    save_meta.version = CURRENT_SKETCH_VERSION;
    EEPROM.put(0, save_meta);
}