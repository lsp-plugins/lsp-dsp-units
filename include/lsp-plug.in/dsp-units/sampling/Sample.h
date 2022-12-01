/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 12 мая 2017 г.
 *
 * lsp-dsp-units is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-dsp-units is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-dsp-units. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLE_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/sampling/types.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/mm/IOutAudioStream.h>
#include <lsp-plug.in/mm/IInAudioStream.h>

#define AUDIO_SAMPLE_CONTENT_TYPE       "application/x-lsp-audio-sample"

namespace lsp
{
    namespace dspu
    {
        /**
         * An audio sample: audio data in PCM format that stores each audio channel in
         * a separate sequential part of the memory as 32-bit float values.
         */
        class LSP_DSP_UNITS_PUBLIC Sample
        {
            private:
                Sample & operator = (const Sample &);
                Sample(const Sample &);

            private:
                float              *vBuffer;        // Sample data
                size_t              nSampleRate;    // Sample rate
                size_t              nLength;        // Current length
                size_t              nMaxLength;     // Maximum possible length
                size_t              nChannels;      // Number of channels
                size_t              nGcRefs;        // GC stuff: Number of references
                Sample             *pGcNext;        // GC stuff: Pointer to the next

            protected:
                static void         put_chunk_linear(float *dst, const float *src, size_t len, size_t fade_in, size_t fade_out);
                static void         put_chunk_const_power(float *dst, const float *src, size_t len, size_t fade_in, size_t fade_out);

                typedef void        (*put_chunk_t)(float *dst, const float *src, size_t len, size_t fade_in, size_t fade_out);

            protected:
                status_t            fast_downsample(Sample *s, size_t new_sample_rate);
                status_t            fast_upsample(Sample *s, size_t new_sample_rate);
                status_t            complex_downsample(Sample *s, size_t new_sample_rate);
                status_t            complex_upsample(Sample *s, size_t new_sample_rate);
                status_t            do_simple_stretch(size_t new_length, size_t start, size_t end, put_chunk_t put_chunk);
                status_t            do_single_crossfade_stretch(size_t new_length, size_t fade_len, size_t start, size_t end, put_chunk_t put_chunk);
                status_t            open_stream_ext(mm::IInAudioStream **is, const io::Path *path);
                status_t            try_open_regular_file(mm::IInAudioStream **is, const io::Path *path);
                status_t            try_open_lspc(mm::IInAudioStream **is, const io::Path *lspc, const io::Path *item);

            public:
                explicit Sample();
                ~Sample();

                /**
                 * Create uninitialied sample
                 */
                void        construct();

                /** Drop sample contents
                 *
                 */
                void        destroy();

            public: // Garbage-collected stuff
                /**
                 * Get number of references
                 * @return number of references
                 */
                inline size_t       gc_references() const   { return nGcRefs;     }

                /**
                 * Incremente reference counter
                 * @return new number of references
                 */
                inline size_t       gc_acquire()            { return ++nGcRefs;   }

                /**
                 * Decrement reference counter
                 * @return new number of references
                 */
                inline size_t       gc_release()            { return --nGcRefs;   }

                /**
                 * Get pointer to the next sample in the single-directional garbage list
                 * @return next sample reference in the garbage list
                 */
                inline Sample      *gc_next()               { return pGcNext;     }

                /**
                 * Link sample to the next in the garbage list
                 * @param next pointer to next sample
                 * @return previously stored pointer to next sample
                 */
                Sample             *gc_link(Sample *next);


            public: // Regular suff
                inline bool         valid() const                   { return (vBuffer != NULL) && (nChannels > 0) && (nLength > 0) && (nMaxLength > 0); }
                inline size_t       max_length() const              { return nMaxLength; }

                inline float       *getBuffer(size_t channel)       { return &vBuffer[nMaxLength * channel]; }
                inline const float *getBuffer(size_t channel) const { return &vBuffer[nMaxLength * channel]; }

                inline float       *channel(size_t channel)         { return &vBuffer[nMaxLength * channel]; }
                inline const float *channel(size_t channel) const   { return &vBuffer[nMaxLength * channel]; }

                inline float       *getBuffer(size_t channel, size_t offset) { return &vBuffer[nMaxLength * channel + offset]; }
                inline const float *getBuffer(size_t channel, size_t offset) const { return &vBuffer[nMaxLength * channel + offset]; }

                /**
                 * Return number of audio channels
                 * @return number of audio channels
                 */
                inline size_t       channels() const                { return nChannels;     }

                /**
                 * Get sample length in samples
                 * @return sample length in samples
                 */
                inline size_t       samples() const                 { return nLength;       }
                inline size_t       length() const                  { return nLength;       }

                /**
                 * Return the sample duration in seconds, available only if sample rate is specified
                 * @return sample duration in seconds
                 */
                inline double       duration() const                { return (nSampleRate > 0) ? double(nLength) / double(nSampleRate) : 0.0; }

                /**
                 * Get sample rate
                 * @return actual sample rate
                 */
                inline size_t       sample_rate() const             { return nSampleRate;   }

                /**
                 * Set sample rate
                 * @param srate sample rate of the sample
                 */
                inline void         set_sample_rate(size_t srate)   { nSampleRate = srate;  }

                /**
                 * Copy sample contents
                 * @param s source sample to perform copy
                 * @return status of operation
                 */
                status_t            copy(const Sample *s);
                inline status_t     copy(const Sample &s)           { return copy(&s);      }

                /** Set length of sample
                 *
                 * @param length length to set
                 * @return actual length of the sample
                 */
                inline size_t set_length(size_t length)
                {
                    if (length > nMaxLength)
                        length = nMaxLength;
                    return nLength = length;
                }

