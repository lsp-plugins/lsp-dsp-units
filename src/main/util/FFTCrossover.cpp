/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 9 авг. 2023 г.
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

#include <lsp-plug.in/dsp-units/misc/fft_crossover.h>
#include <lsp-plug.in/dsp-units/util/FFTCrossover.h>
#include <lsp-plug.in/common/alloc.h>

namespace lsp
{
    namespace dspu
    {
        FFTCrossover::FFTCrossover()
        {
            construct();
        }

        FFTCrossover::~FFTCrossover()
        {
            destroy();
        }

        void FFTCrossover::construct()
        {
            sSplitter.construct();

            vBands          = NULL;
            nSampleRate     = 0;

            pData           = NULL;
        }

        void FFTCrossover::destroy()
        {
            sSplitter.destroy();

            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }

            vBands          = NULL;
            nSampleRate     = 0;

            pData           = NULL;
        }

        status_t FFTCrossover::init(size_t max_rank, size_t bands)
        {
            status_t res    = sSplitter.init(max_rank, bands);
            if (res != STATUS_OK)
                return res;

            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }
            vBands          = NULL;

            const size_t bins           = 1 << max_rank;
            const size_t szof_bands     = align_size(sizeof(band_t) * bands, DEFAULT_ALIGN);
            const size_t szof_buffer    = align_size(sizeof(float) * bins, DEFAULT_ALIGN);
            const size_t to_alloc       = szof_bands + bands * szof_buffer;

            // Allocate the data
            uint8_t *ptr                = alloc_aligned<uint8_t>(pData, to_alloc);
            if (ptr == NULL)
            {
                sSplitter.destroy();
                return STATUS_NO_MEM;
            }

            // Initialize data structures
            vBands                      = reinterpret_cast<band_t *>(ptr);
            ptr                        += szof_bands;

            for (size_t i=0; i<bands; ++i)
            {
                band_t *b                   = &vBands[i];

                b->fHpfFreq                 = 100.0f;
                b->fLpfFreq                 = 1000.0f;
                b->fHpfSlope                = -24.0f;
                b->fLpfSlope                = -24.0f;
                b->fGain                    = 1.0f;
                b->fFlatten                 = 1.0f;
                b->bLpf                     = false;
                b->bHpf                     = false;
                b->bEnabled                 = false;
                b->bUpdate                  = true;

                b->pObject                  = NULL;
                b->pSubject                 = NULL;
                b->pFunc                    = NULL;

                b->vFFT                     = reinterpret_cast<float *>(ptr);
                ptr                        += szof_buffer;
            }

