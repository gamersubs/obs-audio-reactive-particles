#include <obs-module.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <util/platform.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#define S_AUDIO_SOURCE "audio_source"
#define S_PARTICLES "particles"
#define S_MAX_SIZE "max_size"
#define S_SPEED "speed"
#define S_GRAVITY "gravity"
#define S_AUDIO_GAIN "audio_gain"
#define S_AUDIO_SMOOTH "audio_smooth"
#define S_AUDIO_BOOST "audio_boost"
#define S_LIFETIME "lifetime"
#define S_EMIT_RATE "emit_rate"
#define S_SPREAD "spread"
#define S_WIND "wind"
#define S_GLOW "glow"
#define S_COLOR1 "color1"
#define S_COLOR2 "color2"
#define S_WIDTH "width"
#define S_HEIGHT "height"
#define S_TEST_MODE "test_mode"

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float life = 0.0f;
    float max_life = 1.0f;
    float size = 4.0f;
    float rot = 0.0f;
    float spin = 0.0f;
    float seed = 0.0f;
};

struct ParticleSource {
    obs_source_t *source = nullptr;
    obs_volmeter_t *meter = nullptr;
    obs_source_t *audio_source = nullptr;
    std::atomic<float> audio_level{0.0f};
    float reactive = 0.0f;
    float time = 0.0f;

    int particle_count = 900;
    float max_size = 14.0f;
    float speed = 120.0f;
    float gravity = -8.0f;
    float audio_gain = 1.5f;
    float audio_smooth = 7.0f;
    float audio_boost = 3.0f;
    float lifetime = 5.0f;
    float emit_rate = 180.0f;
    float spread = 1.0f;
    float wind = 0.0f;
    float glow = 1.0f;
    uint32_t color1 = 0xFF7A00FF;
    uint32_t color2 = 0xFF00D9FF;
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool test_mode = false;

    gs_effect_t *effect = nullptr;

    std::vector<Particle> particles;
    std::mt19937 rng{0xA11CE123u};
    float spawn_accumulator = 0.0f;
    size_t spawn_cursor = 0;
};

static inline float clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

static inline float frand(std::mt19937 &rng, float lo, float hi)
{
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}

static void audio_levels(void *param, const float magnitude[MAX_AUDIO_CHANNELS],
                         const float peak[MAX_AUDIO_CHANNELS],
                         const float input_peak[MAX_AUDIO_CHANNELS])
{
    UNUSED_PARAMETER(peak);
    UNUSED_PARAMETER(input_peak);

    auto *s = static_cast<ParticleSource *>(param);
    float db = -60.0f;
    const int channels = s->meter ? obs_volmeter_get_nr_channels(s->meter) : 0;
    for (int i = 0; i < channels && i < MAX_AUDIO_CHANNELS; ++i)
        db = std::max(db, magnitude[i]);

    float level = clamp01(obs_db_to_mul(std::max(-60.0f, std::min(0.0f, db))));
    level = clamp01(level * s->audio_gain);
    s->audio_level.store(level, std::memory_order_relaxed);
}

static bool add_audio_source(void *data, obs_source_t *src)
{
    auto *list = static_cast<obs_property_t *>(data);
    if (!(obs_source_get_output_flags(src) & OBS_SOURCE_AUDIO))
        return true;
    const char *name = obs_source_get_name(src);
    if (name && *name)
        obs_property_list_add_string(list, name, obs_source_get_uuid(src));
    return true;
}

