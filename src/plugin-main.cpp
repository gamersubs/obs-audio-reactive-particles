#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("audio-reactive-particles", "en-US")

extern struct obs_source_info audio_reactive_particles_source;

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Audio-reactive particle system source for OBS Studio.";
}

bool obs_module_load(void)
{
    obs_register_source(&audio_reactive_particles_source);
    blog(LOG_INFO, "[audio-reactive-particles] loaded");
    return true;
}
