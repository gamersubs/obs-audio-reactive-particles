# OBS Audio Reactive Particles

A native OBS Studio source plugin that creates a transparent particle field driven by an OBS audio source.

## What it does

- Selects any active OBS source that provides audio.
- Uses OBS's volume-meter API for audio reactivity.
- Smooths the audio signal so particles do not jitter.
- Increases particle spawn, size, and alpha response on louder audio.
- Supports up to 5,000 particles.
- Adjustable speed, gravity, lifetime, emission, spread, wind, colors, and canvas size.
- Transparent output: put it above gameplay, webcam, artwork, or a music visualizer background.

## Adding it to OBS

After installing/building the plugin, restart OBS. Add a new **Audio Reactive Particles** source. Select an audio source such as Desktop Audio, Mic/Aux, or another source that carries audio.

## Building on Windows

The current OBS plugin template uses CMake and Visual Studio 17 2022. This project expects an OBS development environment where `find_package(libobs REQUIRED)` resolves to the installed OBS SDK/libobs package.

Typical build commands from a developer command prompt are:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake --install build --config Release
```

For a standard OBS-plugin development environment, the official OBS plugin template is the recommended starting point.

## Performance notes

The default is 900 particles. Start around 500–1500 particles for normal streaming workloads. 5,000 particles are supported, but GPU/CPU cost increases with the particle count and scene resolution.

## Current implementation notes

This first release uses a CPU-simulated particle state and OBS's graphics immediate-mode vertex path. The audio analysis is intentionally broadband (volume/peak based) rather than FFT-based. That makes the plugin small and dependable; a future version can add bass/mid/treble bands, beat detection, trails, sprites/textures, presets, and a richer properties UI.
