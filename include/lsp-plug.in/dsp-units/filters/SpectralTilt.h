/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 Sept 2021
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

/** Based on Faust's spectral_tilt filter by Julius O. Smith III.
 *
 * References:
 *
 * J.O. Smith and H.F. Smith,
 * "Closed Form Fractional Integration and Differentiation via Real Exponentially Spaced Pole-Zero Pairs",
 * arXiv.org publication arXiv:1606.06154 [cs.CE], June 7, 2016,
 * <http://arxiv.org/abs/1606.06154>
 *
 * <https://github.com/grame-cncm/faustlibraries/blob/cabc562a79b36160c492b6f8128981994c0203da/filters.lib#L2311>
 *
 */


#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_SPECTRALTILT_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_SPECTRALTILT_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/filters/FilterBank.h>

namespace lsp
{
    namespace dspu
    {
        enum stlt_slope_unit_t
        {
            STLT_SLOPE_UNIT_NEPER_PER_NEPER,
            STLT_SLOPE_UNIT_DB_PER_OCTAVE,
            STLT_SLOPE_UNIT_DB_PER_DECADE,
            STLT_SLOPE_UNIT_NONE,
            STLT_SLOPE_UNIT_MAX
        };

        enum stlt_norm_t
        {
            STLT_NORM_AT_DC,
            STLT_NORM_AT_NYQUIST,
            STLT_NORM_AUTO,
            STLT_NORM_NONE,
            STLT_NORM_MAX
        };

        class SpectralTilt
        {
            private:
                SpectralTilt & operator = (const SpectralTilt &);
                SpectralTilt(const SpectralTilt &);

            protected:

                typedef struct bilinear_spec_t
                {
                    float b0;
                    float b1;

                    float a0;
                    float a1;
                } bq_spec_t;

            private:
                size_t              nOrder;

                stlt_slope_unit_t   enSlopeUnit;
                stlt_norm_t         enNorm;
                float               fSlopeVal;
                float               fSlopeNepNep;

                float               fLowerFrequency;
                float               fUpperFrequency;

                size_t              nSampleRate;

                bool                bBypass;
                bool                bSync;

                dspu::FilterBank    sFilter;

            public:
                explicit SpectralTilt();
                ~SpectralTilt();

                void construct();

            protected:
                float               bilinear_coefficient(float angularFrequency, float samplerate);
                bilinear_spec_t     compute_bilinear_element(float negZero, float negPole);

            public:
                /** Check that SpectralTilt needs settings update.
                 *
                 * @return true if SpectralTilt needs settings update.
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** This method should be called if needs_update() returns true.
                 * before calling processing methods.
                 *
                 */
                void update_settings();

                /** Set the order of the Spectral Tilt filter.
                 *
                 * @param order order of the filter.
                 */
                inline void set_order(size_t order)
                {
                    if (order == nOrder)
                        return;

                    nOrder  = order;
                    bSync   = true;
                }

                /** Set the slope of the Spectral Tilt filter.
                 *
                 * @param slope value.
                 * @param slopeType units of the slope value.
                 */
                inline void set_slope(float slope, stlt_slope_unit_t slopeType)
                {
                    if ((slope == fSlopeVal) && (slopeType == enSlopeUnit))
                        return;

                    if ((slopeType < STLT_SLOPE_UNIT_NEPER_PER_NEPER) || (slopeType >= STLT_SLOPE_UNIT_MAX))
                        return;

                    fSlopeVal   = slope;
                    enSlopeUnit = slopeType;
                    bSync       = true;
                }

                /** Set the normalisation policy of the Spectral Tilt filter.
                 *
                 * @param norm normalization policy.
                 */
                inline void set_norm(stlt_norm_t norm)
                {
                    if ((norm < STLT_NORM_AT_DC) || (norm >= STLT_NORM_MAX))
                        return;

                    enNorm = norm;
                    bSync = true;
                }

                /** Set the lower frequency of the coverage bandwidth.
                 *
                 * @param lowerFrequency lower frequency.
                 */
                inline void set_lower_frequency(float lowerFrequency)
                {
                    if (lowerFrequency == fLowerFrequency)
                        return;

                    fLowerFrequency = lowerFrequency;
                    bSync           = true;
                }

                /** Set the upper frequency of the coverage bandwidth.
                 *
                 * @param upperFrequency lower frequency.
                 */
                inline void set_upper_frequency(float upperFrequency)
                {
                    if (upperFrequency == fUpperFrequency)
                        return;

                    fUpperFrequency = upperFrequency;
                    bSync           = true;
                }

                /** Set sample rate for the Spectral Tilt filter.
                 *
                 * @param sr sample rate.
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (nSampleRate == sr)
                        return;

                    nSampleRate = sr;
                    bSync       = true;
                }

                /** Output sequence to the destination buffer in additive mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output sequence to the destination buffer in multiplicative mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output sequence to a destination buffer overwriting its content
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULLL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, const float *src, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };

    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_SPECTRALTILT_H_ */
