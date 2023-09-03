/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 20 Mar 2017
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

#include <lsp-plug.in/dsp-units/util/Oscillator.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/stdlib/math.h>

#define PROCESS_BUF_LIMIT_SIZE  (12 * 1024) // Multiple of 3, 4 and 8

namespace lsp
{
    namespace dspu
    {
        Oscillator::Oscillator()
        {
            construct();
        }

        Oscillator::~Oscillator()
        {
            destroy();
        }

        void Oscillator::construct()
        {
            enFunction                  = FG_SINE;
            fAmplitude                  = 1.0f;
            fFrequency                  = 0.0f;
            fDCOffset                   = 0.0f;
            enDCReference               = DC_WAVEDC;
            fReferencedDC               = 0.0f;
            fInitPhase                  = 0.0f;

            nSampleRate                 = -1;
            nPhaseAcc                   = 0;
            nPhaseAccBits               = sizeof(phacc_t) * 8;
            nPhaseAccMaxBits            = sizeof(phacc_t) * 8;
            nPhaseAccMask               = 0;
            fAcc2Phase                  = 0.0f;

            nFreqCtrlWord               = 0;
            nInitPhaseWord              = 0;

            sSquaredSinusoid.bInvert    = false;
            sSquaredSinusoid.fAmplitude = 0.0f;
            sSquaredSinusoid.fWaveDC    = 0.0f;

            sRectangular.fDutyRatio     = 0.5f;
            sRectangular.nDutyWord      = 0;
            sRectangular.fWaveDC        = 0.0f;
            sRectangular.fBLPeakAtten   = 0.0f;

            sSawtooth.fWidth            = 1.0f;
            sSawtooth.nWidthWord        = 0;
            sSawtooth.fCoeffs[0]        = 0.0f;
            sSawtooth.fCoeffs[1]        = 0.0f;
            sSawtooth.fCoeffs[2]        = 0.0f;
            sSawtooth.fCoeffs[3]        = 0.0f;
            sSawtooth.fWaveDC           = 0.0f;
            sSawtooth.fBLPeakAtten      = 0.0f;

            sTrapezoid.fRaiseRatio      = 0.25f;
            sTrapezoid.fFallRatio       = 0.25f;
            sTrapezoid.nPoints[0]       = 0;
            sTrapezoid.nPoints[1]       = 0;
            sTrapezoid.nPoints[2]       = 0;
            sTrapezoid.nPoints[3]       = 0;
            sTrapezoid.fCoeffs[0]       = 0.0f;
            sTrapezoid.fCoeffs[1]       = 0.0f;
            sTrapezoid.fCoeffs[2]       = 0.0f;
            sTrapezoid.fCoeffs[3]       = 0.0f;
            sTrapezoid.fWaveDC          = 0.0f;
            sTrapezoid.fBLPeakAtten     = 0.0f;

            sPulse.fPosWidthRatio       = 0.0f;
            sPulse.fNegWidthRatio       = 0.0f;
            sPulse.nTrainPoints[0]      = 0;
            sPulse.nTrainPoints[1]      = 0;
            sPulse.nTrainPoints[2]      = 0;
            sPulse.fWaveDC              = 0.0f;
            sPulse.fBLPeakAtten         = 0.0f;

            sParabolic.bInvert          = false;
            sParabolic.fAmplitude       = 1.0f;
            sParabolic.fWidth           = 0.0f;
            sParabolic.nWidthWord       = 0;
            sParabolic.fWaveDC          = 0.0f;
            sParabolic.fBLPeakAtten     = 0.0f;

            nOversampling               = 0;
            enOverMode                  = OM_NONE;
            vProcessBuffer              = NULL;
            vSynthBuffer                = NULL;
            pData                       = NULL;

            nFreqCtrlWord_Over          = 0;

            bSync                       = true;
        }

        bool Oscillator::init()
        {
            size_t samples      = PROCESS_BUF_LIMIT_SIZE + PROCESS_BUF_LIMIT_SIZE;
            uint8_t *ptr        = alloc_aligned<uint8_t>(pData, samples * sizeof(float), DEFAULT_ALIGN);
            if (ptr == NULL)
                return false;

            vProcessBuffer      = reinterpret_cast<float *>(ptr);
            ptr                += PROCESS_BUF_LIMIT_SIZE * sizeof(float);
            vSynthBuffer        = reinterpret_cast<float *>(ptr);
            ptr                += PROCESS_BUF_LIMIT_SIZE * sizeof(float);

            bool sOverInitialized           = sOver.init();
            bool sOverGetPeriodsInitialized = sOverGetPeriods.init();

            return sOverInitialized && sOverGetPeriodsInitialized;
        }

