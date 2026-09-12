# Audio Reactive Particles for OBS Studio

This version uses OBS raw audio capture callbacks for the selected audio source instead of the OBS volume-meter wrapper. Particle images are rendered as OBS texture sprites.

## Test

Enable **Test particles (ignore audio)**. Particles should appear without an audio source.

## Audio

Select an OBS audio source such as **Desktop Audio**. The raw audio callback calculates RMS and peak directly from the source samples. Louder audio increases emission and particle size.

## Build

The GitHub Actions workflow bootstraps the official OBS plugin-template CMake infrastructure and builds a Windows x64 artifact.
