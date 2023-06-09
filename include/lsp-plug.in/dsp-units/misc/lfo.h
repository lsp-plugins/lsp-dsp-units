/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 июн. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_LFO_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_LFO_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * This header defines different kind of normalized LFO functions.
         *
         * Each function gives an LFO amplitude depending on the phase and
         * has the following features:
         *   - When phase is 0 or 1, the return value is 0
         *   - When phase is 0.5, the return value is 1
         *   - The function is symmetric around the phase of value 0.5, that means
         *     that f(0.5 - delta) == f(0.5 + delta)
         *   - Return value for phase less than zero or greater than 1 is unspecified
         */
        namespace lfo
        {
            /**
             * Pointer LFO function
             * @param phase phase argument
             * @return LFO function value
             */
            typedef float (*function_t)(float phase);

            /** Simple triangular function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        triangular(float phase);

            /** Simple sine function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        sine(float phase);

            /** Two-stage sine function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        step_sine(float phase);

            /** The bell interpolated with two symmetric functions
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        cubic(float phase);

            /** Two-stage tooth interpolated with two cubic function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        step_cubic(float phase);

            /** Parabolic function with maximum in (0.5, 1)
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        parabolic(float phase);

            /** Parabolic function with maximum in (0, 0) or (0, 1)
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        rev_parabolic(float phase);

            /** Logarithmic function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        logarithmic(float phase);

            /** Reverse logarithmic function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        rev_logarithmic(float phase);

            /** Square root function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        sqrt(float phase);

            /** Inverse square root function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        rev_sqrt(float phase);

            /** Circular function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        circular(float phase);

            /** Inverse circular function
             * @param phase phase argument
             * @return LFO function value
             */
            LSP_DSP_UNITS_PUBLIC
            float        rev_circular(float phase);

        } /* namespace lfo */
    } /* dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_LFO_H_ */
