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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_BSP_TYPES_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_BSP_TYPES_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        namespace bsp
        {
        #pragma pack(push, 1)
            typedef struct triangle_t: public dsp::raw_triangle_t
            {
                dsp::vector3d_t         n[3];       // Normals
                dsp::color3d_t          c;          // Color
                ssize_t                 oid;        // Object identifier
                size_t                  face;       // Face identifier
                bsp::triangle_t        *next;       // Pointer to next triangle
                size_t                  __pad;      // Alignment to be sizeof() multiple of 16
            } bsp_triangle_t;
        #pragma pack(pop)

            typedef struct node_t
            {
                dsp::vector3d_t         pl;
                bsp::node_t            *in;
                bsp::node_t            *out;
                bsp::triangle_t        *on;
                bool                    emit;
            } node_t;
        } // namespace bsp
    } // namespace dspu
} // namespace lsp



#endif /* LSP_PLUG_IN_DSP_UNITS_3D_BSP_TYPES_H_ */
