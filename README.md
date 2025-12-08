# Open Control - Teensy 4.1 Minimal Example

A minimal [Open Control Framework](https://github.com/open-control/framework) example demonstrating:
- 4 rotary encoders → MIDI CC output
- 2 buttons with press, release, and long press handling
- Clean architecture with `AppBuilder` and fluent binding API

## Hardware Requirements

- Teensy 4.1
- 4x Rotary encoders (quadrature, 24 PPR recommended)
- 2x Momentary push buttons
- USB cable for MIDI and power

## Default Wiring

| Component | Pin A | Pin B | Notes |
|-----------|-------|-------|-------|
| Encoder 1 | 22 | 23 | Macro 1 |
| Encoder 2 | 18 | 19 | Macro 2 |
| Encoder 3 | 40 | 41 | Macro 3 |
| Encoder 4 | 36 | 37 | Macro 4 |
| Button 1 | 32 | GND | Navigation |
| Button 2 | 35 | GND | Auxiliary |

> Buttons use internal pull-up resistors. Connect one leg to the pin, the other to GND.

## MIDI Mapping

| Control | MIDI Message | Channel |
|---------|--------------|---------|
| Encoder 1 | CC 16 (0-127) | 1 |
| Encoder 2 | CC 17 (0-127) | 1 |
| Encoder 3 | CC 18 (0-127) | 1 |
| Encoder 4 | CC 19 (0-127) | 1 |
| Button 1 Press | CC 20 = 127 | 1 |
| Button 1 Release | CC 20 = 0 | 1 |
| Button 2 Toggle | CC 21 = 127/0 | 1 |

## Quick Start

### 1. Install PlatformIO

```bash
# VS Code extension (recommended)
# Or CLI: pip install platformio
```

### 2. Clone and Build

```bash
git clone https://github.com/open-control/example-teensy41-minimal.git
cd example-teensy41-minimal
pio run -e release
```

### 3. Upload

```bash
pio run -e release -t upload
```

### 4. Monitor Serial Output

```bash
pio device monitor -b 115200
```

## Project Structure

```
example-teensy41-minimal/
├── include/
│   └── Config.hpp      # Hardware pin definitions (edit this!)
├── src/
│   └── main.cpp        # Application entry point
├── platformio.ini      # Build configuration
└── README.md
```

## Customization

### Change Pin Assignments

Edit `include/Config.hpp`:

```cpp
constexpr std::array<oc::common::EncoderDef, 4> ENCODERS = {{
    {.id = 1, .pinA = 22, .pinB = 23, ...},  // Change pins here
    // ...
}};
```

### Change MIDI Mapping

Edit `include/Config.hpp`:

```cpp
constexpr uint8_t MIDI_CHANNEL = 0;      // 0-15 (displayed as 1-16)
constexpr uint8_t ENCODER_CC_BASE = 16;  // First encoder CC number
constexpr uint8_t BUTTON1_CC = 20;
constexpr uint8_t BUTTON2_CC = 21;
```

### Change Encoder Behavior

```cpp
// In Config.hpp, per encoder:
.rangeAngle = 270,      // Degrees for full 0-1 range (270° = ~3/4 turn)
.ticksPerEvent = 4,     // Events per detent (4 = smooth, 1 = sensitive)
.invertDirection = true // Flip direction if wired backwards
```

## Code Walkthrough

### 1. Configuration (`Config.hpp`)

All hardware is defined as `constexpr` for zero runtime overhead:

```cpp
constexpr std::array<EncoderDef, 4> ENCODERS = {{...}};
constexpr std::array<ButtonDef, 2> BUTTONS = {{...}};
```

### 2. Context (`MinimalContext`)

Implements `IContext` interface with fluent binding API:

```cpp
class MinimalContext : public oc::context::IContext {
public:
    // Declare required APIs (validated at context registration)
    static constexpr oc::context::Requirements REQUIRES{
        .button = true,
        .encoder = true,
        .midi = true
    };

    bool initialize() override {
        // Encoder binding - value is normalized 0.0-1.0
        onEncoder(encoderId).turn().then([this](float value) {
            midi().sendCC(channel, cc, static_cast<uint8_t>(value * 127.0f));
        });

        // Button bindings
        onButton(buttonId).press().then([this]() { /* ... */ });
        onButton(buttonId).release().then([this]() { /* ... */ });
        onButton(buttonId).longPress(500).then([]() { /* ... */ });

        return true;
    }

    void update() override { /* Called every frame */ }
    void cleanup() override { /* Called on context switch */ }
    const char* getName() const override { return "Context Name"; }
};
```

The `REQUIRES` structure ensures that all needed APIs (buttons, encoders, MIDI) are available at registration time. If a required API is missing, registration fails with a descriptive error.

### 3. Application Setup (`main.cpp`)

Uses `AppBuilder` for clean dependency injection:

```cpp
app.emplace(oc::app::AppBuilder()
    .timeProvider(millis)
    .midi(std::make_unique<oc::teensy::UsbMidi>())
    .encoders(std::make_unique<oc::teensy::EncoderController<4>>(
        Config::ENCODERS, oc::teensy::encoderFactory()))
    .buttons(std::make_unique<oc::teensy::ButtonController<2>>(
        Config::BUTTONS, oc::teensy::gpio(), nullptr, Config::DEBOUNCE_MS))
    .inputConfig(inputConfig)
    .build());

app->registerContext<MinimalContext>(ContextID::MINIMAL, "Minimal");
app->begin();
```

### 4. Main Loop

Single call updates everything:

```cpp
void loop() {
    app->update();  // Polls inputs, processes events, updates context
}
```

## Input Binding API Reference

### Button Bindings

```cpp
// Simple press
onButton(buttonId).press().then([]() { /* action */ });

// Release
onButton(buttonId).release().then([]() { /* action */ });

// Long press (0 = use config default, or specify ms)
onButton(buttonId).longPress().then([]() { /* action */ });
onButton(buttonId).longPress(500).then([]() { /* 500ms */ });

// Double tap (0 = use config default, or specify window ms)
onButton(buttonId).doubleTap().then([]() { /* action */ });
onButton(buttonId).doubleTap(300).then([]() { /* 300ms window */ });

// Button combo (triggers when both buttons pressed)
onButton(BTN_1).combo(BTN_2).then([]() { /* action */ });

// Toggle/latch behavior
onButton(buttonId).press().latch().then([]() { /* toggles on each press */ });

// Conditional activation
onButton(buttonId).press().when(condition).then([]() { /* only if condition() == true */ });
```

### Encoder Bindings

```cpp
// Turn - value is always 0.0-1.0 (normalized)
onEncoder(encoderId).turn().then([](float value) {
    uint8_t midiValue = static_cast<uint8_t>(value * 127.0f);  // Map to 0-127
});

// Conditional activation (e.g., shift+encoder)
onEncoder(encoderId).turn().when(shiftPressed).then([](float value) {
    // Only triggers when shift button is held
});
```

### MIDI Output

```cpp
midi().sendCC(channel, cc, value);              // Control Change (channel 0-15, cc/value 0-127)
midi().sendNoteOn(channel, note, velocity);     // Note On
midi().sendNoteOff(channel, note, velocity);    // Note Off
midi().sendProgramChange(channel, program);     // Program Change
midi().sendPitchBend(channel, value);           // Pitch Bend (-8192 to 8191)
midi().sendChannelPressure(channel, pressure);  // Aftertouch
midi().sendSysEx(data, length);                 // System Exclusive
midi().allNotesOff();                           // Panic - stops all notes
```

## Troubleshooting

### No MIDI Output

1. Check USB mode is `USB_MIDI_SERIAL` in `platformio.ini`
2. Verify Teensy appears as MIDI device in your DAW
3. Check serial monitor for debug messages

### Encoder Direction Wrong

Set `invertDirection = true` in the encoder definition.

### Encoder Too Sensitive/Not Sensitive

Adjust `ticksPerEvent`:
- `1` = Very sensitive (event per tick)
- `4` = Standard (event per detent)
- `8` = Less sensitive

### Button Not Responding

1. Check wiring (button should connect pin to GND)
2. Verify `activeLow = true` for pull-up configuration
3. Increase `DEBOUNCE_MS` if bouncing

## Development Mode

To use local development versions of the framework:

```bash
# Clone all repos in same parent directory
git clone https://github.com/open-control/framework.git
git clone https://github.com/open-control/hal-common.git
git clone https://github.com/open-control/hal-teensy.git
git clone https://github.com/open-control/example-teensy41-minimal.git

# Build with local deps
cd example-teensy41-minimal
pio run -e dev
```

## License

Apache 2.0 - See [LICENSE](LICENSE)
