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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RT_TYPES_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RT_TYPES_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        namespace rt
        {
            struct split_t;
            struct material_t;
            struct view_t;
            struct triangle_t;

            enum edge_flags_t
            {
                RT_EF_PLANE         = 1 << 0,       // The edge is part of split plane
                RT_EF_APPLY         = 1 << 1,       // The flag that requires the edge to be applied
            };

            enum rt_split_flags_t
            {
                SF_REMOVE           = 1 << 0,   // Remove the edge from the plan
            };

            /**
             * Progress reporting function
             * @param progress the progress value between 0 and 1
             * @param data user data
             * @return status of operation
             */
            typedef status_t    (*progress_func_t)(float progress, void *data);

        #pragma pack(push, 1)
            typedef struct split_t
            {
                dsp::point3d_t      p[2];       // Split points
                size_t              flags;      // Splitting flags
                __IF_64(uint64_t    __pad;)     // Alignment to be sizeof() multiple of 16
                __IF_32(uint32_t    __pad[3];)  // Alignment to be sizeof() multiple of 16
            } split_t;

            typedef struct triangle_t: public dsp::raw_triangle_t
            {
                dsp::vector3d_t     n;          // Normal
                ssize_t             oid;        // Object identifier
                ssize_t             face;       // Face identifier
                rt::material_t     *m;          // Material
                size_t              __pad;      // Alignment to be sizeof() multiple of 16
            } triangle_t;

            typedef struct material_t
            {
                float               absorption[2];      // The amount of energy that will be absorpted
                float               diffusion[2];       // The diffusion coefficients for reflected signal
                float               dispersion[2];      // The dispersion coefficients for refracted signal
                float               transparency[2];    // The amount of energy that will be passed-through the material
                float               permeability;       // Sound permeability of the object (inner sound speed / outer sound speed)
                float               __pad[3];           // Padding
            } material_t;

            typedef struct group_t
            {
                dsp::point3d_t      s;          // Source point
                dsp::point3d_t      p[3];       // View points
            } group_t;

            typedef struct view_t: public group_t
            {
                dsp::vector3d_t     pl[4];      // Culling planes
                float               time[3];    // The corresponding start time for each source point
                float               amplitude;  // The amplitude of the signal
                float               speed;      // This value indicates the current sound speed [m/s]
                float               location;   // The expected co-location to the next surface
                ssize_t             oid;        // Last interacted object identifier
                ssize_t             face;       // Last interacted object's face identifier
                ssize_t             rnum;       // The reflection number
                __IF_32(uint32_t    __pad[3];)  // Alignment to be sizeof() multiple of 16
            } view_t;
        #pragma pack(pop)
        } // namespace rt

        namespace rtx
        {
        #pragma pack(push, 1)
            typedef struct edge_t
            {
                dsp::point3d_t      v[2];       // Edge points
                ssize_t             itag;       // Tag
                __IF_64(uint64_t    __pad;)     // Alignment to be sizeof() multiple of 16
                __IF_32(uint32_t    __pad[3];)  // Alignment to be sizeof() multiple of 16
            } edge_t;

            typedef struct triangle_t: public dsp::raw_triangle_t
            {
                dsp::vector3d_t     n;          // Normal
                ssize_t             oid;        // Object identifier
                ssize_t             face;       // Face identifier
                rt::material_t     *m;          // Material
                rtx::edge_t        *e[3];       // Pointer to edges
                __IF_32(uint32_t    __pad[2];)  // Alignment to be sizeof() multiple of 16
            } triangle_t;
        #pragma pack(pop)
        } // namespace rtx

        namespace rtm
        {
            struct vertex_t;
            struct edge_t;
            struct triangle_t;

        #pragma pack(push, 1)
            typedef struct vertex_t: public dsp::point3d_t
            {
                void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
                ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
                __IF_32(uint32_t    __pad[2];)  // Alignment to be sizeof() multiple of 16
            } vertex_t;

            typedef struct edge_t
            {
                rtm::vertex_t      *v[2];       // Pointers to vertexes
                rtm::triangle_t    *vt;         // List of linked triangles
                void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
                ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
                rtm::edge_t        *vlnk[2];    // Link to the next edge for the vetex v[i]
                __IF_64(uint64_t    __pad;)     // Alignment to be sizeof() multiple of 16
                __IF_32(uint32_t    __pad;)     // Alignment to be sizeof() multiple of 16
            } edge_t;

            typedef struct triangle_t
            {
                rtm::vertex_t      *v[3];       // Vertexes
                rtm::edge_t        *e[3];       // Edges
                rtm::triangle_t    *elnk[3];    // Link to next triangle for the edge e[i]
                dsp::vector3d_t     n;          // Normal
                void               *ptag;       // Pointer tag, may be used by user for any data manipulation purpose
                ssize_t             itag;       // Integer tag, may be used by user for any data manipulation purpose
                ssize_t             oid;        // Object identifier
                ssize_t             face;       // Object's face identifier
                rt::material_t     *m;          // Material
                __IF_32(uint32_t    __pad[2];)  // Alignment to be sizeof() multiple of 16
            } triangle_t;

        #pragma pack(pop)
        } // namespace rtm
    } // namespace dspu
} // namespace lsp


#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RT_TYPES_H_ */
