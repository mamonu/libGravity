#ifndef EUCLIDEAN_H
#define EUCLIDEAN_H

#define MAX_PATTERN_LEN 16

struct PatternState {
    uint8_t steps;
    uint8_t hits;
};

const PatternState DEFAULT_PATTERN = {16, 4};

class Pattern {
   public:
    Pattern() {}
    ~Pattern() {}

    enum Step {
        HIT,
        REST,
        PADDING,
    };

    void Init(PatternState state) {
        steps_ = constrain(state.steps, 0, MAX_PATTERN_LEN);
        hits_ = constrain(state.hits, 1, steps_);
        updatePattern();
    }

    PatternState GetState() { return {steps_, hits_}; }

    // Get the current step value and advance the euclidean rhythm step index
    // to the next step in the pattern.
    Step NextStep() {
        byte padding_ = 0;
        if (steps_ == 0) return REST;

        Step value = GetCurrentStep(current_step_);
        current_step_ =
            (current_step_ < steps_ + padding_ - 1) ? current_step_ + 1 : 0;
        return value;
    }

    Step GetCurrentStep(byte i) { return pattern_[i]; }

    void SetSteps(byte steps) {
        steps_ = constrain(steps, 0, MAX_PATTERN_LEN);
        hits_ = min(hits_, steps_);
        updatePattern();
    }

    void SetHits(byte hits) {
        hits_ = constrain(hits, 0, steps_);
    }

    // void ChangeOffset(byte val) {
    //     offset_ = constrain(offset_ + val, 0, (steps_ + padding_));
    //     updatePattern();
    // }

    // void ChangePadding(byte val) {
    //     if (val == 1 && padding_ + steps_ < MAX_PATTERN_LEN) {
    //         padding_++;
    //         updatePattern();
    //     } else if (val == -1 && padding_ > 0) {
    //         padding_--;
    //         offset_ = min(offset_, (padding_ + steps_) - 1);
    //         updatePattern();
    //     }
    // }

    void Reset() { current_step_ = 0; }
    bool IsActive() { return steps_ != 0 && hits_ != 0; }

    inline uint8_t GetSteps() { return steps_; }
    inline uint8_t GetHits() { return hits_; }
    inline uint8_t GetStepIndex() { return current_step_; }

   private:
    uint8_t steps_ = 0;
    uint8_t hits_ = 0;
    volatile uint8_t current_step_ = 0;
    Step pattern_[MAX_PATTERN_LEN];

    // Update the euclidean rhythm pattern when attributes change.
    void updatePattern() {
        // Fill current pattern with "padding" steps, then overwrite with hits
        // and rests.
        for (int i = 0; i < MAX_PATTERN_LEN; i++) {
            pattern_[i] = PADDING;
        }

        // Populate the euclidean rhythm pattern according to the current
        // instance variables.
        byte bucket = 0;
        byte offset_ = 0;  // temp disable
        byte padding_ = 0;  // temp disable
        pattern_[offset_] = (hits_ > 0) ? HIT : REST;
        for (int i = 1; i < steps_; i++) {
            bucket += hits_;
            if (bucket >= steps_) {
                bucket -= steps_;
                pattern_[(i + offset_) % (steps_ + padding_)] = HIT;
            } else {
                pattern_[(i + offset_) % (steps_ + padding_)] = REST;
            }
        }
    }
};

#endif