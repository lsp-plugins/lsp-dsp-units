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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_TYPES_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_TYPES_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        typedef ssize_t         vertex_index_t;     // Vertex index
        typedef ssize_t         normal_index_t;     // Normal index
        typedef ssize_t         edge_index_t;       // Edge index
        typedef ssize_t         triangle_index_t;   // Triangle index
        typedef ssize_t         face_index_t;       // Face index

        // Object structures
        struct obj_normal_t;
        struct obj_vertex_t;
        struct obj_edge_t;
        struct obj_triangle_t;
        struct obj_boundbox_t;

        typedef struct obj_normal_t: public dsp::vector3d_t
        {
            normal_index_t      id;         // Normal index
            void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
            ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
        } obj_normal_t;

        typedef struct obj_vertex_t: public dsp::point3d_t
        {
            vertex_index_t      id;         // Vertex index
            obj_edge_t         *ve;         // Edge list
            void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
            ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
        } obj_vertex_t;

        typedef struct obj_edge_t
        {
            edge_index_t        id;         // Edge index
            obj_vertex_t       *v[2];       // Pointers to vertexes
            obj_edge_t         *vlnk[2];    // Link to next edge for the vertex v[i]
            void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
            ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
        } obj_edge_t;

        typedef struct obj_triangle_t
        {
            triangle_index_t    id;         // Triangle index
            face_index_t        face;       // Face number
            obj_vertex_t       *v[3];       // Vertexes
            obj_edge_t         *e[3];       // Edges
            obj_normal_t       *n[3];       // Normals
            void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
            ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
        } obj_triangle_t;

        typedef struct obj_boundbox_t: public dsp::bound_box3d_t
        {
        } obj_boundbox_t;
    }
}



#endif /* LSP_PLUG_IN_DSP_UNITS_3D_TYPES_H_ */
