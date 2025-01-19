# WavKit

A tool for converting mouse movements and keystrokes to audio and back using FM modulation.

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