                /** Extend length of sample
                 *
                 * @param length length to extend
                 * @return actual length of the sample
                 */
                inline size_t extend(size_t length)
                {
                    if (length > nMaxLength)
                        length = nMaxLength;
                    if (nLength < length)
                        nLength = length;
                    return nLength;
                }

                /** Clear sample (make length equal to zero
                 *
                 */
                inline void clear()
                {
                    nLength     = 0;
                }

                /** Initialize sample, all previously allocated data will be lost
                 *
                 * @param channels number of channels
                 * @param max_length maximum possible sample length
                 * @param length initial sample length
                 * @return true if data was successful allocated
                 */
                bool init(size_t channels, size_t max_length, size_t length = 0);

                /** Resize sample, all previously allocated data will be kept
                 *
                 * @param channels number of channels
                 * @param max_length maximum possible sample length
                 * @param length initial sample length
                 * @return true if data was successful resized
                 */
                bool resize(size_t channels, size_t max_length, size_t length = 0);

                /** Stretch part of the sample
                 *
                 * @param new_length the new length of the stretched region in samples
                 * @param chunk_size chunk size in samples, 0 means automatic chunk size selection
                 * @param fade_type the crossfade type between chunks
                 * @param fade_size the relative size of the crossfade region between two chunks in range of 0 to 1
                 * @param start the number of the sample associated with the start of the range to be stretched
                 * @param end the number of the first sample after the end of the range to be stretched
                 * @return status of operation
                 */
                status_t stretch(
                    size_t new_length, size_t chunk_size,
                    sample_crossfade_t fade_type, float fade_size,
                    size_t start, size_t end);

                /** Stretch the whole sample
                 *
                 * @param new_length the new length of the sample in samples
                 * @param chunk_size chunk size in samples, 0 means automatic chunk size selection
                 * @param fade_type the crossfade type between chunks
                 * @param fade_size the relative size of the crossfade region between two chunks in range of 0 to 1
                 * @return true if data was successfuly stretched
                 */
                status_t stretch(size_t new_length, size_t chunk_size, sample_crossfade_t fade_type, float fade_size);

                /** Resize sample to match the specified number of audio channels,
                 * all previously allocated data will be kept
                 *
                 * @param channels number of channels
                 */
                bool set_channels(size_t channels);

                /** Resample sample
                 *
                 * @param new_sample_rate new sample rate
                 * @return status of operation
                 */
                status_t resample(size_t new_sample_rate);

                /** Reverse track
                 *
                 * @param channel channel to reverse
                 * @return true on success
                 */
                bool reverse(size_t channel);

                /** Reverse sample
                 *
                 * @return true on success
                 */
                void reverse();

                /**
                 * Normalize the sample
                 * @param gain the maximum peak gain
                 * @param mode the normalization mode
                 */
                void normalize(float gain, sample_normalize_t mode);

                /**
                 * Swap contents with another sample
                 * @param dst sample to perform swap
                 */
                void swap(Sample *dst);
                inline void swap(Sample &dst)                   { swap(&dst);                           }

                /**
                 * Save sample contents to file
                 * @param path path to the file
                 * @param offset first sample to save
                 * @param count maximum number of samples to save, all available if negative
                 * @return actual number of samples written or negative error code
                 */
                ssize_t save_range(const char *path, size_t offset, ssize_t count = -1);
                ssize_t save_range(const LSPString *path, size_t offset, ssize_t count = -1);
                ssize_t save_range(const io::Path *path, size_t offset, ssize_t count = -1);
                ssize_t save_range(mm::IOutAudioStream *out, size_t offset, ssize_t count = -1);

                /**
                 * Save sample contents to file
                 * @param path path to the file
                 * @return actual number of samples written or negative error code
                 */
                inline ssize_t save(const char *path)           { return save_range(path, 0, nLength);  }
                inline ssize_t save(const LSPString *path)      { return save_range(path, 0, nLength);  }
                inline ssize_t save(const io::Path *path)       { return save_range(path, 0, nLength);  }
                ssize_t save(mm::IOutAudioStream *out)          { return save_range(out, 0, nLength);   }

                /**
                 * Load file
                 * @param path location of the file
                 * @param max_duration maximum duration in seconds
                 * @return status of operation
                 */
                status_t load(const char *path, float max_duration = -1);
                status_t load(const LSPString *path, float max_duration = -1);
                status_t load(const io::Path *path, float max_duration = -1);
                status_t load(mm::IInAudioStream *in, float max_duration = -1);

                /**
                 * Load file
                 * @param path location of the file
                 * @param max_samples maximum number of samples
                 * @return status of operation
                 */
                status_t loads(const char *path, ssize_t max_samples = -1);
                status_t loads(const LSPString *path, ssize_t max_samples = -1);
                status_t loads(const io::Path *path, ssize_t max_samples = -1);
                status_t loads(mm::IInAudioStream *in, ssize_t max_samples = -1);

                /**
                 * Extended load file: if no success, try to locate file in the archive like LSPC, etc.
                 * @param path path to the file
                 * @param max_duration maximum duration in seconds
                 * @return status of operation
                 */
                status_t load_ext(const char *path, float max_duration = -1);
                status_t load_ext(const LSPString *path, float max_duration = -1);
                status_t load_ext(const io::Path *path, float max_duration = -1);

                /**
                 * Extended load file: if no success, try to locate file in the archive like LSPC, etc.
                 * @param path location of the file
                 * @param max_samples maximum number of samples
                 * @return status of operation
                 */
                status_t loads_ext(const char *path, ssize_t max_samples = -1);
                status_t loads_ext(const LSPString *path, ssize_t max_samples = -1);
                status_t loads_ext(const io::Path *path, ssize_t max_samples = -1);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLE_H_ */
