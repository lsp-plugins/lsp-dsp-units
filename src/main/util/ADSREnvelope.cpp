/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 11 июн. 2025 г.
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

#include <lsp-plug.in/dsp-units/util/ADSREnvelope.h>

namespace lsp
{
    namespace dspu
    {
        ADSREnvelope::ADSREnvelope()
        {
            construct();
        }

        ADSREnvelope::~ADSREnvelope()
        {
            destroy();
        }

        void ADSREnvelope::construct()
        {
            for (size_t i=0; i<P_TOTAL; ++i)
            {
                curve_t *cv     = &vCurve[i];

                cv->fTime               = 0.0f;
                cv->fCurve              = 0.5f;
                cv->enFunction          = ADSR_NONE;
                cv->pGenerator          = none_generator;
                cv->sParams.sNone.fK    = 0.0f;
                cv->sParams.sNone.fB    = 0.0f;
            }

            fHoldTime       = 0.0f;
            fBreakLevel     = 0.0f;
            fSustainLevel   = 0.0f;
            nFlags          = F_RECONFIGURE;
        }

        void ADSREnvelope::destroy()
        {
        }


        void ADSREnvelope::set_param(float & param, float value)
        {
            const float normalized = lsp_limit(value, 0.0f, 1.0f);
            if (param == normalized)
                return;

            param           = normalized;
            nFlags         |= F_RECONFIGURE;
        }

        void ADSREnvelope::set_function(function_t & func, function_t value)
        {
            if (func == value)
                return;

            func            = value;
            nFlags         |= F_RECONFIGURE;
        }

        void ADSREnvelope::set_flag(uint32_t flag, bool set)
        {
            bool on                 = nFlags & flag;
            if (set != on)
                nFlags                  = lsp_setflag(nFlags, flag, set) | F_RECONFIGURE;
        }

        void ADSREnvelope::set_curve(part_t part, float time, float curve, function_t func)
        {
            curve_t *cv             = &vCurve[part];

            time                    = lsp_limit(time, 0.0f, 1.0f);
            curve                   = lsp_limit(curve, 0.0f, 1.0f);

            if ((cv->fTime == time) && (cv->fCurve == curve) && (cv->enFunction == func))
                return;

            cv->fTime               = time;
            cv->fCurve              = curve;
            cv->enFunction          = func;
            nFlags                 |= F_RECONFIGURE;
        }

        void ADSREnvelope::set_hold(float time, bool enabled)
        {
            set_param(fHoldTime, time);
            set_flag(F_USE_HOLD, enabled);
        }

        void ADSREnvelope::set_break(float level, bool enabled)
        {
            set_param(fBreakLevel, level);
            set_flag(F_USE_BREAK, enabled);
        }

        inline float ADSREnvelope::limit_range(float t, float prev)
        {
            return lsp_limit(lsp_max(t, prev), 0.0f, 1.0f);
        }

        void ADSREnvelope::configure_curve(curve_t *curve, float x0, float x1, float y0, float y1)
        {
            switch (curve->enFunction)
            {
                case ADSR_LINE:
                {
                    curve->pGenerator       = line_generator;
                    gen_line_t *g           = &curve->sParams.sLine;

                    g->fT1                  = x0;
                    g->fT2                  = 0.5f * (x0 + x1);
                    const float cy          = y0 + curve->fCurve * (y1 - y0);

                    g->fK1                  = (cy - y0) / (g->fT2 - x0);
                    g->fB1                  = y0;

                    g->fK2                  = (y1 - cy) / (x1 - g->fT2);
                    g->fB2                  = cy;
                    break;
                }

                case ADSR_NONE:
                default:
                {
                    curve->pGenerator       = none_generator;
                    gen_none_t *g           = &curve->sParams.sNone;

                    g->fT                   = x0;
                    g->fK                   = (y1 - y0) / (x1 - x0);
                    g->fB                   = y0;
                    break;
                }
            }
        }

        void ADSREnvelope::update_settings()
        {
            if (!(nFlags & F_RECONFIGURE))
                return;

            // Update time ranges
            vCurve[P_ATTACK].fTime  = limit_range(vCurve[P_ATTACK].fTime, 0.0f);
            if (nFlags & F_USE_HOLD)
            {
                fHoldTime               = limit_range(fHoldTime, vCurve[P_ATTACK].fTime);
                vCurve[P_DECAY].fTime   = limit_range(vCurve[P_DECAY].fTime, fHoldTime);
            }
            else
                vCurve[P_DECAY].fTime   = limit_range(vCurve[P_DECAY].fTime, vCurve[P_ATTACK].fTime);

            if (nFlags & F_USE_BREAK)
            {
                vCurve[P_SLOPE].fTime   = limit_range(vCurve[P_SLOPE].fTime, vCurve[P_DECAY].fTime);
                vCurve[P_RELEASE].fTime = limit_range(vCurve[P_RELEASE].fTime, vCurve[P_SLOPE].fTime);
            }
            else
                vCurve[P_RELEASE].fTime = limit_range(vCurve[P_RELEASE].fTime, vCurve[P_DECAY].fTime);

            // Configure curves
            configure_curve(
                &vCurve[P_ATTACK],
                0.0f, vCurve[P_ATTACK].fTime,
                0.0f, 1.0f);
            const float hold        = (nFlags & F_USE_HOLD) ? fHoldTime : vCurve[P_ATTACK].fTime;
            const float decay       = vCurve[P_DECAY].fTime;
            if (nFlags & F_USE_BREAK)
            {
                configure_curve(
                    &vCurve[P_DECAY],
                    hold, decay,
                    1.0f, fBreakLevel);
                configure_curve(
                    &vCurve[P_SLOPE],
                    decay, vCurve[P_SLOPE].fTime,
                    fBreakLevel, fSustainLevel);
            }
            else
            {
                configure_curve(
                    &vCurve[P_DECAY],
                    hold, decay,
                    1.0f, fSustainLevel);
            }

            configure_curve(
                &vCurve[P_RELEASE],
                vCurve[P_RELEASE].fTime, 1.0f,
                fSustainLevel, 0.0f);

            nFlags         &= ~F_RECONFIGURE;
        }

