/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 29 авг. 2026 г.
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
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/dsp-units/misc/fft_crossover.h>
#include <lsp-plug.in/dsp-units/util/LPCrossover.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        constexpr size_t temp_buf_size      = 0x200;

        LPCrossover::LPCrossover()
        {
            construct();
        }

        LPCrossover::~LPCrossover()
        {
            destroy();
        }

        void LPCrossover::construct()
        {
            sSplitter.construct();

            nSampleRate     = LSP_DSP_UNITS_DEFAULT_SAMPLE_RATE;
            nPlanSize       = 0;
            nReconfigure    = R_ALL;
            vSplits         = NULL;
            vBands          = NULL;
            vPlan           = NULL;
            vBuffer         = NULL;

            pData           = NULL;
        }

        void LPCrossover::destroy()
        {
            sSplitter.destroy();

            nSampleRate     = LSP_DSP_UNITS_DEFAULT_SAMPLE_RATE;
            nPlanSize       = 0;
            nReconfigure    = R_ALL;
            vSplits         = NULL;
            vBands          = NULL;
            vBuffer         = NULL;
            vPlan           = NULL;

            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }
        }

        status_t LPCrossover::init(size_t max_rank, size_t bands)
        {
            if ((max_rank == sSplitter.max_rank()) &&
                (bands == sSplitter.handlers()))
                return STATUS_OK;

            status_t res    = sSplitter.init(max_rank, bands);
            if (res != STATUS_OK)
                return res;

            nSampleRate     = 0;
            nPlanSize       = 0;
            nReconfigure    = R_ALL;
            vSplits         = NULL;
            vBands          = NULL;
            vPlan           = NULL;

            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }

            const size_t splits         = bands - 1;
            const size_t bins           = 1 << max_rank;
            const size_t szof_splits    = align_size(sizeof(split_t) * splits, DEFAULT_ALIGN);
            const size_t szof_bands     = align_size(sizeof(band_t) * bands, DEFAULT_ALIGN);
            const size_t szof_plan      = align_size(sizeof(split_t *) * splits, DEFAULT_ALIGN);
            const size_t szof_buffer    = align_size(sizeof(float) * bins, DEFAULT_ALIGN);
            const size_t szof_temp      = align_size(sizeof(float) * lsp_max(bins, temp_buf_size * 2), DEFAULT_ALIGN);
            const size_t to_alloc       = szof_splits + szof_bands + szof_plan + bands * szof_buffer + szof_temp;

            // Allocate the data
            uint8_t *ptr                = alloc_aligned<uint8_t>(pData, to_alloc);
            if (ptr == NULL)
            {
                sSplitter.destroy();
                return STATUS_NO_MEM;
            }

            // Initialize data structures
            vSplits                     = advance_ptr_bytes<split_t>(ptr, szof_splits);
            vBands                      = advance_ptr_bytes<band_t>(ptr, szof_bands);
            vPlan                       = advance_ptr_bytes<split_t *>(ptr, szof_plan);
            vBuffer                     = advance_ptr_bytes<float>(ptr, szof_temp);

            // Construct all splits
            float step          = logf(LSP_DSP_UNITS_SPEC_FREQ_MAX / LSP_DSP_UNITS_SPEC_FREQ_MIN) / bands;

            for (size_t i=0; i<splits; ++i)
            {
                split_t * const sp  = &vSplits[i];

                // Initialize split point parameters
                sp->nBandId         = i + 1; // Band N+1 is attached to split point N
                sp->fSlope          = 0.0f;
                sp->fConfigFreq     = LSP_DSP_UNITS_SPEC_FREQ_MIN * expf((i+1) * step);
                sp->fFreq           = sp->fConfigFreq;
            }

            // Construct all bands
            for (size_t i=0; i<bands; ++i)
            {
                band_t * const b    = &vBands[i];

                b->nId              = uint32_t(i);
                b->fGain            = GAIN_AMP_0_DB;
                b->fStart           = (i > 0) ? vSplits[i-1].fFreq : LSP_DSP_UNITS_SPEC_FREQ_MIN;
                b->fEnd             = (i < splits) ? vSplits[i].fFreq : nSampleRate >> 1;
                b->bEnabled         = false;
                b->pStart           = NULL;
                b->pEnd             = NULL;

                b->pObject          = NULL;
                b->pSubject         = NULL;
                b->pFunc            = NULL;
                b->vFFT             = advance_ptr_bytes<float>(ptr, szof_buffer);

                dsp::fill_zero(b->vFFT, bins);
            }

            for (size_t i=0; i<splits; ++i)
                vPlan[i]                = NULL;

            return STATUS_OK;
        }

        void LPCrossover::set_rank(size_t rank)
        {
            rank            = lsp_limit(rank, 0u, sSplitter.max_rank());
            if (sSplitter.rank() == rank)
                return;
            sSplitter.set_rank(rank);
            nReconfigure       |= R_ALL;
        }

        void LPCrossover::set_phase(float phase)
        {
            sSplitter.set_phase(phase);
        }

        bool LPCrossover::set_slope(size_t sp, float slope)
        {
            if (sp >= num_splits())
                return false;
            slope       = lsp_min(slope, 0.0f);
            if (slope == vSplits[sp].fSlope)
                return true;

            vSplits[sp].fSlope  = slope;
            nReconfigure       |= R_SPLIT;

            return true;
        }

        float LPCrossover::slope(size_t sp) const
        {
            return (sp < num_splits()) ? vSplits[sp].fSlope : float(sp);
        }

        bool LPCrossover::set_frequency(size_t sp, float freq)
        {
            if (sp >= num_splits())
                return false;
            if (freq == vSplits[sp].fConfigFreq)
                return true;

            vSplits[sp].fConfigFreq = freq;
            nReconfigure           |= R_SPLIT;
            return true;
        }

        float LPCrossover::frequency(size_t sp) const
        {
            return (sp < num_splits()) ? vSplits[sp].fConfigFreq : -1.0f;
        }

        bool LPCrossover::set_gain(size_t band, float gain)
        {
            if (band >= num_bands())
                return false;
            if (gain == vBands[band].fGain)
                return true;

            vBands[band].fGain = gain;
            return true;
        }

        float LPCrossover::gain(size_t band) const
        {
            return (band <= num_splits()) ? vBands[band].fGain: -1.0f;
        }

        float LPCrossover::band_start(size_t band)
        {
            update_settings();
            return (band < num_bands()) ? vBands[band].fStart : -1.0f;
        }

        float LPCrossover::band_end(size_t band)
        {
            update_settings();
            return (band < num_bands()) ? vBands[band].fEnd : -1.0f;
        }

        bool LPCrossover::band_active(size_t band)
        {
            if (band >= num_bands())
                return false;
            else if (band == 0)
                return true;

            update_settings();
            return vBands[band].bEnabled;
        }

        void LPCrossover::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate     = uint32_t(sr);
            nReconfigure   |= R_ALL;
        }

        bool LPCrossover::set_handler(size_t band, crossover_func_t func, void *object, void *subject)
        {
            if (band >= num_bands())
                return false;

            band_t * const b    = &vBands[band];

            b->pFunc            = func;
            b->pObject          = object;
            b->pSubject         = subject;

            sync_binding(b);

            return true;
        }

        bool LPCrossover::unset_handler(size_t band)
        {
            return set_handler(band, NULL, NULL, NULL);
        }

        void LPCrossover::sync_binding(band_t *b)
        {
            const bool bound = sSplitter.bound(b->nId);
            if ((b->bEnabled) && (b->pFunc != NULL))
            {
                if (!bound)
                    sSplitter.bind(b->nId, this, b, spectral_func, spectral_sink);
            }
            else if (bound)
                sSplitter.unbind(b->nId);
        }

        void LPCrossover::update_settings()
        {
            if (!nReconfigure)
                return;

            // Update splitter settings
            sSplitter.update_settings();

            // Form the plan and reset band state
            const size_t splits     = num_splits();
            const float max_freq    = nSampleRate * 0.5f;
            nPlanSize       = 0;
            for (size_t i=0; i<splits; ++i)
            {
                split_t * const sp  = &vSplits[i];
                sp->fFreq           = lsp_limit(sp->fConfigFreq, 0.0f, max_freq);
                if (sp->fSlope < 0.0f)
                    vPlan[nPlanSize++]  = sp;
            }
            for (size_t i=0; i<=splits; ++i)
                vBands[i].bEnabled  = false;

            // Sort split bands by the frequency in ascending order
            for (ssize_t si=0, n=nPlanSize; si < n-1; ++si)
                for (ssize_t sj=si+1; sj < n; ++sj)
                    if (vPlan[sj]->fFreq < vPlan[si]->fFreq)
                        lsp::swap(vPlan[si], vPlan[sj]);


            // Configure start and end frequency ranges for each band
            band_t *left        = &vBands[0];
            left->fStart        = LSP_DSP_UNITS_SPEC_FREQ_MIN;
            left->bEnabled      = true;
            left->pStart        = NULL;

            for (size_t i=0; i<nPlanSize; ++i)
            {
                split_t * const sp  = vPlan[i];
                band_t * const right= &vBands[sp->nBandId];

                left->fEnd          = sp->fFreq;
                left->pEnd          = sp;
                right->fStart       = sp->fFreq;
                right->pStart       = sp;
                right->bEnabled     = true;

                // Move to next band
                left                = right;
            }

            // Update frequency of the last band
            left->fEnd          = max_freq;
            left->pEnd          = NULL;

            // Update bindings
            for (size_t i=0; i<=splits; ++i)
                sync_binding(&vBands[i]);

            // Form the frequency response for each enabled band
            update_band_freq_responses();

            // DEBUG BEGIN
        #ifdef LSP_TRACE
            lsp_trace("Execution plan:");
            for (size_t i=0; i<nPlanSize; ++i)
            {
                split_t *sp         = vPlan[i];
                lsp_trace("  split point #%d: this=%p, band=%d, freq=%.2f, slope=%.2f",
                    int(i), sp, int(sp->nBandId), sp->fFreq, int(sp->fSlope)
                );
            }
            lsp_trace("Bands:");
            for (size_t i=0; i<=splits; ++i)
            {
                band_t *b           = &vBands[i];
                lsp_trace("  band #%d: this=%p, enabled=%s, gain=%f, start=%.2f, end=%.2f, start=%p, end=%p",
                    int(i), b, (b->bEnabled) ? "true " : "false",
                    b->fGain, b->fStart, b->fEnd, b->pStart, b->pEnd);
            }
        #endif
            // DEBUG END

            // Reset reconfiguration flag
            nReconfigure        = 0;
        }

        void LPCrossover::spectral_func(
            void *object, void *subject,
            float *out, const float *in,
            size_t rank)
        {
            band_t * const b            = static_cast<band_t *>(subject);

            // Copy spectrum and apply FFT envelope
            const size_t bins           = 1 << rank;
            dsp::mul_k3(out, in, b->fGain, bins * 2);
            dsp::pcomplex_r2c_mul2(out, b->vFFT, bins);
        }

        void LPCrossover::spectral_sink(
            void *object, void *subject,
            const float *samples,
            size_t first, size_t count)
        {
            band_t * const b            = static_cast<band_t *>(subject);
            if (!b->pFunc)
                return;

            LPCrossover * const self    = static_cast<LPCrossover *>(object);
            b->pFunc(b->pObject, b->pSubject, b - self->vBands, samples, first, count);
        }

        void LPCrossover::process(const float *in, size_t samples)
        {
            update_settings();
            sSplitter.process(in, samples);
        }

        bool LPCrossover::freq_chart(size_t band, float *m, const float *f, size_t count)
        {
            if (band >= num_bands())
                return false;

            update_settings();

            // Band is enabled ?
            band_t * const b    = &vBands[band];
            if (!b->bEnabled)
                dsp::fill_zero(m, count);
            else if (nPlanSize == 0)
                dsp::fill(m, vBands[0].fGain, count);
            else
            {
                const size_t rank   = lsp_max(sSplitter.rank(), size_t(1)) - 1;
                const size_t buf_sz = lsp_max(size_t(1) << rank, temp_buf_size);

                while (count > 0)
                {
                    // Obtain frequency chart for the band
                    const size_t to_do  = lsp_min(count, buf_sz);
                    band_freq_chart(band, m, f, to_do);

                    // Update pointers
                    m              += to_do;
                    f              += to_do;
                    count          -= to_do;
                }
            }

            return true;
        }

        void LPCrossover::band_freq_chart(size_t band, float *m, const float *f, size_t count)
        {
            float * const lpf_buf   = vBuffer;
            float * const hpf_buf   = &vBuffer[count];

            // Compute the low-pass filter characteristics for band 0
            band_t *pband       = &vBands[0];

            // Process each band except last
            for (size_t i=0; i<nPlanSize; ++i)
            {
                split_t * const sp      = vPlan[i];

                // Compute low-pass filter magnitude
                crossover::lopass_set(lpf_buf, f, sp->fFreq, sp->fSlope, count);
                if (band == 0)
                {
                    dsp::mul_k3(m, lpf_buf, pband->fGain, count);
                    return;
                }
                else if (pband->nId == band)
                {
                    dsp::fmmul_k4(m, lpf_buf, hpf_buf, pband->fGain, count);
                    return;
                }

                // Compute high-pass filter magnitude
                if (i > 0)
                {
                    dsp::rsub_k2(lpf_buf, 1.0f, count);
                    dsp::mul2(hpf_buf, lpf_buf, count);
                }
                else
                    dsp::rsub_k3(hpf_buf, lpf_buf, 1.0f, count);

                // Store the identifier of last processed band
                pband       = &vBands[sp->nBandId];
            }

            // Return contents of hpf_buf if last band was requested
            dsp::mul_k3(m, hpf_buf, pband->fGain, count);
        }

        void LPCrossover::update_band_freq_responses()
        {
            // Compute the low-pass filter characteristics for band 0
            band_t *lb      = &vBands[0];

            const size_t rank   = sSplitter.rank();
            const size_t bins   = 1 << rank;

            // Process each band except last
            for (size_t i=0; i<nPlanSize; ++i)
            {
                split_t * const sp      = vPlan[i];
                band_t * const rb       = &vBands[sp->nBandId];

                // Compute low-pass filter magnitude
                if (i > 0)
                {
                    crossover::lopass_fft_set(vBuffer, sp->fFreq, sp->fSlope, nSampleRate, rank);   // Compute LPF magnitude of left band
                    dsp::rsub_k3(rb->vFFT, vBuffer, 1.0f, bins);                                    // Compute HPF magnitude of right band
                    dsp::mul2(rb->vFFT, lb->vFFT, bins);                                            // Apply HPF magnitude from left band to right band
                    dsp::mul2(lb->vFFT, vBuffer, bins);                                             // Apply LPF magnitude to left band
                }
                else
                {
                    crossover::lopass_fft_set(lb->vFFT, sp->fFreq, sp->fSlope, nSampleRate, rank);  // Compute LPF magnitude for left band
                    dsp::rsub_k3(rb->vFFT, lb->vFFT, 1.0f, bins);                                   // Compute HPF magnitude for right band
                }

                // Store the identifier of last processed band
                lb          = rb;
            }
        }

        void LPCrossover::dump(IStateDumper *v) const
        {
            const size_t bands  = num_bands();
            const size_t splits = num_splits();

            v->write_object("sSplitter", &sSplitter);

            v->write("nSampleRate", nSampleRate);
            v->write("nPlanSize", nPlanSize);
            v->write("nReconfigure", nReconfigure);

            v->begin_array("vSplits", vSplits, splits);
            {
                for (size_t i=0; i<splits; ++i)
                {
                    const split_t * const s = &vSplits[i];

                    v->begin_object(s, sizeof(split_t));
                    {
                        v->write("nBandId", s->nBandId);
                        v->write("fSlope", s->fSlope);
                        v->write("fConfigFreq", s->fConfigFreq);
                        v->write("fFreq", s->fFreq);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->begin_array("vBands", vBands, bands);
            {
                for (size_t i=0; i<bands; ++i)
                {
                    const band_t * const b = &vBands[i];

                    v->begin_object(b, sizeof(band_t));
                    {
                        v->write("nId", b->nId);
                        v->write("fGain", b->fGain);
                        v->write("fStart", b->fStart);
                        v->write("fEnd", b->fEnd);
                        v->write("bEnabled", b->bEnabled);
                        v->write("pStart", b->pStart);
                        v->write("pEnd", b->pEnd);

                        v->write("pObject", b->pObject);
                        v->write("pSubject", b->pSubject);
                        v->write("pFunc", b->pFunc);
                        v->write("vFFT", b->vFFT);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */


