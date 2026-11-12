# Audio Reactive Particles for OBS Studio 33.0.1 (Windows x64)

This is the v10 image-sprite particle source updated to build against OBS Studio 33.0.1. The image loader uses OBS 33's replacement `gs_image_file_ex` API.

## Features

- Uses OBS raw audio capture callbacks for the selected audio source.
- Calculates RMS and peak directly from captured source samples.
- Renders PNG/JPG/BMP/GIF particle sprites using OBS graphics textures.
- Uses OBS 33 `gs_image_file_ex_*` image-helper functions and supports animated GIF frames.
- Test particles mode remains available to validate rendering without audio.
- Audio level affects particle emission and particle size.

## OBS 33 changes

- Build pins OBS Studio source version 33.0.1.
- Uses the official OBS dependency packages dated 2026-08-26 (Windows x64).
- Packages the module in the OBS 33+ plugin folder layout:
  `obs-audio-reactive-particles/obs-audio-reactive-particles.dll`
- Packages plugin assets under the adjacent `data` folder.

## Build

Run **Actions → Build Windows plugin (OBS 33.0.1) → Run workflow**.
The workflow keeps the requested `ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true` environment variable.

## Install

Extract the generated artifact and copy the `obs-audio-reactive-particles` folder into:

`C:\ProgramData\obs-studio\plugins\`

See `INSTALL-WINDOWS.txt` for the exact final file paths.
