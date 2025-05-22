/* ==== PRESET MANAGER – split for left/right hemispheres ========================= */
#include "HemisphereApplet.h"
#include "OC_core.h"
#include "OC_digital_inputs.h"

// ------------------- PresetMgrLeft (LEFT hemisphere) -------------------
class PresetMgrLeft : public HemisphereApplet {
public:
    static constexpr int MAX_BANKS = 16;
    static constexpr int PUSH_DURATION_MS = 5000; // 5 seconds
    static constexpr int GATE_DURATION_MS = 4000; // 4 seconds
    static constexpr int GATE_OFFSET_MS = 1000;   // 1 second

    static const int lut1_[MAX_BANKS]; // For OUT1
    static const int lut2_[MAX_BANKS]; // For OUT2
    static const int lut3_[MAX_BANKS]; // For OUT3

    static bool pushed_;
    static int push_countdown_; // ms
    static int gate_countdown_; // ms

    static int GetBank() { return bank_; }
    static void SetBank(int new_bank) {
        bank_ = constrain(new_bank, 0, MAX_BANKS - 1);
    }
    static void IncrementBank(int dir) {
        int old_bank = bank_;
        bank_ = constrain(bank_ + dir, 0, MAX_BANKS - 1);
        if (old_bank != bank_) {
            // Bank changed, activate preset mode
            pushed_ = true;
            push_countdown_ = PUSH_DURATION_MS;
            gate_countdown_ = PUSH_DURATION_MS;
        }
    }

    const char* applet_name() override { return "Prst"; }
    void Start() override {}

    void Controller() override {
        // Update timers
        if (PresetMgrLeft::push_countdown_ > 0) {
            --PresetMgrLeft::push_countdown_;
            if (PresetMgrLeft::push_countdown_ == 0) PresetMgrLeft::pushed_ = false;
        }
        if (PresetMgrLeft::gate_countdown_ > 0) {
            --PresetMgrLeft::gate_countdown_;
        }
        
        // OUT1/2/3: pass IN1/2/3, or preset voltage if pushed (full 5s window)
        bool use_preset = PresetMgrLeft::pushed_;
        Out(0, use_preset ? PresetMgrLeft::lut1_[GetBank()] : In(0)); // OUT1
        Out(1, use_preset ? PresetMgrLeft::lut2_[GetBank()] : In(1)); // OUT2
        Out(2, use_preset ? PresetMgrLeft::lut3_[GetBank()] : In(2)); // OUT3

        // TEMP: Output a simple triangle LFO on OUT3 (D) to test hardware
        static int lfo_phase = 0;
        lfo_phase = (lfo_phase + 1) % 128;
        int lfo_value = (lfo_phase < 64) ? (lfo_phase * 120) : ((127 - lfo_phase) * 120); // 0..7680
        Out(3, lfo_value); // Output triangle LFO (0-7680, approx 0-5V) on physical output D
    }
    
    // Button handlers - button press is no longer used to activate preset mode
    void OnButtonDown() { }
    void OnButtonUp() { }
    
    void OnEncoderMove(int dir) override {
        // No longer changes bank_ here, right encoder does.
    }

    uint64_t OnDataRequest() override {
        uint64_t data = 0;
        Pack(data, PackLocation {0, 4}, GetBank());
        return data;
    }

    void OnDataReceive(uint64_t data) override {
        SetBank(Unpack(data, PackLocation {0, 4}));
    }

    void SetHelp() override {
        help[HEMISPHERE_HELP_CVS]     = "I1‑3 pass";
        help[HEMISPHERE_HELP_OUTS]    = "O1‑3 CV";
        help[HEMISPHERE_HELP_ENCODER] = "Trn=Chng Bnk";
        help[HEMISPHERE_HELP_DIGITALS]= "";
    }
    void View() override {
        gfxHeader("Prst L");
        if (PresetMgrLeft::pushed_) {
            gfxPrint(1, 20, "Bank:"); // No space padding
            gfxPrint(PresetMgrLeft::GetBank() + 1);
            gfxPrint("/");
            gfxPrint(MAX_BANKS);
            // More concise CV output display
            gfxIcon(1, 30, CV_ICON);
            gfxPrint(10, 30, "ALL=");
            gfxPrintVoltage(PresetMgrLeft::lut1_[PresetMgrLeft::GetBank()]);
            gfxIcon(52, 40, FAVORITE_ICON); // Indicate preset output active
        } else {
            gfxPrint(1, 20, "Bank:"); // No space padding
            gfxPrint(PresetMgrLeft::GetBank() + 1);
            // Display CV1, CV2, CV3 pass-through more concisely
            for (int i = 0; i < 3; ++i) {
                gfxIcon(1, 30 + (i * 10), CV_ICON);
                gfxPrint(10, 30 + (i * 10), i + 1);
                gfxPrint("=");
                gfxPrintVoltage(ViewIn(i));
            }
        }
    }
private:
    static int bank_; // Keep bank_ private, manage with static methods
};