static void attach_audio_source(ParticleSource *s, const char *uuid)
{
    if (!s)
        return;

    if (s->meter) {
        obs_volmeter_remove_callback(s->meter, audio_levels, s);
        obs_volmeter_detach_source(s->meter);
    }
    if (s->audio_source) {
        obs_source_release(s->audio_source);
        s->audio_source = nullptr;
    }

    if (!uuid || !*uuid) {
        s->audio_level.store(0.0f, std::memory_order_relaxed);
        return;
    }

    obs_source_t *src = obs_get_source_by_uuid(uuid);
    if (!src || !(obs_source_get_output_flags(src) & OBS_SOURCE_AUDIO)) {
        if (src)
            obs_source_release(src);
        s->audio_level.store(0.0f, std::memory_order_relaxed);
        return;
    }

    if (!s->meter)
        s->meter = obs_volmeter_create(OBS_FADER_LOG);

    if (s->meter) {
        obs_volmeter_set_peak_meter_type(s->meter, SAMPLE_PEAK_METER);
        obs_volmeter_add_callback(s->meter, audio_levels, s);
        if (!obs_volmeter_attach_source(s->meter, src)) {
            obs_volmeter_remove_callback(s->meter, audio_levels, s);
            obs_volmeter_detach_source(s->meter);
            obs_source_release(src);
            return;
        }
        s->audio_source = src;
    } else {
        obs_source_release(src);
    }
}

static void respawn_particle(ParticleSource *s, Particle &p, bool initial)
{
    p.x = frand(s->rng, 0.0f, static_cast<float>(s->width));
    p.y = initial ? frand(s->rng, 0.0f, static_cast<float>(s->height)) : static_cast<float>(s->height) + frand(s->rng, 1.0f, 30.0f);

    const float center_bias = frand(s->rng, -1.0f, 1.0f) * s->spread;
    p.vx = center_bias * s->speed * 0.55f + s->wind;
    p.vy = -frand(s->rng, s->speed * 0.55f, s->speed * 1.15f);
    p.max_life = frand(s->rng, s->lifetime * 0.55f, s->lifetime * 1.35f);
    p.life = initial ? frand(s->rng, 0.0f, p.max_life) : 0.0f;
    p.size = frand(s->rng, 1.0f, s->max_size);
    p.rot = frand(s->rng, 0.0f, 6.2831853f);
    p.spin = frand(s->rng, -1.8f, 1.8f);
    p.seed = frand(s->rng, 0.0f, 1000.0f);
}

static void resize_particles(ParticleSource *s)
{
    s->particles.resize(static_cast<size_t>(s->particle_count));
    for (auto &p : s->particles)
        respawn_particle(s, p, true);
}

static inline void unpack_color(uint32_t argb, float out[4])
{
    out[0] = ((argb >> 16) & 0xFF) / 255.0f;
    out[1] = ((argb >> 8) & 0xFF) / 255.0f;
    out[2] = (argb & 0xFF) / 255.0f;
    out[3] = ((argb >> 24) & 0xFF) / 255.0f;
}