            return STATUS_OK;
        }

        void FFTCrossover::spectral_func(
            void *object,
            void *subject,
            float *out,
            const float *in,
            size_t rank)
        {
            band_t *b           = static_cast<band_t *>(subject);
            FFTCrossover *self  = static_cast<FFTCrossover *>(object);

            self->update_band(b);

            // Copy spectrum and apply FFT envelope
            const size_t bins = 1 << rank;
            dsp::copy(out, in, bins * 2);
            dsp::pcomplex_r2c_mul2(out, b->vFFT, bins);
        }

        void FFTCrossover::spectral_sink(
            void *object,
            void *subject,
            const float *samples,
            size_t first,
            size_t count)
        {
            band_t *b           = static_cast<band_t *>(subject);
            if (!b->pFunc)
                return;

            FFTCrossover *self  = static_cast<FFTCrossover *>(object);
            b->pFunc(b->pObject, b->pSubject, b - self->vBands, samples, first, count);
        }

        void FFTCrossover::set_slope(size_t band, float lpf, float hpf)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate =
                    ((b->bLpf) && (b->fLpfSlope != lpf)) ||
                    ((b->bHpf) && (b->fHpfSlope != hpf));

            b->fLpfSlope    = lpf;
            b->fHpfSlope    = hpf;
        }

        void FFTCrossover::set_lpf_slope(size_t band, float slope)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = ((b->bLpf) && (b->fLpfSlope != slope));

            b->fLpfSlope    = slope;
        }

        void FFTCrossover::set_hpf_slope(size_t band, float slope)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = ((b->bHpf) && (b->fHpfSlope != slope));

            b->fHpfSlope    = slope;
        }

        float FFTCrossover::lpf_slope(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fLpfSlope : -1.0f;
        }

        float FFTCrossover::hpf_slope(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fHpfSlope : -1.0f;
        }

        void FFTCrossover::set_frequency(size_t band, float lpf, float hpf)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate =
                    ((b->bLpf) && (b->fLpfFreq != lpf)) ||
                    ((b->bHpf) && (b->fHpfFreq != hpf));

            b->fLpfFreq     = lpf;
            b->fHpfFreq     = hpf;
        }

        void FFTCrossover::set_lpf_frequency(size_t band, float freq)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = ((b->bLpf) && (b->fLpfFreq != freq));

            b->fLpfFreq     = freq;
        }

        void FFTCrossover::set_hpf_frequency(size_t band, float freq)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = ((b->bHpf) && (b->fHpfFreq != freq));

            b->fHpfFreq     = freq;
        }

        float FFTCrossover::lpf_frequency(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fLpfFreq : -1.0f;
        }

        float FFTCrossover::hpf_frequency(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fHpfFreq : -1.0f;
        }

        void FFTCrossover::enable_filters(size_t band, bool lpf, bool hpf)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];
            if (!b->bUpdate)
                b->bUpdate =
                    (b->bLpf != lpf) ||
                    (b->bHpf != hpf);

            b->bLpf         = lpf;
            b->bHpf         = hpf;
        }

        void FFTCrossover::enable_lpf(size_t band, bool enable)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];
            if (!b->bUpdate)
                b->bUpdate = (b->bLpf != enable);
            b->bLpf         = enable;
        }

        void FFTCrossover::enable_hpf(size_t band, bool enable)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];
            if (!b->bUpdate)
                b->bUpdate = (b->bHpf != enable);
            b->bHpf         = enable;
        }

        bool FFTCrossover::lpf_enabled(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].bLpf : false;
        }

        bool FFTCrossover::hpf_enabled(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].bHpf : false;
        }

        void FFTCrossover::set_lpf(size_t band, float freq, float slope, bool enabled)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = (enabled) &&
                    ((b->fLpfFreq != freq) || (b->fLpfSlope != slope) || (b->bLpf != enabled));

            b->fLpfFreq     = freq;
            b->fLpfSlope    = slope;
            b->bLpf         = enabled;
        }

        void FFTCrossover::set_hpf(size_t band, float freq, float slope, bool enabled)
        {
            if (band >= sSplitter.handlers())
                return;
            band_t *b   = &vBands[band];

            if (!b->bUpdate)
                b->bUpdate = (enabled) &&
                    ((b->fHpfFreq != freq) || (b->fHpfSlope != slope) || (b->bHpf != enabled));

            b->fHpfFreq     = freq;
            b->fHpfSlope    = slope;
            b->bHpf         = enabled;
        }

        void FFTCrossover::set_gain(size_t band, float gain)
        {
            if (band >= sSplitter.handlers())
                return;

            band_t *b   = &vBands[band];
            if (b->fGain == gain)
                return;

            b->bUpdate      = true;
            b->fGain        = gain;
        }

        float FFTCrossover::gain(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fGain : -1.0f;
        }

        void FFTCrossover::set_flatten(size_t band, float amount)
        {
            if (band >= sSplitter.handlers())
                return;

            band_t *b   = &vBands[band];
            if (b->fFlatten == amount)
                return;

            b->bUpdate      = true;
            b->fFlatten     = amount;
        }

        float FFTCrossover::flatten(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].fFlatten : -1.0f;
        }

        void FFTCrossover::sync_binding(size_t band, band_t *b)
        {
            bool bound = sSplitter.bound(band);
            if ((!bound) && (b->bEnabled) && (b->pFunc != NULL))
                sSplitter.bind(band, this, b, spectral_func, spectral_sink);
            else if (bound)
                sSplitter.unbind(band);
        }

        void FFTCrossover::enable_band(size_t band, bool enable)
        {
            if (band >= sSplitter.handlers())
                return;

            band_t *b       = &vBands[band];
            if (enable == b->bEnabled)
                return;
            b->bEnabled     = enable;

            sync_binding(band, b);
        }

        bool FFTCrossover::band_enabled(size_t band) const
        {
            return (band < sSplitter.handlers()) ? vBands[band].bEnabled : false;
        }

        bool FFTCrossover::set_handler(size_t band, crossover_func_t func, void *object, void *subject)
        {
            if (band >= sSplitter.handlers())
                return false;

            band_t *b       = &vBands[band];

            b->pFunc        = func;
            b->pObject      = object;
            b->pSubject     = subject;

            sync_binding(band, b);

            return true;
        }

        bool FFTCrossover::unset_handler(size_t band)
        {
            return set_handler(band, NULL, NULL, NULL);
        }

        void FFTCrossover::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;
            nSampleRate     = sr;
            mark_bands_for_update();
        }

        void FFTCrossover::set_rank(size_t rank)
        {
            rank            = lsp_limit(rank, 0u, sSplitter.max_rank());
            if (sSplitter.rank() == rank)
                return;
            sSplitter.set_rank(rank);
            mark_bands_for_update();
        }

        void FFTCrossover::set_phase(float phase)
        {
            sSplitter.set_phase(phase);
        }

        bool FFTCrossover::needs_update() const
        {
            for (size_t i=0, n=sSplitter.handlers(); i<n; ++i)
            {
                const band_t *b   = &vBands[i];
                if ((b->bEnabled) && (b->bUpdate))
                    return true;
            }
            return false;
        }

        void FFTCrossover::update_settings()
        {
            sSplitter.update_settings();

            for (size_t i=0, n=sSplitter.handlers(); i<n; ++i)
                if (vBands[i].bEnabled)
                    update_band(&vBands[i]);
        }

        void FFTCrossover::update_band(band_t *b)
        {
            if (!b->bUpdate)
                return;

            const size_t rank   = sSplitter.rank();
            const size_t bins   = 1 << rank;

            if (b->bHpf)
            {
                crossover::hipass_fft_set(b->vFFT, b->fHpfFreq, b->fHpfSlope, nSampleRate, rank);
                if (b->bLpf)
                    crossover::lopass_fft_apply(b->vFFT, b->fLpfFreq, b->fLpfSlope, nSampleRate, rank);

                dsp::limit1(b->vFFT, 0.0f, b->fFlatten, bins);
                dsp::mul_k2(b->vFFT, b->fGain, bins);
            }
            else if (b->bLpf)
            {
                crossover::lopass_fft_set(b->vFFT, b->fLpfFreq, b->fLpfSlope, nSampleRate, rank);
                dsp::limit1(b->vFFT, 0.0f, b->fFlatten, bins);
                dsp::mul_k2(b->vFFT, b->fGain, bins);
            }
            else
                dsp::fill(b->vFFT, b->fFlatten * b->fGain, bins);

            b->bUpdate      = false;
        }

        void FFTCrossover::mark_bands_for_update()
        {
            for (size_t i=0, n=sSplitter.handlers(); i<n; ++i)
                vBands[i].bUpdate   = true;
        }

        void FFTCrossover::process(const float *in, size_t samples)
        {
            sSplitter.process(in, samples);
        }

        bool FFTCrossover::freq_chart(size_t band, float *m, const float *f, size_t count)
        {
            if (band >= sSplitter.handlers())
                return false;

            band_t *b       = &vBands[band];
            if (!b->bEnabled)
                dsp::fill_zero(m, count);
            else if (b->bHpf)
            {
                dspu::crossover::hipass_set(m, f, b->fHpfFreq, b->fHpfSlope, count);
                if (b->bLpf)
                    dspu::crossover::lopass_apply(m, f, b->fLpfFreq, b->fLpfSlope, count);
                dsp::limit1(m, 0.0f, b->fFlatten, count);
                dsp::mul_k2(m, b->fGain, count);
            }
            else if (b->bLpf)
            {
                dspu::crossover::lopass_set(m, f, b->fLpfFreq, b->fLpfSlope, count);
                dsp::limit1(m, 0.0f, b->fFlatten, count);
                dsp::mul_k2(m, b->fGain, count);
            }
            else
                dsp::fill(m, b->fFlatten * b->fGain, count);

            return true;
        }

        void FFTCrossover::dump(IStateDumper *v) const
        {
            v->write_object("sSplitter", &sSplitter);

            v->begin_array("vBands", vBands, sSplitter.handlers());
            {
                for (size_t i=0, n=sSplitter.handlers(); i<n; ++i)
                {
                    const band_t *b = &vBands[i];

                    v->begin_object(b, sizeof(band_t));
                    {
                        v->write("fHpfFreq", b->fHpfFreq);
                        v->write("fLpfFreq", b->fLpfFreq);
                        v->write("fHpfSlope", b->fHpfSlope);
                        v->write("fLpfSlope", b->fLpfSlope);
                        v->write("fGain", b->fGain);
                        v->write("fFlatten", b->fFlatten);
                        v->write("bLpf", b->bLpf);
                        v->write("bHpf", b->bHpf);
                        v->write("bEnabled", b->bEnabled);
                        v->write("bUpdate", b->bUpdate);

                        v->write("pObject", b->pObject);
                        v->write("pSubject", b->pSubject);
                        v->write("pFunc", b->pFunc);
                        v->write("vFFT", b->vFFT);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->write("nSampleRate", nSampleRate);
            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */


