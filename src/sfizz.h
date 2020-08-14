// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
  @file
  @brief sfizz public C API.
*/

#pragma once
#include "sfizz_message.h"
#include <stddef.h>
#include <stdbool.h>

#if defined SFIZZ_EXPORT_SYMBOLS
  #if defined _WIN32
    #define SFIZZ_EXPORTED_API __declspec(dllexport)
  #else
    #define SFIZZ_EXPORTED_API __attribute__ ((visibility ("default")))
  #endif
#else
  #define SFIZZ_EXPORTED_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Synth handle.
 * @since 0.2.0
 */
typedef struct sfizz_synth_t sfizz_synth_t;

/**
 * @brief Oversampling factor
 * @since 0.2.0
 */
typedef enum {
    SFIZZ_OVERSAMPLING_X1 = 1,
    SFIZZ_OVERSAMPLING_X2 = 2,
    SFIZZ_OVERSAMPLING_X4 = 4,
    SFIZZ_OVERSAMPLING_X8 = 8
} sfizz_oversampling_factor_t;

/**
 * @brief Processing mode
 * @since 0.5.0
 */
typedef enum {
    SFIZZ_PROCESS_LIVE,
    SFIZZ_PROCESS_FREEWHEELING,
} sfizz_process_mode_t;

/**
 * @brief Creates a sfizz synth.
 *
 * This object has to be freed by the caller using sfizz_free().
 * The synth by default is set at 48 kHz and a maximum block size of 1024.
 * You should change these values if they are not correct for your application.
 * @since 0.2.0
 */
SFIZZ_EXPORTED_API sfizz_synth_t* sfizz_create_synth();

/**
 * @brief Frees an existing sfizz synth.
 * @since 0.2.0
 *
 * @param synth  The synth to destroy.
 */
SFIZZ_EXPORTED_API void sfizz_free(sfizz_synth_t* synth);

/**
 * @brief Loads an SFZ file.
 *
 * The file path can be absolute or relative. All file operations for this SFZ
 * file will be relative to the parent directory of the SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 * @param path   A null-terminated string representing a path to an SFZ file.
 *
 * @return @true when file loading went OK,
 *         @false if some error occured while loading.
 */
SFIZZ_EXPORTED_API bool sfizz_load_file(sfizz_synth_t* synth, const char* path);

/**
 * @brief Loads an SFZ file from textual data.
 *
 * This accepts a virtual path name for the imaginary sfz file, which is not
 * required to exist on disk. The purpose of the virtual path is to locate
 * samples with relative paths.
 * @since 0.4.0
 *
 * @param synth  The synth.
 * @param path   The virtual path of the SFZ file.
 * @param text   The contents of the virtual SFZ file.
 *
 * @return @true when file loading went OK,
 *         @false if some error occured while loading.
 */
SFIZZ_EXPORTED_API bool sfizz_load_string(sfizz_synth_t* synth, const char* path, const char* text);

/**
 * @brief Sets the tuning from a Scala file loaded from the file system.
 * @since 0.4.0
 *
 * @param synth  The synth.
 * @param path   The path to the file in Scala format.
 *
 * @return @true when tuning scale loaded OK,
 *         @false if some error occurred.
 */
SFIZZ_EXPORTED_API bool sfizz_load_scala_file(sfizz_synth_t* synth, const char* path);

/**
 * @brief Sets the tuning from a Scala file loaded from memory.
 * @since 0.4.0
 *
 * @param synth  The synth.
 * @param text   The contents of the file in Scala format.
 *
 * @return @true when tuning scale loaded OK,
 *         @false if some error occurred.
 */
SFIZZ_EXPORTED_API bool sfizz_load_scala_string(sfizz_synth_t* synth, const char* text);

/**
 * @brief Sets the scala root key.
 * @since 0.4.0
 *
 * @param synth     The synth.
 * @param root_key  The MIDI number of the Scala root key (default 60 for C4).
 */
SFIZZ_EXPORTED_API void sfizz_set_scala_root_key(sfizz_synth_t* synth, int root_key);

/**
 * @brief Gets the scala root key.
 * @since 0.4.0
 *
 * @param synth  The synth.
 *
 * @return The MIDI number of the Scala root key (default 60 for C4).
 */
