# WavKit

_A tool for converting mouse movements and keystrokes to audio and back using FM modulation._

It is rather proof of concept than useful tool.

## Features

* **X11 HID Capture:** Captures the HID user actions using the X11 library.  Currently only selects and captures the primary screen for user actions.  Multi-monitor support could be added.
* **FM Modulation:** Encodes mouse X and Y coordinates as two separate FM signals.  The carrier frequency and modulation index are configurable. Other carrier is used for keystrokes. (mouseclick to be added)
* **PCM WAV Encoding:** Encodes the modulated audio signal as a PCM WAV file.  Sample rate and bit depth are configurable.
* **Listen Mode:**  Plays back the recorded audio file and repeats original instructions.
* **Mono WAV output** All the frequencies are stored in MONO sound... of course you can already think of STEREO giving more options.

## Dependencies

* **X11 Library:**  Required for screen capture.  Usually installed by default on most Linux systems.
* **libsndfile:**  Used for WAV file I/O.  Install using your system's package manager (e.g., `sudo apt-get install libsndfile1-dev` on Debian/Ubuntu).
* **(Optional) A suitable audio playback library:**  Required for the listen mode.  Examples include PortAudio or SDL.  If not included, the listen mode will be disabled.

## Installation

1.  **Install Dependencies:** Install the necessary libraries using your system's package manager.
2.  **Clone the Repository:**
    ```bash
    git clone https://github.com/K0F/wavkit
    ```
3.  **Compile:** Compile the code using a C compiler (like GCC) and the appropriate flags for your libraries.  A Makefile is provided for convenience.  (See below)

## Compilation

No autotools there, mostly ALSA and X11 dev packages, sorry WIP.

```bash
make
```

## Description

WavKit records mouse movements and keystrokes as FM-modulated audio signals and can play them back, effectively recreating the original input. It uses three carrier frequencies:
- 440 Hz (A4) for X position
- 660 Hz (E5) for Y position
- 880 Hz (A5) for keystrokes

## Requirements

- X11 development libraries
- GCC compiler
- Make

* **Debian/Ubuntu:** `sudo apt update && sudo apt install libx11-dev libsndfile1-dev libasound2-dev libxtst-dev`
* **Fedora/RHEL:** `sudo dnf install xorg-x11-devel sndfile-devel alsa-lib-devel libXtst-devel`
* **Arch Linux:** `sudo pacman -S xorg-server libsndfile alsa-lib libxtst`

## Usage

The program is controlled via command-line arguments:

* To run the Recorder:
```
        ./wavkit record <filename>: Records mouse movements and keystrokes to a WAV file.
        ./wavkit play <filename>: Plays back a WAV file, moving the mouse cursor and simulating key presses according to the recorded data.
        ./wavkit listen: Enters listen mode, processing audio input to control the mouse cursor and simulate key presses.
```
* To stop Recording/Playback/Listening: Press Ctrl+C to stop.

## Configuration

    Sample Rate: Currently fixed at 44100 Hz (configurable in main.c).
    Carrier Frequencies: Defined in main.c (FREQ_X, FREQ_Y, FREQ_KEYS). These determine the frequencies used for encoding X, Y coordinates, and key presses respectively.

## Contributing

Contributions are welcome!

## License

[GPL 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text)
