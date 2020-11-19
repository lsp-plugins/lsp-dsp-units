/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 12 мая 2017 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        Sample::Sample()
        {
            construct();
        }

        Sample::~Sample()
        {
            destroy();
        }

        void Sample::construct()
        {
            vBuffer     = NULL;
            nLength     = 0;
            nMaxLength  = 0;
            nChannels   = 0;
        }

        void Sample::destroy()
        {
    //        lsp_trace("Sample::destroy this=%p", this);
            if (vBuffer != NULL)
            {
    //            lsp_trace("free vBuffer=%p", vBuffer);
                free(vBuffer);
                vBuffer     = NULL;
            }
            nMaxLength      = 0;
            nLength         = 0;
            nChannels       = 0;
        }

        bool Sample::init(size_t channels, size_t max_length, size_t length)
        {
            if (channels <= 0)
                return false;

            // Destroy previous data
            destroy();

            // Allocate new data
            max_length      = align_size(max_length, DEFAULT_ALIGN);    // Make multiple of 4
            float *buf      = reinterpret_cast<float *>(::malloc(max_length * channels * sizeof(float)));
            if (buf == NULL)
                return false;
            dsp::fill_zero(buf, max_length * channels);

            vBuffer         = buf;
            nLength         = length;
            nMaxLength      = max_length;
            nChannels       = channels;
            return true;
        }

        bool Sample::resize(size_t channels, size_t max_length, size_t length)
        {
            if (channels <= 0)
                return false;

            // Allocate new data
            max_length      = align_size(max_length, DEFAULT_ALIGN);    // Make multiple of 4
            float *buf      = reinterpret_cast<float *>(::malloc(max_length * channels * sizeof(float)));
            if (buf == NULL)
                return false;

            // Copy previously allocated data
            if (vBuffer != NULL)
            {
                float *dptr         = buf;
                const float *sptr   = vBuffer;
                size_t to_copy      = (nMaxLength > max_length) ? max_length : nMaxLength;

                // Copy channels
                for (size_t ch=0; ch < channels; ++ch)
                {
                    if (ch < nChannels)
                    {
                        // Copy data and clear data
                        dsp::copy(dptr, sptr, to_copy);
                        dsp::fill_zero(&dptr[to_copy], max_length - to_copy);
                        sptr           += nMaxLength;
                    }
                    else
                        dsp::fill_zero(dptr, max_length);

                    dptr           += max_length;
                }

                // Destroy previously allocated data
                destroy();
            }
            else
                dsp::fill_zero(buf, max_length * channels);

            vBuffer         = buf;
            nLength         = length;
            nMaxLength      = max_length;
            nChannels       = channels;
            return true;
        }

        void Sample::swap(Sample *dst)
        {
            lsp::swap(vBuffer, dst->vBuffer);
            lsp::swap(nMaxLength, dst->nMaxLength);
            lsp::swap(nLength, dst->nLength);
            lsp::swap(nChannels, dst->nChannels);
        }

        void Sample::dump(IStateDumper *v) const
        {
            v->write("vBuffer", vBuffer);
            v->write("nLength", nLength);
            v->write("nMaxLength", nMaxLength);
            v->write("nChannels", nChannels);
        }
    }
} /* namespace lsp */