        void Oscillator::destroy()
        {
            sOver.destroy();
            sOverGetPeriods.destroy();

            if (pData != NULL)
            {
                free_aligned(pData);
                pData = NULL;
            }
            vProcessBuffer = NULL;
            vSynthBuffer   = NULL;
        }

        void Oscillator::update_settings()
        {
            if (!bSync)
                return;

            if (nPhaseAccBits == nPhaseAccMaxBits)
                nPhaseAccMask    = phacc_t(-1);
            else
                nPhaseAccMask    = (phacc_t(1) << nPhaseAccBits) - phacc_t(1);

            fAcc2Phase       = 2.0 * M_PI * (1.0 / (nPhaseAccMask + 1.0));
            nFreqCtrlWord    = ((nPhaseAccMask + 1.0) * fFrequency) / nSampleRate;

            nPhaseAcc        = (nPhaseAcc - nInitPhaseWord) & nPhaseAccMask;
            nInitPhaseWord   = (nPhaseAccMask + 1.0) * 0.5 * M_1_PI * (fInitPhase - 2.0 * M_PI * floor(fInitPhase * 0.5 * M_1_PI));
            nPhaseAcc        = (nPhaseAcc + nInitPhaseWord) & nPhaseAccMask;

            switch (enFunction)
            {
                case FG_SINE:
                case FG_COSINE:
                    fReferencedDC = fDCOffset;
                    break;
                case FG_SQUARED_SINE:
                case FG_SQUARED_COSINE:
                {
                    if (sSquaredSinusoid.bInvert)
                        sSquaredSinusoid.fAmplitude = -fAmplitude;
                    else
                        sSquaredSinusoid.fAmplitude = fAmplitude;

                    sSquaredSinusoid.fWaveDC = 0.5f * sSquaredSinusoid.fAmplitude;

                    switch (enDCReference)
                    {
                        case DC_ZERO:
                            fReferencedDC = fDCOffset - sSquaredSinusoid.fWaveDC;
                            break;
                        case DC_WAVEDC:
                        default:
                            fReferencedDC = fDCOffset;
                            break;
                    }
                }
                break;

                case FG_RECTANGULAR:
                case FG_BL_RECTANGULAR:
                {
                    if (sRectangular.fDutyRatio == 1.0f)
                        sRectangular.nDutyWord  = nPhaseAccMask;
                    else
                        sRectangular.nDutyWord  = sRectangular.fDutyRatio * (nPhaseAccMask + 1.0f);

                    sRectangular.fWaveDC        = fAmplitude * (2.0f * sRectangular.fDutyRatio - 1.0f);

                    switch (enDCReference)
                    {
                        case DC_ZERO:
                            fReferencedDC = fDCOffset - sRectangular.fWaveDC;
                            break;
                        case DC_WAVEDC:
                        default:
                            fReferencedDC = fDCOffset;
                            break;
                    }

                    sRectangular.fBLPeakAtten = 0.6f;
                }
                break;

                case FG_SAWTOOTH:
                case FG_BL_SAWTOOTH:
                {
                    if (sSawtooth.fWidth == 1.0f) // Prevent overflow
                        sSawtooth.nWidthWord = nPhaseAccMask;
                    else
                        sSawtooth.nWidthWord       = sSawtooth.fWidth * (nPhaseAccMask + 1.0f);

                    sSawtooth.fCoeffs[0]    = 2.0f * fAmplitude / sSawtooth.nWidthWord;
                    sSawtooth.fCoeffs[1]    = -fAmplitude;
                    sSawtooth.fCoeffs[2]    = (-2.0f * fAmplitude) / (nPhaseAccMask + 1.0f - sSawtooth.nWidthWord);
                    sSawtooth.fCoeffs[3]    = fAmplitude * (nPhaseAccMask + 1.0f + sSawtooth.nWidthWord) / (nPhaseAccMask + 1.0f - sSawtooth.nWidthWord);
                    sSawtooth.fWaveDC       = 0.0f;

                    fReferencedDC           = fDCOffset; //sSawtooth.fWaveDC == 0.0f

                    // Gibbs starts being noticeable at 6% or 94% with, so we drop
                    // linearly the amplitude.
                    if (sSawtooth.fWidth > 0.60f)
                        sSawtooth.fBLPeakAtten = 0.64f / 0.4f -sSawtooth.fWidth;
                    else if (sSawtooth.fWidth < 0.40f)
                        sSawtooth.fBLPeakAtten =  sSawtooth.fWidth + 0.6f;
                    else
                        sSawtooth.fBLPeakAtten = 1.0f;
                }
                break;

                case FG_TRAPEZOID:
                case FG_BL_TRAPEZOID:
                {
                    sTrapezoid.nPoints[0]   = sTrapezoid.fRaiseRatio * 0.5f * (nPhaseAccMask + 1.0f);
                    sTrapezoid.nPoints[1]   = (1.0f - sTrapezoid.fFallRatio) * 0.5f * (nPhaseAccMask + 1.0f);

                    if (sTrapezoid.fFallRatio < 1.0f) // Prevent overflow
                        sTrapezoid.nPoints[2]   = (1.0f + sTrapezoid.fFallRatio) * 0.5f * (nPhaseAccMask + 1.0f);
                    else
                        sTrapezoid.nPoints[2]   = nPhaseAccMask;

                    if (sTrapezoid.fRaiseRatio > 0.0f) // Prevent overflow
                        sTrapezoid.nPoints[3]   = (2.0f - sTrapezoid.fRaiseRatio) * 0.5f * (nPhaseAccMask + 1.0f);
                    else
                        sTrapezoid.nPoints[3]   = nPhaseAccMask;

                    sTrapezoid.fCoeffs[0]   = fAmplitude / sTrapezoid.nPoints[0];
                    sTrapezoid.fCoeffs[1]   = -2.0f * fAmplitude / (sTrapezoid.nPoints[2] - sTrapezoid.nPoints[1]);
                    sTrapezoid.fCoeffs[2]   = fAmplitude / sTrapezoid.fFallRatio;
                    sTrapezoid.fCoeffs[3]   = -2.0f * fAmplitude / sTrapezoid.fRaiseRatio;
                    sTrapezoid.fWaveDC      = 0.0f;

                    fReferencedDC           = fDCOffset; //sSawtooth.fWaveDC == 0.0f

                    // Gibbs starts being noticeable at 6% or 94% with, so we drop
                    // linearly the amplitude.
                    float minRatio = (sTrapezoid.fRaiseRatio < sTrapezoid.fFallRatio) ? sTrapezoid.fRaiseRatio : sTrapezoid.fFallRatio;

                    sTrapezoid.fBLPeakAtten = (minRatio < 0.4f) ? minRatio + 0.6f : 1.0f;
                }
                break;

                case FG_PULSETRAIN:
                case FG_BL_PULSETRAIN:
                {
                    sPulse.nTrainPoints[0]  = sPulse.fPosWidthRatio * 0.5f * (nPhaseAccMask + 1.0f);
                    sPulse.nTrainPoints[1]  = 0.5f * (nPhaseAccMask + 1.0f);
                    if (sPulse.fNegWidthRatio == 1.0f) // Prevent overflow
                        sPulse.nTrainPoints[2]  = nPhaseAccMask;
                    else
                        sPulse.nTrainPoints[2]  = (1.0f + sPulse.fNegWidthRatio) * 0.5f * (nPhaseAccMask + 1.0f);

                    sPulse.fWaveDC          = 0.5f * fAmplitude * (sPulse.fPosWidthRatio - sPulse.fNegWidthRatio);

                    switch (enDCReference)
                    {
                        case DC_ZERO:
                            fReferencedDC       = fDCOffset - sPulse.fWaveDC;
                            break;
                        case DC_WAVEDC:
                        default:
                            fReferencedDC       = fDCOffset;
                            break;
                    }

                    float maxRatio = (sPulse.fNegWidthRatio > sPulse.fPosWidthRatio) ? sPulse.fNegWidthRatio : sPulse.fPosWidthRatio;

                    sPulse.fBLPeakAtten     = (maxRatio > 0.5f) ? 0.6f : M_SQRT1_2;
                }
                break;

                case FG_PARABOLIC:
                case FG_BL_PARABOLIC:
                {
                    if (sParabolic.bInvert)
                        sParabolic.fAmplitude = -fAmplitude;
                    else
                        sParabolic.fAmplitude = fAmplitude;

                    if (sParabolic.fWidth == 1) // Prevent overflow
                        sParabolic.nWidthWord       = nPhaseAccMask;
                    else
                        sParabolic.nWidthWord       = sParabolic.fWidth * (nPhaseAccMask + 1.0f);

                    sParabolic.fWaveDC              = 2.0f * sParabolic.fAmplitude * sParabolic.fWidth / 3.0f;

                    switch (enDCReference)
                    {
                        case DC_ZERO:
                            fReferencedDC = fDCOffset - sParabolic.fWaveDC;
                            break;
                        case DC_WAVEDC:
                        default:
                            fReferencedDC = fDCOffset;
                            break;
                    }

                    sParabolic.fBLPeakAtten = 1.0f;
                }
                break;

            }

            // Oversamplers stuff:
            sOver.set_sample_rate(nSampleRate);
            sOver.set_mode(enOverMode);
            if (sOver.modified())
                sOver.update_settings();

            sOverGetPeriods.set_sample_rate(nSampleRate);
            sOverGetPeriods.set_mode(enOverMode);
            if (sOverGetPeriods.modified())
                sOverGetPeriods.update_settings();

            nOversampling       = sOver.get_oversampling();
            nFreqCtrlWord_Over  = nFreqCtrlWord / nOversampling;

            bSync               = false;
        }

