/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 27 Jun 2021
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

#ifndef LSP_PLUG_IN_DSP_UNITS_NOISE_VELVET_H_
#define LSP_PLUG_IN_DSP_UNITS_NOISE_VELVET_H_

#include <lsp-plug.in/dsp-units/util/Randomizer.h>
#include <lsp-plug.in/dsp-units/noise/MLS.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum vn_core_t
        {
            VN_CORE_MLS, // Only compatible with OVN, OVNA and ARN (not crushed)
            VN_CORE_LCG
        };

        enum vn_velvet_type_t
        {
            VN_VELVET_OVN,
            VN_VELVET_OVNA,
            VN_VELVET_ARN,
            VN_VELVET_TRN
        };

        /** As in GENERALIZATIONS OF VELVET NOISE AND THEIR USE IN 1-BIT MUSIC by Kurt James Werner
         * Link: https://dafx2019.bcu.ac.uk/papers/DAFx2019_paper_53.pdf
         * Archive: https://web.archive.org/web/20210711144324/https://dafx2019.bcu.ac.uk/papers/DAFx2019_paper_53.pdf
         *
         * Modified to use MLS samples for OVN, OVNA and ARN (not crushed).
         */
        class LSP_DSP_UNITS_PUBLIC Velvet
        {
            private:
                Velvet & operator = (const Velvet &);
                Velvet(const Velvet &);

            protected:
                typedef struct crush_t
                {
                    bool    bCrush;
                    float   fCrushProb;
                } crush_t;

            private:
                Randomizer          sRandomizer;

                MLS                 sMLS;

                vn_core_t           enCore;

                vn_velvet_type_t    enVelvetType;

                crush_t             sCrushParams;

                float               fWindowWidth;
                float               fARNdelta;
                float               fAmplitude;
                float               fOffset;

            public:
                explicit Velvet();
                ~Velvet();

                void construct();
                void destroy();

            protected:

                /** Get a random value in [0, 1) using the Randomizer.
                 *
                 */
                float get_random_value();

                /** Get a random spike in [-1, 1] using the core generator.
                 *
                 */
                float get_spike();

                /** Get a crushed spike in [-1, 1] using the core generator.
                 *
                 */
                float get_crushed_spike();

                void do_process(float *dst, size_t count);

            public:

                /** Initialise the velvet generator.
                 *
                 * @param seed seed for the generator.
                 */
                void init(uint32_t randseed, uint8_t mlsnbits, MLS::mls_t mlsseed);

                /** Initialise the velvet generator with defaults:
                 *
                 * Time as seed for randomizer, Max size and max seed for the MLS.
                 */
                void init();

                /** Set the core generator for velvet noise.
                 *
                 * @param core generator for the random sequence.
                 */
                void set_core_type(vn_core_t core);

                /** Set the type of the velvet noise.
                 *
                 * @param type velvet noise type.
                 */
                void set_velvet_type(vn_velvet_type_t type);

                /** Set velvet noise window width in samples.
                 *
                 * @param width velvet noise width.
                 */
                void set_velvet_window_width(float width);

                /** Set delta value for ARN generator.
                 *
                 * @param delta delta value.
                 */
                void set_delta_value(float delta);

                /** Set the velvet noise amplitude.
                 *
                 * @param amplitude noise amplitude.
                 */
                void set_amplitude(float amplitude);

                /** Set the velvet noise offset.
                 *
                 * @param offset noise offset.
                 */
                void set_offset(float offset);

                /** Set whether to produce crushed noise.
                 *
                 * @param crush true to activate crushed noise.
                 */
                void set_crush(bool crush);

                /** Set the crushing probability.
                 *
                 * @param prob crushing probability.
                 */
                void set_crush_probability(float prob);

                /** Output velvet noise to the destination buffer in additive mode.
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output velvet noise to the destination buffer in multiplicative mode.
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output velvet noise to a destination buffer overwriting its content.
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULLL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_NOISE_VELVET_H_ */