static inline uint32_t pack_argb(const float color[4])
{
    const uint32_t a = static_cast<uint32_t>(clamp01(color[3]) * 255.0f + 0.5f);
    const uint32_t r = static_cast<uint32_t>(clamp01(color[0]) * 255.0f + 0.5f);
    const uint32_t g = static_cast<uint32_t>(clamp01(color[1]) * 255.0f + 0.5f);
    const uint32_t b = static_cast<uint32_t>(clamp01(color[2]) * 255.0f + 0.5f);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static void source_update(void *data, obs_data_t *settings);

static const char *source_name(void *)
{
    return obs_module_text("AudioReactiveParticles");
}

static void *source_create(obs_data_t *settings, obs_source_t *source)
{
    auto *s = new ParticleSource();
    s->source = source;
    s->meter = obs_volmeter_create(OBS_FADER_LOG);

    source_update(s, settings);
    return s;
}

static void source_destroy(void *data)
{
    auto *s = static_cast<ParticleSource *>(data);
    if (!s)
        return;

    if (s->meter) {
        obs_volmeter_remove_callback(s->meter, audio_levels, s);
        obs_volmeter_detach_source(s->meter);
        obs_volmeter_destroy(s->meter);
        s->meter = nullptr;
    }

    if (s->audio_source) {
        obs_source_release(s->audio_source);
        s->audio_source = nullptr;
    }

    if (s->effect) {
        obs_enter_graphics();
        gs_effect_destroy(s->effect);
        obs_leave_graphics();
        s->effect = nullptr;
    }

    delete s;
}

static void source_update(void *data, obs_data_t *settings)
{
    auto *s = static_cast<ParticleSource *>(data);
    if (!s)
        return;

    const int old_particles = s->particle_count;
    const uint32_t old_width = s->width;
    const uint32_t old_height = s->height;

    s->particle_count = static_cast<int>(obs_data_get_int(settings, S_PARTICLES));
    s->max_size = static_cast<float>(obs_data_get_double(settings, S_MAX_SIZE));
    s->speed = static_cast<float>(obs_data_get_double(settings, S_SPEED));
    s->gravity = static_cast<float>(obs_data_get_double(settings, S_GRAVITY));
    s->audio_gain = static_cast<float>(obs_data_get_double(settings, S_AUDIO_GAIN));
    s->audio_smooth = static_cast<float>(obs_data_get_double(settings, S_AUDIO_SMOOTH));
    s->audio_boost = static_cast<float>(obs_data_get_double(settings, S_AUDIO_BOOST));
    s->lifetime = static_cast<float>(obs_data_get_double(settings, S_LIFETIME));
    s->emit_rate = static_cast<float>(obs_data_get_double(settings, S_EMIT_RATE));
    s->spread = static_cast<float>(obs_data_get_double(settings, S_SPREAD));
    s->wind = static_cast<float>(obs_data_get_double(settings, S_WIND));
    s->glow = static_cast<float>(obs_data_get_double(settings, S_GLOW));
    s->color1 = static_cast<uint32_t>(obs_data_get_int(settings, S_COLOR1));
    s->color2 = static_cast<uint32_t>(obs_data_get_int(settings, S_COLOR2));
    s->width = static_cast<uint32_t>(obs_data_get_int(settings, S_WIDTH));
    s->height = static_cast<uint32_t>(obs_data_get_int(settings, S_HEIGHT));
    s->test_mode = obs_data_get_bool(settings, S_TEST_MODE);

    s->particle_count = std::max(50, std::min(5000, s->particle_count));
    s->max_size = std::max(1.0f, std::min(64.0f, s->max_size));
    s->speed = std::max(0.0f, std::min(1000.0f, s->speed));
    s->lifetime = std::max(0.25f, std::min(20.0f, s->lifetime));
    s->width = std::max(64u, std::min(7680u, s->width));
    s->height = std::max(64u, std::min(4320u, s->height));

    const char *audio_uuid = obs_data_get_string(settings, S_AUDIO_SOURCE);
    attach_audio_source(s, audio_uuid);

    if (old_particles != s->particle_count || old_width != s->width || old_height != s->height)
        resize_particles(s);
}

static void source_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, S_PARTICLES, 900);
    obs_data_set_default_double(settings, S_MAX_SIZE, 14.0);
    obs_data_set_default_double(settings, S_SPEED, 120.0);
    obs_data_set_default_double(settings, S_GRAVITY, -8.0);
    obs_data_set_default_double(settings, S_AUDIO_GAIN, 1.5);
    obs_data_set_default_double(settings, S_AUDIO_SMOOTH, 7.0);
    obs_data_set_default_double(settings, S_AUDIO_BOOST, 3.0);
    obs_data_set_default_double(settings, S_LIFETIME, 5.0);
    obs_data_set_default_double(settings, S_EMIT_RATE, 180.0);
    obs_data_set_default_double(settings, S_SPREAD, 1.0);
    obs_data_set_default_double(settings, S_WIND, 0.0);
    obs_data_set_default_double(settings, S_GLOW, 1.0);
    obs_data_set_default_int(settings, S_COLOR1, 0xFF7A00FF);
    obs_data_set_default_int(settings, S_COLOR2, 0xFF00D9FF);
    obs_data_set_default_int(settings, S_WIDTH, 1920);
    obs_data_set_default_int(settings, S_HEIGHT, 1080);
    obs_data_set_default_string(settings, S_AUDIO_SOURCE, "");
    obs_data_set_default_bool(settings, S_TEST_MODE, true);
}

