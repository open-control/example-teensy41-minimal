/**
 * @file main.cpp
 * @brief Open Control Framework - Minimal Teensy 4.1 Example
 *
 * Demonstrates:
 * - Hardware configuration with constexpr definitions
 * - Simplified oc::teensy::AppBuilder API
 * - Fluent input binding API (onButton, onEncoder)
 * - MIDI CC output via MidiAPI
 *
 * Features shown:
 * - Button press → MIDI CC 127
 * - Button release → MIDI CC 0
 * - Button long press → Different action
 * - Encoder turn → MIDI CC (0-127 mapped from 0.0-1.0)
 */

#include <Arduino.h>

#include <optional>

// ═══════════════════════════════════════════════════════════════════
// Framework includes
// ═══════════════════════════════════════════════════════════════════

#include <oc/app/OpenControlApp.hpp>
#include <oc/context/IContext.hpp>
#include <oc/context/Requirements.hpp>
#include <oc/core/Result.hpp>
#include <oc/teensy/Teensy.hpp>

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
class MinimalContext : public oc::context::IContext {
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
        Serial.println("[MinimalContext] Initialized");
        return true;
    }

    void update() override {
        // Nothing to do each frame for this simple context
    }

    void cleanup() override {
        Serial.println("[MinimalContext] Cleanup");
    }

    const char* getName() const override { return "Minimal Controller"; }

private:
    void setupEncoderBindings() {
        // Encoder 1-4: Send MIDI CC on turn
        // Value is normalized [0.0-1.0], we map to [0-127]
        for (uint8_t i = 0; i < Config::ENCODERS.size(); ++i) {
            oc::hal::EncoderID id = Config::ENCODERS[i].id;
            uint8_t cc = Config::ENCODER_CC_BASE + i;

            onEncoder(id).turn().then([this, cc](float value) {
                uint8_t midiValue = static_cast<uint8_t>(value * 127.0f);
                midi().sendCC(Config::MIDI_CHANNEL, cc, midiValue);
                Serial.printf("[Encoder] CC %d = %d\n", cc, midiValue);
            });
        }
    }

    void setupButtonBindings() {
        // Button 1: Press sends CC 127, release sends CC 0
        onButton(Config::BUTTONS[0].id).press().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 127);
            Serial.println("[Button 1] Pressed -> CC 127");
        });

        onButton(Config::BUTTONS[0].id).release().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 0);
            Serial.println("[Button 1] Released -> CC 0");
        });

        // Button 1: Long press for alternative action
        onButton(Config::BUTTONS[0].id).longPress(Config::LONG_PRESS_MS).then([]() {
            Serial.println("[Button 1] Long press!");
            // Example: could trigger a different CC or mode switch
        });

        // Button 2: Toggle behavior (press sends 127, press again sends 0)
        onButton(Config::BUTTONS[1].id).press().then([this]() {
            button2_state_ = !button2_state_;
            uint8_t value = button2_state_ ? 127 : 0;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, value);
            Serial.printf("[Button 2] Toggle -> CC %d\n", value);
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
    // Initialize serial for debug output
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // Wait for serial (with timeout for standalone operation)
    }
    Serial.println("\n========================================");
    Serial.println("Open Control - Minimal Teensy 4.1 Example");
    Serial.println("========================================\n");

    // ─────────────────────────────────────────────────────
    // Build application
    // ─────────────────────────────────────────────────────
    app = oc::teensy::AppBuilder()
        .midi()
        .encoders(Config::ENCODERS)
        .buttons(Config::BUTTONS, Config::DEBOUNCE_MS)
        .inputConfig({
            .longPressMs = Config::LONG_PRESS_MS,
            .doubleTapWindowMs = Config::DOUBLE_TAP_MS
        });

    // ─────────────────────────────────────────────────────
    // Register context and start
    // ─────────────────────────────────────────────────────
    app->registerContext<MinimalContext>(ContextID::MINIMAL, "Minimal");

    auto result = app->begin();
    if (!result) {
        Serial.printf("[ERROR] Failed to initialize: %s\n",
                      oc::core::errorCodeToString(result.error().code));
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[OK] Application started successfully");
    Serial.println("\nControls:");
    Serial.println("  Encoders 1-4: Send MIDI CC 16-19");
    Serial.println("  Button 1: Press=CC20:127, Release=CC20:0, LongPress=debug");
    Serial.println("  Button 2: Toggle CC21 (127/0)");
    Serial.println("\n");
}

// ═══════════════════════════════════════════════════════════════════
// Arduino Loop
// ═══════════════════════════════════════════════════════════════════

void loop() {
    // Update the application (polls inputs, processes events, updates context)
    app->update();
}
