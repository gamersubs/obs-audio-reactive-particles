# OBS Audio Reactive Particles

Native OBS Studio source plugin that renders an image-based particle system and reacts to an OBS audio source.

## Particle image

Open **Audio Reactive Particles → Properties → Particle Image** and select a small PNG/JPG/JPEG/BMP/GIF to use as the particle sprite. If the field is left empty, the plugin uses the included `particle.png` automatically.

The particle image is rendered with OBS's normal image texture path, making the particle itself a real image with its own transparency.

## Test mode

Enable **Test particles (ignore audio)** to force a full-strength particle system without requiring any audio input. This is useful for checking the renderer and your selected particle image.

## Windows build

The GitHub Actions workflow bootstraps OBS 32.2.1 and its Windows dependencies, then produces a standard OBS plugin install archive.


## Audio reaction

The selected OBS audio source is measured with `obs_volmeter`. Its magnitude and peak values are already normalized to 0..1 by OBS; the plugin uses that normalized level directly, then applies Audio Gain and smoothing. In normal mode, louder audio increases emission and particle size. Test Particles can be enabled to force a full-strength visual signal without audio.