static obs_properties_t *source_properties(void *)
{
    obs_properties_t *props = obs_properties_create();

    obs_property_t *audio = obs_properties_add_list(props, S_AUDIO_SOURCE,
        obs_module_text("AudioSource"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(audio, obs_module_text("None"), "");
    obs_enum_sources(add_audio_source, audio);

    obs_properties_add_int_slider(props, S_PARTICLES, obs_module_text("Particles"), 50, 5000, 50);
    obs_properties_add_float_slider(props, S_MAX_SIZE, obs_module_text("MaxSize"), 1.0, 64.0, 0.5);
    obs_properties_add_float_slider(props, S_SPEED, obs_module_text("Speed"), 0.0, 1000.0, 5.0);
    obs_properties_add_float_slider(props, S_GRAVITY, obs_module_text("Gravity"), -200.0, 200.0, 1.0);
    obs_properties_add_float_slider(props, S_LIFETIME, obs_module_text("Lifetime"), 0.25, 20.0, 0.25);
    obs_properties_add_float_slider(props, S_EMIT_RATE, obs_module_text("Emission"), 0.0, 1000.0, 5.0);
    obs_properties_add_float_slider(props, S_SPREAD, obs_module_text("Spread"), 0.0, 3.0, 0.05);
    obs_properties_add_float_slider(props, S_WIND, obs_module_text("Wind"), -500.0, 500.0, 5.0);

    obs_properties_t *audio_group = obs_properties_create();
    obs_properties_add_float_slider(audio_group, S_AUDIO_GAIN, obs_module_text("AudioGain"), 0.0, 5.0, 0.05);
    obs_properties_add_float_slider(audio_group, S_AUDIO_SMOOTH, obs_module_text("AudioSmoothing"), 0.0, 20.0, 0.25);
    obs_properties_add_float_slider(audio_group, S_AUDIO_BOOST, obs_module_text("AudioBoost"), 0.0, 8.0, 0.1);
    obs_properties_add_group(props, "audio_group", obs_module_text("AudioReactivity"), OBS_GROUP_NORMAL, audio_group);

    obs_properties_add_float_slider(props, S_GLOW, obs_module_text("Glow"), 0.0, 3.0, 0.05);
    obs_properties_add_color(props, S_COLOR1, obs_module_text("Color1"));
    obs_properties_add_color(props, S_COLOR2, obs_module_text("Color2"));
    obs_properties_add_int(props, S_WIDTH, obs_module_text("Width"), 64, 7680, 8);
    obs_properties_add_int(props, S_HEIGHT, obs_module_text("Height"), 64, 4320, 8);
    obs_properties_add_bool(props, S_TEST_MODE, obs_module_text("TestMode"));

    return props;
}

static uint32_t source_width(void *data)
{
    return static_cast<ParticleSource *>(data)->width;
}

static uint32_t source_height(void *data)
{
    return static_cast<ParticleSource *>(data)->height;
}

static void source_render(void *data, gs_effect_t *)
{
    auto *s = static_cast<ParticleSource *>(data);
    if (!s || s->particles.empty())
        return;

    // The custom effect is loaded lazily on the graphics thread.
    if (!s->effect) {
        char *effect_path = obs_module_file("particles.effect");
        char *error = nullptr;
        s->effect = gs_effect_create_from_file(effect_path, &error);
        if (!s->effect) {
            blog(LOG_ERROR, "[audio-reactive-particles] Failed to load effect: %s", error ? error : "unknown error");
        }
        if (error)
            bfree(error);
        bfree(effect_path);
        if (!s->effect)
            return;
    }

    static uint64_t last_ns = 0;
    const uint64_t now = os_gettime_ns();
    float dt = last_ns ? static_cast<float>(now - last_ns) / 1.0e9f : (1.0f / 60.0f);
    last_ns = now;
    dt = std::max(0.0001f, std::min(0.05f, dt));

    const float raw_audio = s->test_mode ? 1.0f : clamp01(s->audio_level.load(std::memory_order_relaxed));
    const float smoothing = std::max(0.0f, s->audio_smooth);
    const float blend = smoothing > 0.0f ? 1.0f - std::exp(-smoothing * dt) : 1.0f;
    s->reactive += (raw_audio - s->reactive) * blend;

    s->time += dt;
    const float audio_push = s->reactive * s->audio_boost;

    s->spawn_accumulator += s->emit_rate * (0.25f + s->reactive * 2.5f) * dt;
    const int to_spawn = std::min(300, static_cast<int>(s->spawn_accumulator));
    s->spawn_accumulator -= static_cast<float>(to_spawn);

    for (int i = 0; i < to_spawn; ++i) {
        if (s->particles.empty())
            break;
        s->spawn_cursor %= s->particles.size();
        respawn_particle(s, s->particles[s->spawn_cursor], false);
        s->particles[s->spawn_cursor].size *= (1.0f + 0.9f * s->reactive);
        ++s->spawn_cursor;
    }

    for (auto &p : s->particles) {
        p.life += dt;
        if (p.life > p.max_life) {
            respawn_particle(s, p, false);
            p.size *= (1.0f + 0.7f * s->reactive);
        }

        p.vy += s->gravity * dt;
        p.vx += std::sin(s->time * 0.7f + p.seed) * 3.0f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.rot += p.spin * dt;
        p.size = std::max(0.25f, std::min(96.0f, p.size));

        if (p.x < -100.0f) p.x = static_cast<float>(s->width) + 100.0f;
        if (p.x > static_cast<float>(s->width) + 100.0f) p.x = -100.0f;
    }

    float c1[4], c2[4];
    unpack_color(s->color1, c1);
    unpack_color(s->color2, c2);

    gs_set_2d_mode();
    matrix4 viewproj;
    gs_matrix_get(&viewproj);
    gs_eparam_t *viewproj_param = gs_effect_get_viewproj_matrix(s->effect);
    if (viewproj_param)
        gs_effect_set_matrix4(viewproj_param, &viewproj);
    gs_enable_blending(true);
    gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA,
                               GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

    gs_render_start(true);

    // Two triangles per particle. Color is computed per-vertex so each particle
    // can fade at birth/death without requiring a second texture.
    for (const auto &p : s->particles) {
        const float pulse = 1.0f + 0.35f * std::sin(s->time * 5.0f + p.seed) + audio_push;
        const float life_t = clamp01(p.life / std::max(0.001f, p.max_life));
        float alpha = s->test_mode ? 1.0f : std::sin(life_t * 3.14159265f);
        alpha *= s->test_mode ? 1.0f : (0.25f + 0.75f * clamp01(0.25f + s->reactive));
        const float color_t = clamp01(0.2f + life_t * 0.8f);
        float col[4];
        for (int k = 0; k < 4; ++k)
            col[k] = c1[k] * (1.0f - color_t) + c2[k] * color_t;
        col[3] = std::max(0.0f, std::min(1.0f, alpha * col[3]));

        const float r = p.size * pulse * (1.0f + 0.8f * s->reactive) * (0.85f + 0.15f * s->glow);
        const float x = p.x;
        const float y = p.y;

        // Diamond-shaped particles keep this source visually distinct from a basic square emitter.
        const uint32_t packed_color = pack_argb(col);
        gs_color(packed_color);
        gs_vertex2f(x, y - r);
        gs_color(packed_color);
        gs_vertex2f(x + r, y);
        gs_color(packed_color);
        gs_vertex2f(x, y + r);

        gs_color(packed_color);
        gs_vertex2f(x, y - r);
        gs_color(packed_color);
        gs_vertex2f(x, y + r);
        gs_color(packed_color);
        gs_vertex2f(x - r, y);
    }

    gs_render_stop(GS_TRIS);

    while (gs_effect_loop(s->effect, "Draw")) {
        // The immediate vertex buffer is already prepared by gs_render_stop().
        // The effect handles the position transform and per-vertex color.
        gs_draw(GS_TRIS, 0, static_cast<uint32_t>(s->particles.size() * 6));
    }

    UNUSED_PARAMETER(s->glow);
}

struct obs_source_info audio_reactive_particles_source = {
    .id = "audio_reactive_particles",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = source_name,
    .create = source_create,
    .destroy = source_destroy,
    .get_width = source_width,
    .get_height = source_height,
    .get_defaults = source_defaults,
    .get_properties = source_properties,
    .update = source_update,
    .video_render = source_render,
};