SFIZZ_EXPORTED_API int sfizz_get_scala_root_key(sfizz_synth_t* synth);

/**
 * @brief Sets the reference tuning frequency.
 * @since 0.4.0
 *
 * @param synth      The synth.
 * @param frequency  The frequency which indicates where standard tuning A4 is (default 440 Hz).
 */
SFIZZ_EXPORTED_API void sfizz_set_tuning_frequency(sfizz_synth_t* synth, float frequency);

/**
 * @brief Gets the reference tuning frequency.
 * @since 0.4.0
 *
 * @param synth  The synth.
 *
 * @return The frequency which indicates where standard tuning A4 is (default 440 Hz).
 */
SFIZZ_EXPORTED_API float sfizz_get_tuning_frequency(sfizz_synth_t* synth);

/**
 * @brief Configure stretch tuning using a predefined parametric Railsback curve.
 *
 * A ratio 1/2 is supposed to match the average piano; 0 disables (the default).
 * @since 0.4.0
 *
 * @param synth  The synth.
 * @param ratio  The parameter in domain 0-1.
 */
SFIZZ_EXPORTED_API void sfizz_load_stretch_tuning_by_ratio(sfizz_synth_t* synth, float ratio);

/**
 * @brief Return the number of regions in the currently loaded SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_regions(sfizz_synth_t* synth);

/**
 * @brief Return the number of groups in the currently loaded SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_groups(sfizz_synth_t* synth);

/**
 * @brief Return the number of masters in the currently loaded SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_masters(sfizz_synth_t* synth);

/**
 * @brief Return the number of curves in the currently loaded SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_curves(sfizz_synth_t* synth);

/**
 * @brief Export a MIDI Name document describing the currently loaded SFZ file.
 * @since 0.3.1
 *
 * @param synth  The synth.
 * @param model  The model name used if a non-empty string, otherwise generated.
 *
 * @return A newly allocated XML string, which must be freed after use.
 */
SFIZZ_EXPORTED_API char* sfizz_export_midnam(sfizz_synth_t* synth, const char* model);

/**
 * @brief Return the number of preloaded samples for the current SFZ file.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API size_t sfizz_get_num_preloaded_samples(sfizz_synth_t* synth);

/**
 * @brief Return the number of active voices.
 *
 * Note that this function is a basic indicator and does not aim to be perfect.
 * In particular, it runs on the calling thread so voices may well start or stop
 * while the function is checking which voice is active.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_active_voices(sfizz_synth_t* synth);

/**
 * @brief Set the expected number of samples per block.
 *
 * If unsure, give an upper bound since right now ugly things may happen if you
 * go over this number.
 * @since 0.2.0
 *
 * @param synth              The synth.
 * @param samples_per_block  The number of samples per block.
 */
SFIZZ_EXPORTED_API void sfizz_set_samples_per_block(sfizz_synth_t* synth, int samples_per_block);

/**
 * @brief Set the sample rate for the synth.
 *
 * This is the output sample rate. This setting does not affect the internal processing.
 * @since 0.2.0
 *
 * @param synth        The synth
 * @param sample_rate  The sample rate.
 */
SFIZZ_EXPORTED_API void sfizz_set_sample_rate(sfizz_synth_t* synth, float sample_rate);

/**
 * @brief Send a note on event to the synth.
 *
 * As with all MIDI events, this needs to happen before the call to
 * sfizz_render_block() in each block and should appear in order of the delays.
 * @since 0.2.0
 *
 * @param synth        The synth.
 * @param delay        The delay of the event in the block, in samples.
 * @param note_number  The MIDI note number.
 * @param velocity     The MIDI velocity.
 */
SFIZZ_EXPORTED_API void sfizz_send_note_on(sfizz_synth_t* synth, int delay, int note_number, char velocity);

/**
 * @brief Send a note off event to the synth.
 *
 * As with all MIDI events, this needs to happen before the call to
 * sfizz_render_block() in each block and should appear in order of the delays.
 * As per the SFZ spec the velocity of note-off events is usually replaced by
 * the note-on velocity.
 * @since 0.2.0
 *
 * @param synth        The synth.
 * @param delay        The delay of the event in the block, in samples.
 * @param note_number  The MIDI note number.
 * @param velocity     The MIDI velocity.
 */
