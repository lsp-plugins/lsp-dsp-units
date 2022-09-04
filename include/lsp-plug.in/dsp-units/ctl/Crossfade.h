/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 22 янв. 2020 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_CROSSFADE_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_CROSSFADE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC Crossfade
        {
            private:
                Crossfade & operator = (const Crossfade &);
                Crossfade(const Crossfade &);

            protected:
                size_t      nSamples;
                size_t      nCounter;
                float       fDelta;
                float       fGain;

            public:
                explicit Crossfade();
                ~Crossfade();

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Destroy object
                 */
                void            destroy();

            public:
                /**
                 * Initialize crossfade
                 * @param sample_rate sample rate of the signal
                 * @param time crossfade time, by default 5 msec
                 */
                void            init(int sample_rate, float time = 0.005f);

                /**
                 * Crossfade the signal
                 * @param dst destination buffer
                 * @param fade_out the signal that will fade out, may be NULL
                 * @param fade_in the signal that will fade in, may be NULL
                 * @param count number of samples to process
                 */
                void            process(float *dst, const float *fade_out, const float *fade_in, size_t count);
    
                /**
                 * Return the remaining number of samples to process
                 * @return the remaining number of samples to process before crossfade becomes inactive
                 */
                inline size_t   remaining() const { return nCounter; }

                /**
                 * Check if crossfade is currently active
                 * @return true if crossfade is currently active
                 */
                inline bool     active() const { return nCounter > 0; }

                /**
                 * Reset the crossfade state, immediately interrupt it's processing
                 */
                void            reset();

                /**
                 * Toggle crossfade processing
                 * @return true if crossfade has been toggled, false
                 * if crossfade is currently active
                 */
                bool            toggle();

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_CROSSFADE_H_ */
