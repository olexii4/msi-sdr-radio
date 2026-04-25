# AI Agent Guidelines for MSi-SDR Radio

This document provides context and guidelines for AI coding assistants working on the MSi-SDR Radio project.

## Project Overview

MSi-SDR Radio is a lightweight, native SDR (Software Defined Radio) receiver application for **MSi.SDR** (MSI2500 + MSI001) and **SDRPlay RSP1/RSP1A clone** devices on macOS. It provides:

- Real-time spectrum and waterfall display
- FM broadcast demodulation
- Band preset switching (LF1, LF2, MW, HF, FM)
- Multi-device selection
- Interactive frequency/gain/volume controls

## Technology Stack

- **Language**: C11
- **Graphics**: OpenGL 2.1 (stroke-font rendering, spectrum/waterfall display)
- **Audio**: SDL2 (48 kHz callback with ring buffer)
- **DSP**: FFTW3 (2048-point FFT), custom FM demodulator
- **USB**: libusb 1.0 via integrated libmirisdr driver
- **Build**: CMake 3.10+
- **Platform**: macOS (Apple Silicon and Intel)

## Project Structure

```text
├── CMakeLists.txt           # Unified build (mirisdr static lib + app)
├── cmake/
│   └── FindLibUSB.cmake     # CMake module for libusb discovery
├── lib/
│   └── mirisdr/             # Integrated libmirisdr driver (GPL-2.0)
│       ├── include/         # Public API headers (mirisdr.h, mirisdr_export.h)
│       └── src/             # Driver sources (single compilation unit: libmirisdr.c)
│           └── convert/     # Sample format converters
└── src/
    ├── main.c               # Entry point, main loop
    ├── device.c / device.h  # SDR device management, IQ ring buffer, async streaming
    ├── dsp.c / dsp.h        # FFT spectrum analysis, FM demodulation
    ├── audio.c / audio.h    # SDL2 audio output with ring buffer
    └── ui.c / ui.h          # OpenGL UI, spectrum/waterfall, controls
```

## Common Dev Commands

```bash
# Install dependencies (macOS)
brew install sdl2 fftw libusb

# Build
mkdir build && cd build
cmake .. -DDETACH_KERNEL_DRIVER=ON
make

# Run
./msi-sdr              # start at 100 MHz FM
./msi-sdr 98.5         # start at 98.5 MHz
./msi-sdr 98.5 40      # start at 98.5 MHz, 40 dB gain
```

## Architecture Notes

### Device Layer (`device.c`)

- Async streaming runs in a separate thread with retry logic (10 retries, 1s delay)
- `device_error` flag guards all API calls — prevents crashes on USB disconnect
- Gain API takes tenths-of-dB internally (e.g. 400 = 40.0 dB), divided by 10 at the `mirisdr_set_tuner_gain()` boundary (expects 0–102 integer dB)
- RSP1A clones use `MIRISDR_HW_DEFAULT` flavour (not `MIRISDR_HW_SDRPLAY`)
- IQ data flows through a lock-free ring buffer (`iq_ring_t`)

### DSP Layer (`dsp.c`)

- FFT: 2048-point with Hann window, DC-centered output
- FM demod: polar discriminator runs at full sample rate, then audio is decimated (NOT pre-decimation of I/Q)
- De-emphasis: 75 µs time constant at 48 kHz audio rate

### UI Layer (`ui.c`)

- All text rendered via OpenGL 2.1 stroke font (no texture atlas or font library)
- Controls area is 120px high at screen bottom
- Band presets set both center frequency and hardware bandwidth
- Adaptive tuning steps: coarse = BW/20 (5%), fine = BW/200 (0.5%), minimum 1 kHz

### libmirisdr (`lib/mirisdr/`)

- `libmirisdr.c` is a single compilation unit that `#include`s all other `.c` files inline
- Uses USB bulk transfer mode (ISOC fails on macOS)
- Supported bandwidths: 200 kHz, 300 kHz, 600 kHz, 1.536 MHz, 5 MHz, 6 MHz, 7 MHz, 8 MHz

## Project Conventions

- **No external font/text libraries** — all rendering uses built-in stroke font
- **Error resilience** — the app must survive transient USB errors without crashing; the main loop continues running even when `device_error` is set
- **Static linking for mirisdr** — the driver is built as a static library, not shared
- **GPL-2.0 license** — required due to integrated libmirisdr code
- **macOS-first** — code targets macOS; Linux/Windows support is not a current goal
