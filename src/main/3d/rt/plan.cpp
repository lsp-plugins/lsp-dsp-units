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

#include <lsp-plug.in/dsp-units/3d/rt/plan.h>

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

            plan_t::plan_t():
                items(1024)
            {
            }

            plan_t::~plan_t()
            {
                items.flush();
            }

            status_t plan_t::cut_out(const dsp::vector3d_t *pl)
            {
                plan_t tmp;
                rt::split_t *sp;

                RT_FOREACH(rt::split_t, s, items)
                    if (s->flags & SF_REMOVE) // Do not analyze the edge, it will be automatically removed
                        continue;

                    size_t tag = dsp::colocation_x2_v1pv(pl, s->p);

                    switch (tag)
                    {
                        case 0x06: // 1 2
                        case 0x09: // 2 1
                        case 0x0a: // 2 2
                            if (!tmp.items.alloc(s)) // Just copy segment to output
                                return STATUS_NO_MEM;
                            break;

                        case 0x02: // 0 2 -- p[0] is under the plane, p[1] is over the plane, cut p[1]
                            if (!(sp = tmp.items.alloc(s)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_pvv1(&sp->p[1], sp->p, pl);
                            break;

                        case 0x08: // 2 0 -- p[1] is under the plane, p[0] is over the plane, cut p[0]
                            if (!(sp = tmp.items.alloc(s)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_pvv1(&sp->p[0], sp->p, pl);
                            break;

                        default:
                            break;
                    }
                RT_FOREACH_END

                tmp.swap(this);
                return STATUS_OK;
            }

            status_t plan_t::cut_in(const dsp::vector3d_t *pl)
            {
                plan_t tmp;
                rt::split_t *sp;

                RT_FOREACH(rt::split_t, s, items)
                    if (s->flags & SF_REMOVE) // Do not analyze the edge, it will be automatically removed
                        continue;

                    size_t tag = dsp::colocation_x2_v1pv(pl, s->p);

                    switch (tag)
                    {
                        case 0x04: // 1 0
                        case 0x01: // 0 1
                        case 0x00: // 0 0
                            if (!tmp.items.alloc(s)) // Just copy segment to output
                                return STATUS_NO_MEM;
                            break;

                        case 0x02: // 0 2 -- p[0] is under the plane, p[1] is over the plane, cut p[0]
                            if (!(sp = tmp.items.alloc(s)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_pvv1(&sp->p[0], sp->p, pl);
                            break;

                        case 0x08: // 2 0 -- p[1] is under the plane, p[0] is over the plane, cut p[1]
                            if (!(sp = tmp.items.alloc(s)))
                                return STATUS_NO_MEM;
                            dsp::calc_split_point_pvv1(&sp->p[1], sp->p, pl);
                            break;

                        default:
                            break;
                    }
                RT_FOREACH_END

                tmp.swap(this);
                return STATUS_OK;
            }

            status_t plan_t::split(plan_t *out, const dsp::vector3d_t *pl)
            {
                plan_t xin, xout;

                dsp::point3d_t sp;
                rt::split_t *si, *so;

                RT_FOREACH(rt::split_t, s, items)
                    size_t tag = dsp::colocation_x2_v1pv(pl, s->p);

                    switch (tag)
                    {
                        case 0x00: // 0 0
                        case 0x01: // 0 1
                        case 0x04: // 1 0
                            // Edge is over the plane
                            if (!xout.items.alloc(s))
                                return STATUS_NO_MEM;
                            break;

                        case 0x06: // 1 2
                        case 0x09: // 2 1
                        case 0x0a: // 2 2
                            // Edge is under the plane
                            if (!xin.items.alloc(s))
                                return STATUS_NO_MEM;
                            break;

                        case 0x02: // 0 2
                            // p[1] is over the plane, p[0] is under
                            si          = xin.items.alloc();
                            so          = xout.items.alloc();
                            if ((!si) || (!so))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_pvv1(&sp, s->p, pl);
                            si->p[0]    = s->p[0];
                            si->p[1]    = sp;
                            si->flags   = s->flags;

                            so->p[0]    = sp;
                            so->p[1]    = s->p[1];
                            so->flags   = s->flags;
                            break;

                        case 0x08: // 2 0
                            // p[0] is over the plane, p[1] is under
                            si          = xin.items.alloc();
                            so          = xout.items.alloc();
                            if ((!si) || (!so))
                                return STATUS_NO_MEM;

                            dsp::calc_split_point_pvv1(&sp, s->p, pl);

                            si->p[0]    = sp;
                            si->p[1]    = s->p[1];
                            si->flags   = s->flags;

                            so->p[0]    = s->p[0];
                            so->p[1]    = sp;
                            so->flags   = s->flags;
                            break;

                        default:
                            break;
                    }
                RT_FOREACH_END

                // Swap contents
                xin.swap(this);
                xout.swap(out);

                return STATUS_OK;
            }

            status_t plan_t::add_triangle(const rtm::triangle_t *t)
            {
                rt::split_t *asp[3];
                if (items.alloc_n(asp, 3) != 3)
                    return STATUS_NO_MEM;

                asp[0]->p[0]    = *(t->v[0]);
                asp[0]->p[1]    = *(t->v[1]);
        //        asp[0]->sp      = *sp;
                asp[0]->flags   = 0;

                asp[1]->p[0]    = *(t->v[1]);
                asp[1]->p[1]    = *(t->v[2]);
        //        asp[1]->sp      = *sp;
                asp[1]->flags   = 0;

                asp[2]->p[0]    = *(t->v[2]);
                asp[2]->p[1]    = *(t->v[0]);
        //        asp[2]->sp      = *sp;
                asp[2]->flags   = 0; //SF_CULLBACK;      // After split of this edge, we need to perform a cull-back

                return STATUS_OK;
            }

            status_t plan_t::add_triangle(const dsp::point3d_t *pv)
            {
                rt::split_t *asp[3];
                if (items.alloc_n(asp, 3) != 3)
                    return STATUS_NO_MEM;

                asp[0]->p[0]    = pv[0];
                asp[0]->p[1]    = pv[1];
        //        asp[0]->sp      = *sp;
                asp[0]->flags   = 0;

                asp[1]->p[0]    = pv[1];
                asp[1]->p[1]    = pv[2];
        //        asp[1]->sp      = *sp;
                asp[1]->flags   = 0;

                asp[2]->p[0]    = pv[2];
                asp[2]->p[1]    = pv[0];
        //        asp[2]->sp      = *sp;
                asp[2]->flags   = 0; //SF_CULLBACK;      // After split of this edge, we need to perform a cull-back

                return STATUS_OK;
            }

        //    rt_split_t *plan_t::add_edge(const point3d_t *pv, const vector3d_t *sp)
            rt::split_t *plan_t::add_edge(const dsp::point3d_t *pv)
            {
                rt::split_t *asp    = items.alloc();
                if (asp != NULL)
                {
                    asp->p[0]       = pv[0];
                    asp->p[1]       = pv[1];
        //            asp->sp         = *sp;
                    asp->flags      = 0;
                }

                return asp;
            }

        //    rt_split_t *plan_t::add_edge(const point3d_t *p1, const point3d_t *p2, const vector3d_t *sp)
            rt::split_t *plan_t::add_edge(const dsp::point3d_t *p1, const dsp::point3d_t *p2)
            {
                rt::split_t *asp    = items.alloc();
                if (asp != NULL)
                {
                    asp->p[0]       = *p1;
                    asp->p[1]       = *p2;
        //            asp->sp         = *sp;
                    asp->flags      = 0;
                }

                return asp;
            }

        } // namespace rt
    } // namespace dspu
} // namespace lsp