// Initialize static members
// Linear ramp from midpoint to 5V over 15 steps
// Calculate the step size for 0V to 5V over 14 intervals (15 steps)
#define LUT_STEPS 15
#define LUT_MAX (LUT_STEPS - 1)
#define LUT_SCALE ((HEMISPHERE_MAX_CV * 5) / 6)
// First step is at half a step above 0
const int PresetMgrLeft::lut1_[PresetMgrLeft::MAX_BANKS] = {
    320, 1601, 1901, 2241, 2881, 3221, 3721, 4261, 4701, 5000, 5441, 6081, 6521, 7061, 8001, 10000
};
const int PresetMgrLeft::lut2_[PresetMgrLeft::MAX_BANKS] = {
    320, 1401, 1901, 2241, 2881, 3221, 3721, 4261, 4701, 5000, 5441, 6081, 6521, 7061, 8001, 10000
};
const int PresetMgrLeft::lut3_[PresetMgrLeft::MAX_BANKS] = {
    320, 1401, 1901, 2241, 2881, 3221, 3721, 4261, 4701, 5000, 5441, 6081, 6521, 7061, 8001, 10000
};
#undef LUT_STEPS
#undef LUT_MAX
#undef LUT_SCALE
int PresetMgrLeft::bank_ = 0;
bool PresetMgrLeft::pushed_ = false;
int  PresetMgrLeft::push_countdown_ = 0;
int  PresetMgrLeft::gate_countdown_ = 0;

// ------------------- PresetMgrRight (RIGHT hemisphere) -------------------
class PresetMgrRight : public HemisphereApplet {
public:
    // No need to redefine MAX_BANKS, use PresetMgrLeft::MAX_BANKS

    const char* applet_name() override { return "Prst R"; }
    void Start() override {}

    void Controller() override {
        bool use_preset = PresetMgrLeft::pushed_;
        Out(0, use_preset ? PresetMgrLeft::lut1_[PresetMgrLeft::GetBank()] : In(0));
        // Out(1): 3 longer bursts when gated, with a small delay before the first burst
        static int burst_counter = 0;
        static int burst_phase = 0;
        static int initial_delay = 0;
        int out1 = In(1);
        if (use_preset) {
            if (initial_delay < 40) { // 40-cycle delay before first burst (was 20)
                out1 = 0;
                initial_delay++;
            } else if (burst_counter < 3) {
                // Each burst lasts 60 cycles (high), off for 20 cycles (low) (was 30/10)
                if (burst_phase < 60) out1 = HEMISPHERE_MAX_CV;
                else out1 = 0;
                burst_phase++;
                if (burst_phase >= 80) {
                    burst_phase = 0;
                    burst_counter++;
                }
            } else {
                out1 = 0;
            }
        } else {
            burst_counter = 0;
            burst_phase = 0;
            initial_delay = 0;
        }
        Out(1, out1);
    }
    
    // Button handlers - button press is no longer used to activate preset mode
    void OnButtonDown() { }
    void OnButtonUp() { }
    
    void OnEncoderMove(int direction) override {
        PresetMgrLeft::IncrementBank(direction);
        // This now handles activating preset mode via the static IncrementBank method
    }

    uint64_t OnDataRequest() override { // Changed return type
        // No longer saves bank_ here, PresetMgrLeft does.
        return 0; // Return empty data
    }

    void OnDataReceive(uint64_t data) override { // Changed parameter type
        // No longer loads bank_ here, PresetMgrLeft does.
        // (void)data; // Mark data as unused if necessary, or just leave empty
    }
    void SetHelp() override {
        help[HEMISPHERE_HELP_CVS]     = "I1‑3 pass";
        help[HEMISPHERE_HELP_OUTS]    = "O1‑3 CV, OUT4=Gate";
        help[HEMISPHERE_HELP_ENCODER] = "Turn=Chng Bnk";
        help[HEMISPHERE_HELP_DIGITALS]= "";
    }
    void View() override {
        // Skip the header as it's drawn by BaseView
        gfxHeader(applet_name());
        if (PresetMgrLeft::pushed_) {
            gfxPrint(1, 20, "Bank:");
            gfxPrint(PresetMgrLeft::GetBank() + 1);
            // Display preset voltage
            gfxIcon(1, 30, CV_ICON);
            gfxPrint(10, 30, "A-C=");
            gfxPrintVoltage(PresetMgrLeft::lut1_[PresetMgrLeft::GetBank()]);

            // Gate indicator (D)
            gfxIcon(1, 42, GATE_ICON);
            bool gate_active = PresetMgrLeft::gate_countdown_ > 0 && 
                              PresetMgrLeft::gate_countdown_ <= (PresetMgrLeft::PUSH_DURATION_MS - PresetMgrLeft::GATE_OFFSET_MS);
            gfxPrint(10, 42, "D=");
            gfxPrint(gate_active ? "ON" : "OFF");
            gfxIcon(52, 5, FAVORITE_ICON);
        } else {
            gfxPrint(1, 20, "Bank:");
            gfxPrint(PresetMgrLeft::GetBank() + 1);
            // Minimal status only
            gfxPrint(1, 35, "Pass-thu");
        }
    }
};
