/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 сент. 2016 г.
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

#include <lsp-plug.in/dsp-units/filters/FilterBank.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        FilterBank::FilterBank()
        {
            construct();
        }

        FilterBank::~FilterBank()
        {
            destroy();
        }

        void FilterBank::construct()
        {
            vFilters    = NULL;
            vChains     = NULL;
            nItems      = 0;
            nMaxItems   = 0;
            nLastItems  = -1;
            vData       = NULL;
            vBackup     = NULL;
        }

        void FilterBank::destroy()
        {
            if (vData != NULL)
            {
                free_aligned(vData);
                vData       = NULL;
            }

            construct();
        }

        bool FilterBank::init(size_t filters)
        {
            destroy();

            // Calculate data size
            size_t n_banks      = (filters/8) + 3;
            size_t bank_alloc   = align_size(sizeof(dsp::biquad_t), LSP_DSP_BIQUAD_ALIGN) * n_banks;
            size_t chain_alloc  = sizeof(dsp::biquad_x1_t) * filters;
            size_t backup_alloc = sizeof(float) * LSP_DSP_BIQUAD_D_ITEMS * n_banks;

            // Allocate data
            size_t allocate     = bank_alloc + chain_alloc + backup_alloc;
            uint8_t *ptr        = alloc_aligned<uint8_t>(vData, allocate, LSP_DSP_BIQUAD_ALIGN);
            if (ptr == NULL)
                return false;

            // Initialize pointers
            vFilters            = reinterpret_cast<dsp::biquad_t *>(ptr);
            ptr                += bank_alloc;
            vChains             = reinterpret_cast<dsp::biquad_x1_t *>(ptr);
            ptr                += chain_alloc;
            vBackup             = reinterpret_cast<float *>(ptr);
            ptr                += backup_alloc;

            // Update parameters
            nItems              = 0;
            nMaxItems           = filters;
            nLastItems          = -1;

            return true;
        }

        dsp::biquad_x1_t *FilterBank::add_chain()
        {
            if (nItems >= nMaxItems)
                return (nItems <= 0) ? NULL : &vChains[nItems-1];
            return &vChains[nItems++];
        }

        dsp::biquad_x1_t *FilterBank::chain(size_t id)
        {
            return (id < nItems) ? &vChains[id] : NULL;
        }

        void FilterBank::end(bool clear)
        {
            size_t items        = nItems;
            dsp::biquad_x1_t *c = vChains;
            dsp::biquad_t *b    = vFilters;

            // Add 8x filter bank
            while (items >= 8)
            {
                dsp::biquad_x8_t *f = &b->x8;

                f->b0[0]    = c[0].b0;
                f->b0[1]    = c[1].b0;
                f->b0[2]    = c[2].b0;
                f->b0[3]    = c[3].b0;
                f->b0[4]    = c[4].b0;
                f->b0[5]    = c[5].b0;
                f->b0[6]    = c[6].b0;
                f->b0[7]    = c[7].b0;

                f->b1[0]    = c[0].b1;
                f->b1[1]    = c[1].b1;
                f->b1[2]    = c[2].b1;
                f->b1[3]    = c[3].b1;
                f->b1[4]    = c[4].b1;
                f->b1[5]    = c[5].b1;
                f->b1[6]    = c[6].b1;
                f->b1[7]    = c[7].b1;

                f->b2[0]    = c[0].b2;
                f->b2[1]    = c[1].b2;
                f->b2[2]    = c[2].b2;
                f->b2[3]    = c[3].b2;
                f->b2[4]    = c[4].b2;
                f->b2[5]    = c[5].b2;
                f->b2[6]    = c[6].b2;
                f->b2[7]    = c[7].b2;

                f->a1[0]    = c[0].a1;
                f->a1[1]    = c[1].a1;
                f->a1[2]    = c[2].a1;
                f->a1[3]    = c[3].a1;
                f->a1[4]    = c[4].a1;
                f->a1[5]    = c[5].a1;
                f->a1[6]    = c[6].a1;
                f->a1[7]    = c[7].a1;

                f->a2[0]    = c[0].a2;
                f->a2[1]    = c[1].a2;
                f->a2[2]    = c[2].a2;
                f->a2[3]    = c[3].a2;
                f->a2[4]    = c[4].a2;
                f->a2[5]    = c[5].a2;
                f->a2[6]    = c[6].a2;
                f->a2[7]    = c[7].a2;

                c          += 8;
                b          ++;
                items      -= 8;
            }

            // Add 4x filter bank
            if (items & 4)
            {
                dsp::biquad_x4_t *f = &b->x4;

                f->b0[0]    = c[0].b0;
                f->b0[1]    = c[1].b0;
                f->b0[2]    = c[2].b0;
                f->b0[3]    = c[3].b0;

                f->b1[0]    = c[0].b1;
                f->b1[1]    = c[1].b1;
                f->b1[2]    = c[2].b1;
                f->b1[3]    = c[3].b1;

                f->b2[0]    = c[0].b2;
                f->b2[1]    = c[1].b2;
                f->b2[2]    = c[2].b2;
                f->b2[3]    = c[3].b2;

                f->a1[0]    = c[0].a1;
                f->a1[1]    = c[1].a1;
                f->a1[2]    = c[2].a1;
                f->a1[3]    = c[3].a1;

                f->a2[0]    = c[0].a2;
                f->a2[1]    = c[1].a2;
                f->a2[2]    = c[2].a2;
                f->a2[3]    = c[3].a2;

                c          += 4;
                b          ++;
            }

            // Add 2x filter bank
            if (items & 2)
            {
                dsp::biquad_x2_t *f = &b->x2;

                f->b0[0]    = c[0].b0;
                f->b0[1]    = c[1].b0;
                f->b1[0]    = c[0].b1;
                f->b1[1]    = c[1].b1;
                f->b2[0]    = c[0].b2;
                f->b2[1]    = c[1].b2;

                f->a1[0]    = c[0].a1;
                f->a1[1]    = c[1].a1;
                f->a2[0]    = c[0].a2;
                f->a2[1]    = c[1].a2;

                f->p[0]     = 0.0f;
                f->p[1]     = 0.0f;

                c          += 2;
                b          ++;
            }

            // Add 1x filter
            if (items & 1)
            {
                b->x1       = *(c++);
                b          ++;
            }

            // Clear delays if structure has changed
            if ((clear) || (nItems != nLastItems))
                reset();
            nLastItems      = nItems;
        }

        void FilterBank::reset()
        {
            size_t items    = nItems >> 3;
            if (nItems & 4)
                items ++;
            if (nItems & 2)
                items ++;
            if (nItems & 1)
                items ++;

            dsp::biquad_t *b     = vFilters;
            while (items--)
            {
                dsp::fill_zero(b->d, LSP_DSP_BIQUAD_D_ITEMS);
                b++;
            }
        }

        void FilterBank::process(float *out, const float *in, size_t samples)
        {
            size_t items        = nItems;
            dsp::biquad_t *f    = vFilters;

            if (items == 0)
            {
                dsp::copy(out, in, samples);
                return;
            }

            while (items >= 8)
            {
                dsp::biquad_process_x8(out, in, samples, f);
                in         = out;  // actual data for the next chain is in output buffer now
                f         ++;
                items     -= 8;
            }

            if (items & 4)
            {
                dsp::biquad_process_x4(out, in, samples, f);
                in         = out;  // actual data for the next chain is in output buffer now
                f         ++;
            }

            if (items & 2)
            {
                dsp::biquad_process_x2(out, in, samples, f);
                in         = out;  // actual data for the next chain is in output buffer now
                f         ++;
            }

            if (items & 1)
                dsp::biquad_process_x1(out, in, samples, f);
        }

        void FilterBank::impulse_response(float *out, size_t samples)
        {
            // Backup and clean all delays
            dsp::biquad_t *f    = vFilters;
            float *dst          = vBackup;

            size_t items        = nItems >> 3;
            if (nItems & 4)
                items ++;
            if (nItems & 2)
                items ++;
            if (nItems & 1)
                items ++;

            for (size_t i=0; i < items; ++i)
            {
                dsp::copy(dst, f->d, LSP_DSP_BIQUAD_D_ITEMS);
                dsp::fill_zero(f->d, LSP_DSP_BIQUAD_D_ITEMS);
                dst                += LSP_DSP_BIQUAD_D_ITEMS;
                f                  ++;
            }

            // Generate impulse response
            dsp::fill_zero(out, samples);
            out[0]              = 1.0f;
            process(out, out, samples);

            // Restore all delays
            dst                 = vBackup;
            f                   = vFilters;

            for (size_t i=0; i < items; ++i)
            {
                dsp::copy(f->d, dst, LSP_DSP_BIQUAD_D_ITEMS);
                dst                += LSP_DSP_BIQUAD_D_ITEMS;
                f                  ++;
            }
        }

        void FilterBank::dump(IStateDumper *v) const
        {
            size_t ni       = nItems;
            size_t nc       = (ni >> 3) + ((ni >> 2) & 1) + ((ni >> 1) & 1) + (ni & 1);
            dsp::biquad_t *b= vFilters;

            v->begin_array("vFilters", vFilters, nc);
            while (ni >= 8)
            {
                v->begin_object(b, sizeof(dsp::biquad_t));
                {
                    v->writev("b0", b->x8.b0, 8);
                    v->writev("b1", b->x8.b1, 8);
                    v->writev("b2", b->x8.b2, 8);
                    v->writev("a1", b->x8.a1, 8);
                    v->writev("a2", b->x8.a2, 8);
                }
                v->end_object();
                b       ++;
                ni      -= 8;
            }
            if (ni & 4)
            {
                v->begin_object(b, sizeof(dsp::biquad_t));
                {
                    v->writev("b0", b->x4.b0, 4);
                    v->writev("b1", b->x4.b1, 4);
                    v->writev("b2", b->x4.b2, 4);
                    v->writev("a1", b->x4.a1, 4);
                    v->writev("a2", b->x4.a2, 4);
                }
                v->end_object();
                b       ++;
                ni      -= 8;
            }
            if (ni & 2)
            {
                v->begin_object(b, sizeof(dsp::biquad_t));
                {
                    v->writev("b0", b->x2.b0, 2);
                    v->writev("b1", b->x2.b1, 2);
                    v->writev("b2", b->x2.b2, 2);
                    v->writev("a1", b->x2.a1, 2);
                    v->writev("a2", b->x2.a2, 2);
                    v->writev("p", b->x2.p, 2);
                }
                v->end_object();
                b       ++;
                ni      -= 8;
            }
            if (ni & 1)
            {
                v->begin_object(b, sizeof(dsp::biquad_t));
                {
                    v->write("b0", b->x1.b0);
                    v->write("b1", b->x1.b1);
                    v->write("b2", b->x1.b2);
                    v->write("a1", b->x1.a1);
                    v->write("a2", b->x1.a2);
                    v->write("p0", b->x1.p0);
                    v->write("p1", b->x1.p1);
                    v->write("p2", b->x1.p2);
                }
                v->end_object();
                b       ++;
                ni      -= 8;
            }
            v->end_array();

            v->begin_array("vChains", vChains, nItems);
            for (size_t i=0; i<nItems; ++i)
            {
                dsp::biquad_x1_t *bq = &vChains[i];
                v->begin_object(bq, sizeof(dsp::biquad_x1_t));
                {
                    v->write("b0", bq->b0);
                    v->write("b1", bq->b1);
                    v->write("b2", bq->b2);
                    v->write("a1", bq->a1);
                    v->write("a2", bq->a2);
                    v->write("p0", bq->p0);
                    v->write("p1", bq->p1);
                    v->write("p2", bq->p2);
                }
                v->end_object();
            }
            v->end_array();
            v->write("nItems", nItems);
            v->write("nMaxItems", nMaxItems);
            v->write("nLastItems", nLastItems);
            v->write("vBackup", vBackup);
            v->write("vData", vData);
        }

    } /* namespace dspu */
} /* namespace lsp */
