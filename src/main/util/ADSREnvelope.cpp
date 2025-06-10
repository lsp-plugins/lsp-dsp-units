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

                cv->fEnd        = 0.0f;
                cv->fCurve      = 0.5f;
                cv->enFunction  = ADSR_LINE;
                cv->sLine.fK1   = 0.0f;
                cv->sLine.fK2   = 0.0f;
                cv->sLine.fB2   = 0.0f;
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

        void ADSREnvelope::update_settings()
        {
            if (!(nFlags & F_RECONFIGURE))
                return;

            // TODO: compute curve parameters

            nFlags         &= ~F_RECONFIGURE;
        }

        float ADSREnvelope::compute(float value)
        {
            update_settings();

            // TODO
            return 0.0f;
        }

        void ADSREnvelope::generate(float *dst, float start, float step, size_t count)
        {
            update_settings();

            // TODO
        }

        void ADSREnvelope::apply(float *dst, float start, float step, size_t count)
        {
            update_settings();

            // TODO
        }

        void ADSREnvelope::apply(float *dst, const float *src, float start, float step, size_t count)
        {
            // TODO
        }

        void ADSREnvelope::dump(IStateDumper *v) const
        {
            // TODO
        }

    } /* namespace dspu */
} /* namespace lsp */


