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

#include <lsp-plug.in/dsp-units/3d/bsp/context.h>

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
        namespace bsp
        {
            context_t::context_t():
                node(256),
                triangle(1024)
            {
                root    = NULL;
            }

            context_t::~context_t()
            {
                flush();
            }

            void context_t::clear()
            {
                root    = NULL;
                node.clear();
                triangle.clear();
            }

            void context_t::flush()
            {
                root    = NULL;
                node.flush();
                triangle.flush();
            }

            status_t context_t::add_object(Object3D *obj, ssize_t oid, const dsp::matrix3d_t *transform, const dsp::color3d_t *col)
            {
                for (size_t i=0, n=obj->num_triangles(); i<n; ++i)
                {
                    obj_triangle_t *st  = obj->triangle(i);
                    bsp_triangle_t *dt  = triangle.alloc();
                    if (dt == NULL)
                        return STATUS_NO_MEM;

                    dsp::apply_matrix3d_mp2(&dt->v[0], st->v[0], transform);
                    dsp::apply_matrix3d_mp2(&dt->v[1], st->v[1], transform);
                    dsp::apply_matrix3d_mp2(&dt->v[2], st->v[2], transform);
                    dsp::calc_normal3d_pv(&dt->n[0], dt->v);
                    dt->n[1]            = dt->n[0];
                    dt->n[2]            = dt->n[0];

                    dt->c               = *col;
                    dt->oid             = oid;
                    dt->face            = st->face;
                }

                return STATUS_OK;
            }

            status_t context_t::build_tree()
            {
                // Build list of triangles for processing
                bsp_triangle_t *list = NULL;
                RT_FOREACH(bsp_triangle_t, t, triangle)
                    t->next     = list;
                    list        = t;
                RT_FOREACH_END;

                if (list == NULL) // There's nothing to do?
                    return STATUS_OK;

                // Create initial task
                lltl::parray<bsp::node_t> queue;
                if (!(root = node.alloc()))
                    return STATUS_NO_MEM;
                root->in    = NULL;
                root->out   = NULL;
                root->on    = list;
                root->emit  = false;
                if (!queue.add(root))
                    return STATUS_NO_MEM;

                // Do main iteration
                status_t res = STATUS_OK;
                bsp::node_t *task;
                while (queue.size() > 0)
                {
                    // Get the task
                    if (!queue.pop(&task))
                    {
                        res     = STATUS_CORRUPTED;
                        break;
                    }

                    // Process the task
                    if ((res = split(queue, task)) != STATUS_OK)
                        break;
                }

                queue.flush();
                return res;
            }

            status_t context_t::split(lltl::parray<bsp::node_t> &queue, bsp::node_t *task)
            {
                bsp_triangle_t *in = NULL, *out = NULL, *on = NULL;

                // Get current triangle
                // TODO: add selection of the best-matching surface for better tree structure
                bsp_triangle_t *ct = task->on;
                if (ct == NULL)
                    return STATUS_OK;

                size_t tag;
                dsp::calc_plane_pv(&task->pl, ct->v);
                bsp_triangle_t *t0, *t1;

                // Add current triangle to 'on' list and walk through
                on          = ct;
                ct          = ct->next;
                on->next    = NULL;

                // Process each triangle
                while (ct != NULL)
                {
                    bsp_triangle_t *nt  = ct->next;

                    tag = dsp::colocation_x3_v1pv(&task->pl, ct->v);

                    switch (tag)
                    {
                        // Triangle is above
                        case 0x00:  // 0 0 0
                        case 0x01:  // 0 0 1
                        case 0x04:  // 0 1 0
                        case 0x05:  // 0 1 1
                        case 0x10:  // 1 0 0
                        case 0x11:  // 1 0 1
                        case 0x14:  // 1 1 0
                            ct->next        = out;
                            out             = ct;
                            break;

                        // Triangle is on the plane
                        case 0x15:  // 1 1 1
                            ct->next        = on;
                            on              = ct;
                            break;

                        // Triangle is below the plane
                        case 0x16:  // 1 1 2
                        case 0x19:  // 1 2 1
                        case 0x1a:  // 1 2 2
                        case 0x25:  // 2 1 1
                        case 0x26:  // 2 1 2
                        case 0x29:  // 2 2 1
                        case 0x2a:  // 2 2 2
                            ct->next        = in;
                            in              = ct;
                            break;

                        // 1 intersection, 1 triangle above, 1 triangle below, clockwise
                        case 0x06:  // 0 1 2
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[0], &ct->v[2], &task->pl);

                            t0->v[0]        = ct->v[2];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;
                        case 0x21:  // 2 0 1
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[1], &ct->v[2], &task->pl);

                            t0->v[2]        = ct->v[1];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;
                        case 0x18:  // 1 2 0
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[0], &ct->v[1], &task->pl);

                            t0->v[1]        = ct->v[0];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;

                        // 1 intersection, 1 triangle above, 1 triangle below, clockwise
                        case 0x24:  // 2 1 0
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[0], &ct->v[2], &task->pl);

                            t0->v[2]        = ct->v[0];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;
                        case 0x12:  // 1 0 2
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[0], &ct->v[1], &task->pl);

                            t0->v[0]        = ct->v[1];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;
                        case 0x09:  // 0 2 1
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[1], &ct->v[2], &task->pl);

                            t0->v[1]        = ct->v[2];
                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            out             = t0;
                            break;

                        // 2 intersections, 1 triangle below, 2 triangles above
                        case 0x02:  // 0 0 2
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[0], &ct->v[1], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[0], &ct->v[2], &task->pl);

                            t0->v[0]        = ct->v[1];
                            t0->v[2]        = ct->v[2];
                            t1->v[0]        = ct->v[2];

                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            t1->next        = t0;
                            out             = t1;
                            break;
                        case 0x08:  // 0 2 0
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[1], &ct->v[0], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[1], &ct->v[2], &task->pl);

                            t0->v[0]        = ct->v[0];
                            t0->v[1]        = ct->v[2];
                            t1->v[1]        = ct->v[0];

                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            t1->next        = t0;
                            out             = t1;
                            break;
                        case 0x20:  // 2 0 0
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[2], &ct->v[0], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[2], &ct->v[1], &task->pl);

                            t0->v[1]        = ct->v[1];
                            t0->v[2]        = ct->v[0];
                            t1->v[2]        = ct->v[1];

                            ct->next        = in;
                            in              = ct;
                            t0->next        = out;
                            t1->next        = t0;
                            out             = t1;
                            break;

                        // 2 intersections, 1 triangle above, 2 triangles below
                        case 0x28:  // 2 2 0
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[0], &ct->v[1], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[0], &ct->v[2], &task->pl);

                            t0->v[0]        = ct->v[1];
                            t0->v[2]        = ct->v[2];
                            t1->v[0]        = ct->v[2];

                            ct->next        = out;
                            out             = ct;
                            t0->next        = in;
                            t1->next        = t0;
                            in              = t1;
                            break;

                        case 0x22:  // 2 0 2
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[1], &ct->v[0], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[2], &ct->v[1], &ct->v[2], &task->pl);

                            t0->v[0]        = ct->v[0];
                            t0->v[1]        = ct->v[2];
                            t1->v[1]        = ct->v[0];

                            ct->next        = out;
                            out             = ct;
                            t0->next        = in;
                            t1->next        = t0;
                            in              = t1;
                            break;

                        case 0x0a:  // 0 2 2
                            if (!(t0 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;
                            if (!(t1 = triangle.alloc(ct)))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_p2v1(&ct->v[0], &ct->v[2], &ct->v[0], &task->pl);
                            dsp::calc_split_point_p2v1(&ct->v[1], &ct->v[2], &ct->v[1], &task->pl);

                            t0->v[1]        = ct->v[1];
                            t0->v[2]        = ct->v[0];
                            t1->v[2]        = ct->v[1];

                            ct->next        = out;
                            out             = ct;
                            t0->next        = in;
                            t1->next        = t0;
                            in              = t1;
                            break;

                        default:
                            return STATUS_UNKNOWN_ERR;
                    }

                    // Move to next triangle
                    ct    = nt;
                }

                // Update task
                task->on    = on;
                if (out != NULL)
                {
                    bsp::node_t *tout   = node.alloc();
                    if (tout == NULL)
                        return STATUS_NO_MEM;
                    tout->in            = NULL;
                    tout->out           = NULL;
                    tout->on            = out;
                    tout->emit          = false;

                    if (!queue.add(tout))
                        return STATUS_NO_MEM;
                    task->out           = tout;
                }

                if (in != NULL)
                {
                    bsp::node_t *tin   = node.alloc();
                    if (tin == NULL)
                        return STATUS_NO_MEM;
                    tin->in            = NULL;
                    tin->out           = NULL;
                    tin->on            = in;
                    tin->emit          = false;

                    if (!queue.add(tin))
                        return STATUS_NO_MEM;
                    task->in           = tin;
                }

                return STATUS_OK;
            }

            status_t context_t::build_mesh(lltl::darray<view::vertex3d_t> *dst, const dsp::point3d_t *pov)
            {
                if (root == NULL)
                    return STATUS_OK;

                // Create queue
                lltl::parray<bsp::node_t> queue;
                bsp::node_t *curr   = root;
                curr->emit          = false;

                if (!queue.push(curr))
                    return STATUS_NO_MEM;

                view::vertex3d_t   *v;
                dsp::vector3d_t     pl;

                do
                {
                    // Get next task
                    if (!queue.pop(&curr))
                        return STATUS_NO_MEM;

                    if (curr->emit)
                    {
                        for (bsp_triangle_t *ct=curr->on; ct != NULL; ct = ct->next)
                        {
                            dsp::calc_plane_pv(&pl, ct->v);
                            float d         = pov->x*pl.dx + pov->y*pl.dy + pov->z*pl.dz + pl.dw;

                            // Allocate vertices
                            v       = dst->add_n(3);
                            if (v == NULL)
                                return STATUS_NO_MEM;

                            if (d < 0.0f)
                            {
                                // Reverse order of vertex and flip normals
                                v[0].p      = ct->v[0];
                                v[0].c      = ct->c;
                                dsp::flip_vector_v2(&v[0].n, &ct->n[0]);

                                v[1].p      = ct->v[2];
                                v[1].c      = ct->c;
                                dsp::flip_vector_v2(&v[1].n, &ct->n[2]);

                                v[2].p      = ct->v[1];
                                v[2].c      = ct->c;
                                dsp::flip_vector_v2(&v[2].n, &ct->n[1]);
                            }
                            else
                            {
                                // Emit as usual
                                v[0].p      = ct->v[0];
                                v[0].c      = ct->c;
                                v[0].n      = ct->n[0];

                                v[1].p      = ct->v[1];
                                v[1].c      = ct->c;
                                v[1].n      = ct->n[1];

                                v[2].p      = ct->v[2];
                                v[2].c      = ct->c;
                                v[2].n      = ct->n[2];
                            }
                        }
                    }
                    else
                    {
                        pl                  = curr->pl;
                        float d             = pov->x*pl.dx + pov->y*pl.dy + pov->z*pl.dz + pl.dw;
                        bsp::node_t *first  = (d < 0.0f) ? curr->out : curr->in;
                        bsp::node_t *last   = (d < 0.0f) ? curr->in : curr->out;

                        if (last != NULL)
                        {
                            last->emit  = false;
                            if (!(queue.push(last)))
                                return STATUS_NO_MEM;
                        }
                        if (curr->on != NULL)
                        {
                            curr->emit  = true;
                            if (!(queue.push(curr)))
                                return STATUS_NO_MEM;
                        }
                        if (first != NULL)
                        {
                            first->emit = false;
                            if (!(queue.push(first)))
                                return STATUS_NO_MEM;
                        }
                    }
                } while (queue.size() > 0);

                return STATUS_OK;
            }
        } // namespace bsp
    } // namespace dspu
} // namespace lsp