        void Oscillator::do_process(Oversampler *os, float *dst, size_t count)
        {
            // Prevent overwrite of vProcessBuffer when the size of processed data is smaller
            // or equal the size of the original data (before oversampling) by imposing
            // dst != vProcessBuffer
            if (dst == vProcessBuffer)
                return;

            switch (enFunction)
            {
                case FG_SINE:
                    while (count--)
                    {
                        *(dst++)    = fAmplitude * sin(fAcc2Phase * nPhaseAcc) + fReferencedDC;
                        nPhaseAcc   = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_COSINE:
                    while (count--)
                    {
                        *(dst++)    = fAmplitude * cos(fAcc2Phase * nPhaseAcc) + fReferencedDC;
                        nPhaseAcc   = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_SQUARED_SINE:
                    while (count--)
                    {
                        // We put a 0.5 as simple squaring sinusoids will double the
                        // fundamental frequency with respect fFrequency.
                        float x     = sin(0.5f * fAcc2Phase * nPhaseAcc);
                        *(dst++)    = sSquaredSinusoid.fAmplitude * x * x + fReferencedDC;
                        nPhaseAcc   = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_SQUARED_COSINE:
                    while (count--)
                    {
                        // We put a 0.5 as simple squaring sinusoids will double the
                        // fundamental frequency with respect fFrequency.
                        float x     = cos(0.5f * fAcc2Phase * nPhaseAcc);
                        *(dst++)    = sSquaredSinusoid.fAmplitude * x * x + fReferencedDC;
                        nPhaseAcc   = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_RECTANGULAR:
                    while (count--)
                    {
                        *(dst++)    = ((nPhaseAcc < sRectangular.nDutyWord) ? fAmplitude : -fAmplitude) + fReferencedDC;
                        nPhaseAcc   = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_SAWTOOTH:
                    while (count--)
                    {
                        if (nPhaseAcc < sSawtooth.nWidthWord)
                            *(dst++)    = sSawtooth.fCoeffs[0] * nPhaseAcc + sSawtooth.fCoeffs[1] + fReferencedDC;
                        else
                            *(dst++)    = sSawtooth.fCoeffs[2] * nPhaseAcc + sSawtooth.fCoeffs[3] + fReferencedDC;

                        nPhaseAcc       = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_TRAPEZOID:
                    while (count--)
                    {
                        if (nPhaseAcc < sTrapezoid.nPoints[0])
                            *(dst++)    = sTrapezoid.fCoeffs[0] * nPhaseAcc + fReferencedDC;

                        if ((nPhaseAcc >= sTrapezoid.nPoints[0]) && (nPhaseAcc <= sTrapezoid.nPoints[1]))
                            *(dst++)    = fAmplitude + fReferencedDC;

                        if ((nPhaseAcc > sTrapezoid.nPoints[1]) && (nPhaseAcc < sTrapezoid.nPoints[2]))
                            *(dst++)    = sTrapezoid.fCoeffs[1] * nPhaseAcc + sTrapezoid.fCoeffs[2] + fReferencedDC;

                        if ((nPhaseAcc >= sTrapezoid.nPoints[2]) && (nPhaseAcc <= sTrapezoid.nPoints[3]))
                            *(dst++)    = fReferencedDC - fAmplitude;

                        if (nPhaseAcc > sTrapezoid.nPoints[3])
                            *(dst++)    = sTrapezoid.fCoeffs[0] * nPhaseAcc + sTrapezoid.fCoeffs[3] + fReferencedDC;

                        nPhaseAcc       = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;

                    }
                    break;

                case FG_PULSETRAIN:
                    while (count--)
                    {
                        if (nPhaseAcc <= sPulse.nTrainPoints[0])
                            *(dst++)    = fAmplitude + fReferencedDC;
                        else if ((nPhaseAcc >= sPulse.nTrainPoints[1]) && (nPhaseAcc <= sPulse.nTrainPoints[2]))
                            *(dst++)    = fReferencedDC - fAmplitude;
                        else
                            *(dst++)    = 0.0f + fReferencedDC;

                        nPhaseAcc       = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_PARABOLIC:
                    while (count--)
                    {
                        if (nPhaseAcc < sParabolic.nWidthWord)
                        {
                            float x     = (2.0f / sParabolic.nWidthWord) * nPhaseAcc - 1.0f;
                            *(dst++)    = sParabolic.fAmplitude * (1.0f - x*x) + fReferencedDC;
                        }
                        else
                            *(dst++)    = 0.0f + fReferencedDC;

                        nPhaseAcc       = (nPhaseAcc + nFreqCtrlWord) & nPhaseAccMask;
                    }
                    break;

                case FG_BL_RECTANGULAR:
                {
                    size_t buf_size     = PROCESS_BUF_LIMIT_SIZE / nOversampling;

                    while (count > 0)
                    {
                        size_t to_do        = (count > buf_size) ? buf_size : count;
                        size_t synthCount   = nOversampling * to_do;

                        for (size_t n = 0; n < synthCount; ++n)
                        {
                            vProcessBuffer[n]   = sRectangular.fBLPeakAtten * (((nPhaseAcc < sRectangular.nDutyWord) ? fAmplitude : -fAmplitude) + fReferencedDC);
                            nPhaseAcc           = (nPhaseAcc + nFreqCtrlWord_Over) & nPhaseAccMask;
                        }

                        os->downsample(dst, vProcessBuffer, to_do);

                        dst             += to_do;
                        count           -= to_do;
                    }

                }
                break;

                case FG_BL_SAWTOOTH:
                {
                    size_t buf_size     = PROCESS_BUF_LIMIT_SIZE / nOversampling;

                    while (count > 0)
                    {
                        size_t to_do        = (count > buf_size) ? buf_size : count;
                        size_t synthCount   = nOversampling * to_do;

                        for (size_t n = 0; n < synthCount; ++n)
                        {
                            if (nPhaseAcc < sSawtooth.nWidthWord)
                                vProcessBuffer[n]    = sSawtooth.fBLPeakAtten * (sSawtooth.fCoeffs[0] * nPhaseAcc + sSawtooth.fCoeffs[1] + fReferencedDC);
                            else
                                vProcessBuffer[n]    = sSawtooth.fBLPeakAtten * (sSawtooth.fCoeffs[2] * nPhaseAcc + sSawtooth.fCoeffs[3] + fReferencedDC);

                            nPhaseAcc           = (nPhaseAcc + nFreqCtrlWord_Over) & nPhaseAccMask;
                        }

                        os->downsample(dst, vProcessBuffer, to_do);

                        dst             += to_do;
                        count           -= to_do;
                    }

                }
                break;

                case FG_BL_TRAPEZOID:
                {
                    size_t buf_size     = PROCESS_BUF_LIMIT_SIZE / nOversampling;

                    while (count > 0)
                    {
                        size_t to_do        = (count > buf_size) ? buf_size : count;
                        size_t synthCount   = nOversampling * to_do;

                        for (size_t n = 0; n < synthCount; ++n)
                        {
                            if (nPhaseAcc < sTrapezoid.nPoints[0])
                                vProcessBuffer[n]    = sTrapezoid.fBLPeakAtten * (sTrapezoid.fCoeffs[0] * nPhaseAcc + fReferencedDC);

                            if ((nPhaseAcc >= sTrapezoid.nPoints[0]) && (nPhaseAcc <= sTrapezoid.nPoints[1]))
                                vProcessBuffer[n]    = sTrapezoid.fBLPeakAtten * (fAmplitude + fReferencedDC);

                            if ((nPhaseAcc > sTrapezoid.nPoints[1]) && (nPhaseAcc < sTrapezoid.nPoints[2]))
                                vProcessBuffer[n]    = sTrapezoid.fBLPeakAtten * (sTrapezoid.fCoeffs[1] * nPhaseAcc + sTrapezoid.fCoeffs[2] + fReferencedDC);

                            if ((nPhaseAcc >= sTrapezoid.nPoints[2]) && (nPhaseAcc <= sTrapezoid.nPoints[3]))
                                vProcessBuffer[n]    = sTrapezoid.fBLPeakAtten * (fReferencedDC - fAmplitude);

                            if (nPhaseAcc > sTrapezoid.nPoints[3])
                                vProcessBuffer[n]    = sTrapezoid.fBLPeakAtten * (sTrapezoid.fCoeffs[0] * nPhaseAcc + sTrapezoid.fCoeffs[3] + fReferencedDC);

                            nPhaseAcc           = (nPhaseAcc + nFreqCtrlWord_Over) & nPhaseAccMask;
                        }

                        os->downsample(dst, vProcessBuffer, to_do);

                        dst             += to_do;
                        count           -= to_do;
                    }

                }
                break;

                case FG_BL_PULSETRAIN:
                {
                    size_t buf_size     = PROCESS_BUF_LIMIT_SIZE / nOversampling;

                    while (count > 0)
                    {
                        size_t to_do        = (count > buf_size) ? buf_size : count;
                        size_t synthCount   = nOversampling * to_do;

                        for (size_t n = 0; n < synthCount; ++n)
                        {
                            if (nPhaseAcc <= sPulse.nTrainPoints[0])
                                vProcessBuffer[n]    = sPulse.fBLPeakAtten * (fAmplitude + fReferencedDC);
                            else if ((nPhaseAcc >= sPulse.nTrainPoints[1]) && (nPhaseAcc <= sPulse.nTrainPoints[2]))
                                vProcessBuffer[n]    = sPulse.fBLPeakAtten * (fReferencedDC - fAmplitude);
                            else
                                vProcessBuffer[n]    = 0.0f + fReferencedDC;

                            nPhaseAcc           = (nPhaseAcc + nFreqCtrlWord_Over) & nPhaseAccMask;
                        }

                        os->downsample(dst, vProcessBuffer, to_do);

                        dst             += to_do;
                        count           -= to_do;
                    }

                }
                break;

                case FG_BL_PARABOLIC:
                {
                    size_t buf_size     = PROCESS_BUF_LIMIT_SIZE / nOversampling;

                    while (count > 0)
                    {
                        size_t to_do        = (count > buf_size) ? buf_size : count;
                        size_t synthCount   = nOversampling * to_do;

                        for (size_t n = 0; n < synthCount; ++n)
                        {
                            if (nPhaseAcc < sParabolic.nWidthWord)
                            {
                                float x             = (2.0f / sParabolic.nWidthWord) * nPhaseAcc - 1.0f;
                                vProcessBuffer[n]   = sParabolic.fBLPeakAtten * (sParabolic.fAmplitude * (1.0f - x*x) + fReferencedDC); // Do not use pow(x, 2). Simple x*x multiplication is faster
                            }
                            else
                                vProcessBuffer[n]    = 0.0f + fReferencedDC;

                            nPhaseAcc           = (nPhaseAcc + nFreqCtrlWord_Over) & nPhaseAccMask;
                        }

                        os->downsample(dst, vProcessBuffer, to_do);

                        dst             += to_do;
                        count           -= to_do;
                    }

                }
                break;

                default:
                    break;
            }
        }

        void Oscillator::get_periods(float *dst, size_t periods, size_t periodsSkip, size_t samples)
        {
            phacc_t nPhaseAcc_Backup    = nPhaseAcc;
            nPhaseAcc                   = nInitPhaseWord;

            float periodDuration        = float(nSampleRate) / fFrequency; /* samples / period */
            float out_samples           = periodDuration * float(periods);
            float skip_samples          = periodDuration * float(periodsSkip);
            float decimationStep        = out_samples / float(samples);

            // Create and assign the samples of the periods to return.
            ssize_t buf_size            = 0;

            // Create the samples of the first periods to skip
            while (skip_samples > 0.0f)
            {
                // Request new buffer
                size_t to_do    = ceil(out_samples + skip_samples + decimationStep);
                if (to_do > PROCESS_BUF_LIMIT_SIZE)
                    to_do           = PROCESS_BUF_LIMIT_SIZE;

                do_process(&sOverGetPeriods, vSynthBuffer, to_do);

                buf_size        = to_do;
                skip_samples   -= to_do;
            }

            float t                 = buf_size + skip_samples; // t points to the beginning of first period

            while (samples > 0)
            {
                if (t < buf_size)
                {
                    *(dst++)        = vSynthBuffer[size_t(t)];
                    t              += decimationStep;
                    samples        --;
                }
                else
                {
                    // Request new buffer
                    size_t to_do    = ceil(out_samples + decimationStep);
                    if (to_do > PROCESS_BUF_LIMIT_SIZE)
                        to_do           = PROCESS_BUF_LIMIT_SIZE;

                    do_process(&sOverGetPeriods, vSynthBuffer, to_do);

                    // Update counters
                    out_samples    -= to_do;
                    buf_size        = PROCESS_BUF_LIMIT_SIZE;
                    t              -= buf_size;
                }
            }

            nPhaseAcc = nPhaseAcc_Backup;
        }

        void Oscillator::process_add(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = (count > PROCESS_BUF_LIMIT_SIZE) ? PROCESS_BUF_LIMIT_SIZE : count;

                do_process(&sOver, vSynthBuffer, to_do);
                dsp::add2(dst, vSynthBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Oscillator::process_mul(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = (count > PROCESS_BUF_LIMIT_SIZE) ? PROCESS_BUF_LIMIT_SIZE : count;

                do_process(&sOver, vSynthBuffer, to_do);
                dsp::mul2(dst, vSynthBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Oscillator::process_overwrite(float *dst, size_t count)
        {
            update_settings();

            while (count > 0)
            {
                size_t to_do = (count > PROCESS_BUF_LIMIT_SIZE) ? PROCESS_BUF_LIMIT_SIZE : count;

                do_process(&sOver, vSynthBuffer, to_do);
                dsp::copy(dst, vSynthBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Oscillator::set_phase(float phase)
        {

            if (fInitPhase == phase)
                return;

            fInitPhase = phase;
            bSync = true;
        }

        float Oscillator::get_exact_phase() const
        {
            double x = (nPhaseAccMask + 1.0);
            return round(x * fInitPhase * 0.5f * M_1_PI) * (2.0 * M_PI / x);
        }

        void Oscillator::set_dc_offset(float dcOffset)
        {
            if (fDCOffset == dcOffset)
                return;

            fDCOffset = dcOffset;
        }

        void Oscillator::set_dc_reference(dc_reference_t dcReference)
        {
            enDCReference   = dcReference;
            bSync           = true;
        }

        void Oscillator::set_squared_sinusoid_inversion(bool invert)
        {
            if (sSquaredSinusoid.bInvert == invert)
                return;

            sSquaredSinusoid.bInvert = invert;
            bSync = true;
        }

        void Oscillator::set_parabolic_inversion(bool invert)
        {
            if (sParabolic.bInvert == invert)
                return;

            sParabolic.bInvert = invert;
            bSync = true;
        }

        void Oscillator::set_duty_ratio(float dutyRatio)
        {
            if ((sRectangular.fDutyRatio == dutyRatio) || (dutyRatio < 0.0f) || (dutyRatio > 1.0f))
                return;

            sRectangular.fDutyRatio = dutyRatio;
            bSync = true;
        }

        void Oscillator::set_width(float width)
        {
            if (width < 0.0f)
                width = 0.0f;
            else if (width > 1.0f)
                width = 1.0f;
            if (sSawtooth.fWidth == width)
                return;

            sSawtooth.fWidth = width;
            bSync = true;
        }

        void Oscillator::set_trapezoid_ratios(float raise, float fall)
        {
            if (raise < 0.0f)
                raise = 0.0f;
            else if (raise > 1.0f)
                raise = 1.0f;

            if (fall < 0.0f)
                fall = 0.0f;
            else if (fall > 1 - raise)
                fall = 1 - raise;

            if ((raise == sTrapezoid.fRaiseRatio) && (fall == sTrapezoid.fFallRatio))
                return;

            sTrapezoid.fRaiseRatio      = raise;
            sTrapezoid.fFallRatio       = fall;
            bSync                       = true;

        }

        void Oscillator::set_pulsetrain_ratios(float posWidthRatio, float negWidthRatio)
        {
            if (posWidthRatio < 0.0f)
                posWidthRatio = 0.0f;
            else if (posWidthRatio > 1.0f)
                posWidthRatio = 1.0f;

            if (negWidthRatio < 0.0f)
                negWidthRatio = 0.0f;
            else if (negWidthRatio > 1.0f)
                negWidthRatio = 1.0f;

            if ((posWidthRatio == sPulse.fPosWidthRatio) && (negWidthRatio == sPulse.fNegWidthRatio))
                return;

            sPulse.fPosWidthRatio   = posWidthRatio;
            sPulse.fNegWidthRatio   = negWidthRatio;
            bSync                   = true;
        }

        void Oscillator::set_parabolic_width(float width)
        {
            if (width < 0.0f)
                width = 0.0f;
            else if (width > 1.0f)
                width = 1.0f;

            if (sParabolic.fWidth == width)
                return;

            sParabolic.fWidth   = width;
            bSync               = true;
        }

        void Oscillator::set_oversampler_mode(over_mode_t mode)
        {
            if (mode == enOverMode)
                return;

            enOverMode = mode;
            bSync = true;
        }

        void Oscillator::set_amplitude(float amplitude)
        {
            if (fAmplitude == amplitude)
                return;

            fAmplitude = amplitude;
            bSync = true;
        }

        void Oscillator::dump(IStateDumper *v) const
        {
            v->write("enFunction", enFunction);
            v->write("fAmplitude", fAmplitude);
            v->write("fFrequency", fFrequency);
            v->write("fDCOffset", fDCOffset);
            v->write("enDCReference", enDCReference);
            v->write("fReferencedDC", fReferencedDC);
            v->write("fInitPhase", fInitPhase);

            v->write("nSampleRate", nSampleRate);
            v->write("nPhaseAcc", nPhaseAcc);
            v->write("nPhaseAccBits", nPhaseAccBits);
            v->write("nPhaseAccMaxBits", nPhaseAccMaxBits);
            v->write("nPhaseAccMask", nPhaseAccMask);
            v->write("fAcc2Phase", fAcc2Phase);

            v->write("nFreqCtrlWord", nFreqCtrlWord);
            v->write("nInitPhaseWord", nInitPhaseWord);

            v->begin_object("sSquaredSinusoid", &sSquaredSinusoid, sizeof(sSquaredSinusoid));
            {
                v->write("bInvert", sSquaredSinusoid.bInvert);
                v->write("fAmplitude", sSquaredSinusoid.fAmplitude);
                v->write("fWaveDC", sSquaredSinusoid.fWaveDC);
            }
            v->end_object();

            v->begin_object("sRectangular", &sRectangular, sizeof(sRectangular));
            {
                v->write("fDutyRatio", sRectangular.fDutyRatio);
                v->write("nDutyWord", sRectangular.nDutyWord);
                v->write("fWaveDC", sRectangular.fWaveDC);
                v->write("fBLPeakAtten", sRectangular.fBLPeakAtten);
            }
            v->end_object();

            v->begin_object("sSawtooth", &sSawtooth, sizeof(sSawtooth));
            {
                v->write("fWidth", sSawtooth.fWidth);
                v->write("nWidthWord", sSawtooth.nWidthWord);
                v->writev("fCoeffs", sSawtooth.fCoeffs, 4);
                v->write("fWaveDC", sSawtooth.fWaveDC);
                v->write("fBLPeakAtten", sSawtooth.fBLPeakAtten);
            }
            v->end_object();

            v->begin_object("sTrapezoid", &sTrapezoid, sizeof(sTrapezoid));
            {
                v->write("fRaiseRatio", sTrapezoid.fRaiseRatio);
                v->write("fFallRatio", sTrapezoid.fFallRatio);
                v->writev("nPoints", sTrapezoid.nPoints, 4);
                v->writev("fCoeffs", sTrapezoid.fCoeffs, 4);
                v->write("fWaveDC", sTrapezoid.fWaveDC);
                v->write("fBLPeakAtten", sTrapezoid.fBLPeakAtten);
            }
            v->end_object();

            v->begin_object("sPulse", &sPulse, sizeof(sPulse));
            {
                v->write("fPosWidthRatio", sPulse.fPosWidthRatio);
                v->write("fNegWidthRatio", sPulse.fNegWidthRatio);
                v->writev("nTrainPoints", sPulse.nTrainPoints, 3);
                v->write("fWaveDC", sPulse.fWaveDC);
                v->write("fBLPeakAtten", sPulse.fBLPeakAtten);
            }
            v->end_object();

            v->begin_object("sParabolic", &sParabolic, sizeof(sParabolic));
            {
                v->write("bInvert", sParabolic.bInvert);
                v->write("fAmplitude", sParabolic.fAmplitude);
                v->write("fWidth", sParabolic.fWidth);
                v->write("nWidthWord", sParabolic.nWidthWord);
                v->write("fWaveDC", sParabolic.fWaveDC);
                v->write("fBLPeakAtten", sParabolic.fBLPeakAtten);

            }
            v->end_object();

            v->write("vProcessBuffer", vProcessBuffer);
            v->write("vSynthBuffer", vSynthBuffer);
            v->write("pData", pData);

            v->write_object("sOver", &sOver);
            v->write_object("sOverGetPeriods", &sOverGetPeriods);

            v->write("nOversampling", nOversampling);
            v->write("enOverMode", enOverMode);
            v->write("nFreqCtrlWord_Over", nFreqCtrlWord_Over);
            v->write("bSync", bSync);
        }

    } /* namespace dspu */
} /* namespace lsp */
