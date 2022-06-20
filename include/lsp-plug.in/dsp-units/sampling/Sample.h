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
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/mm/IOutAudioStream.h>
#include <lsp-plug.in/mm/IInAudioStream.h>

#define AUDIO_SAMPLE_CONTENT_TYPE       "application/x-lsp-audio-sample"

namespace lsp
{
    namespace dspu
    {
    #pragma pack(push, 1)
        typedef struct sample_header_t
        {
            uint16_t    version;        // Version + endianess
            uint16_t    channels;
            uint32_t    sample_rate;
            uint32_t    samples;
        } sample_header_t;
    #pragma pack(pop)

        enum sample_normalize_t
        {
            /**
             * No normalization
             */
            SAMPLE_NORM_NONE,

            /**
             * Normalize if maximum peak is above threshold
             */
            SAMPLE_NORM_ABOVE,

            /**
             * Normalize if maximum peak is below threshold
             */
            SAMPLE_NORM_BELOW,

            /**
             * Normalize in any case
             */
            SAMPLE_NORM_ALWAYS
        };

        class Sample
        {
            private:
                Sample & operator = (const Sample &);
                Sample(const Sample &);

            private:
                float      *vBuffer;
                size_t      nSampleRate;
                size_t      nLength;
                size_t      nMaxLength;
                size_t      nChannels;

            protected:
                status_t            fast_downsample(Sample *s, size_t new_sample_rate);
                status_t            fast_upsample(Sample *s, size_t new_sample_rate);
                status_t            complex_downsample(Sample *s, size_t new_sample_rate);
                status_t            complex_upsample(Sample *s, size_t new_sample_rate);

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

            public:
                inline bool         valid() const                   { return (vBuffer != NULL) && (nChannels > 0) && (nLength > 0) && (nMaxLength > 0); }
                inline size_t       length() const                  { return nLength; }
                inline size_t       max_length() const              { return nMaxLength; }

                inline float       *getBuffer(size_t channel)       { return &vBuffer[nMaxLength * channel]; }
                inline const float *getBuffer(size_t channel) const { return &vBuffer[nMaxLength * channel]; }

                inline float       *channel(size_t channel)         { return &vBuffer[nMaxLength * channel]; }
                inline const float *channel(size_t channel) const   { return &vBuffer[nMaxLength * channel]; }

                inline float       *getBuffer(size_t channel, size_t offset) { return &vBuffer[nMaxLength * channel + offset]; }
                inline const float *getBuffer(size_t channel, size_t offset) const { return &vBuffer[nMaxLength * channel + offset]; }

                inline size_t       channels() const                { return nChannels;     }
                inline size_t       samples() const                 { return nLength;       }

                inline size_t       sample_rate() const             { return nSampleRate;   }
                inline void         set_sample_rate(size_t srate)   { nSampleRate = srate;  }

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
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLE_H_ */
