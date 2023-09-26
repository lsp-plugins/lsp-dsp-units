/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 сент. 2023 г.
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/bits.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/dynamics/AutoGain.h>
#include <lsp-plug.in/dsp-units/units.h>

namespace lsp
{
    namespace dspu
    {
        AutoGain::AutoGain()
        {
            construct();
        }

        AutoGain::~AutoGain()
        {
            destroy();
        }

        void AutoGain::construct()
        {
            nSampleRate     = 0;
            nLookHead       = 0;
            nLookSize       = 0;
            nLookOffset     = 0;
            vLookBack       = NULL;

            sShort.fAttack  = 0.0f;
            sShort.fRelease = 0.0f;
            sShort.fKAttack = 0.0f;
            sShort.fKRelease= 0.0f;
            sLong.fAttack   = 0.0f;
            sLong.fRelease  = 0.0f;
            sLong.fKAttack  = 0.0f;
            sLong.fKRelease = 0.0f;

            sComp.x1        = GAIN_AMP_M_6_DB;
            sComp.x2        = GAIN_AMP_P_6_DB;
            sComp.a         = 0.0f;
            sComp.b         = 0.0f;
            sComp.c         = 0.0f;
            sComp.d         = 0.0f;

            fSilence        = GAIN_AMP_M_72_DB;
//            fDeviation      = GAIN_AMP_P_6_DB;
//            fRevDeviation   = GAIN_AMP_M_6_DB;
            fCurrEnv        = 0.0f;
            fCurrGain       = 1.0f;
            fMinGain        = GAIN_AMP_M_48_DB;
            fMaxGain        = GAIN_AMP_P_48_DB;
            fLookBack       = 0.0f;
            fMaxLookBack    = 0.0f;

            nFlags          = F_UPDATE;
        }

        void AutoGain::destroy()
        {
            if (vLookBack != NULL)
            {
                free(vLookBack);
                vLookBack       = NULL;
            }
        }

        status_t AutoGain::init(float max_lookback)
        {
            destroy();
            fMaxLookBack    = max_lookback;

            return STATUS_OK;
        }

        void AutoGain::set_timing(float *ptr, float value)
        {
            value           = lsp_max(value, 0.0f);
            if (*ptr == value)
                return;

            *ptr            = value;
            nFlags         |= F_UPDATE;
        }

        status_t AutoGain::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return STATUS_OK;

            // Reallocate look-back buffer
            size_t max_lookback = dspu::millis_to_samples(sample_rate, fMaxLookBack);
            size_t buf_len      = round_pow2(max_lookback + 1);

            float *ptr          = static_cast<float *>(realloc(vLookBack, sizeof(float) * buf_len));
            if (ptr == NULL)
                return STATUS_NO_MEM;
            vLookBack           = ptr;
            dsp::fill_zero(vLookBack, buf_len);

            // Update other settings
            nSampleRate         = sample_rate;
            nLookSize           = buf_len;
            nLookHead           = 0;
            nLookOffset         = 0;
            nFlags             |= F_UPDATE;

            return STATUS_OK;
        }

        void AutoGain::set_silence_threshold(float threshold)
        {
            fSilence        = lsp_max(0.0f, threshold);
        }

        void AutoGain::set_deviation(float deviation)
        {
            sComp.x2        = lsp_max(1.0f, deviation);
            sComp.x1        = 1.0f / sComp.x1;
            nFlags         |= F_UPDATE;
        }

        void AutoGain::set_min_gain(float value)
        {
            fMinGain        = lsp_max(0.0f, value);
        }

        void AutoGain::set_max_gain(float value)
        {
            fMaxGain        = lsp_max(1.0f, value);
        }

        void AutoGain::set_gain(float min, float max)
        {
            fMinGain        = lsp_max(0.0f, min);
            fMaxGain        = lsp_max(1.0f, max);
        }

        void AutoGain::set_short_timing(float attack, float release)
        {
            set_timing(&sShort.fAttack, attack);
            set_timing(&sShort.fRelease, release);
        }

        void AutoGain::set_long_timing(float attack, float release)
        {
            set_timing(&sLong.fAttack, attack);
            set_timing(&sLong.fRelease, release);
        }

        void AutoGain::set_lookback(float value)
        {
            set_timing(&fLookBack, value);
        }

        size_t AutoGain::latency() const
        {
            if (nFlags & F_UPDATE)
            {
                float look_back     = lsp_limit(fLookBack, 0.0f, fMaxLookBack);
                return millis_to_samples(nSampleRate, look_back);
            }
            return nLookOffset;
        }