SFIZZ_EXPORTED_API void sfizz_send_note_off(sfizz_synth_t* synth, int delay, int note_number, char velocity);

/**
 * @brief Send a CC event to the synth.
 *
 * As with all MIDI events, this needs to happen before the call to
 * sfizz_render_block() in each block and should appear in order of the delays.
 * @since 0.2.0
 *
 * @param synth      The synth.
 * @param delay      The delay of the event in the block, in samples.
 * @param cc_number  The MIDI CC number.
 * @param cc_value   The MIDI CC value.
 */
SFIZZ_EXPORTED_API void sfizz_send_cc(sfizz_synth_t* synth, int delay, int cc_number, char cc_value);

/**
 * @brief Send a high precision CC event to the synth.
 *
 * As with all MIDI events, this needs to happen before the call to
 * sfizz_render_block() in each block and should appear in order of the delays.
 * @since 0.4.0
 *
 * @param synth       The synth.
 * @param delay       The delay of the event in the block, in samples.
 * @param cc_number   The MIDI CC number.
 * @param norm_value  The normalized CC value, in domain 0 to 1.
 */
SFIZZ_EXPORTED_API void sfizz_send_hdcc(sfizz_synth_t* synth, int delay, int cc_number, float norm_value);

/**
 * @brief Send a pitch wheel event.
 *
 * As with all MIDI events, this needs to happen before the call to
 * sfizz_render_block() in each block and should appear in order of the delays.
 * @since 0.4.0
 *
 * @param synth  The synth.
 * @param delay  The delay.
 * @param pitch  The pitch.
 */
SFIZZ_EXPORTED_API void sfizz_send_pitch_wheel(sfizz_synth_t* synth, int delay, int pitch);

/**
 * @brief Send an aftertouch event. (CURRENTLY UNIMPLEMENTED)
 * @since 0.2.0
 *
 * @param synth      The synth.
 * @param delay      The delay at which the event occurs; this should be lower
     *               than the size of the block in the next call to renderBlock().
 * @param aftertouch The aftertouch value.
 */
SFIZZ_EXPORTED_API void sfizz_send_aftertouch(sfizz_synth_t* synth, int delay, char aftertouch);

/**
 * @brief Send a tempo event.
 * @since 0.2.0
 *
 * @param synth             The synth.
 * @param delay             The delay.
 * @param seconds_per_beat  The seconds per beat.
 */
SFIZZ_EXPORTED_API void sfizz_send_tempo(sfizz_synth_t* synth, int delay, float seconds_per_beat);

/**
 * @brief Send the time signature.
 * @since 0.5.0
 *
 * @param synth          The synth.
 * @param delay          The delay.
 * @param beats_per_bar  The number of beats per bar, or time signature numerator.
 * @param beat_unit      The note corresponding to one beat, or time signature denominator.
 */
SFIZZ_EXPORTED_API void sfizz_send_time_signature(sfizz_synth_t* synth, int delay, int beats_per_bar, int beat_unit);

/**
 * @brief Send the time position.
 * @since 0.5.0
 *
 * @param synth     The synth.
 * @param delay     The delay.
 * @param bar       The current bar.
 * @param bar_beat  The fractional position of the current beat within the bar.
 */
SFIZZ_EXPORTED_API void sfizz_send_time_position(sfizz_synth_t* synth, int delay, int bar, double bar_beat);

/**
 * @brief Send the playback state.
 * @since 0.5.0
 *
 * @param synth           The synth.
 * @param delay           The delay.
 * @param playback_state  The playback state, 1 if playing, 0 if stopped.
 */
SFIZZ_EXPORTED_API void sfizz_send_playback_state(sfizz_synth_t* synth, int delay, int playback_state);

