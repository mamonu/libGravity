#include "save_state.h"

#include <EEPROM.h>

#include "app_state.h"

StateManager::StateManager() : _isDirty(false), _lastChangeTime(0) {}

bool StateManager::initialize(AppState& app) {
    if (_isDataValid()) {
        static EepromData load_data;
        EEPROM.get(sizeof(Metadata), load_data);

        // Restore main app state
        app.tempo = load_data.tempo;
        app.encoder_reversed = load_data.encoder_reversed;
        app.selected_param = load_data.selected_param;
        app.selected_channel = load_data.selected_channel;
        app.selected_source = static_cast<Clock::Source>(load_data.selected_source);
        app.selected_pulse = static_cast<Clock::Pulse>(load_data.selected_pulse);

        // Loop through and restore each channel's state.
        for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
            auto& ch = app.channel[i];
            const auto& saved_ch_state = load_data.channel_data[i];

            ch.setClockMod(saved_ch_state.base_clock_mod_index);
            ch.setProbability(saved_ch_state.base_probability);
            ch.setDutyCycle(saved_ch_state.base_duty_cycle);
            ch.setOffset(saved_ch_state.base_offset);
            ch.setSwing(saved_ch_state.base_shuffle);
            ch.setCvSource(static_cast<CvSource>(saved_ch_state.cv_source));
            ch.setCvDestination(static_cast<CvDestination>(saved_ch_state.cv_destination));
            ch.setSteps(saved_ch_state.euc_steps);
            ch.setHits(saved_ch_state.euc_hits);
        }

        return true;
    } else {
        reset(app);
        return false;
    }
}

void StateManager::_save(const AppState& app) {
    // Ensure interrupts do not cause corrupt data writes.
    noInterrupts();
    _saveState(app);
    interrupts();
}

void StateManager::reset(AppState& app) {
    app.tempo = Clock::DEFAULT_TEMPO;
    app.encoder_reversed = false;
    app.selected_param = 0;
    app.selected_channel = 0;
    app.selected_source = Clock::SOURCE_INTERNAL;
    app.selected_pulse = Clock::PULSE_PPQN_24;

    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].Init();
    }

    noInterrupts();
    _saveMetadata();  // Write the new metadata
    _saveState(app);   // Write the new (default) app state
    interrupts();

    _isDirty = false;
}

void StateManager::update(const AppState& app) {
    // Check if a save is pending and if enough time has passed.
    if (_isDirty && (millis() - _lastChangeTime > SAVE_DELAY_MS)) {
        _save(app);
        _isDirty = false;  // Clear the flag, we are now "clean".
    }
}

void StateManager::markDirty() {
    _isDirty = true;
    _lastChangeTime = millis();
}

bool StateManager::_isDataValid() {
    Metadata load_meta;
    EEPROM.get(0, load_meta);
    bool name_match = (strcmp(load_meta.sketch_name, SKETCH_NAME) == 0);
    bool version_match = (load_meta.version == SKETCH_VERSION);
    return name_match && version_match;
}

void StateManager::_saveState(const AppState& app) {
    static EepromData save_data;

    // Populate main app state
    save_data.tempo = app.tempo;
    save_data.encoder_reversed = app.encoder_reversed;
    save_data.selected_param = app.selected_param;
    save_data.selected_channel = app.selected_channel;
    save_data.selected_source = static_cast<byte>(app.selected_source);
    save_data.selected_pulse = static_cast<byte>(app.selected_pulse);

    // Loop through and populate each channel's state
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        const auto& ch = app.channel[i];
        auto& save_ch = save_data.channel_data[i];

        // Use the getters with 'withCvMod = false' to get the base values
        save_ch.base_clock_mod_index = ch.getClockModIndex(false);
        save_ch.base_probability = ch.getProbability(false);
        save_ch.base_duty_cycle = ch.getDutyCycle(false);
        save_ch.base_offset = ch.getOffset(false);
        save_ch.base_shuffle = ch.getSwing();
        save_ch.cv_source = static_cast<byte>(ch.getCvSource());
        save_ch.cv_destination = static_cast<byte>(ch.getCvDestination());
        save_ch.euc_steps = ch.getSteps();
        save_ch.euc_hits = ch.getHits();
    }
    EEPROM.put(sizeof(Metadata), save_data);
}

void StateManager::_saveMetadata() {
    Metadata current_meta;
    strcpy(current_meta.sketch_name, SKETCH_NAME);
    current_meta.version = SKETCH_VERSION;
    EEPROM.put(0, current_meta);
}
