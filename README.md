# OBS Audio Reactive Particles

Native OBS Studio source plugin that renders an image-based particle system and reacts to an OBS audio source.

## Particle image

Open **Audio Reactive Particles → Properties → Particle Image** and select a small PNG/JPG/JPEG/BMP/GIF to use as the particle sprite. If the field is left empty, the plugin uses the included `particle.png` automatically.

The particle image is rendered with OBS's normal image texture path, making the particle itself a real image with its own transparency.

## Test mode

Enable **Test particles (ignore audio)** to force a full-strength particle system without requiring any audio input. This is useful for checking the renderer and your selected particle image.

## Windows build

The GitHub Actions workflow bootstraps OBS 32.2.1 and its Windows dependencies, then produces a standard OBS plugin install archive.
