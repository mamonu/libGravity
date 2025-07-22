/**
 * @file save_state.cpp
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Alt firmware version of Gravity by Sitka Instruments.
 * @version 2.0.1
 * @date 2025-07-04
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#include "save_state.h"

#include <EEPROM.h>

#include "app_state.h"

// Calculate the starting address for EepromData, leaving space for metadata.
static const int METADATA_START_ADDR = 0;
static const int EEPROM_DATA_START_ADDR = sizeof(StateManager::Metadata);

StateManager::StateManager() : _isDirty(false), _lastChangeTime(0) {}

bool StateManager::initialize(AppState& app) {
    if (_isDataValid()) {
        // Load data from the transient slot.
        return loadData(app, TRANSIENT_SLOT);
    }
    // EEPROM does not contain save data for this firmware & version.
    else {
        // Erase EEPROM and initialize state. Save default pattern to all save slots.
        factoryReset();
        // Initialize eeprom and save default patter to all save slots.
        _saveMetadata(app);
        reset(app);
        for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
            app.selected_save_slot = i;
            _saveState(app, i);
        }
        return false;
    }
}

bool StateManager::loadData(AppState& app, byte slot_index) {
    // Check if slot_index is within max range + 1 for transient.
    if (slot_index >= MAX_SAVE_SLOTS + 1) return false;

    _loadState(app, slot_index);
    _loadMetadata(app);

    return true;
}

// Save app state to user specified save slot.
void StateManager::saveData(const AppState& app) {
    // Check if slot_index is within max range + 1 for transient.
    if (app.selected_save_slot >= MAX_SAVE_SLOTS + 1) return;

    _saveState(app, app.selected_save_slot);
    _isDirty = false;
}

// Save transient state if it has changed and enough time has passed since last save.
void StateManager::update(const AppState& app) {
    if (_isDirty && (millis() - _lastChangeTime > SAVE_DELAY_MS)) {
        _saveState(app, TRANSIENT_SLOT);
        _saveMetadata(app);
        _isDirty = false;
    }
}

void StateManager::reset(AppState& app) {
    app.tempo = Clock::DEFAULT_TEMPO;
    app.selected_param = 0;
    app.selected_channel = 0;
    app.selected_source = Clock::SOURCE_INTERNAL;
    app.selected_pulse = Clock::PULSE_PPQN_24;
    app.selected_save_slot = 0;

    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].Init();
    }

    // Load global settings from Metadata
    _loadMetadata(app);

    _isDirty = false;
}

void StateManager::markDirty() {
    _isDirty = true;
    _lastChangeTime = millis();
}

// Erases all data in the EEPROM by writing 0 to every address.
void StateManager::factoryReset() {
    noInterrupts();
    for (unsigned int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
    interrupts();
}

bool StateManager::_isDataValid() {
    Metadata load_meta;
    EEPROM.get(METADATA_START_ADDR, load_meta);
    bool name_match = (strcmp(load_meta.sketch_name, SKETCH_NAME) == 0);
    bool version_match = (load_meta.version == SKETCH_VERSION);
    return name_match && version_match;
}

void StateManager::_saveState(const AppState& app, byte slot_index) {
    // Check if slot_index is within max range + 1 for transient.
    if (app.selected_save_slot >= MAX_SAVE_SLOTS + 1) return;

    noInterrupts();
    static EepromData save_data;

    save_data.tempo = app.tempo;
    save_data.encoder_reversed = app.encoder_reversed;
    save_data.selected_param = app.selected_param;
    save_data.selected_channel = app.selected_channel;
    save_data.selected_source = static_cast<byte>(app.selected_source);
    save_data.selected_pulse = static_cast<byte>(app.selected_pulse);
    save_data.selected_save_slot = app.selected_save_slot;

    // TODO: break this out into a separate function. Save State should be
    // broken out into global / per-channel save methods. When saving via
    // "update" only save state for the current channel since other channels
    // will not have changed when saving user edits.
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        const auto& ch = app.channel[i];
        auto& save_ch = save_data.channel_data[i];
        save_ch.base_clock_mod_index = ch.getClockModIndex(false);
        save_ch.base_probability = ch.getProbability(false);
        save_ch.base_duty_cycle = ch.getDutyCycle(false);
        save_ch.base_offset = ch.getOffset(false);
        save_ch.base_swing = ch.getSwing(false);
        save_ch.base_euc_steps = ch.getSteps(false);
        save_ch.base_euc_hits = ch.getHits(false);
        save_ch.cv1_dest = static_cast<byte>(ch.getCv1Dest());
        save_ch.cv2_dest = static_cast<byte>(ch.getCv2Dest());
    }

    int address = EEPROM_DATA_START_ADDR + (slot_index * sizeof(EepromData));
    EEPROM.put(address, save_data);
    interrupts();
}

void StateManager::_loadState(AppState& app, byte slot_index) {
    // Check if slot_index is within max range + 1 for transient.
    if (slot_index >=  MAX_SAVE_SLOTS + 1) return;

    noInterrupts();
    static EepromData load_data;
    int address = EEPROM_DATA_START_ADDR + (slot_index * sizeof(EepromData));
    EEPROM.get(address, load_data);

    // Restore app state from loaded data.
    app.tempo = load_data.tempo;
    app.selected_param = load_data.selected_param;
    app.selected_channel = load_data.selected_channel;
    app.selected_source = static_cast<Clock::Source>(load_data.selected_source);
    app.selected_pulse = static_cast<Clock::Pulse>(load_data.selected_pulse);
    app.selected_save_slot = slot_index;

    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        auto& ch = app.channel[i];
        const auto& saved_ch_state = load_data.channel_data[i];

        ch.setClockMod(saved_ch_state.base_clock_mod_index);
        ch.setProbability(saved_ch_state.base_probability);
        ch.setDutyCycle(saved_ch_state.base_duty_cycle);
        ch.setOffset(saved_ch_state.base_offset);
        ch.setSwing(saved_ch_state.base_swing);
        ch.setSteps(saved_ch_state.base_euc_steps);
        ch.setHits(saved_ch_state.base_euc_hits);
        ch.setCv1Dest(static_cast<CvDestination>(saved_ch_state.cv1_dest));
        ch.setCv2Dest(static_cast<CvDestination>(saved_ch_state.cv2_dest));
    }
    interrupts();
}

void StateManager::_saveMetadata(const AppState& app) {
    noInterrupts();
    Metadata current_meta;
    strcpy(current_meta.sketch_name, SKETCH_NAME);
    current_meta.version = SKETCH_VERSION;

    // Global user settings
    current_meta.encoder_reversed = app.encoder_reversed;

    EEPROM.put(METADATA_START_ADDR, current_meta);
    interrupts();
}

void StateManager::_loadMetadata(AppState& app) {
    noInterrupts();
    Metadata metadata;
    EEPROM.get(METADATA_START_ADDR, metadata);
    app.encoder_reversed = metadata.encoder_reversed;
    interrupts();
}