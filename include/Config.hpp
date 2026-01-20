#pragma once

/**
 * @file Config.hpp
 * @brief Hardware configuration for Teensy 4.1 minimal example
 *
 * All hardware definitions are constexpr for zero runtime overhead.
 * Adjust pin numbers to match your wiring.
 *
 * Wiring reference:
 * - Encoders use quadrature signals (A/B pins)
 * - Buttons use internal pull-up (connect to GND when pressed)
 */

#include <array>

#include <oc/hal/common/embedded/ButtonDef.hpp>
#include <oc/hal/common/embedded/EncoderDef.hpp>
#include <oc/types/Ids.hpp>
#include <oc/types/Callbacks.hpp>

namespace Config {

// ═══════════════════════════════════════════════════════════════════
// Timing Configuration
// ═══════════════════════════════════════════════════════════════════

/// Long press threshold in milliseconds
constexpr uint32_t LONG_PRESS_MS = 500;

/// Double tap window in milliseconds
constexpr uint32_t DOUBLE_TAP_MS = 300;

/// Button debounce time in milliseconds
constexpr uint8_t DEBOUNCE_MS = 5;

// ═══════════════════════════════════════════════════════════════════
// Encoder Configuration
// ═════════════════════════════════════════════════════════════════

/**
 * @brief Encoder hardware definitions
 *
 * Parameters:
 * - id: Unique identifier (1-based for MIDI CC mapping)
 * - pinA/pinB: Quadrature signal pins
 * - ppr: Pulses per revolution (check encoder datasheet)
 * - rangeAngle: Rotation angle for full 0.0-1.0 range
 * - ticksPerEvent: Ticks per event emission (4 = one detent)
 * - invertDirection: Flip rotation direction if wired backwards
 */
constexpr std::array<oc::hal::common::embedded::EncoderDef, 4> ENCODERS = {{
    // EncoderDef(id, pinA, pinB, ppr, rangeAngle, ticksPerEvent, invertDirection)
    oc::hal::common::embedded::EncoderDef(1, 22, 23, 24, 270, 4, false),  // MACRO_1
    oc::hal::common::embedded::EncoderDef(2, 18, 19, 24, 270, 4, false),  // MACRO_2
    oc::hal::common::embedded::EncoderDef(3, 40, 41, 24, 270, 4, false),  // MACRO_3
    oc::hal::common::embedded::EncoderDef(4, 36, 37, 24, 270, 4, false),  // MACRO_4
}};

// ═══════════════════════════════════════════════════════════════════
// Button Configuration
// ═══════════════════════════════════════════════════════════════════

/**
 * @brief Button hardware definitions
 *
 * Parameters:
 * - id: Unique identifier
 * - pin: GPIO pin with source (MCU direct)
 * - activeLow: true = pressed when LOW (pull-up), false = pressed when HIGH
 */
constexpr std::array<oc::hal::common::embedded::ButtonDef, 2> BUTTONS = {{
    // ButtonDef(id, GpioPin{pin, source}, activeLow)
    oc::hal::common::embedded::ButtonDef(1, oc::hal::common::embedded::GpioPin{32, oc::hal::common::embedded::GpioPin::Source::MCU}, true),  // NAV
    oc::hal::common::embedded::ButtonDef(2, oc::hal::common::embedded::GpioPin{35, oc::hal::common::embedded::GpioPin::Source::MCU}, true),  // AUX
}};

// ═══════════════════════════════════════════════════════════════════
// MIDI Configuration
// ═══════════════════════════════════════════════════════════════════

/// MIDI channel (0-15, displayed as 1-16 in DAWs)
constexpr uint8_t MIDI_CHANNEL = 0;

/// Base CC number for encoders (encoder 1 = CC 16, encoder 2 = CC 17, etc.)
constexpr uint8_t ENCODER_CC_BASE = 16;

/// CC number for button 1
constexpr uint8_t BUTTON1_CC = 20;

/// CC number for button 2
constexpr uint8_t BUTTON2_CC = 21;

}  // namespace Config