/**
 * @brief Render a block audio data into a stereo channel.
 *
 * No other channel configuration is supported. The synth will gracefully ignore
 * your request if you provide a value. You should pass all the relevant events
 * for the block (midi notes, CCs, ...) before rendering each block.
 * The synth will memorize the inputs and render sample accurates envelopes
 * depending on the input events passed to it.
 * @since 0.2.0
 *
 * @param synth         The synth.
 * @param channels      Pointers to the left and right channel of the output.
 * @param num_channels  Should be equal to 2 for the time being.
 * @param num_frames    Number of frames to fill. This should be less than
 *                      or equal to the expected samples_per_block.
 */
SFIZZ_EXPORTED_API void sfizz_render_block(sfizz_synth_t* synth, float** channels, int num_channels, int num_frames);

/**
 * @brief Get the size of the preloaded data.
 *
 * This returns the number of floats used in the preloading buffers.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API unsigned int sfizz_get_preload_size(sfizz_synth_t* synth);

/**
 * @brief Set the size of the preloaded data in number of floats (not bytes).
 *
 * This will disable the callbacks for the duration of the load.
 * This function takes a lock ; prefer calling it out of the RT thread.
 * It can also take a long time to return. If the new preload size is the same
 * as the current one, it will release the lock immediately and exit.
 * @since 0.2.0
 *
 * @param      synth         The synth.
 * @param[in]  preload_size  The preload size.
 */
SFIZZ_EXPORTED_API void sfizz_set_preload_size(sfizz_synth_t* synth, unsigned int preload_size);

/**
 * @brief Get the internal oversampling rate.
 *
 * This is the sampling rate of the engine, not the output or expected rate of
 * the calling function. For the latter use the sfizz_get_sample_rate() function.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API sfizz_oversampling_factor_t sfizz_get_oversampling_factor(sfizz_synth_t* synth);

/**
 * @brief Set the internal oversampling rate.
 *
 * This is the sampling rate of the engine, not the output or expected rate of
 * the calling function. For the latter use the sfizz_set_sample_rate() function.
 *
 * Increasing this value (up to x8 oversampling) improves the quality of the
 * output at the expense of memory consumption and background loading speed.
 * The main render path still uses the same linear interpolation algorithm and
 * should not see its performance decrease, but the files are oversampled upon
 * loading which increases the stress on the background loader and reduce
 * the loading speed. You can tweak the size of the preloaded data to compensate
 * for the memory increase, but the full loading will need to take place anyway.
 *
 * This function takes a lock and disables the callback; prefer calling it out
 * of the RT thread. It can also take a long time to return.
 * If the new oversampling factor is the same as the current one, it will
 * release the lock immediately and exit.
 * @since 0.2.0
 *
 * @param      synth         The synth.
 * @param[in]  oversampling  The oversampling factor.
 *
 * @return @true if the oversampling factor was correct, @false otherwise.
 */
SFIZZ_EXPORTED_API bool sfizz_set_oversampling_factor(sfizz_synth_t* synth, sfizz_oversampling_factor_t oversampling);

/**
 * @brief Get the default resampling quality.
 *
 * This is the quality setting which the engine uses when the instrument
 * does not use the opcode `sample_quality`. The engine uses distinct
 * default quality settings for live mode and freewheeling mode,
 * which both can be accessed by the means of this function.
 * @since 0.4.0
 *
 * @param      synth  The synth.
 * @param[in]  mode   The processing mode.
 *
 * @return The sample quality for the given mode, in the range 1 to 10.
 */
SFIZZ_EXPORTED_API int sfizz_get_sample_quality(sfizz_synth_t* synth, sfizz_process_mode_t mode);

/**
 * @brief Set the default resampling quality.
 *
 * This is the quality setting which the engine uses when the instrument
 * does not use the opcode `sample_quality`. The engine uses distinct
 * default quality settings for live mode and freewheeling mode,
 * which both can be accessed by the means of this function.
 * @since 0.4.0
 *
 * @param      synth    The synth.
 * @param[in]  mode     The processing mode.
 * @param[in]  quality  The desired sample quality, in the range 1 to 10.
 */
SFIZZ_EXPORTED_API void sfizz_set_sample_quality(sfizz_synth_t* synth, sfizz_process_mode_t mode, int quality);

/**
 * @brief Set the global instrument volume.
 * @since 0.2.0
 *
 * @param synth   The synth.
 * @param volume  The new volume.
 */
