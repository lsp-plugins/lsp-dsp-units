/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/dsp-units/3d/rt/context.h>
#include <lsp-plug.in/dsp-units/units.h>

#define RT_FOREACH(type, var, collection) \
    for (size_t __ci=0,__ne=collection.size(), __nc=collection.chunks(); (__ci<__nc) && (__ne>0); ++__ci) \
    { \
        type *var       = collection.chunk(__ci); \
        size_t __loops  = collection.chunk_size(); \
        if (__loops > __ne) __loops = __ne; \
        __ne -= __loops; \
        for ( ; __loops > 0; ++var, --__loops) \
        {

#define RT_FOREACH_BREAK    { __ne = 0; break; }

#define RT_FOREACH_END      } }

namespace lsp
{
    namespace dspu
    {
        namespace rt
        {
            context_t::context_t():
                triangle(1024)
            {
                this->state     = S_SCAN_OBJECTS;

                // Initialize point of view
                view.amplitude  = 0.0f;
                view.location   = 0.0f; // Undefined
                view.face       = -1;
                view.oid        = -1;
                view.speed      = LSP_DSP_UNITS_SOUND_SPEED_M_S;
                view.rnum       = 0;

                dsp::init_point_xyz(&view.s, 0.0f, 0.0f, 0.0f);
                dsp::init_point_xyz(&view.p[0], 0.0f, 0.0f, 0.0f);
                dsp::init_point_xyz(&view.p[1], 0.0f, 0.0f, 0.0f);
                dsp::init_point_xyz(&view.p[2], 0.0f, 0.0f, 0.0f);
            }

            context_t::context_t(const rt::view_t *view):
                triangle(1024)
            {
                this->state     = S_SCAN_OBJECTS;
                this->view      = *view;
            }

            context_t::context_t(const rt::view_t *view, rt::context_state_t state):
                triangle(1024)
            {
                this->state     = state;
                this->view      = *view;
            }

            context_t::~context_t()
            {
                plan.flush();
                triangle.flush();
            }

            int context_t::compare_triangles(const void *p1, const void *p2)
            {
                const rt_triangle_sort_t *t1 = reinterpret_cast<const rt_triangle_sort_t *>(p1);
                const rt_triangle_sort_t *t2 = reinterpret_cast<const rt_triangle_sort_t *>(p2);

                float x = t1->w - t2->w;
                if (x < -DSP_3D_TOLERANCE)
                    return -1;
                return (x > DSP_3D_TOLERANCE) ? 1 : 0;
            }

            status_t context_t::add_triangle(const rt::triangle_t *t)
            {
                size_t tag;
                dsp::point3d_t sp[2];

                // Initialize data structures
                dsp::raw_triangle_t buf1[16], buf2[16];
                size_t nbuf1, nbuf2;

                const dsp::vector3d_t *pl = view.pl;

                // Loop 0: 1 input triangle
                nbuf1 = 0;
                dsp::cull_triangle_raw(buf1, &nbuf1, pl, t);
                if (!nbuf1)
                    return STATUS_SKIP;
                ++pl;

                // Loop 1
                nbuf2 = 0;
                for (size_t j=0; j<nbuf1; ++j)
                    dsp::cull_triangle_raw(buf2, &nbuf2, pl, &buf1[j]);
                if (!nbuf2)
                    return STATUS_SKIP;
                ++pl;

                // Loop 2
                nbuf1 = 0;
                for (size_t j=0; j<nbuf2; ++j)
                    dsp::cull_triangle_raw(buf1, &nbuf1, pl, &buf2[j]);
                if (!nbuf1)
                    return STATUS_SKIP;
                ++pl;

                // Last step
                rt::triangle_t *nt;
                dsp::raw_triangle_t  *in = buf1;
                nbuf2 = 0;

                for (size_t j=0; j<nbuf1; ++j, ++in)
                {
                    tag = dsp::colocation_x3_v1pv(pl, in->v);

                    switch (tag)
                    {
                        // 0 intersections, 0 triangles
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            // Triangle is above, skip
                            continue;

                        case 0x15:  // 1 1 1
                            // Triangle is on the plane, skip
                            continue;

                        // 0 intersections, 1 triangle
                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            // Triangle is below the plane, copy and continue
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            ++nbuf2;
                            break;

                        // 1 intersection, 1 triangle
                        case 0x06:  // 0 1 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[0], &in->v[2], pl);
                            ++nbuf2;
                            break;
                        case 0x24:  // 2 1 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[0], &in->v[2], pl);
                            ++nbuf2;
                            break;

                        case 0x12:  // 1 0 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[0], &in->v[1], pl);
                            ++nbuf2;
                            break;
                        case 0x18:  // 1 2 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[0], &in->v[1], pl);
                            ++nbuf2;
                            break;

                        case 0x09:  // 0 2 1
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[1], &in->v[2], pl);
                            ++nbuf2;
                            break;
                        case 0x21:  // 2 0 1
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[1], &in->v[2], pl);
                            ++nbuf2;
                            break;

                        // 2 intersections, 1 triangle
                        case 0x02:  // 0 0 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[0], &in->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[0], &in->v[2], pl);
                            ++nbuf2;
                            break;
                        case 0x08:  // 0 2 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[1], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[1], &in->v[2], pl);
                            ++nbuf2;
                            break;
                        case 0x20:  // 2 0 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[2], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[2], &in->v[1], pl);
                            ++nbuf2;
                            break;

                        // 2 intersections, 2 triangles
                        case 0x28:  // 2 2 0
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[0], &in->v[1], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[0], &in->v[2], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[1];
                            nt->v[1]    = sp[0];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nbuf2 += 2;
                            break;

                        case 0x22:  // 2 0 2
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[1], &in->v[2], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[1], &in->v[0], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = sp[0];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = sp[1];
                            nt->v[2]    = sp[0];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nbuf2 += 2;
                            break;

                        case 0x0a:  // 0 2 2
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[2], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[2], &in->v[1], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = sp[0];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = sp[1];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nbuf2 += 2;
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }
                }

                return (nbuf2) ? STATUS_OK : STATUS_SKIP;
            }

            status_t context_t::add_triangle(const rtm::triangle_t *t)
            {
                size_t tag;
                dsp::point3d_t sp[2];

                // Initialize data structures
                dsp::raw_triangle_t buf1[16], buf2[16];
                size_t                  nout;
                size_t                  nin = 1;
                dsp::raw_triangle_t    *in  = buf1;
                dsp::raw_triangle_t    *out = buf2;

                in[0].v[0]      = *(t->v[0]);
                in[0].v[1]      = *(t->v[1]);
                in[0].v[2]      = *(t->v[2]);

                const dsp::vector3d_t *pl = view.pl;

                for (size_t i=0; i<3; ++i, ++pl)
                {
                    // Spit each triangle
                    nout = 0;

                    for (size_t j=0; j<nin; ++j, ++in)
                        dsp::cull_triangle_raw(out, &nout, pl, in);

                    if (!nout)
                        return STATUS_SKIP;

                    // Update state
                    nin     = nout;
                    if (i & 1)
                    {
                        in = buf1;
                        out = buf2;
                    }
                    else
                    {
                        in = buf2;
                        out = buf1;
                    }
                }

                // Last step
                rt::triangle_t *nt;
                nout = 0;

                for (size_t j=0; j<nin; ++j, ++in)
                {
                    tag = dsp::colocation_x3_v1pv(pl, in->v);

                    switch (tag)
                    {
                        // 0 intersections, 0 triangles
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            // Triangle is above, skip
                            continue;

                        case 0x15:  // 1 1 1
                            // Triangle is on the plane, skip
                            continue;

                        // 0 intersections, 1 triangle
                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            // Triangle is below the plane, copy and continue
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            ++out;
                            ++nout;
                            break;

                        // 1 intersection, 1 triangle
                        case 0x06:  // 0 1 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[0], &in->v[2], pl);
                            ++out;
                            ++nout;
                            break;
                        case 0x24:  // 2 1 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[0], &in->v[2], pl);
                            ++out;
                            ++nout;
                            break;

                        case 0x12:  // 1 0 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[0], &in->v[1], pl);
                            ++out;
                            ++nout;
                            break;
                        case 0x18:  // 1 2 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[0], &in->v[1], pl);
                            ++out;
                            ++nout;
                            break;

                        case 0x09:  // 0 2 1
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[1], &in->v[2], pl);
                            ++out;
                            ++nout;
                            break;
                        case 0x21:  // 2 0 1
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[1], &in->v[2], pl);
                            ++out; ++nout;
                            break;

                        // 2 intersections, 1 triangle
                        case 0x02:  // 0 0 2
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[0], &in->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[0], &in->v[2], pl);
                            ++out;
                            ++nout;
                            break;
                        case 0x08:  // 0 2 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                          //nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[1], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt->v[2], &in->v[1], &in->v[2], pl);
                            ++out;
                            ++nout;
                            break;
                        case 0x20:  // 2 0 0
                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                          //nt->v[0]    = in->v[0];
                          //nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            dsp::calc_split_point_p2v1(&nt->v[0], &in->v[2], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt->v[1], &in->v[2], &in->v[1], pl);
                            ++out;
                            ++nout;
                            break;

                        // 2 intersections, 2 triangles
                        case 0x28:  // 2 2 0
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[0], &in->v[1], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[0], &in->v[2], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[1];
                            nt->v[1]    = sp[0];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nout += 2;
                            break;

                        case 0x22:  // 2 0 2
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[1], &in->v[2], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[1], &in->v[0], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = sp[0];
                            nt->v[2]    = in->v[2];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = sp[1];
                            nt->v[2]    = sp[0];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nout += 2;
                            break;

                        case 0x0a:  // 0 2 2
                            dsp::calc_split_point_p2v1(&sp[0], &in->v[2], &in->v[0], pl);
                            dsp::calc_split_point_p2v1(&sp[1], &in->v[2], &in->v[1], pl);

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = in->v[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = sp[0];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            if (!(nt = triangle.alloc()))
                                return STATUS_NO_MEM;

                            nt->v[0]    = sp[0];
                            nt->v[1]    = in->v[1];
                            nt->v[2]    = sp[1];
                            nt->n       = t->n;
                            nt->oid     = t->oid;
                            nt->face    = t->face;
                            nt->m       = t->m;

                            nout += 2;
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }
                }

                return (nout) ? STATUS_OK : STATUS_SKIP;
            }

            status_t context_t::add_edge(const rtm::edge_t *e)
            {
                size_t tag;
                dsp::point3d_t sp[2];

                // Cut the split edge
                sp[0]       = *(e->v[0]);
                sp[1]       = *(e->v[1]);

                // Process each plane
                const dsp::vector3d_t *pl   = view.pl;

                for (size_t j=0; j<4; ++j, ++pl)
                {
                    tag = dsp::colocation_x2_v1pv(pl, sp);

                    switch (tag)
                    {
                        case 0x06: // 1 2
                        case 0x09: // 2 1
                        case 0x0a: // 2 2
                            // Split edge is under the plane
                            break;

                        case 0x02: // 0 2 -- p[0] is under the plane, p[1] is over the plane, cut p[1]
                            dsp::calc_split_point_pvv1(&sp[1], sp, pl);
                            break;

                        case 0x08: // 2 0 -- p[1] is under the plane, p[0] is over the plane, cut p[0]
                            dsp::calc_split_point_pvv1(&sp[0], sp, pl);
                            break;

                        default: // Split edge is over the plane or on the plane
                            return STATUS_OK;
                    }
                }

                // Add split plane to plan
                return (plan.add_edge(sp)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t context_t::add_edge(const rtx::edge_t *e)
            {
                size_t tag;
                dsp::point3d_t sp[2];

                // Cut the split edge
                sp[0]       = e->v[0];
                sp[1]       = e->v[1];

                // Process each plane
                const dsp::vector3d_t *pl   = view.pl;

                for (size_t j=0; j<4; ++j, ++pl)
                {
                    tag = dsp::colocation_x2_v1pv(pl, sp);

                    switch (tag)
                    {
                        case 0x06: // 1 2
                        case 0x09: // 2 1
                        case 0x0a: // 2 2
                            // Split edge is under the plane
                            break;

                        case 0x02: // 0 2 -- p[0] is under the plane, p[1] is over the plane, cut p[1]
                            dsp::calc_split_point_pvv1(&sp[1], sp, pl);
                            break;

                        case 0x08: // 2 0 -- p[1] is under the plane, p[0] is over the plane, cut p[0]
                            dsp::calc_split_point_pvv1(&sp[0], sp, pl);
                            break;

                        default: // Split edge is over the plane or on the plane
                            return STATUS_OK;
                    }
                }

                // Add split plane to plan
                return (plan.add_edge(sp)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t context_t::fetch_objects(rt::mesh_t *src, size_t n, const size_t *mask)
            {
                // Clean state
                triangle.clear();
                plan.clear();
                if (n <= 0) // Check size
                    return STATUS_OK;

                // Initialize cull planes
                size_t part, off;

                // Initialize itag
                RT_FOREACH(rtm::edge_t, e, src->edge)
                    e->itag     = 1;
                RT_FOREACH_END;

                // Build set of triangles
                status_t res;
                RT_FOREACH(rtm::triangle_t, t, src->triangle)
                    // Check ID match
                    part = t->oid / (sizeof(size_t) * 8);
                    off  = t->oid % (sizeof(size_t) * 8);
                    if (!(mask[part] & (size_t(1) << off)))
                        continue;

                    // Check that triangle matches specified object
                    if ((t->oid == view.oid) && (t->face == view.face))
                        continue;

                    // Add triangle
                    res = add_triangle(t);
                    if (res == STATUS_SKIP)
                        continue;
                    else if (res != STATUS_OK)
                        return res;

                    // Add edges to plan
                    if (t->e[0]->itag)
                    {
                        if ((res = add_edge(t->e[0])) != STATUS_OK)
                            return res;
                        t->e[0]->itag   = 0;
                    }
                    if (t->e[1]->itag)
                    {
                        if ((res = add_edge(t->e[1])) != STATUS_OK)
                            return res;
                        t->e[1]->itag   = 0;
                    }
                    if (t->e[2]->itag)
                    {
                        if ((res = add_edge(t->e[2])) != STATUS_OK)
                            return res;
                        t->e[2]->itag   = 0;
                    }
                RT_FOREACH_END;

                return STATUS_OK;
            }

            status_t context_t::add_opaque_object(const rt::triangle_t *vt, size_t n)
            {
                status_t res;

                // Process all input triangles
                for (size_t i=0; i<n; ++i)
                {
                    // Get triangle
                    const rt::triangle_t *t = &vt[i];

                    // Check that triangle is properly arranged to the source point
                    float d = t->n.dx * view.s.x + t->n.dy * view.s.y + t->n.dz * view.s.z + t->n.dw;
                    if (d <= DSP_3D_TOLERANCE)
                        continue;

                    // Add triangle
                    res = add_triangle(t);
                    if ((res != STATUS_SKIP) && (res != STATUS_OK))
                        return res;
                }

                return STATUS_OK;
            }

            status_t context_t::add_object(rtx::triangle_t *vt, rtx::edge_t *ve, size_t nt, size_t ne)
            {
                status_t res;

                // Set-up tag for the edge
                for (size_t i=0; i<ne; ++i)
                    ve[i].itag      = 1;

                // Add all triangles
                for (size_t i=0; i<nt; ++i)
                {
                    const rtx::triangle_t *t = &vt[i];
                    // Skip ignored triangles
                    if ((t->oid == view.oid) && (t->face == view.face))
                        continue;

                    // Add triangle
                    res = add_triangle(reinterpret_cast<const rt::triangle_t *>(&vt[i]));
                    if (res == STATUS_SKIP)
                        continue;
                    else if (res != STATUS_OK)
                        return res;

                    // Add edges to plan
                    if (t->e[0]->itag)
                    {
                        if ((res = add_edge(t->e[0])) != STATUS_OK)
                            return res;
                        t->e[0]->itag   = 0;
                    }
                    if (t->e[1]->itag)
                    {
                        if ((res = add_edge(t->e[1])) != STATUS_OK)
                            return res;
                        t->e[1]->itag   = 0;
                    }
                    if (t->e[2]->itag)
                    {
                        if ((res = add_edge(t->e[2])) != STATUS_OK)
                            return res;
                        t->e[2]->itag   = 0;
                    }
                }

                return STATUS_OK;
            }

            status_t context_t::cull_view()
            {
                dsp::vector3d_t pl[4]; // Split plane
                status_t res;

                // Initialize cull planes
                dsp::calc_rev_oriented_plane_p3(&pl[0], &view.s, &view.p[0], &view.p[1], &view.p[2]);
                dsp::calc_oriented_plane_p3(&pl[1], &view.p[2], &view.s, &view.p[0], &view.p[1]);
                dsp::calc_oriented_plane_p3(&pl[2], &view.p[0], &view.s, &view.p[1], &view.p[2]);
                dsp::calc_oriented_plane_p3(&pl[3], &view.p[1], &view.s, &view.p[2], &view.p[0]);

                for (size_t pi=0; pi<4; ++pi)
                {
                    res = cut(&pl[pi]);
                    if (res != STATUS_OK)
                        return res;

                    // Check that there is data for processing and take it for next iteration
                    if (triangle.size() <= 0)
                        break;
                }

                return STATUS_OK;
            }

            status_t context_t::cut(const dsp::vector3d_t *pl)
            {
                Allocator3D<rt::triangle_t> in(triangle.chunk_size());
                rt::triangle_t *nt1, *nt2;

                RT_FOREACH(rt::triangle_t, t, triangle)
                    size_t tag  = dsp::colocation_x3_v1pv(pl, t->v);

                    switch (tag)
                    {
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            // Triangle is above, skip
                            break;

                        case 0x15:  // 1 1 1
                            // Triangle is on the plane, skip
                            break;

                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            // Triangle is below, add and continue
                            if (!in.alloc(t))
                                return STATUS_NO_MEM;
                            break;

                        case 0x06:  // 0 1 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            break;
                        case 0x24:  // 2 1 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[2], pl);
                            break;

                        case 0x12:  // 1 0 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            break;
                        case 0x18:  // 1 2 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[1], pl);
                            break;

                        case 0x09:  // 0 2 1
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            break;
                        case 0x21:  // 2 0 1
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[1], &t->v[2], pl);
                            break;

                        case 0x02:  // 0 0 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            break;
                        case 0x08:  // 0 2 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[1], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            break;
                        case 0x20:  // 2 0 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[2], &t->v[1], pl);
                            break;

                        case 0x28:  // 2 2 0
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[0], &t->v[0], &t->v[2], pl);
                            nt2->v[1]   = nt1->v[0];
                            break;

                        case 0x22:  // 2 0 2
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[1], &t->v[2], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[1], &t->v[1], &t->v[0], pl);
                            nt2->v[2]   = nt1->v[1];
                            break;

                        case 0x0a:  // 0 2 2
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[2], &t->v[2], &t->v[1], pl);
                            nt2->v[0]   = nt1->v[2];
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }
                RT_FOREACH_END

                // Swap data and proceed
                in.swap(&this->triangle);

                return plan.cut_out(pl);
            }

            status_t context_t::cullback(const dsp::vector3d_t *pl)
            {
                Allocator3D<rt::triangle_t> in(triangle.chunk_size());
                rt::triangle_t *nt1, *nt2;

                RT_FOREACH(rt::triangle_t, t, triangle)
                    size_t tag  = dsp::colocation_x3_v1pv(pl, t->v);

                    switch (tag)
                    {
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            // Triangle is above, skip
                            break;

                        case 0x15:  // 1 1 1
                            // Triangle is on the plane, add and continue
                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            // Triangle is below, add and continue
                            if (!in.alloc(t))
                                return STATUS_NO_MEM;
                            break;

                        case 0x06:  // 0 1 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            break;
                        case 0x24:  // 2 1 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[2], pl);
                            break;

                        case 0x12:  // 1 0 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            break;
                        case 0x18:  // 1 2 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[1], pl);
                            break;

                        case 0x09:  // 0 2 1
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            break;
                        case 0x21:  // 2 0 1
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[1], &t->v[2], pl);
                            break;

                        case 0x02:  // 0 0 2
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            break;
                        case 0x08:  // 0 2 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[1], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            break;
                        case 0x20:  // 2 0 0
                            if (!(nt1 = in.alloc(t)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[2], &t->v[1], pl);
                            break;

                        case 0x28:  // 2 2 0
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[0], &t->v[0], &t->v[2], pl);
                            nt2->v[1]   = nt1->v[0];
                            break;

                        case 0x22:  // 2 0 2
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[1], &t->v[2], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[1], &t->v[1], &t->v[0], pl);
                            nt2->v[2]   = nt1->v[1];
                            break;

                        case 0x0a:  // 0 2 2
                            if ((!(nt1 = in.alloc(t))) || (!(nt2 = in.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt2->v[2], &t->v[2], &t->v[1], pl);
                            nt2->v[0]   = nt1->v[2];
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }
                RT_FOREACH_END

                // Swap data and proceed
                in.swap(&this->triangle);

                return plan.cut_out(pl);
            }

            status_t context_t::split(context_t *out, const dsp::vector3d_t *pl)
            {
                Allocator3D<rt::triangle_t> xin(triangle.chunk_size()), xout(triangle.chunk_size());
                rt::triangle_t *nt1, *nt2, *nt3;

                RT_FOREACH(rt::triangle_t, t, triangle)
                    size_t tag  = dsp::colocation_x3_v1pv(pl, t->v);

                    switch (tag)
                    {
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            // Triangle is above, add to xout
                            if (!xout.alloc(t))
                                return STATUS_NO_MEM;
                            break;

                        case 0x15:  // 1 1 1
                            // Triangle is on the plane, skip
                            break;

                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            // Triangle is below, add to xin
                            if (!xin.alloc(t))
                                return STATUS_NO_MEM;
                            break;

                        // Split into 2 triangles
                        case 0x06:  // 0 1 2
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            nt2->v[0]   = nt1->v[2];
                            break;
                        case 0x24:  // 2 1 0
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[2], pl);
                            nt2->v[2]   = nt1->v[0];
                            break;

                        case 0x12:  // 1 0 2
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            nt2->v[0]   = nt1->v[1];
                            break;
                        case 0x18:  // 1 2 0
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[0], &t->v[1], pl);
                            nt2->v[1]   = nt1->v[0];
                            break;

                        case 0x09:  // 0 2 1
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            nt2->v[1]   = nt1->v[2];
                            break;
                        case 0x21:  // 2 0 1
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[1], &t->v[2], pl);
                            nt2->v[2]   = nt1->v[1];
                            break;


                        // 2 triangles over the plane
                        case 0x02:  // 0 0 2
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))) || (!(nt3 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            nt2->v[0]   = nt1->v[2];
                            nt3->v[0]   = nt1->v[1];
                            nt3->v[2]   = nt1->v[2];
                            break;
                        case 0x08:  // 0 2 0
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))) || (!(nt3 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[1], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            nt2->v[1]   = nt1->v[0];
                            nt3->v[0]   = nt1->v[0];
                            nt3->v[1]   = nt1->v[2];
                            break;
                        case 0x20:  // 2 0 0
                            if ((!(nt1 = xin.alloc(t))) || (!(nt2 = xout.alloc(t))) || (!(nt3 = xout.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[2], &t->v[1], pl);
                            nt2->v[2]   = nt1->v[0];
                            nt3->v[0]   = nt1->v[0];
                            nt3->v[2]   = nt1->v[1];
                            break;

                        // 2 triangles under the plane
                        case 0x28:  // 2 2 0
                            if ((!(nt1 = xout.alloc(t))) || (!(nt2 = xin.alloc(t))) || (!(nt3 = xin.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[0], &t->v[1], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[0], &t->v[2], pl);
                            nt2->v[0]   = nt1->v[2];
                            nt3->v[0]   = nt1->v[1];
                            nt3->v[2]   = nt1->v[2];
                            break;

                        case 0x22:  // 2 0 2
                            if ((!(nt1 = xout.alloc(t))) || (!(nt2 = xin.alloc(t))) || (!(nt3 = xin.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[1], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[2], &t->v[1], &t->v[2], pl);
                            nt2->v[1]   = nt1->v[0];
                            nt3->v[0]   = nt1->v[0];
                            nt3->v[1]   = nt1->v[2];
                            break;

                        case 0x0a:  // 0 2 2
                            if ((!(nt1 = xout.alloc(t))) || (!(nt2 = xin.alloc(t))) || (!(nt3 = xin.alloc(t))))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&nt1->v[0], &t->v[2], &t->v[0], pl);
                            dsp::calc_split_point_p2v1(&nt1->v[1], &t->v[2], &t->v[1], pl);
                            nt2->v[2]   = nt1->v[0];
                            nt3->v[0]   = nt1->v[0];
                            nt3->v[2]   = nt1->v[1];
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }
                RT_FOREACH_END

                // Swap data and proceed
                xin.swap(&this->triangle);
                xout.swap(&out->triangle);

                return plan.split(&out->plan, pl);
            }

            status_t context_t::edge_split(context_t *out)
            {
                // Find edge to apply split
                if (plan.items.size() <= 0)
                    return STATUS_NOT_FOUND;

                dsp::vector3d_t pl;

                RT_FOREACH(rt::split_t, se, plan.items)
                    if (se->flags & SF_REMOVE)
                        continue;
                    se->flags      |= SF_REMOVE;        // Mark edge for removal

                    rt::split_t xe  = *se;

                    // Process split only with valid plane
                    if (dsp::calc_plane_p3(&pl, &view.s, &xe.p[0], &xe.p[1]) > DSP_3D_TOLERANCE)
                    {
                        status_t res = split(out, &pl);
                        if (res != STATUS_OK)
                            return res;
                    }

                    return STATUS_OK;
                RT_FOREACH_END

                return STATUS_NOT_FOUND;
            }

            status_t context_t::depth_test()
            {
                // Find the nearest to the point of view triangle
                rt::triangle_t *st = NULL;
                float d, dmin = 0.0f;

                RT_FOREACH(rt::triangle_t, t, triangle)
                    // Opaque triangles can not affect depth test
                    if (t->m == NULL)
                        continue;

                    d = dsp::calc_min_distance_pv(&view.s, t->v);
                    if ((st == NULL) || (d < dmin))
                    {
                        st      = t;
                        dmin    = d;
                    }
                RT_FOREACH_END;

                // There is no triangle to perform cull-back?
                if (st == NULL)
                    return STATUS_OK;

                // Build plane equation and perform cull-back
                dsp::vector3d_t v;
                dsp::orient_plane_v1p1(&v, &view.s, &st->n);
                return cullback(&v);
            }
        } /* namespace rt */
    } /* namespace dspu */
} /* namespace lsp */