        float ADSREnvelope::do_process(float t)
        {
            if ((t < 0.0f) || (t > 1.0f))
                return 0.0f;

            // Attack
            const curve_t *cv   = &vCurve[P_ATTACK];
            if (t < cv->fTime)
                return cv->pGenerator(t, &cv->sParams);
            const float hold    = (nFlags & F_USE_HOLD) ? fHoldTime : cv->fTime;

            // Hold
            if (t < hold)
                return 1.0f;

            // Decay
            cv                  = &vCurve[P_DECAY];
            if (t < cv->fTime)
                return cv->pGenerator(t, &cv->sParams);

            // Slope
            if (nFlags & F_USE_BREAK)
            {
                cv                  = &vCurve[P_SLOPE];
                if (t < cv->fTime)
                    return cv->pGenerator(t, &cv->sParams);
            }

            // Sustain and Release
            cv          = &vCurve[P_RELEASE];
            return (t < cv->fTime) ? fSustainLevel : cv->pGenerator(t, &cv->sParams);
        }

        float ADSREnvelope::process(float value)
        {
            update_settings();
            return do_process(value);
        }

        void ADSREnvelope::process(float *dst, const float *src, size_t count)
        {
            update_settings();

            for (size_t i=0; i<count; ++i)
                dst[i]      = do_process(src[i]);
        }

        void ADSREnvelope::process_mul(float *dst, const float *src, size_t count)
        {
            update_settings();

            for (size_t i=0; i<count; ++i)
                dst[i]     *= do_process(src[i]);
        }

        float ADSREnvelope::none_generator(float t, const gen_params_t *params)
        {
            const gen_none_t *none = &params->sNone;
            return (t - none->fT) * none->fK + none->fB;
        }

        float ADSREnvelope::line_generator(float t, const gen_params_t *params)
        {
            const gen_line_t *line = &params->sLine;
            return (t < line->fT2) ?
                (t - line->fT1) * line->fK1 + line->fB1 :
                (t - line->fT2) * line->fK2 + line->fB2;
        }

        void ADSREnvelope::generate(float *dst, float start, float step, size_t count)
        {
            update_settings();

            size_t i            = 0;
            float t             = start;

            // Time before attack
            for ( ; (t < 0.0f) && (i < count); t = start + i * step)
                dst[i++]            = 0.0f;

            // Attack segment
            const curve_t *cv   = &vCurve[P_ATTACK];
            for ( ; (t < cv->fTime) && (i < count); t = start + i * step)
                dst[i++]            = cv->pGenerator(t, &cv->sParams);

            // Hold segment
            if (nFlags & F_USE_HOLD)
            {
                for ( ; (t < fHoldTime) && (i < count); t = start + i * step)
                    dst[i++]            = 1.0f;
            }

            // Decay segment
            cv                  = &vCurve[P_DECAY];
            for ( ; (t < cv->fTime) && (i < count); t = start + i * step)
                dst[i++]            = cv->pGenerator(t, &cv->sParams);

            // Slope segment
            if (nFlags & F_USE_BREAK)
            {
                cv                  = &vCurve[P_SLOPE];
                for ( ; (t < cv->fTime) && (i < count); t = start + i * step)
                    dst[i++]            = cv->pGenerator(t, &cv->sParams);
            }

            // Sustain segment
            cv                  = &vCurve[P_RELEASE];
            for ( ; (t < cv->fTime) && (i < count); t = start + i * step)
                dst[i++]            = fSustainLevel;

            // Release segment
            for ( ; (t <= 1.0f) && (i < count); t = start + i * step)
                dst[i++]            = cv->pGenerator(t, &cv->sParams);

            // Time after release
            for ( ; (i < count); t = start + i * step)
                dst[i++]            = 0.0f;
        }

        void ADSREnvelope::generate_mul(float *dst, float start, float step, size_t count)
        {
            update_settings();

            // TODO
        }

        void ADSREnvelope::generate_mul(float *dst, const float *src, float start, float step, size_t count)
        {
            update_settings();

            // TODO
        }

        void ADSREnvelope::dump(IStateDumper *v) const
        {
            // TODO
        }

    } /* namespace dspu */
} /* namespace lsp */


