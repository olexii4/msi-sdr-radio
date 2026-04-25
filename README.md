# MSi-SDR Radio

A lightweight, native SDR receiver application for **MSi.SDR** (MSI2500 + MSI001) and **SDRPlay RSP1/RSP1A clone** devices on macOS and Linux.

MSi.SDR is a budget-friendly software-defined radio receiver based on the MSI2500 USB bridge and MSI001 tuner chipsets. It offers a wide frequency range (100 kHz - 1.9 GHz) with a 12-bit ADC, making it a popular affordable alternative to higher-end SDRs.

## Screenshot

![Radio](docs/radio.svg)

## Features

- **Real-time spectrum & waterfall display** - 2048-point FFT with Hann window, DC-centered frequency axis, color-mapped waterfall
- **FM broadcast demodulation** - polar discriminator at full sample rate with 75 us de-emphasis and boxcar low-pass filtering
- **Band presets** - one-click switching between LF1, LF2, MW, HF, and FM bands with automatic bandwidth and frequency step adjustment
- **Device selector** - startup screen for choosing between multiple connected SDR devices
- **Interactive controls** - clickable gain/volume sliders, click-to-tune on spectrum, mouse wheel tuning
- **USB error resilience** - automatic retry on transient USB errors, graceful disconnect handling

## Architecture

| Module | Description |
|--------|-------------|
| `device.c` | SDR device management, IQ ring buffer, async streaming thread |
| `dsp.c` | FFT spectrum analysis, FM demodulation with decimation |
| `audio.c` | SDL2 audio output with ring buffer |
| `ui.c` | OpenGL 2.1 stroke-font UI, spectrum/waterfall rendering, controls |
| `lib/mirisdr/` | Integrated libmirisdr driver with ISOC-to-BULK fallback and macOS fixes |

## Requirements

- macOS (Apple Silicon or Intel) or Linux
- SDL2, FFTW3, libusb 1.0
- CMake 3.10+, C compiler

## Build

### macOS

```bash
brew install sdl2 fftw libusb
mkdir build && cd build
cmake .. -DDETACH_KERNEL_DRIVER=ON
make
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake libsdl2-dev libfftw3-dev libusb-1.0-0-dev
mkdir build && cd build
cmake .. -DDETACH_KERNEL_DRIVER=ON
make
```

### Linux (Fedora)

```bash
sudo dnf install gcc cmake SDL2-devel fftw-devel libusb1-devel
mkdir build && cd build
cmake .. -DDETACH_KERNEL_DRIVER=ON
make
```

## Usage

```bash
./msi-sdr                       # start at 100 MHz FM
./msi-sdr 101.0                  # start at 101.0 MHz
./msi-sdr 101.0 65               # start at 101.0 MHz, 65 dB gain
./msi-sdr -L debug.log 101.0 65  # same, with debug logging to file
```

### Controls

| Key | Action |
|-----|--------|
| Up/Down | Fine tune (0.5% of bandwidth) |
| Left/Right | Coarse tune (5% of bandwidth) |
| Mouse wheel | Fine tune |
| Click on spectrum | Tune to frequency |
| PgUp/PgDn | Gain +/- 5 dB |
| +/- | Volume +/- 10% |
| 1-5 | Band presets (LF1/LF2/MW/HF/FM) |
| D | Toggle FM demod |
| M | Mute/unmute |
| Q/Esc | Quit |

### Band Presets

| # | Band | Range | Center Freq | Bandwidth |
|---|------|-------|-------------|-----------|
| 1 | LF1 | 30-100 kHz | 65 kHz | 200 kHz |
| 2 | LF2 | 100-300 kHz | 200 kHz | 300 kHz |
| 3 | MW | 300 kHz-3 MHz | 1.65 MHz | 1.536 MHz |
| 4 | HF | 3-30 MHz | 16.5 MHz | 5 MHz |
| 5 | FM | 88-108 MHz | 100 MHz | 8 MHz |

## License

GPL-2.0 — includes integrated [libmirisdr](https://github.com/ericek111/libmirisdr-5) driver code (GPL-2.0).