SFIZZ_EXPORTED_API void sfizz_set_volume(sfizz_synth_t* synth, float volume);

/**
 * @brief Return the global instrument volume.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API float sfizz_get_volume(sfizz_synth_t* synth);

/**
 * @brief Set the number of voices used by the synth.
 *
 * This function takes a lock and disables the callback; prefer calling
 * it out of the RT thread. It can also take a long time to return.
 * If the new number of voices is the same as the current one, it will
 * release the lock immediately and exit.
 * @since 0.2.0
 *
 * @param synth       The synth.
 * @param num_voices  The number of voices.
 */
SFIZZ_EXPORTED_API void sfizz_set_num_voices(sfizz_synth_t* synth, int num_voices);

/**
 * @brief Return the number of voices.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_voices(sfizz_synth_t* synth);

/**
 * @brief Return the number of allocated buffers from the synth.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_buffers(sfizz_synth_t* synth);

/**
 * @brief Get the number of bytes allocated from the synth.
 *
 * Note that this value can be less than the actual memory usage since it only
 * counts the buffer objects managed by sfizz.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API int sfizz_get_num_bytes(sfizz_synth_t* synth);

/**
 * @brief Enable freewheeling on the synth.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_enable_freewheeling(sfizz_synth_t* synth);

/**
 * @brief Disable freewheeling on the synth.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_disable_freewheeling(sfizz_synth_t* synth);

/**
 * @brief Return a comma separated list of unknown opcodes.
 *
 * The caller has to free() the string returned. This function allocates memory,
 * do not call on the audio thread.
 * @since 0.2.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API char* sfizz_get_unknown_opcodes(sfizz_synth_t* synth);

/**
 * @brief Check if the SFZ should be reloaded.
 *
 * Depending on the platform this can create file descriptors.
 * @since 0.2.0
 *
 * @param synth  The synth.
 *
 * @return @true if any included files (including the root file)
           have been modified since the sfz file was loaded, @false otherwise.
 */
SFIZZ_EXPORTED_API bool sfizz_should_reload_file(sfizz_synth_t* synth);

/**
 * @brief Check if the scala file should be reloaded.
 *
 * Depending on the platform this can create file descriptors.
 * @since 0.4.0
 *
 * @param synth  The synth.
 *
 * @return @true if the scala file has been modified since loading.
 */
SFIZZ_EXPORTED_API bool sfizz_should_reload_scala(sfizz_synth_t* synth);

/**
 * @brief Enable logging of timings to sidecar CSV files.
 * @since 0.3.0
 *
 * @note This can produce many outputs so use with caution.
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_enable_logging(sfizz_synth_t* synth);

/**
 * @brief Disable logging.
 * @since 0.3.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_disable_logging(sfizz_synth_t* synth);

/**
 * @brief Enable logging of timings to sidecar CSV files.
 * @since 0.3.2
 *
 * @note This can produce many outputs so use with caution.
 *
 * @param synth  The synth.
 * @param prefix The prefix.
 */
SFIZZ_EXPORTED_API void sfizz_set_logging_prefix(sfizz_synth_t* synth, const char* prefix);

/**
 * @brief Shuts down the current processing, clear buffers and reset the voices.
 * @since 0.3.2
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_all_sound_off(sfizz_synth_t* synth);

/**
 * @brief Add external definitions prior to loading.
 * @since 0.4.0
 *
 * @note  These do not get reset by loading or resetting the synth.
 *        You need to call sfizz_clear_external_definitions() to erase them.
 *
 * @param synth  The synth.
 * @param id     The definition variable name.
 * @param value  The definition value.
 */
SFIZZ_EXPORTED_API void sfizz_add_external_definitions(sfizz_synth_t* synth, const char* id, const char* value);

/**
 * @brief Clears external definitions for the next file loading.
 * @since 0.4.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API void sfizz_clear_external_definitions(sfizz_synth_t* synth);

/**
 * @brief Index out of bound error for the requested CC/key label.
 * @since 0.4.0
 */
#define SFIZZ_OUT_OF_BOUNDS_LABEL_INDEX -1

