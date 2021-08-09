/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 9 авг. 2021 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_VIEW_TYPES_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_VIEW_TYPES_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        namespace view
        {
            // Viewing structures
            #pragma pack(push, 1)
                typedef struct vertex3d_t
                {
                    dsp::point3d_t      p;          // Position
                    dsp::vector3d_t     n;          // Normal
                    dsp::color3d_t      c;          // Color
                } v_vertex3d_t;

                typedef struct triangle3d_t
                {
                    dsp::point3d_t      p[3];       // Positions
                    dsp::vector3d_t     n[3];       // Normals
                } v_triangle3d_t;

                typedef struct point3d_t
                {
                    dsp::point3d_t      p;          // Position
                    dsp::color3d_t      c;          // Color
                } v_point3d_t;

                typedef struct ray3d_t
                {
                    dsp::point3d_t      p;          // Position
                    dsp::vector3d_t     v;          // Direction
                    dsp::color3d_t      c;          // Color
                } v_ray3d_t;

                typedef struct segment3d_t
                {
                    dsp::point3d_t      p[2];       // Position
                    dsp::color3d_t      c[2];       // Color
                } v_segment3d_t;

            #pragma pack(pop)
        } // namespace view
    } // namespace dspu
} // namespace lsp



#endif /* LSP_PLUG_IN_DSP_UNITS_3D_VIEW_TYPES_H_ */
