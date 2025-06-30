#ifndef EUCLIDEAN_H
#define EUCLIDEAN_H

#define MAX_PATTERN_LEN 16

struct PatternState {
    uint8_t steps;
    uint8_t hits;
};

const PatternState DEFAULT_PATTERN = {1, 1};

class Pattern {
   public:
    Pattern() {}
    ~Pattern() {}

    enum Step {
        REST,
        HIT,
    };

    void Init(PatternState state) {
        steps_ = constrain(state.steps, 1, MAX_PATTERN_LEN);
        hits_ = constrain(state.hits, 1, steps_);
        updatePattern();
    }

    PatternState GetState() { return {steps_, hits_}; }

    Step GetCurrentStep(byte i) { return pattern_[i]; }

    void SetSteps(int steps) {
        steps_ = constrain(steps, 1, MAX_PATTERN_LEN);
        hits_ = min(hits_, steps_);
        updatePattern();
    }

    void SetHits(int hits) {
        hits_ = constrain(hits, 1, steps_);
        updatePattern();
    }

    void Reset() { step_index_ = 0; }

    uint8_t GetSteps() { return steps_; }
    uint8_t GetHits() { return hits_; }
    uint8_t GetStepIndex() { return step_index_; }

    // Get the current step value and advance the euclidean rhythm step index
    // to the next step in the pattern.
    Step NextStep() {
        if (steps_ == 0) return REST;

        Step value = GetCurrentStep(step_index_);
        step_index_ = (step_index_ < steps_ - 1) ? step_index_ + 1 : 0;
        return value;
    }

   private:
    uint8_t steps_ = 0;
    uint8_t hits_ = 0;
    volatile uint8_t step_index_ = 0;
    Step pattern_[MAX_PATTERN_LEN];

    // Update the euclidean rhythm pattern when attributes change.
    void updatePattern() {
        byte bucket = 0;
        pattern_[0] = HIT;
        for (int i = 1; i < steps_; i++) {
            bucket += hits_;
            if (bucket >= steps_) {
                bucket -= steps_;
                pattern_[i] = HIT;
            } else {
                pattern_[i] = REST;
            }
        }
    }
};

#endif