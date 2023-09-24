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

#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp-units/dynamics/AutoGain.h>

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

            sShort.fAttack  = 0.0f;
            sShort.fRelease = 0.0f;
            sShort.fKAttack = 0.0f;
            sShort.fKRelease= 0.0f;
            sLong.fAttack   = 0.0f;
            sLong.fRelease  = 0.0f;
            sLong.fKAttack  = 0.0f;
            sLong.fKRelease = 0.0f;

            fSilence        = GAIN_AMP_M_72_DB;
            fDeviation      = GAIN_AMP_P_6_DB;
            fRevDeviation   = GAIN_AMP_M_6_DB;
            fCurrGain       = 1.0f;
            fMinGain        = GAIN_AMP_M_48_DB;
            fMaxGain        = GAIN_AMP_P_48_DB;

            bUpdate         = true;
        }

        void AutoGain::destroy()
        {
        }

        void AutoGain::set_timing(float *ptr, float value)
        {
            value           = lsp_max(value, 0.0f);
            if (*ptr == value)
                return;

            *ptr            = value;
            bUpdate         = true;
        }

        void AutoGain::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return;

            nSampleRate     = sample_rate;
            bUpdate         = true;
        }

        void AutoGain::set_silence_threshold(float threshold)
        {
            fSilence        = lsp_max(0.0f, threshold);
        }

        void AutoGain::set_deviation(float deviation)
        {
            fDeviation      = lsp_max(1.0f, deviation);
            fRevDeviation   = 1.0f / fDeviation;
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

        void AutoGain::update()
        {
            if (!bUpdate)
                return;

            constexpr float t_const = -4.60517018599f; // log(0.01)

            float min_time      = 1.0f / nSampleRate;
            float ktime         = t_const * 2000.0f / nSampleRate;

            sShort.fKAttack     = expf(ktime / lsp_max(min_time, sShort.fAttack * 1000.0f));
            sShort.fKRelease    = expf(ktime / lsp_max(min_time, sShort.fRelease * 1000.0f));
            sLong.fKAttack      = expf(ktime / lsp_max(min_time, sLong.fAttack * 1000.0f));
            sLong.fKRelease     = expf(ktime / lsp_max(min_time, sLong.fRelease * 1000.0f));

            bUpdate             = false;
        }

        float AutoGain::process_sample(float sl, float ss, float le)
        {
            // Do not perform any gain adjustment if we are in silence
            float gain;
            if (ss <= fSilence)
                return fCurrGain    = lsp_limit(fCurrGain, fMinGain, fMaxGain);

            // Short-time changes
            float nss           = ss * fCurrGain;
            if (nss >= fDeviation * le)
            {
                gain                = fCurrGain * sShort.fKAttack + (nss / le) * (1.0f - sShort.fKAttack);
                return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
            }
            else if (nss <= fRevDeviation * le)
            {
                gain                = fCurrGain * sShort.fKRelease + (le / nss) * (1.0f - sShort.fKRelease);
                return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
            }

            // Long-time changes
            float nsl           = sl * fCurrGain;
            if (nsl >= le)
            {
                gain                = fCurrGain * sLong.fKAttack + (nsl / le) * (1.0f - sLong.fKAttack);
                return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
            }

            gain                = fCurrGain * sLong.fKRelease + (le / nsl) * (1.0f - sLong.fKRelease);
            return fCurrGain    = lsp_limit(gain, fMinGain, fMaxGain);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, const float *lexp, size_t count)
        {
            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp[i]);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, float lexp, size_t count)
        {
            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp);
        }

        void AutoGain::dump(IStateDumper *v) const
        {
        }

    } /* namespace dspu */
} /* namespace lsp */