/**
 * @brief Get the number of key labels registered in the current sfz file.
 * @since 0.4.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API unsigned int sfizz_get_num_key_labels(sfizz_synth_t* synth);

/**
 * @brief Get the key number for the label registered at index label_index.
 * @since 0.4.0
 *
 * @param synth        The synth.
 * @param label_index  The label index.
 *
 * @returns the number or SFIZZ_OUT_OF_BOUNDS_LABEL_INDEX if the index is out of bounds.
 */
SFIZZ_EXPORTED_API int sfizz_get_key_label_number(sfizz_synth_t* synth, int label_index);

/**
 * @brief Get the key text for the label registered at index label_index.
 * @since 0.4.0
 *
 * @param synth        The synth.
 * @param label_index  The label index.
 *
 * @returns the label or @null if the index is out of bounds.
 */
SFIZZ_EXPORTED_API const char * sfizz_get_key_label_text(sfizz_synth_t* synth, int label_index);

/**
 * @brief Get the number of CC labels registered in the current sfz file
 * @since 0.4.0
 *
 * @param synth  The synth.
 */
SFIZZ_EXPORTED_API unsigned int sfizz_get_num_cc_labels(sfizz_synth_t* synth);

/**
 * @brief Get the CC number for the label registered at index label_index.
 * @since 0.4.0
 *
 * @param synth        The synth.
 * @param label_index  The label index.
 *
 * @returns the number or SFIZZ_OUT_OF_BOUNDS_LABEL_INDEX if the index is out of bounds.
 */
SFIZZ_EXPORTED_API int sfizz_get_cc_label_number(sfizz_synth_t* synth, int label_index);

/**
 * @brief Get the CC text for the label registered at index label_index.
 * @since 0.4.0
 *
 * @param synth        The synth.
 * @param label_index  The label index.
 *
 * @returns the label or @null if the index is out of bounds.
 */
SFIZZ_EXPORTED_API const char * sfizz_get_cc_label_text(sfizz_synth_t* synth, int label_index);

/**
 * @addtogroup Messaging
 * @{
 */

/**
 * @brief Client for communicating with the synth engine in either direction
 * @since 0.6.0
 */
typedef struct sfizz_client_t sfizz_client_t;

/**
 * @brief Create a new messaging client
 * @since 0.6.0
 *
 * @param data         The opaque data pointer which is passed to the receiver.
 * @return             The new client.
 */
SFIZZ_EXPORTED_API sfizz_client_t* sfizz_create_client(void* data);

/**
 * @brief Destroy a messaging client
 * @since 0.6.0
 *
 * @param client       The client.
 */
SFIZZ_EXPORTED_API void sfizz_delete_client(sfizz_client_t* client);

/**
 * @brief Get the client data
 * @since 0.6.0
 *
 * @param client       The client.
 * @return             The client data.
 */
SFIZZ_EXPORTED_API void* sfizz_get_client_data(sfizz_client_t* client);

/**
 * @brief Set the function which receives reply messages from the synth engine.
 * @since 0.6.0
 *
 * @param client       The client.
 * @param receive      The pointer to the receiving function.
 */
SFIZZ_EXPORTED_API void sfizz_set_receive_callback(sfizz_client_t* client, sfizz_receive_t* receive);

/**
 * @brief Send a message to the synth engine
 * @since 0.6.0
 *
 * @param synth        The synth.
 * @param client       The client sending the message.
 * @param delay        The delay of the message in the block, in samples.
 * @param path         The OSC address pattern.
 * @param sig          The OSC type tag string.
 * @param args         The OSC arguments, whose number and format is determined the type tag string.
 */
SFIZZ_EXPORTED_API void sfizz_send_message(sfizz_synth_t* synth, sfizz_client_t* client, int delay, const char* path, const char* sig, const sfizz_arg_t* args);

/**
 * @brief Set the function which receives broadcast messages from the synth engine.
 * @since 0.6.0
 *
 * @param synth        The synth.
 * @param broadcast    The pointer to the receiving function.
 * @param data         The opaque data pointer which is passed to the receiver.
 */
SFIZZ_EXPORTED_API void sfizz_set_broadcast_callback(sfizz_synth_t* synth, sfizz_receive_t* broadcast, void* data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
