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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_BSP_CONTEXT_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_BSP_CONTEXT_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/3d/Object3D.h>
#include <lsp-plug.in/dsp-units/3d/Allocator3D.h>
#include <lsp-plug.in/dsp-units/3d/bsp/types.h>
#include <lsp-plug.in/dsp-units/3d/view/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace dspu
    {
        namespace bsp
        {
            typedef struct LSP_DSP_UNITS_PUBLIC context_t
            {
                private:
                    context_t (const context_t &);
                    context_t & operator = (const context_t &);

                public:
                    Allocator3D<bsp::node_t>        node;
                    Allocator3D<bsp::triangle_t>    triangle;
                    bsp::node_t                    *root;

                public:
                    explicit context_t();
                    ~context_t();

                protected:
                    status_t split(lltl::parray<bsp::node_t> &queue, bsp::node_t *task);

                public:
                    void clear();
                    void flush();

                    inline void swap(context_t *dst)
                    {
                        lsp::swap(root, dst->root);
                        node.swap(&dst->node);
                        triangle.swap(&dst->triangle);
                    }

                    /**
                     * Add object to context
                     * @param obj object to add
                     * @param oid unique id to identify the object
                     * @param col object color
                     * @return status of operation
                     */
                    inline status_t add_object(Object3D *obj, const dsp::color3d_t *col)
                    {
                        return add_object(obj, obj->matrix(), col);
                    }

                    /**
                     * Add object to context
                     * @param obj object to add
                     * @param oid unique id to identify the object
                     * @param transform transformation matrix to apply to object
                     * @param col object color
                     * @return status of operation
                     */
                    status_t add_object(Object3D *obj, const dsp::matrix3d_t *transform, const dsp::color3d_t *col);

                    /**
                     * Add triangles to the BSP tree
                     * @param v_vertices list of triangle vertices
                     * @param n_triangles number of triangles
                     * @param transform object transformation
                     * @param color the color for all triangles
                     * @return status of operation
                     */
                    status_t add_triangles
                    (
                        const dsp::point3d_t *v_vertices,
                        size_t n_triangles,
                        const dsp::matrix3d_t *transform,
                        const dsp::color3d_t *color
                    );

                    /**
                     * Build the BSP tree
                     * @return status of operation
                     */
                    status_t build_tree();

                    /**
                     * Build the final mesh according to the viewer's plane
                     * @param dst collection to store the mesh
                     * @param pov the viewer's point-of-view location
                     * @return status of operation
                     */
                    status_t build_mesh(lltl::darray<view::vertex3d_t> *dst, const dsp::point3d_t *pov);

            } context_t;
        } // namespace bsp
    } // namespace dspu
} // namespace lsp

#endif /* LSP_PLUG_IN_DSP_UNITS_3D_BSP_CONTEXT_H_ */
