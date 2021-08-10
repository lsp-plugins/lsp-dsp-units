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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RT_CONTEXT_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RT_CONTEXT_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/3d/rt/types.h>
#include <lsp-plug.in/dsp-units/3d/rt/mesh.h>
#include <lsp-plug.in/dsp-units/3d/rt/plan.h>
#include <lsp-plug.in/dsp-units/3d/Allocator3D.h>
#include <lsp-plug.in/dsp-units/3d/Object3D.h>

namespace lsp
{
    namespace dspu
    {
        namespace rt
        {
            enum context_state_t
            {
                S_SCAN_OBJECTS,
                S_SPLIT,
                S_CULL_BACK,
                S_REFLECT
            };

            /**
             * Ray tracing context
             */
            typedef struct context_t
            {
                private:
                    context_t & operator = (const context_t &);
                    context_t(const context_t &);

                protected:
                    typedef struct rt_triangle_sort_t
                    {
                        rtm::triangle_t    *t;          // Pointer to triangle
                        float               w;          // Weight of edge
                    } rt_triangle_sort_t;

                public:
                    rt::view_t                  view;       // Ray tracing point of view
                    rt::context_state_t         state;      // Context state

                    rt::plan_t                  plan;       // Split plan
                    Allocator3D<rt::triangle_t> triangle;   // Triangle for raytracint

                protected:
                    static int      compare_triangles(const void *p1, const void *p2);
                    status_t        add_triangle(const rtm::triangle_t *t);
                    status_t        add_triangle(const rt::triangle_t *t);
                    status_t        add_edge(const rtm::edge_t *e);
                    status_t        add_edge(const rtx::edge_t *e);

                public:
                    // Construction/destruction
                    explicit context_t();
                    explicit context_t(const rt::view_t *view);
                    explicit context_t(const rt::view_t *view, rt::context_state_t state);

                    ~context_t();

                public:
                    inline void init_view()
                    {
                        dsp::calc_rev_oriented_plane_p3(&view.pl[0], &view.s, &view.p[0], &view.p[1], &view.p[2]);
                        dsp::calc_oriented_plane_p3(&view.pl[1], &view.p[2], &view.s, &view.p[0], &view.p[1]);
                        dsp::calc_oriented_plane_p3(&view.pl[2], &view.p[0], &view.s, &view.p[1], &view.p[2]);
                        dsp::calc_oriented_plane_p3(&view.pl[3], &view.p[1], &view.s, &view.p[2], &view.p[0]);
                    }

                    /**
                     * Clear context: clear underlying structures
                     */
                    inline void     clear()
                    {
                        plan.clear();
                        triangle.clear();
                    }

                    /**
                     * Flush context: clear underlying structures and release memory
                     */
                    inline void     flush()
                    {
                        plan.flush();
                        triangle.flush();
                    }

                    /**
                     * Swap internal mesh contents with another context
                     * @param dst target context to perform swap
                     */
                    inline void     swap(context_t *dst)
                    {
                        plan.swap(&dst->plan);
                        triangle.swap(&dst->triangle);
                    }

                    /**
                     * Fetch data for all objects identified by specified identifiers
                     * @param src source context to perform fetch
                     * @param n number of object identifiers in array
                     * @param mask pointer to array that contains mask for object identifiers
                     * @return status of operation
                     */
                    status_t        fetch_objects(rt::mesh_t *src, size_t n, const size_t *mask);

                    /**
                     * Add opaque object for capturing data. Edges of opaque object will not be
                     * added to the split plan.
                     *
                     * @param vt array of raw triangles
                     * @param n number of triangles
                     */
                    status_t        add_opaque_object(const rt::triangle_t *vt, size_t n);

                    /**
                     * Add object for capturing data.
                     * @param vt array of raw triangles
                     * @param ve array of edges that should be added to plan
                     * @param nt number of raw triangles
                     * @param ne number of edges that should be added to plan
                     * @return status of operation
                     */
                    status_t        add_object(rtx::triangle_t *vt, rtx::edge_t *ve, size_t nt, size_t ne);

                    /**
                     * Cull view with the view planes
                     * @return status of operation
                     */
                    status_t        cull_view();

                    /**
                     * Keep the only triangles below the specified plane
                     * @param pl plane equation
                     * @return status of operation
                     */
                    status_t        cut(const dsp::vector3d_t *pl);

                    /**
                     * Keep the only triangles below the specified plane or on the plane
                     * @param pl plane equation
                     * @return status of operation
                     */
                    status_t        cullback(const dsp::vector3d_t *pl);

                    /**
                     * Perform context split by plan
                     * @param out output context
                     * @return status of operation
                     */
                    status_t        edge_split(context_t *out);

                    /**
                     * Split context into two separate contexts
                     * @return status of operation
                     */
                    status_t        split(context_t *out, const dsp::vector3d_t *pl);

                    /**
                     * Perform depth-testing cullback of faces and remove invalid faces
                     * @return status of operation
                     */
                    status_t        depth_test();
            } context_t;

        } // namespace rt
    } // namespace dspu
} // namespace lsp




#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RT_CONTEXT_H_ */
