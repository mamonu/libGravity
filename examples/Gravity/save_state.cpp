#include "save_state.h"

#include <EEPROM.h>

#include "app_state.h"

// Calculate the starting address for EepromData, leaving space for metadata.
static const int EEPROM_DATA_START_ADDR = sizeof(StateManager::Metadata);

StateManager::StateManager() : _isDirty(false), _lastChangeTime(0) {}

bool StateManager::initialize(AppState& app) {
    if (_isDataValid()) {
        // Load data from the transient slot.
        return loadData(app, MAX_SAVE_SLOTS);
    } else {
        // EEPROM does not contain save data for this firmware & version.
        // Initialize eeprom and save default patter to all save slots.
        reset(app);
        _saveMetadata();
        // MAX_SAVE_SLOTS slot is reserved for transient state.
        for (int i = 0; i <= MAX_SAVE_SLOTS; i++) {
            app.selected_save_slot = i;
            _saveState(app, i);
        }
        return false;
    }
}

bool StateManager::loadData(AppState& app, byte slot_index) {
    if (slot_index >= MAX_SAVE_SLOTS) return false;

    _loadState(app, slot_index);

    return true;
}

void StateManager::saveData(const AppState& app) {
    if (app.selected_save_slot >= MAX_SAVE_SLOTS) return;

    _saveState(app, app.selected_save_slot);
    _isDirty = false;
}

void StateManager::update(const AppState& app) {
    if (_isDirty && (millis() - _lastChangeTime > SAVE_DELAY_MS)) {
        // MAX_SAVE_SLOTS slot is reserved for transient state.
        _saveState(app, MAX_SAVE_SLOTS);
        _isDirty = false;
    }
}

void StateManager::reset(AppState& app) {
    app.tempo = Clock::DEFAULT_TEMPO;
    app.encoder_reversed = false;
    app.selected_param = 0;
    app.selected_channel = 0;
    app.selected_source = Clock::SOURCE_INTERNAL;
    app.selected_pulse = Clock::PULSE_PPQN_24;
    app.selected_save_slot = 0;

    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].Init();
    }

    _isDirty = false;
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

void StateManager::_saveState(const AppState& app, byte slot_index) {
    if (app.selected_save_slot >= MAX_SAVE_SLOTS) return;

    noInterrupts();
    static EepromData save_data;

    save_data.tempo = app.tempo;
    save_data.encoder_reversed = app.encoder_reversed;
    save_data.selected_param = app.selected_param;
    save_data.selected_channel = app.selected_channel;
    save_data.selected_source = static_cast<byte>(app.selected_source);
    save_data.selected_pulse = static_cast<byte>(app.selected_pulse);
    save_data.selected_save_slot = app.selected_save_slot;

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
    noInterrupts();
    static EepromData load_data;
    int address = EEPROM_DATA_START_ADDR + (slot_index * sizeof(EepromData));
    EEPROM.get(address, load_data);

    // Restore app state from loaded data.
    app.tempo = load_data.tempo;
    app.encoder_reversed = load_data.encoder_reversed;
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

void StateManager::_saveMetadata() {
    noInterrupts();
    Metadata current_meta;
    strcpy(current_meta.sketch_name, SKETCH_NAME);
    current_meta.version = SKETCH_VERSION;
    EEPROM.put(0, current_meta);
    interrupts();
}
