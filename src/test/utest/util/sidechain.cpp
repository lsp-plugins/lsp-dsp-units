/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 7 нояб. 2022 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/util/Sidechain.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/helpers.h>


#define SRATE       48000u
#define BUF_SIZE    (SRATE * 4)
#define BLOCK_SIZE  511u

UTEST_BEGIN("dspu.util", sidechain)

    UTEST_MAIN
    {
        static const dspu::sidechain_mode_t modes[] =
        {
            dspu::SCM_PEAK,
            dspu::SCM_LPF,
            dspu::SCM_RMS,
            dspu::SCM_UNIFORM
        };
        static const dspu::sidechain_source_t sources[] =
        {
            dspu::SCS_MIDDLE,
            dspu::SCS_SIDE,
            dspu::SCS_LEFT,
            dspu::SCS_RIGHT,
            dspu::SCS_AMIN,
            dspu::SCS_AMAX
        };

        static const dspu::sidechain_stereo_mode_t scmodes[] =
        {
            dspu::SCSM_STEREO,
            dspu::SCSM_MIDSIDE
        };

        FloatBuffer out(BUF_SIZE), a(BUF_SIZE), b(BUF_SIZE);
        out.randomize_sign();
        a.randomize_sign();
        b.randomize_sign();

        dspu::Sidechain sc;

        for (size_t channels = 1; channels <= 2; ++channels)
        {
            UTEST_ASSERT(sc.init(channels, 50.0f));
            sc.set_sample_rate(SRATE);

            for (size_t mode=0; mode < sizeof(modes)/sizeof(modes[0]); ++mode)
            {
                sc.set_mode(modes[mode]);

                for (size_t source=0; source < sizeof(sources)/sizeof(sources[0]); ++source)
                {
                    sc.set_source(sources[source]);

                    for (size_t scmode=0; scmode < sizeof(scmodes)/sizeof(scmodes[0]); ++scmode)
                    {
                        sc.set_stereo_mode(scmodes[scmode]);

                        float *src[2], *dst;
                        src[0]  = a.data();
                        src[1]  = b.data();
                        dst     = out.data();

                        for (size_t i=0; i<BUF_SIZE; )
                        {
                            size_t count    = lsp_min(BUF_SIZE - i, BLOCK_SIZE);
                            sc.process(dst, const_cast<const float **>(src), count);

                            dst            += count;
                            src[0]         += count;
                            src[1]         += count;
                            i              += count;
                        }

                        // Validate buffers
                        if (out.corrupted())
                        {
                            UTEST_FAIL_MSG(
                                "Output buffer corrupted channels=%d, mode=%d, source=%d, scmode=%d",
                                int(channels), int(mode), int(source), int(scmode));
                        }
                        if (a.corrupted())
                        {
                            UTEST_FAIL_MSG(
                                "First buffer corrupted channels=%d, mode=%d, source=%d, scmode=%d",
                                int(channels), int(mode), int(source), int(scmode));
                        }
                        if (b.corrupted())
                        {
                            UTEST_FAIL_MSG(
                                "First buffer corrupted channels=%d, mode=%d, source=%d, scmode=%d",
                                int(channels), int(mode), int(source), int(scmode));
                        }
                    } /* scmode */
                } /* source */
            } /* mode */
        } /* channels */

        sc.destroy();
    }

UTEST_END



