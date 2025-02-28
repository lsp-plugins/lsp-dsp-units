/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RT_PLAN_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RT_PLAN_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/3d/rt/types.h>
#include <lsp-plug.in/dsp-units/3d/Allocator3D.h>
#include <lsp-plug.in/dsp-units/3d/Object3D.h>

namespace lsp
{
    namespace dspu
    {
        namespace rt
        {
            /**
             * This is a space cutting plan for the raytracing algorithm
             */
            typedef struct LSP_DSP_UNITS_PUBLIC plan_t
            {
                public:
                    Allocator3D<rt::split_t>    items;

                public:
                    explicit plan_t();
                    plan_t(const plan_t &) = delete;
                    plan_t(plan_t &&) = delete;
                    ~plan_t();
                
                    plan_t & operator = (const plan_t &) = delete;
                    plan_t & operator = (plan_t &&) = delete;

                public:
                    /**
                     * Clear plan: clear underlying structures
                     */
                    inline void     clear() { items.clear(); };

                    /**
                     * Flush plan: clear underlying structures and release memory
                     */
                    inline void     flush() { items.flush(); };

                    /**
                     * Check that the cutting plan is empty
                     * @return true if the cutting plan is empty
                     */
                    inline bool     is_empty() const { return items.size() == 0; }

                    /**
                     * Swap contents with another plan
                     * @param dst target plan to perform swap
                     */
                    inline void     swap(plan_t *dst) { items.swap(&dst->items);  }

                    /**
                     * Split raytrace plan and keep the only edges that are below the cutting plane
                     * @param pl cutting plane
                     * @return status of operation
                     */
                    status_t        cut_out(const dsp::vector3d_t *pl);

                    /**
                     * Split raytrace plan and keep the only edges that are above the cutting plane
                     * @param pl cutting plane
                     * @return status of operation
                     */
                    status_t        cut_in(const dsp::vector3d_t *pl);

                    /**
                     * Split raytrace plan and keep the only edges that are below the cutting plane,
                     * store all edges above the cutting plane to the other plan passed as parameter
                     * @param out plan to store all edges above the cutting plane
                     * @param pl cutting plane
                     * @return status of operation
                     */
                    status_t        split(plan_t *out, const dsp::vector3d_t *pl);

                    /**
                     * Add triangle to the plan
                     * @param pv three triangle points
                     * @return status of operation
                     */
                    status_t        add_triangle(const dsp::point3d_t *pv);

                    /**
                     * Add triangle to the plan
                     * @param t triangle to add
                     * @return status of operation
                     */
                    status_t        add_triangle(const rtm::triangle_t *t);

                    /**
                     * Add triangle to the plan
                     * @param pv three triangle points
                     * @return status of operation
                     */
                    rt::split_t    *add_edge(const dsp::point3d_t *pv);

                    /**
                     * Add triangle to the plan
                     * @param p1 the first point of edge
                     * @param p2 the second point of edge
                     * @return status of operation
                     */
                    rt::split_t    *add_edge(const dsp::point3d_t *p1, const dsp::point3d_t *p2);

            } plan_t;
        } /* namespace rt */
    } /* namespace dspu */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RT_PLAN_H_ */
