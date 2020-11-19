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

        class Sample
        {
            private:
                Sample & operator = (const Sample &);

            private:
                float      *vBuffer;
                size_t      nLength;
                size_t      nMaxLength;
                size_t      nChannels;

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
                inline bool valid() const { return (vBuffer != NULL) && (nChannels > 0) && (nLength > 0) && (nMaxLength > 0); }
                inline size_t length() const { return nLength; }
                inline size_t max_length() const { return nMaxLength; }

                inline float *getBuffer(size_t channel) { return &vBuffer[nMaxLength * channel]; }
                inline const float *getBuffer(size_t channel) const { return &vBuffer[nMaxLength * channel]; }

                inline float *getBuffer(size_t channel, size_t offset) { return &vBuffer[nMaxLength * channel + offset]; }
                inline const float *getBuffer(size_t channel, size_t offset) const { return &vBuffer[nMaxLength * channel + offset]; }

                inline size_t channels() const { return nChannels; };

                /** Set length of sample
                 *
                 * @param length length to set
                 * @return actual length of the sample
                 */
                inline size_t setLength(size_t length)
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
                 * @return if data was successful resized
                 */
                bool resize(size_t channels, size_t max_length, size_t length = 0);

                /**
                 * Swap contents with another sample
                 * @param dst sample to perform swap
                 */
                void swap(Sample *dst);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLE_H_ */
