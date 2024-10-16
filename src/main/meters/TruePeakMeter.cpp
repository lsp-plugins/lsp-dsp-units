/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 окт. 2024 г.
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

#include <lsp-plug.in/dsp-units/meters/TruePeakMeter.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BASE_FREQUENCY          = 44100;
        static constexpr size_t TRUE_PEAK_FREQUENCY     = BASE_FREQUENCY * 4;
        static constexpr size_t TRUE_PEAK_LATENCY       = 10;
        static constexpr size_t MAX_BUFFER_TAIL         = TRUE_PEAK_LATENCY * 2 * 8;
        static constexpr size_t BUFFER_SIZE             = 0x1000;

        TruePeakMeter::TruePeakMeter()
        {
            construct();
        }

        TruePeakMeter::~TruePeakMeter()
        {
            destroy();
        }

        void TruePeakMeter::construct()
        {
            nSampleRate     = 0;
            nHead           = 0;
            nTimes          = 0;
            bUpdate         = true;

            pFunc           = NULL;
            pReduce         = NULL;
            vBuffer         = NULL;
            pData           = NULL;
        }

        void TruePeakMeter::destroy()
        {
            if (pData != NULL)
                free_aligned(pData);

            pFunc           = NULL;
            vBuffer         = NULL;
            pData           = NULL;
        }

        void TruePeakMeter::init()
        {
            vBuffer         = alloc_aligned<float>(pData, BUFFER_SIZE + MAX_BUFFER_TAIL, 0x40);
            clear();
        }

        uint8_t TruePeakMeter::calc_oversampling_multiplier(size_t sample_rate)
        {
            if (sample_rate >= TRUE_PEAK_FREQUENCY)
                return 0;
            if (sample_rate * 2 >= TRUE_PEAK_FREQUENCY)
                return 2;
            if (sample_rate * 3 >= TRUE_PEAK_FREQUENCY)
                return 3;
            if (sample_rate * 4 >= TRUE_PEAK_FREQUENCY)
                return 4;
            if (sample_rate * 6 >= TRUE_PEAK_FREQUENCY)
                return 6;

            return 8;
        }

        void TruePeakMeter::set_sample_rate(uint32_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate         = sr;
            bUpdate             = true;
        }

        size_t TruePeakMeter::sample_rate() const
        {
            return nSampleRate;
        }

        void TruePeakMeter::reduce_2x(float *dst, const float *src, size_t count)
        {
            for (size_t i=0; i<count; ++i, src += 2)
                dst[i] = lsp_max(fabsf(src[0]), fabsf(src[1]));
        }

        void TruePeakMeter::reduce_3x(float *dst, const float *src, size_t count)
        {
            for (size_t i=0; i<count; ++i, src += 3)
                dst[i] = lsp_max(fabsf(src[0]), fabsf(src[1]), fabsf(src[2]));
        }

        void TruePeakMeter::reduce_4x(float *dst, const float *src, size_t count)
        {
            for (size_t i=0; i<count; ++i, src += 4)
                dst[i] = lsp_max(fabsf(src[0]), fabsf(src[1]), fabsf(src[2]), fabsf(src[3]));
        }

        void TruePeakMeter::reduce_6x(float *dst, const float *src, size_t count)
        {
            for (size_t i=0; i<count; ++i, src += 6)
                dst[i] = lsp_max(
                    lsp_max(fabsf(src[0]), fabsf(src[1]), fabsf(src[2])),
                    lsp_max(fabsf(src[3]), fabsf(src[4]), fabsf(src[5])));
        }

        void TruePeakMeter::reduce_8x(float *dst, const float *src, size_t count)
        {
            for (size_t i=0; i<count; ++i, src += 8)
                dst[i] = lsp_max(
                    lsp_max(fabsf(src[0]), fabsf(src[1]), fabsf(src[2]), fabsf(src[3])),
                    lsp_max(fabsf(src[4]), fabsf(src[5]), fabsf(src[6]), fabsf(src[7])));
        }

        void TruePeakMeter::update_settings()
        {
            if (!bUpdate)
                return;
            bUpdate             = false;

            uint8_t times       = calc_oversampling_multiplier(nSampleRate);
            if (nTimes == times)
                return;

            nTimes              = times;
            switch (times)
            {
                case 2:
                    pFunc   = dsp::lanczos_resample_2x16bit;
                    pReduce = reduce_2x;
                    break;
                case 3:
                    pFunc   = dsp::lanczos_resample_3x16bit;
                    pReduce = reduce_3x;
                    break;
                case 4:
                    pFunc   = dsp::lanczos_resample_4x16bit;
                    pReduce = reduce_4x;
                    break;
                case 6:
                    pFunc   = dsp::lanczos_resample_6x16bit;
                    pReduce = reduce_6x;
                    break;
                case 8:
                    pFunc   = dsp::lanczos_resample_8x16bit;
                    pReduce = reduce_8x;
                    break;
                default:
                    pFunc   = NULL;
                    pReduce = NULL;
                    break;
            }

            clear();
        }

        void TruePeakMeter::clear()
        {
            nHead               = 0;
            dsp::fill_zero(vBuffer, BUFFER_SIZE + MAX_BUFFER_TAIL);
        }

        void TruePeakMeter::process(float *dst, const float *src, size_t count)
        {
            update_settings();

            // Check that we have nothing to reample
            if (pFunc == NULL)
            {
                dsp::abs2(dst, src, count);
                return;
            }

            // Proceed with oversampling
            for (size_t offset = 0; offset < count; )
            {
                // Ensure that we have enough space for processing
                size_t to_process   = lsp_min(count - offset, BUFFER_SIZE - nHead);
                if (to_process <= 0)
                {
                    dsp::copy(vBuffer, &vBuffer[BUFFER_SIZE], MAX_BUFFER_TAIL);
                    dsp::fill_zero(&vBuffer[MAX_BUFFER_TAIL], BUFFER_SIZE);
                    nHead           = 0;
                    continue;
                }

                // Oversample the signal
                pFunc(&vBuffer[nHead], &src[offset], to_process);
                pReduce(dst, &vBuffer[nHead], to_process);

                // Update pointers
                nHead          += nTimes * to_process;
                offset         += to_process;
            }
        }

        void TruePeakMeter::process(float *buf, size_t count)
        {
            process(buf, buf, count);
        }

        float TruePeakMeter::process_max(const float *src, size_t count)
        {
            update_settings();

            // Check that we have nothing to reample
            if (pFunc == NULL)
                return dsp::abs_max(src, count);

            float result        = 0.0f;

            // Proceed with oversampling
            for (size_t offset = 0; offset < count; )
            {
                // Ensure that we have enough space for processing
                size_t to_process   = lsp_min(count - offset, BUFFER_SIZE - nHead);
                if (to_process <= 0)
                {
                    dsp::copy(vBuffer, &vBuffer[BUFFER_SIZE], MAX_BUFFER_TAIL);
                    dsp::fill_zero(&vBuffer[MAX_BUFFER_TAIL], BUFFER_SIZE);
                    nHead           = 0;
                    continue;
                }

                // Oversample the signal
                pFunc(&vBuffer[nHead], &src[offset], to_process);
                result          = lsp_max(result, dsp::abs_max(&vBuffer[nHead], to_process));

                // Update pointers
                nHead          += nTimes * to_process;
                offset         += to_process;
            }

            return 0.0f;
        }

        size_t TruePeakMeter::latency() const
        {
            return (nTimes != 0) ? TRUE_PEAK_LATENCY : 0;
        }

        void TruePeakMeter::dump(IStateDumper *v) const
        {
            v->write("nSampleRate", nSampleRate);
            v->write("nHead", nHead);
            v->write("nTimes", nTimes);
            v->write("bUpdate", bUpdate);

            v->write("pFunc", pFunc);
            v->write("pReduce", pReduce);
            v->write("vBuffer", vBuffer);
            v->write("pData", pData);
        }

    } /* namespce dspu */
} /* namespace lsp */


