# WavKit

_A tool for converting mouse movements and keystrokes to audio and back using FM modulation._

## Features

* **X11 Screen Capture:** Captures the screen using the X11 library.  Currently only captures the primary screen.  Multi-monitor support could be added.
* **FM Modulation:** Encodes mouse X and Y coordinates as an FM signal.  The carrier frequency and modulation index are configurable.
* **PCM WAV Encoding:** Encodes the modulated audio signal as a PCM WAV file.  Sample rate and bit depth are configurable.
* **Listen Mode:**  Plays back the recorded audio file and repeats original instructions.

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

On Arch Linux, install dependencies with:
```bash
sudo pacman -S libx11 libxtst base-devel
```

On Debian like systems:

```
sudo apt install libx11-dev libsndfile1-dev libportaudio2-dev # Or libsdl2-dev instead of libportaudio2-dev
```
