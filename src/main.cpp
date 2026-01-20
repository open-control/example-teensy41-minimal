/**
 * @file main.cpp
 * @brief Open Control Framework - Minimal Teensy 4.1 Example
 *
 * Demonstrates:
 * - Hardware configuration with constexpr definitions
 * - Simplified oc::hal::teensy::AppBuilder API
 * - Fluent input binding API (onButton, onEncoder)
 * - MIDI CC output via MidiAPI
 *
 * Features shown:
 * - Button press → MIDI CC 127
 * - Button release → MIDI CC 0
 * - Button long press → Different action
 * - Encoder turn → MIDI CC (0-127 mapped from 0.0-1.0)
 *
 * NOTE: Enable -D OC_LOG in platformio.ini build_flags to see debug output.
 *       Remove it for production (zero overhead, instant boot).
 *
 * Hardware configuration is in Config.hpp - ADAPT pins to your wiring.
 */

#include <optional>

#include <oc/hal/teensy/Teensy.hpp>
#include <oc/app/OpenControlApp.hpp>
#include <oc/context/ContextBase.hpp>
#include <oc/context/Requirements.hpp>

// Local configuration
#include "Config.hpp"

// ═══════════════════════════════════════════════════════════════════
// Context ID (user-defined enum)
// ═══════════════════════════════════════════════════════════════════

enum class ContextID : uint8_t { MINIMAL = 0 };

// ═══════════════════════════════════════════════════════════════════
// Minimal Context Implementation
// ═══════════════════════════════════════════════════════════════════

/**
 * @brief Simple standalone context for MIDI controller
 *
 * Sets up all input bindings during initialization.
 * Encoders send CC, buttons toggle CC values.
 */
class MinimalContext : public oc::context::ContextBase {
public:
    /// Declare required APIs (validated at registration time)
    static constexpr oc::context::Requirements REQUIRES{
        .button = true,
        .encoder = true,
        .midi = true
    };

    bool initialize() override {
        setupEncoderBindings();
        setupButtonBindings();
        return true;
    }

    void update() override {}
    void cleanup() override {}

    const char* getName() const override { return "Minimal Controller"; }

private:
    void setupEncoderBindings() {
        // Encoder 1-4: Send MIDI CC on turn
        // Value is normalized [0.0-1.0], we map to [0-127]
        for (uint8_t i = 0; i < Config::ENCODERS.size(); ++i) {
            oc::EncoderID id = Config::ENCODERS[i].id;
            uint8_t cc = Config::ENCODER_CC_BASE + i;

            onEncoder(id).turn().then([this, cc](float value) {
                uint8_t midiValue = static_cast<uint8_t>(value * 127.0f);
                midi().sendCC(Config::MIDI_CHANNEL, cc, midiValue);
                OC_LOG_DEBUG("Encoder: CC {} = {}", cc, midiValue);
            });
        }
    }

    void setupButtonBindings() {
        // Button 1: Press sends CC 127, release sends CC 0
        onButton(Config::BUTTONS[0].id).press().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 127);
            OC_LOG_DEBUG("Button 1: Press -> CC 127");
        });

        onButton(Config::BUTTONS[0].id).release().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 0);
            OC_LOG_DEBUG("Button 1: Release -> CC 0");
        });

        // Button 1: Long press for alternative action
        onButton(Config::BUTTONS[0].id).longPress(Config::LONG_PRESS_MS).then([]() {
            OC_LOG_INFO("Button 1: Long press!");
        });

        // Button 2: Toggle behavior (press sends 127, press again sends 0)
        onButton(Config::BUTTONS[1].id).press().then([this]() {
            button2_state_ = !button2_state_;
            uint8_t value = button2_state_ ? 127 : 0;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, value);
            OC_LOG_DEBUG("Button 2: Toggle -> CC {}", value);
        });
    }

    bool button2_state_ = false;
};

// ═══════════════════════════════════════════════════════════════════
// Global Application Instance
// ═══════════════════════════════════════════════════════════════════

std::optional<oc::app::OpenControlApp> app;

// ═══════════════════════════════════════════════════════════════════
// Arduino Setup
// ═══════════════════════════════════════════════════════════════════

void setup() {
    OC_LOG_INFO("Minimal Example");

    app = oc::hal::teensy::AppBuilder()
        .midi()
        .encoders(Config::ENCODERS)
        .buttons(Config::BUTTONS, Config::DEBOUNCE_MS)
        .inputConfig({
            .longPressMs = Config::LONG_PRESS_MS,
            .doubleTapWindowMs = Config::DOUBLE_TAP_MS
        });

    app->registerContext<MinimalContext>(ContextID::MINIMAL, "Minimal");
    app->begin();

    OC_LOG_INFO("Ready");
}

// ═══════════════════════════════════════════════════════════════════
// Arduino Loop
// ═══════════════════════════════════════════════════════════════════

void loop() {
    // Update the application (polls inputs, processes events, updates context)
    app->update();
}
