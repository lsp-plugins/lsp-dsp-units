/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 авг. 2021 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/3d/rt/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace dspu
    {
        enum rt_audio_source_t
        {
            RT_AS_TRIANGLE,     // For debug purposes
            RT_AS_TETRA,        // Tetrahedron source
            RT_AS_OCTA,         // Octa source
            RT_AS_BOX,          // Simple box source
            RT_AS_ICO,          // Icosahedron source
            RT_AS_CYLINDER,     // Cylinder
            RT_AS_CONE,         // Cone
            RT_AS_OCTASPHERE,   // Omni source (octasphere)
            RT_AS_ICOSPHERE,    // Omni source (icosphere)
            RT_AS_FSPOT,        // Flat spot
            RT_AS_CSPOT,        // Cylindric spot
            RT_AS_SSPOT,        // Spherical spot
        };

        enum rt_audio_capture_t
        {
            RT_AC_CARDIO,
            RT_AC_SCARDIO,
            RT_AC_HCARDIO,
            RT_AC_BIDIR,
            RT_AC_EIGHT,
            RT_AC_OMNI
        };

        enum rt_capture_config_t
        {
            RT_CC_MONO,
            RT_CC_XY,
            RT_CC_AB,
            RT_CC_ORTF,
            RT_CC_MS
        };

        typedef struct room_source_config_t
        {
            dsp::point3d_t          sPos;       // Position in 3D space
            float                   fYaw;       // Yaw angle (degrees)
            float                   fPitch;     // Pitch angle (degrees)
            float                   fRoll;      // Roll angle (degrees)
            rt_audio_source_t       enType;     // Type of source
            float                   fSize;      // Size/radius [m]
            float                   fHeight;    // Height [m]
            float                   fAngle;     // Dispersion angle [0..100] %
            float                   fCurvature; // Additional curvature [0..100] %
            float                   fAmplitude; // Initial amplitude of the signal
        } room_source_config_t;

        // Source configuration
        typedef struct rt_source_settings_t
        {
            dsp::matrix3d_t         pos;        // Position and direction of source
            rt_audio_source_t       type;       // Type of the the source
            float                   size;       // Size/radius [m]
            float                   height;     // Height [m]
            float                   angle;      // Dispersion angle [0..100] %
            float                   curvature;  // Additional curvature [0..100] %
            float                   amplitude;  // Initial amplitude of the signal
        } room_source_settings_t;

        // Capture configuration
        typedef struct room_capture_config_t
        {
            dsp::point3d_t          sPos;       // Position in 3D space
            float                   fYaw;       // Yaw angle (degrees)
            float                   fPitch;     // Pitch angle (degrees)
            float                   fRoll;      // Roll angle (degrees)
            float                   fCapsule;   // Capsule size
            rt_capture_config_t     sConfig;    // Capture configuration
            float                   fAngle;     // Capture angle between microphones
            float                   fDistance;  // Capture distance between AB microphones
            rt_audio_capture_t      enDirection;// Capture microphone direction
            rt_audio_capture_t      enSide;     // Side microphone direction
        } room_capture_config_t;

        typedef struct rt_capture_settings_t
        {
            dsp::matrix3d_t         pos;        // Position in 3D space
            float                   radius;     // Capture radius
            rt_audio_capture_t      type;       // Capture type
        } rt_capture_settings_t;

        /**
         * Generate raytracing source groups' mesh according to settings of the audio source
         * The function does not apply transform matrix to the output
         *
         * @param out raytracing groups
         * @param cfg source configuration
         * @return status of operation
         */
        LSP_DSP_UNITS_PUBLIC
        status_t rt_gen_source_mesh(lltl::darray<rt::group_t> &out, const rt_source_settings_t *cfg);

        /**
         * Generate raytracing capture mesh groups according to settings of the audio capture
         * The function does not apply transform matrix to the output
         *
         * @param out triangle mesh
         * @param cfg source configuration
         * @return status of operation
         */
        LSP_DSP_UNITS_PUBLIC
        status_t rt_gen_capture_mesh(lltl::darray<dsp::raw_triangle_t> &out, const rt_capture_settings_t *cfg);

        /**
         * Configure capture
         * @param n number of captures generated
         * @param settings array of two structures to store capture settings
         * @param cfg capture configuration
         * @return status of operation
         */
        LSP_DSP_UNITS_PUBLIC
        status_t rt_configure_capture(size_t *n, rt_capture_settings_t *settings, const room_capture_config_t *cfg);

        /**
         * Configure source settings
         * @param out source settings
         * @param in source configuration
         * @return status of operation
         */
        LSP_DSP_UNITS_PUBLIC
        status_t rt_configure_source(rt_source_settings_t *out, const room_source_config_t *in);

    } // namespace dspu
} // namespace lsp

#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RAYTRACE_H_ */