        void AutoGain::update()
        {
            if (!(nFlags & F_UPDATE))
                return;

            sShort.fKAttack     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, sShort.fAttack)));
            sShort.fKRelease    = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, sShort.fRelease)));
            sLong.fKAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, sLong.fAttack)));
            sLong.fKRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, sLong.fRelease)));

            float look_back     = lsp_limit(fLookBack, 0.0f, fMaxLookBack);
            nLookOffset         = millis_to_samples(nSampleRate, look_back);

            calc_compressor();

            nFlags             &= ~size_t(F_UPDATE);
        }

        void AutoGain::calc_compressor()
        {
            float dy    = 1.0f - sComp.x1;
            float dx1   = 1.0f/(sComp.x2 - sComp.x1);
            float dx2   = dx1*dx1;

            sComp.d     = sComp.x1;
            sComp.c     = 1.0f;
            sComp.b     = (3.0f * dy)*dx2 - 2.0f *dx1;
            sComp.a     = (1.0f - (2.0f*dy)*dx1)*dx2;
        }

        float AutoGain::process_sample(float sl, float ss, float le)
        {
            // Do not perform any gain adjustment if we are in silence
            float gain;

            if (ss <= fSilence)
                return fCurrGain;

            vLookBack[nLookHead]    = sl;
            float prev_sl           = vLookBack[(nLookHead + nLookSize - nLookOffset) & (nLookSize - 1)];
            nLookHead               = (nLookHead + 1) & (nLookSize - 1);

            float nl    = sl * fCurrGain;
            float xl    = sl / le;
            float xs    = ss / le;

            if (xl < sComp.x1)
            {
                if ((ss >= sl * sComp.x2) && (sl >= GAIN_AMP_P_3_DB * prev_sl))
                {
                    if (xs <= sComp.x1)
                        gain    = fCurrGain + (le/ss - fCurrGain) * sShort.fKRelease;
                    else if (xs >= sComp.x2)
                        gain    = sComp.x2 / xs;
                    else
                    {
                        float v     = xs - sComp.x1;
                        gain        = (((sComp.a*v + sComp.b)*v + sComp.c)*v + sComp.d) / xl;
                    }
                }
                else
                {
                    float rt    = (nl >= le) ? sLong.fKAttack : sLong.fKRelease;
                    gain        = fCurrGain + (le/sl - fCurrGain) * rt;
                }
            }
            else if (xl <= sComp.x2)
            {
                // Long-time changes
                float rt    = (nl >= le) ? sLong.fKAttack : sLong.fKRelease;
                gain        = fCurrGain + (le/sl - fCurrGain) * rt;
//                gain        = lsp_limit(gain, fMinGain, fMaxGain);
            }
            else // xl > sComp.x2
            {
                gain        = sComp.x2 / lsp_max(xl, xs);
            }

//            // Get the previous sample from look-back buffer
//            vLookBack[nLookHead]    = ss;
//            float prev_ss           = vLookBack[(nLookHead + nLookSize - nLookOffset) & (nLookSize - 1)];
//            nLookHead               = (nLookHead + 1) & (nLookSize - 1);
//
//            float diff              = ss/lsp_max(prev_ss, fSilence);
//            if (diff >= dspu::db_to_gain(9.0f))
//                nFlags                 |= F_SURGE;
//
//            if (nFlags & F_SURGE)
//            {
//                // Sync gain rapidly
//                if (ss <= le * fDeviation)
//                {
//                    // Reset surge state if levels became almost equal
//                    gain            = fCurrGain + (ss - fCurrGain) * sShort.fKRelease;
//                    if ((diff <= 1.0f) && (ss <= sl))
//                        nFlags &= ~F_SURGE;
//                }
//                else
//                    gain            = (le * fDeviation) / ss;
//            }
//            else
//            {
//                // Long-time changes
//                if (sl < le * fDeviation)
//                {
//                    r_time              = (sl >= le) ? sLong.fKAttack : sLong.fKRelease;
//                    gain                = fCurrGain + (le/sl - fCurrGain) * r_time;
//                }
//                else
//                    gain                = le / sl;
//
//                gain                = lsp_limit(gain, fMinGain, fMaxGain);
//            }

            return fCurrGain = gain;

//
//            fCurrEnv            = (ss >= fCurrEnv) ? ss : fCurrEnv + (ss - fCurrEnv) * sShort.fKRelease;
//            float xsl           = fCurrEnv * fCurrGain;
//            float up_th         = le * fDeviation;
//            float down_th       = le * fRevDeviation;
//            if (xsl >= up_th)
//                gain                = up_th / xsl;
//            else if ((sl <= up_th) && (sl >= down_th))
//            {
//                // Long-time changes
//                r_time              = (sl >= le) ? sLong.fKAttack : sLong.fKRelease;
//                gain                = fCurrGain + (le/sl - fCurrGain) * r_time;
//            }
//            else
//                gain                = fCurrGain + (le / fCurrEnv - fCurrEnv) * sShort.fKAttack;


//            // Detemine how to deal with short-time loudness
//            if ((sl >= le * fDeviation) || (sl <= le * fRevDeviation))
//            {
//                // Short-time changes
//                r_time              = (ss >= le) ? sShort.fAttack : sShort.fKRelease;
//                gain                = fCurrGain + (le/ss - fCurrGain) * r_time;
//                return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
//            }
//
//            // Long-time changes
//            r_time              = (sl >= le) ? sLong.fKAttack : sLong.fKRelease;
//            gain                = fCurrGain + (le/sl - fCurrGain) * r_time;
//            return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, const float *lexp, size_t count)
        {
            update();

            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp[i]);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, float lexp, size_t count)
        {
            update();

            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp);
        }

        void AutoGain::dump(IStateDumper *v) const
        {
        }

    } /* namespace dspu */
} /* namespace lsp */



