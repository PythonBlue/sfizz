/*
  SPDX-License-Identifier: ISC

  Sfizz LV2 plugin

  Copyright 2019-2020, Paul Ferrand <paul@ferrand.cc>

  This file was based on skeleton and example code from the LV2 plugin
  distribution available at http://lv2plug.in/

  The LV2 sample plugins have the following copyright and notice, which are
  extended to the current work:
  Copyright 2011-2016 David Robillard <d@drobilla.net>
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
  Copyright 2011 James Morris <jwm.art.net@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  Compiling this plugin statically against libsndfile implies distributing it
  under the terms of the LGPL v3 license. See the LICENSE.md file for more
  information. If you did not receive a LICENSE.md file, inform the current
  maintainer.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "sfizz_lv2.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/patch/patch.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>
#include <lv2/log/logger.h>
#include <lv2/log/log.h>
#include <lv2/time/time.h>

#include <ardour/lv2_extensions.h>

#include <math.h>
#include <sfizz.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "atomic_compat.h"

#define CHANNEL_MASK 0x0F
#define MIDI_CHANNEL(byte) (byte & CHANNEL_MASK)
#define MIDI_STATUS(byte) (byte & ~CHANNEL_MASK)
#define PITCH_BUILD_AND_CENTER(first_byte, last_byte) (int)(((unsigned int)last_byte << 7) + (unsigned int)first_byte) - 8192
#define MAX_BLOCK_SIZE 8192
#define MAX_VOICES 256
#define DEFAULT_VOICES 64
#define DEFAULT_OVERSAMPLING SFIZZ_OVERSAMPLING_X1
#define DEFAULT_PRELOAD 8192
#define LOG_SAMPLE_COUNT 48000
#define UNUSED(x) (void)(x)

#define DEFAULT_SCALA_FILE  "Contents/Resources/DefaultScale.scl"
#define DEFAULT_SFZ_FILE    "Contents/Resources/DefaultInstrument.sfz"
// This assumes that the longest path is the default sfz file; if not, change it
#define MAX_BUNDLE_PATH_SIZE (MAX_PATH_SIZE - sizeof(DEFAULT_SFZ_FILE))

#ifndef NDEBUG
#define LV2_DEBUG(...) lv2_log_note(&self->logger, "[DEBUG] " __VA_ARGS__)
#else
#define LV2_DEBUG(...)
#endif

typedef struct
{
    // Features
    LV2_URID_Map *map;
    LV2_URID_Unmap *unmap;
    LV2_Worker_Schedule *worker;
    LV2_Log_Log *log;
    LV2_Midnam *midnam;

    // Ports
    const LV2_Atom_Sequence *control_port;
    LV2_Atom_Sequence *notify_port;
    float *output_buffers[2];
    const float *volume_port;
    const float *polyphony_port;
    const float *oversampling_port;
    const float *preload_port;
    const float *freewheel_port;
    const float *scala_root_key_port;
    const float *tuning_frequency_port;
    const float *stretch_tuning_port;
    float *active_voices_port;
    float *num_curves_port;
    float *num_masters_port;
    float *num_groups_port;
    float *num_regions_port;
    float *num_samples_port;

    // Atom forge
    LV2_Atom_Forge forge;              ///< Forge for writing atoms in run thread
    LV2_Atom_Forge forge_secondary;    ///< Forge for writing into other buffers

    // Logger
    LV2_Log_Logger logger;

    // URIs
    LV2_URID midi_event_uri;
    LV2_URID options_interface_uri;
    LV2_URID max_block_length_uri;
    LV2_URID nominal_block_length_uri;
    LV2_URID sample_rate_uri;
    LV2_URID atom_object_uri;
    LV2_URID atom_blank_uri;
    LV2_URID atom_float_uri;
    LV2_URID atom_double_uri;
    LV2_URID atom_int_uri;
    LV2_URID atom_long_uri;
    LV2_URID atom_urid_uri;
    LV2_URID atom_path_uri;
    LV2_URID patch_set_uri;
    LV2_URID patch_get_uri;
    LV2_URID patch_put_uri;
    LV2_URID patch_property_uri;
    LV2_URID patch_value_uri;
    LV2_URID patch_body_uri;
    LV2_URID state_changed_uri;
    LV2_URID sfizz_sfz_file_uri;
    LV2_URID sfizz_scala_file_uri;
    LV2_URID sfizz_num_voices_uri;
    LV2_URID sfizz_preload_size_uri;
    LV2_URID sfizz_oversampling_uri;
    LV2_URID sfizz_log_status_uri;
    LV2_URID sfizz_check_modification_uri;
    LV2_URID sfizz_active_voices_uri;
    LV2_URID sfizz_osc_blob_uri;
    LV2_URID time_position_uri;
    LV2_URID time_bar_uri;
    LV2_URID time_bar_beat_uri;
    LV2_URID time_beat_unit_uri;
    LV2_URID time_beats_per_bar_uri;
    LV2_URID time_beats_per_minute_uri;
    LV2_URID time_speed_uri;

    // Sfizz related data
    sfizz_synth_t *synth;
    sfizz_client_t *client;
    bool expect_nominal_block_length;
    char sfz_file_path[MAX_PATH_SIZE];
    char scala_file_path[MAX_PATH_SIZE];
    int num_voices;
    unsigned int preload_size;
    sfizz_oversampling_factor_t oversampling;
    float stretch_tuning;
    volatile bool check_modification;
    int max_block_size;
    int sample_counter;
    float sample_rate;
    atomic_int must_update_midnam;

    // Timing data
    int bar;
    double bar_beat;
    int beats_per_bar;
    int beat_unit;
    double bpm_tempo;
    double speed;

    // Paths
    char bundle_path[MAX_BUNDLE_PATH_SIZE];

    // OSC
    uint8_t osc_temp[OSC_TEMP_SIZE];
} sfizz_plugin_t;

enum
{
    SFIZZ_TIMEINFO_POSITION = 1 << 0,
    SFIZZ_TIMEINFO_SIGNATURE = 1 << 1,
    SFIZZ_TIMEINFO_TEMPO = 1 << 2,
    SFIZZ_TIMEINFO_SPEED = 1 << 3,
};

static void
sfizz_lv2_state_free_path(LV2_State_Free_Path_Handle handle,
                          char *path)
{
    (void)handle;
    free(path);
}

static LV2_State_Free_Path sfizz_State_Free_Path =
{
    .handle = NULL,
    .free_path = &sfizz_lv2_state_free_path,
};

static void
sfizz_lv2_map_required_uris(sfizz_plugin_t *self)
{
    LV2_URID_Map *map = self->map;
    self->midi_event_uri = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->max_block_length_uri = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
    self->nominal_block_length_uri = map->map(map->handle, LV2_BUF_SIZE__nominalBlockLength);
    self->sample_rate_uri = map->map(map->handle, LV2_PARAMETERS__sampleRate);
    self->atom_float_uri = map->map(map->handle, LV2_ATOM__Float);
    self->atom_double_uri = map->map(map->handle, LV2_ATOM__Double);
    self->atom_int_uri = map->map(map->handle, LV2_ATOM__Int);
    self->atom_long_uri = map->map(map->handle, LV2_ATOM__Long);
    self->atom_path_uri = map->map(map->handle, LV2_ATOM__Path);
    self->atom_urid_uri = map->map(map->handle, LV2_ATOM__URID);
    self->atom_object_uri = map->map(map->handle, LV2_ATOM__Object);
    self->atom_blank_uri = map->map(map->handle, LV2_ATOM__Blank);
    self->patch_set_uri = map->map(map->handle, LV2_PATCH__Set);
    self->patch_get_uri = map->map(map->handle, LV2_PATCH__Get);
    self->patch_put_uri = map->map(map->handle, LV2_PATCH__Put);
    self->patch_body_uri = map->map(map->handle, LV2_PATCH__body);
    self->patch_property_uri = map->map(map->handle, LV2_PATCH__property);
    self->patch_value_uri = map->map(map->handle, LV2_PATCH__value);
    self->state_changed_uri = map->map(map->handle, LV2_STATE__StateChanged);
    self->sfizz_sfz_file_uri = map->map(map->handle, SFIZZ__sfzFile);
    self->sfizz_scala_file_uri = map->map(map->handle, SFIZZ__tuningfile);
    self->sfizz_num_voices_uri = map->map(map->handle, SFIZZ__numVoices);
    self->sfizz_preload_size_uri = map->map(map->handle, SFIZZ__preloadSize);
    self->sfizz_oversampling_uri = map->map(map->handle, SFIZZ__oversampling);
    self->sfizz_log_status_uri = map->map(map->handle, SFIZZ__logStatus);
    self->sfizz_log_status_uri = map->map(map->handle, SFIZZ__logStatus);
    self->sfizz_check_modification_uri = map->map(map->handle, SFIZZ__checkModification);
    self->sfizz_osc_blob_uri = map->map(map->handle, SFIZZ__OSCBlob);
    self->time_position_uri = map->map(map->handle, LV2_TIME__Position);
    self->time_bar_uri = map->map(map->handle, LV2_TIME__bar);
    self->time_bar_beat_uri = map->map(map->handle, LV2_TIME__barBeat);
    self->time_beat_unit_uri = map->map(map->handle, LV2_TIME__beatUnit);
    self->time_beats_per_bar_uri = map->map(map->handle, LV2_TIME__beatsPerBar);
    self->time_beats_per_minute_uri = map->map(map->handle, LV2_TIME__beatsPerMinute);
    self->time_speed_uri = map->map(map->handle, LV2_TIME__speed);
}

static bool
sfizz_atom_extract_real(sfizz_plugin_t *self, const LV2_Atom *atom, double *real)
{
    if (!atom)
        return false;

    const LV2_URID type = atom->type;

    if (type == self->atom_int_uri && atom->size >= sizeof(int32_t)) {
        *real = ((const LV2_Atom_Int *)atom)->body;
        return true;
    }
    if (type == self->atom_long_uri && atom->size >= sizeof(int64_t)) {
        *real = ((const LV2_Atom_Long *)atom)->body;
        return true;
    }
    if (type == self->atom_float_uri && atom->size >= sizeof(float)) {
        *real = ((const LV2_Atom_Float *)atom)->body;
        return true;
    }
    if (type == self->atom_double_uri && atom->size >= sizeof(double)) {
        *real = ((const LV2_Atom_Double *)atom)->body;
        return true;
    }

    return false;
}

static bool
sfizz_atom_extract_integer(sfizz_plugin_t *self, const LV2_Atom *atom, int64_t *integer)
{
    if (!atom)
        return false;

    const LV2_URID type = atom->type;

    if (type == self->atom_int_uri && atom->size >= sizeof(int32_t)) {
        *integer = ((const LV2_Atom_Int *)atom)->body;
        return true;
    }
    if (type == self->atom_long_uri && atom->size >= sizeof(int64_t)) {
        *integer = ((const LV2_Atom_Long *)atom)->body;
        return true;
    }
    if (type == self->atom_float_uri && atom->size >= sizeof(float)) {
        *integer = (int64_t)((const LV2_Atom_Float *)atom)->body;
        return true;
    }
    if (type == self->atom_double_uri && atom->size >= sizeof(double)) {
        *integer = (int64_t)((const LV2_Atom_Double *)atom)->body;
        return true;
    }

    return false;
}

static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void *data)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    switch (port)
    {
    case SFIZZ_CONTROL:
        self->control_port = (const LV2_Atom_Sequence *)data;
        break;
    case SFIZZ_NOTIFY:
        self->notify_port = (LV2_Atom_Sequence *)data;
        break;
    case SFIZZ_LEFT:
        self->output_buffers[0] = (float *)data;
        break;
    case SFIZZ_RIGHT:
        self->output_buffers[1] = (float *)data;
        break;
    case SFIZZ_VOLUME:
        self->volume_port = (const float *)data;
        break;
    case SFIZZ_POLYPHONY:
        self->polyphony_port = (const float *)data;
        break;
    case SFIZZ_OVERSAMPLING:
        self->oversampling_port = (const float *)data;
        break;
    case SFIZZ_PRELOAD:
        self->preload_port = (const float *)data;
        break;
    case SFIZZ_FREEWHEELING:
        self->freewheel_port = (const float *)data;
        break;
    case SFIZZ_SCALA_ROOT_KEY:
        self->scala_root_key_port = (const float *)data;
        break;
    case SFIZZ_TUNING_FREQUENCY:
        self->tuning_frequency_port = (const float *)data;
        break;
    case SFIZZ_STRETCH_TUNING:
        self->stretch_tuning_port = (const float *)data;
        break;
    case SFIZZ_ACTIVE_VOICES:
        self->active_voices_port = (float *)data;
        break;
    case SFIZZ_NUM_CURVES:
        self->num_curves_port = (float *)data;
        break;
    case SFIZZ_NUM_MASTERS:
        self->num_masters_port = (float *)data;
        break;
    case SFIZZ_NUM_GROUPS:
        self->num_groups_port = (float *)data;
        break;
    case SFIZZ_NUM_REGIONS:
        self->num_regions_port = (float *)data;
        break;
    case SFIZZ_NUM_SAMPLES:
        self->num_samples_port = (float *)data;
        break;
    default:
        break;
    }
}

// This function sets the sample rate in the self parameter but does not update it.
static void
sfizz_lv2_parse_sample_rate(sfizz_plugin_t* self, const LV2_Options_Option* opt)
{
    if (opt->type == self->atom_float_uri)
    {
        // self->sample_rate = *(float*)opt->value;
        LV2_DEBUG("Attempted to change the sample rate to %.2f (original was %.2f); ignored",
                  *(float*)opt->value,
                  self->sample_rate);
    }
    else if (opt->type == self->atom_int_uri)
    {
        // self->sample_rate = *(int*)opt->value;
        LV2_DEBUG("Attempted to change the sample rate to %d (original was %.2f); ignored",
                  *(int*)opt->value,
                  self->sample_rate);
    }
    else
    {
        lv2_log_warning(&self->logger, "[sfizz] Got a sample rate but could not resolve the type of the atom\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "[sfizz] Atom URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, opt->type));
    }
}

static void
sfizz_lv2_get_default_sfz_path(LV2_Handle instance, char *path, size_t size)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    snprintf(path, size, "%s/%s", self->bundle_path, DEFAULT_SFZ_FILE);
}

static void
sfizz_lv2_get_default_scala_path(LV2_Handle instance, char *path, size_t size)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    snprintf(path, size, "%s/%s", self->bundle_path, DEFAULT_SCALA_FILE);
}

static void
sfizz_lv2_update_timeinfo(sfizz_plugin_t *self, int delay, int updates)
{
    if (updates & SFIZZ_TIMEINFO_POSITION)
        sfizz_send_time_position(self->synth, delay, self->bar, self->bar_beat);
    if (updates & SFIZZ_TIMEINFO_SIGNATURE)
        sfizz_send_time_signature(self->synth, delay, self->beats_per_bar, self->beat_unit);
    if (updates & SFIZZ_TIMEINFO_TEMPO)
        sfizz_send_tempo(self->synth, delay, 60.0f / self->bpm_tempo);
    if (updates & SFIZZ_TIMEINFO_SPEED)
        sfizz_send_playback_state(self->synth, delay, self->speed > 0);
}

static void
sfizz_lv2_receive_message(void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    (void)delay;

    sfizz_plugin_t *self = (sfizz_plugin_t *)data;

    // transmit to UI as OSC blob
    uint8_t *osc_temp = self->osc_temp;
    uint32_t osc_size = sfizz_prepare_message(osc_temp, OSC_TEMP_SIZE, path, sig, args);
    if (osc_size > OSC_TEMP_SIZE)
        return;

    bool write_ok =
        lv2_atom_forge_frame_time(&self->forge, 0) &&
        lv2_atom_forge_atom(&self->forge, osc_size, self->sfizz_osc_blob_uri) &&
        lv2_atom_forge_raw(&self->forge, osc_temp, osc_size);
    lv2_atom_forge_pad(&self->forge, osc_size);

    (void)write_ok;
}

static LV2_Handle
instantiate(const LV2_Descriptor *descriptor,
            double rate,
            const char *bundle_path,
            const LV2_Feature *const *features)
{
    UNUSED(descriptor);
    LV2_Options_Option *options = NULL;
    bool supports_bounded_block_size = false;
    bool options_has_block_size = false;
    bool supports_fixed_block_size = false;

    // Allocate and initialise instance structure.
    sfizz_plugin_t *self = (sfizz_plugin_t *)calloc(1, sizeof(sfizz_plugin_t));
    if (!self)
        return NULL;

    LV2_Handle instance = (LV2_Handle)self;

    strncpy(self->bundle_path, bundle_path, MAX_BUNDLE_PATH_SIZE);
    self->bundle_path[MAX_BUNDLE_PATH_SIZE - 1] = '\0';

    // Set defaults
    self->max_block_size = MAX_BLOCK_SIZE;
    self->sample_rate = (float)rate;
    self->expect_nominal_block_length = false;
    self->sfz_file_path[0] = '\0';
    self->scala_file_path[0] = '\0';
    self->num_voices = DEFAULT_VOICES;
    self->oversampling = DEFAULT_OVERSAMPLING;
    self->preload_size = DEFAULT_PRELOAD;
    self->stretch_tuning = 0.0f;
    self->check_modification = false;
    self->sample_counter = 0;

    // Initial timing
    self->bar = 0;
    self->bar_beat = 0;
    self->beats_per_bar = 4;
    self->beat_unit = 4;
    self->bpm_tempo = 120;
    self->speed = 1;

    // Get the features from the host and populate the structure
    for (const LV2_Feature *const *f = features; *f; f++)
    {
        // lv2_log_note(&self->logger, "Feature URI: %s\n", (**f).URI);

        if (!strcmp((**f).URI, LV2_URID__map))
            self->map = (**f).data;

        if (!strcmp((**f).URI, LV2_URID__unmap))
            self->unmap = (**f).data;

        if (!strcmp((**f).URI, LV2_BUF_SIZE__boundedBlockLength))
            supports_bounded_block_size = true;

        if (!strcmp((**f).URI, LV2_BUF_SIZE__fixedBlockLength))
            supports_fixed_block_size = true;

        if (!strcmp((**f).URI, LV2_OPTIONS__options))
            options = (**f).data;

        if (!strcmp((**f).URI, LV2_WORKER__schedule))
            self->worker = (**f).data;

        if (!strcmp((**f).URI, LV2_LOG__log))
            self->log = (**f).data;

        if (!strcmp((**f).URI, LV2_MIDNAM__update))
            self->midnam = (**f).data;
    }

    // Setup the loggers
    lv2_log_logger_init(&self->logger, self->map, self->log);

    // The map feature is required
    if (!self->map)
    {
        lv2_log_error(&self->logger, "Map feature not found, aborting..\n");
        free(self);
        return NULL;
    }

    // The worker feature is required
    if (!self->worker)
    {
        lv2_log_error(&self->logger, "Worker feature not found, aborting..\n");
        free(self);
        return NULL;
    }

    // Map the URIs we will need
    sfizz_lv2_map_required_uris(self);

    // Initialize the forge
    lv2_atom_forge_init(&self->forge, self->map);
    lv2_atom_forge_init(&self->forge_secondary, self->map);

    // Check the options for the block size and sample rate parameters
    if (options)
    {
        for (const LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
        {
            if (opt->key == self->sample_rate_uri)
            {
                sfizz_lv2_parse_sample_rate(self, opt);
            }
            else if (!self->expect_nominal_block_length && opt->key == self->max_block_length_uri)
            {
                if (opt->type != self->atom_int_uri)
                {
                    lv2_log_warning(&self->logger, "Got a max block size but the type was wrong\n");
                    continue;
                }
                self->max_block_size = *(int *)opt->value;
                options_has_block_size = true;
            }
            else if (opt->key == self->nominal_block_length_uri)
            {
                if (opt->type != self->atom_int_uri)
                {
                    lv2_log_warning(&self->logger, "Got a nominal block size but the type was wrong\n");
                    continue;
                }
                self->max_block_size = *(int *)opt->value;
                self->expect_nominal_block_length = true;
                options_has_block_size = true;
            }
        }
    }
    else
    {
        lv2_log_warning(&self->logger,
                        "No option array was given upon instantiation; will use default values\n.");
    }

    // We need _some_ information on the block size
    if (!supports_bounded_block_size && !supports_fixed_block_size && !options_has_block_size)
    {
        lv2_log_error(&self->logger,
                      "Bounded block size not supported and options gave no block size, aborting..\n");
        free(self);
        return NULL;
    }

    self->synth = sfizz_create_synth();
    self->client = sfizz_create_client(self);
    sfizz_set_broadcast_callback(self->synth, &sfizz_lv2_receive_message, self);
    sfizz_set_receive_callback(self->client, &sfizz_lv2_receive_message);

    sfizz_lv2_get_default_sfz_path(instance, self->sfz_file_path, MAX_PATH_SIZE);
    sfizz_lv2_get_default_scala_path(instance, self->scala_file_path, MAX_PATH_SIZE);

    sfizz_load_file(self->synth, self->sfz_file_path);
    sfizz_load_scala_file(self->synth, self->scala_file_path);

    sfizz_lv2_update_timeinfo(self, 0, ~0);

    return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_delete_client(self->client);
    sfizz_free(self->synth);
    free(self);
}

static void
activate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_set_samples_per_block(self->synth, self->max_block_size);
    sfizz_set_sample_rate(self->synth, self->sample_rate);
    atomic_store(&self->must_update_midnam, 1);
}

static void
deactivate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_all_sound_off(self->synth);
}

static void
sfizz_lv2_send_file_path(sfizz_plugin_t *self, LV2_URID urid, const char *path)
{
    LV2_Atom_Forge_Frame frame;

    bool write_ok =
        lv2_atom_forge_frame_time(&self->forge, 0) &&
        lv2_atom_forge_object(&self->forge, &frame, 0, self->patch_set_uri) &&
        lv2_atom_forge_key(&self->forge, self->patch_property_uri) &&
        lv2_atom_forge_urid(&self->forge, urid) &&
        lv2_atom_forge_key(&self->forge, self->patch_value_uri) &&
        lv2_atom_forge_path(&self->forge, path, (uint32_t)strlen(path));

    if (write_ok)
        lv2_atom_forge_pop(&self->forge, &frame);
}

static void
sfizz_lv2_handle_atom_object(sfizz_plugin_t *self, const LV2_Atom_Object *obj)
{
    const LV2_Atom *property = NULL;
    lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
    if (!property)
    {
        lv2_log_error(&self->logger,
                      "[sfizz] Could not get the property from the patch object, aborting\n");
        return;
    }

    if (property->type != self->atom_urid_uri)
    {
        lv2_log_error(&self->logger,
                      "[sfizz] Atom type was not a URID, aborting\n");
        return;
    }

    const uint32_t key = ((const LV2_Atom_URID *)property)->body;
    const LV2_Atom *atom = NULL;
    lv2_atom_object_get(obj, self->patch_value_uri, &atom, 0);
    if (!atom)
    {
        lv2_log_error(&self->logger, "[sfizz] Error retrieving the atom, aborting\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Atom URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }

    typedef struct
    {
        LV2_Atom atom;
        char body[MAX_PATH_SIZE];
    } sfizz_path_atom_buffer_t;

    if (key == self->sfizz_sfz_file_uri)
    {
        LV2_Atom_Forge *forge = &self->forge_secondary;
        sfizz_path_atom_buffer_t buffer;
        lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
        if (lv2_atom_forge_typed_string(forge, self->sfizz_sfz_file_uri, LV2_ATOM_BODY_CONST(atom), strnlen(LV2_ATOM_BODY_CONST(atom), atom->size)))
            self->worker->schedule_work(self->worker->handle, lv2_atom_total_size(&buffer.atom), &buffer.atom);
        self->check_modification = false;
    }
    else if (key == self->sfizz_scala_file_uri)
    {
        LV2_Atom_Forge *forge = &self->forge_secondary;
        sfizz_path_atom_buffer_t buffer;
        lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
        if (lv2_atom_forge_typed_string(forge, self->sfizz_scala_file_uri, LV2_ATOM_BODY_CONST(atom), strnlen(LV2_ATOM_BODY_CONST(atom), atom->size)))
            self->worker->schedule_work(self->worker->handle, lv2_atom_total_size(&buffer.atom), &buffer.atom);
        self->check_modification = false;
    }
    else
    {
        lv2_log_warning(&self->logger, "[sfizz] Unknown or unsupported object\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Object URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }
}

static void
sfizz_lv2_process_midi_event(sfizz_plugin_t *self, const LV2_Atom_Event *ev)
{
    const uint8_t *const msg = (const uint8_t *)(ev + 1);
    switch (lv2_midi_message_type(msg))
    {
    case LV2_MIDI_MSG_NOTE_ON:
        if (msg[2] == 0)
            goto noteoff; // 0 velocity note-ons should be forbidden but just in case...

        sfizz_send_note_on(self->synth,
                           (int)ev->time.frames,
                           (int)msg[1],
                           msg[2]);
        break;
    case LV2_MIDI_MSG_NOTE_OFF: noteoff:
        sfizz_send_note_off(self->synth,
                            (int)ev->time.frames,
                            (int)msg[1],
                            msg[2]);
        break;
    case LV2_MIDI_MSG_CONTROLLER:
        sfizz_send_cc(self->synth,
                      (int)ev->time.frames,
                      (int)msg[1],
                      msg[2]);
        break;
    case LV2_MIDI_MSG_BENDER:
        sfizz_send_pitch_wheel(self->synth,
                        (int)ev->time.frames,
                        PITCH_BUILD_AND_CENTER(msg[1], msg[2]));
        break;
    default:
        break;
    }
}

static void
sfizz_lv2_status_log(sfizz_plugin_t *self)
{
    UNUSED(self);
    // lv2_log_note(&self->logger, "[sfizz] Allocated buffers: %d\n", sfizz_get_num_buffers(self->synth));
    // lv2_log_note(&self->logger, "[sfizz] Allocated bytes: %d bytes\n", sfizz_get_num_bytes(self->synth));
    // lv2_log_note(&self->logger, "[sfizz] Active voices: %d\n", sfizz_get_num_active_voices(self->synth));
}

static int next_pow_2(int v)
{
    if (v < 1)
        return 1;

    // Bit twiddling hack
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static void
sfizz_lv2_check_oversampling(sfizz_plugin_t* self)
{
    int port_value = next_pow_2((int)*self->oversampling_port);
    if (port_value == (int)self->oversampling)
        return;

    self->oversampling = (sfizz_oversampling_factor_t)port_value;

    LV2_Atom_Int atom = {
        .atom.type = self->sfizz_oversampling_uri,
        .atom.size = sizeof(int),
        .body = self->oversampling
    };
    if (self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom *)&atom),
                                    &atom) != LV2_WORKER_SUCCESS)
    {
        lv2_log_error(&self->logger, "[sfizz] There was an issue changing the oversampling factor\n");
    }
}

static void
sfizz_lv2_check_preload_size(sfizz_plugin_t* self)
{
    unsigned int preload_size = (int)*self->preload_port;
    if (preload_size != self->preload_size)
    {
        LV2_Atom_Int atom = {
            .atom.type = self->sfizz_preload_size_uri,
            .atom.size = sizeof(int),
            .body = preload_size
        };
        if (self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) != LV2_WORKER_SUCCESS)
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue changing the preload size\n");
        }
        self->preload_size = preload_size;
    }
}

static void
sfizz_lv2_check_num_voices(sfizz_plugin_t* self)
{
    int num_voices = (int)*self->polyphony_port;
    if (num_voices != self->num_voices)
    {
        LV2_Atom_Int atom = {
            .atom.type = self->sfizz_num_voices_uri,
            .atom.size = sizeof(int),
            .body = num_voices
        };
        if (self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) != LV2_WORKER_SUCCESS)
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue changing the number of voices\n");
        }
        self->num_voices = num_voices;
    }
}

static void
sfizz_lv2_check_freewheeling(sfizz_plugin_t* self)
{
    if (*(self->freewheel_port) > 0)
    {
        sfizz_enable_freewheeling(self->synth);
    }
    else
    {
        sfizz_disable_freewheeling(self->synth);
    }
}

static void
sfizz_lv2_check_stretch_tuning(sfizz_plugin_t* self)
{
    float stretch_tuning = (float)*self->stretch_tuning_port;
    if (stretch_tuning != self->stretch_tuning)
    {
        sfizz_load_stretch_tuning_by_ratio(self->synth, stretch_tuning);
        self->stretch_tuning = stretch_tuning;
    }
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    if (!self->control_port || !self->notify_port)
        return;

    // Set up forge to write directly to notify output port.
    const size_t notify_capacity = self->notify_port->atom.size;
    lv2_atom_forge_set_buffer(&self->forge, (uint8_t *)self->notify_port, notify_capacity);

    // Start a sequence in the notify output port.
    LV2_Atom_Forge_Frame notify_frame;
    if (!lv2_atom_forge_sequence_head(&self->forge, &notify_frame, 0))
        assert(false);

    LV2_ATOM_SEQUENCE_FOREACH(self->control_port, ev)
    {
        const int delay = (int)ev->time.frames;

        // If the received atom is an object/patch message
        if (ev->body.type == self->atom_object_uri || ev->body.type == self->atom_blank_uri)
        {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
            if (obj->body.otype == self->patch_set_uri)
            {
                sfizz_lv2_handle_atom_object(self, obj);
            }
            else if (obj->body.otype == self->patch_get_uri)
            {
                const LV2_Atom_URID *property = NULL;
                lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
                if (!property) // Send the full state
                {
                    sfizz_lv2_send_file_path(self, self->sfizz_sfz_file_uri, self->sfz_file_path);
                    sfizz_lv2_send_file_path(self, self->sfizz_scala_file_uri, self->scala_file_path);
                }
                else if (property->body == self->sfizz_sfz_file_uri)
                {
                    sfizz_lv2_send_file_path(self, self->sfizz_sfz_file_uri, self->sfz_file_path);
                }
                else if (property->body == self->sfizz_scala_file_uri)
                {
                    sfizz_lv2_send_file_path(self, self->sfizz_scala_file_uri, self->scala_file_path);
                }
            }
            else if (obj->body.otype == self->time_position_uri)
            {
                const LV2_Atom *bar_atom = NULL;
                const LV2_Atom *bar_beat_atom = NULL;
                const LV2_Atom *beat_unit_atom = NULL;
                const LV2_Atom *beats_per_bar_atom = NULL;
                const LV2_Atom *beats_per_minute_atom = NULL;
                const LV2_Atom *speed_atom = NULL;

                lv2_atom_object_get(
                    obj,
                    self->time_bar_uri, &bar_atom,
                    self->time_bar_beat_uri, &bar_beat_atom,
                    self->time_beats_per_bar_uri, &beats_per_bar_atom,
                    self->time_beats_per_minute_uri, &beats_per_minute_atom,
                    self->time_beat_unit_uri, &beat_unit_atom,
                    self->time_speed_uri, &speed_atom,
                    0);

                int updates = 0;

                int64_t bar;
                double bar_beat;
                if (sfizz_atom_extract_integer(self, bar_atom, &bar)) {
                    self->bar = (int)bar;
                    updates |= SFIZZ_TIMEINFO_POSITION;
                }
                if (sfizz_atom_extract_real(self, bar_beat_atom, &bar_beat)) {
                    self->bar_beat = (float)bar_beat;
                    updates |= SFIZZ_TIMEINFO_POSITION;
                }

                double beats_per_bar;
                int64_t beat_unit;
                if (sfizz_atom_extract_real(self, beats_per_bar_atom, &beats_per_bar)) {
                    self->beats_per_bar = (int)beats_per_bar;
                    updates |= SFIZZ_TIMEINFO_SIGNATURE;
                }
                if (sfizz_atom_extract_integer(self, beat_unit_atom, &beat_unit)) {
                    self->beat_unit = (int)beat_unit;
                    updates |= SFIZZ_TIMEINFO_SIGNATURE;
                }

                double tempo;
                if (sfizz_atom_extract_real(self, beats_per_minute_atom, &tempo)) {
                    self->bpm_tempo = (float)tempo;
                    updates |= SFIZZ_TIMEINFO_TEMPO;
                }

                double speed;
                if (sfizz_atom_extract_real(self, speed_atom, &speed)) {
                    self->speed = (float)speed;
                    updates |= SFIZZ_TIMEINFO_SPEED;
                }

                sfizz_lv2_update_timeinfo(self, delay, updates);
            }
            else
            {
                lv2_log_warning(&self->logger, "[sfizz] Got an Object atom but it was not supported\n");
                if (self->unmap)
                    lv2_log_warning(&self->logger,
                                    "Object URI: %s\n",
                                    self->unmap->unmap(self->unmap->handle, obj->body.otype));
                continue;
            }
        }
        else if (ev->body.type == self->midi_event_uri)
        {
            // Got an atom that is a MIDI event
            sfizz_lv2_process_midi_event(self, ev);
        }
        else if (ev->body.type == self->sfizz_osc_blob_uri)
        {
            // Got an atom that is a OSC event
            const char *path;
            const char *sig;
            const sfizz_arg_t *args;
            uint8_t buffer[1024];
            if (sfizz_extract_message(LV2_ATOM_BODY_CONST(&ev->body), ev->body.size, buffer, sizeof(buffer), &path, &sig, &args) > 0)
                sfizz_send_message(self->synth, self->client, (int)ev->time.frames, path, sig, args);
        }
    }


    // Check and update parameters if needed
    sfizz_lv2_check_freewheeling(self);
    sfizz_set_volume(self->synth, *(self->volume_port));
    sfizz_set_scala_root_key(self->synth, *(self->scala_root_key_port));
    sfizz_set_tuning_frequency(self->synth, *(self->tuning_frequency_port));
    sfizz_lv2_check_stretch_tuning(self);
    sfizz_lv2_check_preload_size(self);
    sfizz_lv2_check_oversampling(self);
    sfizz_lv2_check_num_voices(self);
    *(self->active_voices_port) = sfizz_get_num_active_voices(self->synth);
    *(self->num_curves_port) = sfizz_get_num_curves(self->synth);
    *(self->num_masters_port) = sfizz_get_num_masters(self->synth);
    *(self->num_groups_port) = sfizz_get_num_groups(self->synth);
    *(self->num_regions_port) = sfizz_get_num_regions(self->synth);
    *(self->num_samples_port) = sfizz_get_num_preloaded_samples(self->synth);

    // Log the buffer usage
    self->sample_counter += (int)sample_count;
    if (self->sample_counter > LOG_SAMPLE_COUNT && self->check_modification)
    {
        LV2_Atom atom;
        atom.size = 0;
#ifndef NDEBUG
        atom.type = self->sfizz_log_status_uri;
        if (!(self->worker->schedule_work(self->worker->handle,
                                         lv2_atom_total_size((LV2_Atom *)&atom),
                                         &atom) == LV2_WORKER_SUCCESS))
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue sending a logging message to the background worker\n");
        }
#endif
        atom.type = self->sfizz_check_modification_uri;
        if ((self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) == LV2_WORKER_SUCCESS)) {
            self->check_modification = false;
        } else {
            lv2_log_error(&self->logger, "[sfizz] There was an issue sending a notice to check the modification of the SFZ file to the background worker\n");
        }
        self->sample_counter = 0;
    }

    // Render the block
    sfizz_render_block(self->synth, self->output_buffers, 2, (int)sample_count);

    if (self->midnam && atomic_exchange(&self->must_update_midnam, 0))
    {
        self->midnam->update(self->midnam->handle);
    }

    lv2_atom_forge_pop(&self->forge, &notify_frame);
}

static uint32_t
lv2_get_options(LV2_Handle instance, LV2_Options_Option *options)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    LV2_DEBUG("[DEBUG] get_options called\n");
    for (LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
    {
        if (self->unmap) {
            LV2_DEBUG("[DEBUG] Called for an option with key (subject): %s (%s) \n",
                      self->unmap->unmap(self->unmap->handle, opt->key),
                      self->unmap->unmap(self->unmap->handle, opt->subject));
        }

        if (opt->key == self->sample_rate_uri)
        {
            opt->type = self->atom_float_uri;
            opt->size = sizeof(float);
            opt->value = (void*)&self->sample_rate;
            return LV2_OPTIONS_SUCCESS;
        }

        if (opt->key == self->max_block_length_uri || opt->key == self->nominal_block_length_uri)
        {
            opt->type = self->atom_int_uri;
            opt->size = sizeof(int);
            opt->value = (void*)&self->max_block_size;
            return LV2_OPTIONS_SUCCESS;
        }
    }
    return LV2_OPTIONS_ERR_UNKNOWN;
}

static uint32_t
lv2_set_options(LV2_Handle instance, const LV2_Options_Option *options)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    // Update the block size and sample rate as needed
    for (const LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
    {
        if (opt->key == self->sample_rate_uri)
        {
            sfizz_lv2_parse_sample_rate(self, opt);
            sfizz_set_sample_rate(self->synth, self->sample_rate);
        }
        else if (!self->expect_nominal_block_length && opt->key == self->max_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "[sfizz] Got a max block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
        }
        else if (opt->key == self->nominal_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "[sfizz] Got a nominal block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
        }
    }
    return LV2_OPTIONS_SUCCESS;
}

static void
sfizz_lv2_update_file_info(sfizz_plugin_t* self, const char *file_path)
{
    if (file_path != self->sfz_file_path)
        strcpy(self->sfz_file_path, file_path);

    lv2_log_note(&self->logger, "[sfizz] File changed to: %s\n", file_path);

    char *unknown_opcodes = sfizz_get_unknown_opcodes(self->synth);
    if (unknown_opcodes)
    {
        lv2_log_note(&self->logger, "[sfizz] Unknown opcodes: %s\n", unknown_opcodes);
        free(unknown_opcodes);
    }
    lv2_log_note(&self->logger, "[sfizz] Number of masters: %d\n", sfizz_get_num_masters(self->synth));
    lv2_log_note(&self->logger, "[sfizz] Number of groups: %d\n", sfizz_get_num_groups(self->synth));
    lv2_log_note(&self->logger, "[sfizz] Number of regions: %d\n", sfizz_get_num_regions(self->synth));

    atomic_store(&self->must_update_midnam, 1);
}

static bool
sfizz_lv2_load_file(LV2_Handle instance, const char *file_path)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    bool status = sfizz_load_file(self->synth, file_path);
    sfizz_lv2_update_file_info(self, file_path);
    return status;
}

static bool
sfizz_lv2_load_scala_file(LV2_Handle instance, const char *file_path)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    bool status = sfizz_load_scala_file(self->synth, file_path);
    if (file_path != self->scala_file_path)
        strcpy(self->scala_file_path, file_path);
    return status;
}

static LV2_State_Status
restore(LV2_Handle instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle handle,
        uint32_t flags,
        const LV2_Feature *const *features)
{
    UNUSED(flags);
    LV2_State_Status status = LV2_STATE_SUCCESS;
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    LV2_State_Map_Path *map_path = NULL;
    LV2_State_Free_Path *free_path = &sfizz_State_Free_Path;
    for (const LV2_Feature *const *f = features; *f; ++f)
    {
        if (!strcmp((*f)->URI, LV2_STATE__mapPath))
            map_path = (LV2_State_Map_Path *)(**f).data;
        else if (!strcmp((*f)->URI, LV2_STATE__freePath))
            free_path = (LV2_State_Free_Path *)(**f).data;
    }

    // Set default values
    sfizz_lv2_get_default_sfz_path(instance, self->sfz_file_path, MAX_PATH_SIZE);
    sfizz_lv2_get_default_scala_path(instance, self->scala_file_path, MAX_PATH_SIZE);
    self->num_voices = DEFAULT_VOICES;
    self->preload_size = DEFAULT_PRELOAD;
    self->oversampling = DEFAULT_OVERSAMPLING;

    // Fetch back the saved file path, if any
    size_t size;
    uint32_t type;
    uint32_t val_flags;
    const void *value;
    value = retrieve(handle, self->sfizz_sfz_file_uri, &size, &type, &val_flags);
    if (value)
    {
        const char *path = (const char *)value;
        if (map_path)
        {
            path = map_path->absolute_path(map_path->handle, path);
            if (!path)
                status = LV2_STATE_ERR_UNKNOWN;
        }

        if (path)
        {
            strncpy(self->sfz_file_path, path, MAX_PATH_SIZE);
            self->sfz_file_path[MAX_PATH_SIZE - 1] = '\0';

            if (map_path)
                free_path->free_path(free_path->handle, (char *)path);
        }
    }

    value = retrieve(handle, self->sfizz_scala_file_uri, &size, &type, &val_flags);
    if (value)
    {
        const char *path = (const char *)value;
        if (map_path)
        {
            path = map_path->absolute_path(map_path->handle, path);
            if (!path)
                status = LV2_STATE_ERR_UNKNOWN;
        }

        if (path)
        {
            strncpy(self->scala_file_path, path, MAX_PATH_SIZE);
            self->scala_file_path[MAX_PATH_SIZE - 1] = '\0';

            if (map_path)
                free_path->free_path(free_path->handle, (char *)path);
        }
    }

    value = retrieve(handle, self->sfizz_num_voices_uri, &size, &type, &val_flags);
    if (value)
    {
        int num_voices = *(const int *)value;
        if (num_voices > 0 && num_voices <= MAX_VOICES)
            self->num_voices = num_voices;
    }

    value = retrieve(handle, self->sfizz_preload_size_uri, &size, &type, &val_flags);
    if (value)
    {
        unsigned int preload_size = *(const unsigned int *)value;
        self->preload_size = preload_size;
    }

    value = retrieve(handle, self->sfizz_oversampling_uri, &size, &type, &val_flags);
    if (value)
    {
        sfizz_oversampling_factor_t oversampling = *(const sfizz_oversampling_factor_t *)value;
        self->oversampling = oversampling;
    }

    // Sync the parameters to the synth

    // Load an empty file to remove the default sine, and then the new file.
    sfizz_load_string(self->synth, "empty.sfz", "");
    self->check_modification = false;
    if (sfizz_lv2_load_file(instance, self->sfz_file_path))
    {
        lv2_log_note(&self->logger,
            "[sfizz] Restoring the file %s\n", self->sfz_file_path);
        self->check_modification = true;
    }
    else
    {
        lv2_log_error(&self->logger,
            "[sfizz] Error while restoring the file %s\n", self->sfz_file_path);
    }

    if (sfizz_load_scala_file(self->synth, self->scala_file_path))
    {
        lv2_log_note(&self->logger,
            "[sfizz] Restoring the scale %s\n", self->scala_file_path);
    }
    else
    {
        lv2_log_error(&self->logger,
            "[sfizz] Error while restoring the scale %s\n", self->scala_file_path);
    }

    lv2_log_note(&self->logger, "[sfizz] Restoring the number of voices to %d\n", self->num_voices);
    sfizz_set_num_voices(self->synth, self->num_voices);

    lv2_log_note(&self->logger, "[sfizz] Restoring the preload size to %d\n", self->preload_size);
    sfizz_set_preload_size(self->synth, self->preload_size);

    lv2_log_note(&self->logger, "[sfizz] Restoring the oversampling to %d\n", self->oversampling);
    sfizz_set_oversampling_factor(self->synth, self->oversampling);

    return status;
}

static LV2_State_Status
save(LV2_Handle instance,
     LV2_State_Store_Function store,
     LV2_State_Handle handle,
     uint32_t flags,
     const LV2_Feature *const *features)
{
    UNUSED(flags);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    LV2_State_Map_Path *map_path = NULL;
    LV2_State_Free_Path *free_path = &sfizz_State_Free_Path;
    for (const LV2_Feature *const *f = features; *f; ++f)
    {
        if (!strcmp((*f)->URI, LV2_STATE__mapPath))
            map_path = (LV2_State_Map_Path *)(**f).data;
        else if (!strcmp((*f)->URI, LV2_STATE__freePath))
            free_path = (LV2_State_Free_Path *)(**f).data;
    }

    const char *path;

    // Save the file path
    path = self->sfz_file_path;
    if (map_path)
    {
        path = map_path->abstract_path(map_path->handle, path);
        if (!path)
            return LV2_STATE_ERR_UNKNOWN;
    }
    store(handle,
          self->sfizz_sfz_file_uri,
          path,
          strlen(path) + 1,
          self->atom_path_uri,
          LV2_STATE_IS_POD);
    if (map_path)
        free_path->free_path(free_path->handle, (char *)path);

    // Save the scala file path
    path = self->scala_file_path;
    if (map_path)
    {
        path = map_path->abstract_path(map_path->handle, path);
        if (!path)
            return LV2_STATE_ERR_UNKNOWN;
    }
    if (!path)
        return LV2_STATE_ERR_UNKNOWN;
    store(handle,
          self->sfizz_scala_file_uri,
          path,
          strlen(path) + 1,
          self->atom_path_uri,
          LV2_STATE_IS_POD);
    if (map_path)
        free_path->free_path(free_path->handle, (char *)path);

    // Save the number of voices
    store(handle,
          self->sfizz_num_voices_uri,
          &self->num_voices,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    // Save the preload size
    store(handle,
          self->sfizz_preload_size_uri,
          &self->preload_size,
          sizeof(unsigned int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    // Save the preload size
    store(handle,
          self->sfizz_oversampling_uri,
          &self->oversampling,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    return LV2_STATE_SUCCESS;
}

static void
sfizz_lv2_activate_file_checking(
    sfizz_plugin_t *self,
    LV2_Worker_Respond_Function respond,
    LV2_Worker_Respond_Handle handle)
{
    LV2_Atom check_modification_atom = {
        .size = 0,
        .type = self->sfizz_check_modification_uri
    };
    respond(handle, lv2_atom_total_size(&check_modification_atom), &check_modification_atom);
}

// This runs in a lower priority thread
static LV2_Worker_Status
work(LV2_Handle instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle handle,
     uint32_t size,
     const void *data)
{
    UNUSED(size);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    if (!data) {
        lv2_log_error(&self->logger, "[sfizz] Ignoring empty data in the worker thread\n");
        return LV2_WORKER_ERR_UNKNOWN;
    }

    const LV2_Atom *atom = (const LV2_Atom *)data;
    if (atom->type == self->sfizz_sfz_file_uri)
    {
        const char *sfz_file_path = LV2_ATOM_BODY_CONST(atom);
        if (!sfizz_lv2_load_file(self, sfz_file_path)) {
            lv2_log_error(&self->logger,
                "[sfizz] Error with %s; no file should be loaded\n", sfz_file_path);
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else if (atom->type == self->sfizz_scala_file_uri)
    {
        const char *scala_file_path = LV2_ATOM_BODY_CONST(atom);
        if (sfizz_lv2_load_scala_file(self->synth, scala_file_path)) {
            lv2_log_note(&self->logger, "[sfizz] Scala file loaded: %s\n", scala_file_path);
        } else {
            lv2_log_error(&self->logger,
                "[sfizz] Error with %s; no new scala file should be loaded\n", scala_file_path);
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else if (atom->type == self->sfizz_num_voices_uri)
    {
        const int num_voices = *(const int *)LV2_ATOM_BODY_CONST(atom);
        sfizz_set_num_voices(self->synth, num_voices);
        if (sfizz_get_num_voices(self->synth) == num_voices) {
            lv2_log_note(&self->logger, "[sfizz] Number of voices changed to: %d\n", num_voices);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the number of voices\n");
        }
    }
    else if (atom->type == self->sfizz_preload_size_uri)
    {
        const unsigned int preload_size = *(const unsigned int *)LV2_ATOM_BODY_CONST(atom);
        sfizz_set_preload_size(self->synth, preload_size);
        if (sfizz_get_preload_size(self->synth) == preload_size) {
            lv2_log_note(&self->logger, "[sfizz] Preload size changed to: %d\n", preload_size);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the preload size\n");
        }
    }
    else if (atom->type == self->sfizz_oversampling_uri)
    {
        const sfizz_oversampling_factor_t oversampling =
            *(const sfizz_oversampling_factor_t *)LV2_ATOM_BODY_CONST(atom);
        sfizz_set_oversampling_factor(self->synth, oversampling);
        if (sfizz_get_oversampling_factor(self->synth) == oversampling) {
            lv2_log_note(&self->logger, "[sfizz] Oversampling changed to: %d\n", oversampling);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the oversampling\n");
        }
    }
    else if (atom->type == self->sfizz_log_status_uri)
    {
        sfizz_lv2_status_log(self);
    }
    else if (atom->type == self->sfizz_check_modification_uri)
    {
        if (sfizz_should_reload_file(self->synth))
        {
            lv2_log_note(&self->logger,
                        "[sfizz] File %s seems to have been updated, reloading\n",
                        self->sfz_file_path);
            if (!sfizz_lv2_load_file(self, self->sfz_file_path)) {
                lv2_log_error(&self->logger,
                    "[sfizz] Error with %s; no file should be loaded\n", self->sfz_file_path);
            }
        }

        if (sfizz_should_reload_scala(self->synth))
        {
            lv2_log_note(&self->logger,
                        "[sfizz] Scala file %s seems to have been updated, reloading\n",
                        self->scala_file_path);
            if (sfizz_lv2_load_scala_file(self->synth, self->scala_file_path)) {
                lv2_log_note(&self->logger, "[sfizz] Scala file loaded: %s\n", self->scala_file_path);
            } else {
                lv2_log_error(&self->logger,
                    "[sfizz] Error with %s; no new scala file should be loaded\n", self->scala_file_path);
            }
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else
    {
        lv2_log_error(&self->logger, "[sfizz] Got an unknown atom in work\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }
    return LV2_WORKER_SUCCESS;
}

// This runs in the audio thread
static LV2_Worker_Status
work_response(LV2_Handle instance,
              uint32_t size,
              const void *data)
{
    UNUSED(size);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    if (!data)
        return LV2_WORKER_ERR_UNKNOWN;

    const LV2_Atom *atom = (const LV2_Atom *)data;
    if (atom->type == self->sfizz_check_modification_uri) {
        self->check_modification = true; // check changes
    } else {
        lv2_log_error(&self->logger, "[sfizz] Got an unexpected atom in work response\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }

    return LV2_WORKER_SUCCESS;
}

static char *
midnam_model(LV2_Handle instance)
{
    char *model = malloc(64);
    if (!model)
        return NULL;

    sprintf(model, "Sfizz LV2:%p", instance);
    return model;
}

static char *
midnam_export(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    char *model = midnam_model(instance);
    if (!model)
        return NULL;

    char *xml = sfizz_export_midnam(self->synth, model);
    free(model);
    return xml;
}

static void
midnam_free(char *string)
{
    free(string);
}

static const void *
extension_data(const char *uri)
{
    static const LV2_Options_Interface options = {lv2_get_options, lv2_set_options};
    static const LV2_State_Interface state = {save, restore};
    static const LV2_Worker_Interface worker = {work, work_response, NULL};
    static const LV2_Midnam_Interface midnam = {midnam_export, midnam_model, midnam_free};

    // Advertise the extensions we support
    if (!strcmp(uri, LV2_OPTIONS__interface))
        return &options;
    else if (!strcmp(uri, LV2_STATE__interface))
        return &state;
    else if (!strcmp(uri, LV2_WORKER__interface))
        return &worker;
    else if (!strcmp(uri, LV2_MIDNAM__interface))
        return &midnam;

    return NULL;
}

static const LV2_Descriptor descriptor = {
    SFIZZ_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &descriptor;
    default:
        return NULL;
    }
}
