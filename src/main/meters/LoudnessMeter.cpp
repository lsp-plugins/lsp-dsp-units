/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 20 сент. 2023 г.
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
#include <lsp-plug.in/common/bits.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/meters/LoudnessMeter.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BUFFER_SIZE     = 0x400;

        LoudnessMeter::LoudnessMeter()
        {
            construct();
        }

        LoudnessMeter::~LoudnessMeter()
        {
            destroy();
        }

        void LoudnessMeter::construct()
        {
            vChannels           = NULL;
            vBuffer             = NULL;

            fPeriod             = 0.0f;
            fMaxPeriod          = 0.0f;
            fAvgCoeff           = 1.0f;

            nPeriod             = 0;
            nMSRefresh          = 0;
            nSampleRate         = 0;
            nChannels           = 0;
            enWeight            = bs::WEIGHT_NONE;
            nFlags              = F_UPD_ALL;
            nDataHead           = 0;
            nDataSize           = 0;

            pData               = NULL;
            pVarData            = NULL;
        }

        void LoudnessMeter::destroy()
        {
            if (pData != NULL)
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c        = &vChannels[i];
                    c->sFilter.destroy();
                    c->sBank.destroy();
                }

                free_aligned(pData);
                pData               = NULL;
                vChannels           = NULL;
                vBuffer             = NULL;
            }

            if (pVarData != NULL)
            {
                free_aligned(pVarData);
                pVarData            = NULL;
            }
        }

        status_t LoudnessMeter::init(size_t channels, float max_period)
        {
            destroy();

            // Allocate data
            size_t szof_channels    = align_size(channels * sizeof(channel_t), DEFAULT_ALIGN);
            size_t szof_buffer      = align_size(sizeof(float) * BUFFER_SIZE, DEFAULT_ALIGN);
            size_t to_alloc =
                szof_channels +
                szof_buffer +
                channels * szof_buffer;

            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, to_alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            // Allocate buffers
            vChannels               = advance_ptr_bytes<channel_t>(ptr, szof_channels);
            vBuffer                 = advance_ptr_bytes<float>(ptr, szof_buffer);

            // Cleanup
            dsp::fill_zero(vBuffer, BUFFER_SIZE);
            for (size_t i=0; i<channels; ++i)
            {
                channel_t *c            = &vChannels[i];
                c->sBank.construct();
                c->sFilter.construct();

                if (!c->sBank.init(4))
                    return STATUS_NO_MEM;
                if (!c->sFilter.init(&c->sBank))
                    return STATUS_NO_MEM;

                c->vIn                  = NULL;
                c->vOut                 = NULL;
                c->vData                = NULL;
                c->vMS                  = advance_ptr_bytes<float>(ptr, szof_buffer);

                c->fMS                  = 0.0f;
                c->fWeight              = 0.0f;
                c->fLink                = 1.0f;
                c->enDesignation        = bs::CHANNEL_NONE;

                c->nFlags               = C_ENABLED;
                c->nOffset              = 0;
            }

            // Set-up default designations
            if (channels == 1)
            {
                channel_t *c            = &vChannels[0];
                c->enDesignation        = bs::CHANNEL_CENTER;
                c->fWeight              = bs::channel_weighting(c->enDesignation);
            }
            else if (channels == 2)
            {
                channel_t *l            = &vChannels[0];
                channel_t *r            = &vChannels[1];

                l->enDesignation        = bs::CHANNEL_LEFT;
                l->fWeight              = bs::channel_weighting(l->enDesignation);
                r->enDesignation        = bs::CHANNEL_RIGHT;
                r->fWeight              = bs::channel_weighting(r->enDesignation);
            }

            // Initialize channels
            for (size_t i=0; i<channels; ++i)
            {
                channel_t *c            = &vChannels[i];

                if (!c->sFilter.init(&c->sBank))
                    return STATUS_NO_MEM;
            }

            // Initialize settings
            fPeriod                 = lsp_min(max_period, dspu::bs::LUFS_MEASURE_PERIOD_MS);  // BS.1770-4: RMS measurement period
            fMaxPeriod              = max_period;
            fAvgCoeff               = 1.0f;

            nPeriod                 = 0;
            nMSRefresh              = 0;
            nSampleRate             = 0;
            nChannels               = channels;
            enWeight                = bs::WEIGHT_K;
            nFlags                  = F_UPD_ALL;

            nDataHead               = 0;
            nDataSize               = 0;

            return STATUS_OK;
        }

        status_t LoudnessMeter::bind(size_t id, float *out, const float *in, size_t pos)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c    = &vChannels[id];
            c->vIn          = in;
            c->vOut         = out;
            c->nOffset      = pos;

            return STATUS_OK;
        }

        void LoudnessMeter::set_weighting(bs::weighting_t weighting)
        {
            if (weighting == enWeight)
                return;

            enWeight        = weighting;
            nFlags         |= F_UPD_FILTERS;
        }

        status_t LoudnessMeter::set_designation(size_t id, bs::channel_t designation)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c        = &vChannels[id];
            c->enDesignation    = designation;
            c->fWeight          = bs::channel_weighting(designation);

            return STATUS_OK;
        }

        bs::channel_t LoudnessMeter::designation(size_t id) const
        {
            return (id < nChannels) ? vChannels[id].enDesignation : bs::CHANNEL_NONE;
        }

        status_t LoudnessMeter::set_link(size_t id, float link)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c    = &vChannels[id];
            c->fLink        = lsp_limit(link, 0.0f, 1.0f);

            return STATUS_OK;
        }

        float LoudnessMeter::link(size_t id) const
        {
            return (id < nChannels) ? vChannels[id].fLink : 0.0f;
        }

        status_t LoudnessMeter::set_active(size_t id, bool active)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c        = &vChannels[id];
            if (bool(c->nFlags & C_ENABLED) == active)
                return STATUS_OK;

            c->nFlags           = lsp_setflag(c->nFlags, C_ENABLED, active);

            if (active)
            {
                dsp::fill_zero(c->vData, nDataSize);
                c->fMS             = 0.0f;
            }

            return STATUS_OK;
        }

        bool LoudnessMeter::active(size_t id) const
        {
            return (id < nChannels) ? bool(vChannels[id].nFlags & C_ENABLED) : false;
        }

        void LoudnessMeter::set_period(float period)
        {
            period          = lsp_limit(period, 0.0f, fMaxPeriod);
            if (fPeriod == period)
                return;

            fPeriod         = period;
            nFlags         |= F_UPD_TIME;
        }

        void LoudnessMeter::clear()
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                c->sFilter.clear();
                if (c->nFlags & C_ENABLED)
                {
                    dsp::fill_zero(c->vData, nDataSize);
                    c->fMS                  = 0.0f;
                }
            }
        }

        status_t LoudnessMeter::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return STATUS_OK;

            // Reallocate ring buffers for RMS estimation and lookahead
            size_t len_period       = round_pow2(size_t(dspu::millis_to_samples(sample_rate, fMaxPeriod)) + BUFFER_SIZE);
            size_t szof_period      = align_size(len_period * sizeof(float), DEFAULT_ALIGN);

            size_t to_alloc         =
                nChannels * szof_period;

            uint8_t *buf            = realloc_aligned<uint8_t>(pVarData, to_alloc, DEFAULT_ALIGN);
            if (buf == NULL)
                return STATUS_NO_MEM;

            // Store new pointers to buffers
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                c->vData                = reinterpret_cast<float *>(buf);
                buf                    += szof_period;
            }

            // Update settings
            nSampleRate             = sample_rate;
            nDataSize               = len_period;
            nDataHead               = 0;
            nFlags                  = F_UPD_ALL;

            // Clear all buffers
            clear();

            return STATUS_OK;
        }

        size_t LoudnessMeter::latency() const
        {
            return dspu::millis_to_samples(nSampleRate, fPeriod);
        }

        void LoudnessMeter::update_settings()
        {
            if (nFlags == 0)
                return;

            if (nFlags & F_UPD_TIME)
            {
                nPeriod     = lsp_max(dspu::millis_to_samples(nSampleRate, fPeriod), 1u);
                fAvgCoeff   = 1.0f / float(nPeriod);
                nMSRefresh  = 0;
            }

            if (nFlags & F_UPD_FILTERS)
            {
                dspu::filter_params_t fp;

                fp.nType        = FLT_NONE;
                fp.nSlope       = 0;
                fp.fFreq        = 0.0f;
                fp.fFreq2       = 0.0f;
                fp.fGain        = 1.0f;
                fp.fQuality     = 0.0f;

                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c            = &vChannels[i];

                    c->sBank.begin();
                    switch (enWeight)
                    {
                        case bs::WEIGHT_A:  fp.nType    = dspu::FLT_A_WEIGHTED; break;
                        case bs::WEIGHT_B:  fp.nType    = dspu::FLT_B_WEIGHTED; break;
                        case bs::WEIGHT_C:  fp.nType    = dspu::FLT_C_WEIGHTED; break;
                        case bs::WEIGHT_D:  fp.nType    = dspu::FLT_D_WEIGHTED; break;
                        case bs::WEIGHT_K:  fp.nType    = dspu::FLT_K_WEIGHTED; break;
                        case bs::WEIGHT_NONE:
                        default:
                            break;
                    }

                    c->sFilter.update(nSampleRate, &fp);
                    c->sFilter.rebuild();
                    c->sBank.end(true);
                }
            }

            // Reset flags
            nFlags      = 0;
        }

        void LoudnessMeter::refresh_rms()
        {
            if (nMSRefresh > 0)
                return;

            size_t tail         = (nDataHead + nDataSize - nPeriod) & (nDataSize - 1);
            if (tail < nDataHead)
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    if (c->nFlags & C_ENABLED)
                        c->fMS         = dsp::h_sum(&c->vData[tail], nDataHead - tail);
                }
            }
            else
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    if (c->nFlags & C_ENABLED)
                        c->fMS         =
                            dsp::h_sum(c->vData, nDataHead) +
                            dsp::h_sum(&c->vData[tail], nDataSize - tail);
                }
            }

            // Reset the counter
            nMSRefresh          = lsp_max(BUFFER_SIZE << 2, nPeriod >> 2);
        }

        size_t LoudnessMeter::process_channels(size_t offset, size_t samples)
        {
            const size_t mask   = nDataSize - 1;
            size_t mixed        = 0;        // Number of loudness channels mixed together

            // Pre-process each channel individually
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                if (!(c->nFlags & C_ENABLED))
                    continue;

                // Apply the weighting filter
                c->sFilter.process(c->vMS, &c->vIn[offset], samples);

                // Put the squared input data to the buffer
                size_t head         = nDataHead;
                size_t head_adv     = (nDataHead + samples) & mask;
                if (head_adv <= head)
                {
                    dsp::sqr2(&c->vData[head], c->vMS, nDataSize - head);
                    dsp::sqr2(&c->vData[0], &c->vMS[nDataSize - head], head_adv);
                }
                else
                    dsp::sqr2(&c->vData[head], c->vMS, samples);

                // Compute the Mean Square value for each sample and store into mean square buffer
                size_t tail         = (nDataHead + nDataSize - nPeriod) & mask;
                float ms            = c->fMS;

                for (size_t j=0; j<samples; ++j)
                {
                    ms                 += c->vData[head] - c->vData[tail];
                    c->vMS[j]           = fAvgCoeff * ms;
                    head                = (head + 1) & mask;
                    tail                = (tail + 1) & mask;
                }
                c->fMS             = ms;

                // Add mean square values to the loudness estimation buffer
                if (mixed++ > 0)
                    dsp::fmadd_k3(vBuffer, c->vMS, c->fWeight, samples);
                else
                    dsp::mul_k3(vBuffer, c->vMS, c->fWeight, samples);
            }

            return mixed;
        }

        void LoudnessMeter::process(float *out, size_t count)
        {
            update_settings();

            for (size_t offset = 0; offset < count; )
            {
                refresh_rms();

                // Apply data
                size_t to_do        = lsp_min(count - offset, nMSRefresh, BUFFER_SIZE);
                size_t mixed        = process_channels(offset, to_do);

                if (mixed == 0)
                    dsp::fill_zero(vBuffer, to_do);

                // Now we have the mean squares computed for each channel, weighted,
                // summed and stored into vBuffer. Convert them into gain values
                dsp::ssqrt1(vBuffer, to_do);
                if (out != NULL)
                    dsp::copy(&out[offset], vBuffer, to_do);

                // Post-process each channel individually: convert mean squares into
                // RMS and perform the linking
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    if (!(c->nFlags & C_ENABLED))
                        continue;

                    if (c->vOut != NULL)
                    {
                        dsp::ssqrt1(c->vMS, to_do);
                        if (c->fLink <= 0.0f)
                            dsp::copy(&c->vOut[c->nOffset], c->vMS, to_do);
                        else if (c->fLink >= 1.0f)
                            dsp::copy(&c->vOut[c->nOffset], vBuffer, to_do);
                        else
                            dsp::mix_copy2(&c->vOut[c->nOffset], vBuffer, c->vMS, c->fLink, 1.0f - c->fLink, to_do);
                    }

                    c->nOffset         += to_do;
                }

                // Update pointers
                nDataHead           = (nDataHead + to_do) & (nDataSize - 1);
                nMSRefresh         -= to_do;
                offset             += to_do;
            }
        }

        void LoudnessMeter::process(float *out, size_t count, float gain)
        {
            update_settings();

            for (size_t offset = 0; offset < count; )
            {
                refresh_rms();

                // Apply data
                size_t to_do        = lsp_min(count - offset, nMSRefresh, BUFFER_SIZE);
                size_t mixed        = process_channels(offset, to_do);

                if (mixed == 0)
                    dsp::fill_zero(vBuffer, to_do);

                // Now we have the mean squares computed for each channel, weighted,
                // summed and stored into vBuffer. Convert them into gain values
                dsp::ssqrt1(vBuffer, to_do);
                if (out != NULL)
                    dsp::mul_k3(&out[offset], vBuffer, gain, to_do);

                // Post-process each channel individually: convert mean squares into
                // RMS and perform the linking
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    if (!(c->nFlags & C_ENABLED))
                        continue;

                    if (c->vOut != NULL)
                    {
                        dsp::ssqrt1(c->vMS, to_do);
                        if (c->fLink <= 0.0f)
                            dsp::mul_k3(&c->vOut[c->nOffset], c->vMS, gain, to_do);
                        else if (c->fLink >= 1.0f)
                            dsp::mul_k3(&c->vOut[c->nOffset], vBuffer, gain, to_do);
                        else
                            dsp::mix_copy2(&c->vOut[c->nOffset], vBuffer, c->vMS, c->fLink * gain, (1.0f - c->fLink) * gain, to_do);
                    }

                    c->nOffset         += to_do;
                }

                // Update pointers
                nDataHead           = (nDataHead + to_do) & (nDataSize - 1);
                nMSRefresh         -= to_do;
                offset             += to_do;
            }
        }

        void LoudnessMeter::dump(IStateDumper *v) const
        {
            v->begin_array("vChannels", vChannels, nChannels);
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];

                    v->begin_object(c, sizeof(channel_t));
                    {
                        v->write_object("sBank", &c->sBank);
                        v->write_object("sFilter", &c->sFilter);

                        v->write("vIn", c->vIn);
                        v->write("vOut", c->vOut);
                        v->write("vData", c->vData);
                        v->write("vMS", c->vMS);

                        v->write("fMS", c->fMS);
                        v->write("fWeight", c->fWeight);
                        v->write("fLink", c->fLink);
                        v->write("enDesignation", c->enDesignation);

                        v->write("nFlags", c->nFlags);
                        v->write("nOffset", c->nOffset);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->write("vBuffer", vBuffer);

            v->write("fPeriod", fPeriod);
            v->write("fMaxPeriod", fMaxPeriod);
            v->write("fAvgCoeff", fAvgCoeff);

            v->write("nSampleRate", nSampleRate);
            v->write("nPeriod", nPeriod);
            v->write("nMSRefresh", nMSRefresh);
            v->write("nChannels", nChannels);
            v->write("nFlags", nFlags);
            v->write("nDataHead", nDataHead);
            v->write("nDataSize", nDataSize);
            v->write("enWeight", enWeight);

            v->write("pData", pData);
            v->write("pVarData", pVarData);
        }

    } /* namespace dspu */
} /* namespace lsp */



