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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_RT_MESH_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_RT_MESH_H_

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
            typedef struct mesh_t
            {
                private:
                    mesh_t(const mesh_t &);
                    mesh_t & operator = (const mesh_t &);

                public:
                    Allocator3D<rtm::vertex_t>      vertex;     // Collection of vertexes
                    Allocator3D<rtm::edge_t>        edge;       // Collection of edges
                    Allocator3D<rtm::triangle_t>    triangle;   // Collection of triangles

                public:
                    explicit mesh_t();
                    ~mesh_t();

                protected:
                    bool            validate_list(rtm::edge_t *e);
                    static ssize_t  linked_count(rtm::triangle_t *t, rtm::edge_t *e);

                    status_t        split_edge(rtm::edge_t* e, rtm::vertex_t* sp);
                    status_t        split_triangle(rtm::triangle_t* t, rtm::vertex_t* sp);
                    static bool     unlink_triangle(rtm::triangle_t *t, rtm::edge_t *e);
                    static status_t arrange_triangle(rtm::triangle_t *ct, rtm::edge_t *e);

                public:
                    /**
                     * Clear mesh: clear underlying structures
                     */
                    inline void     clear()
                    {
                        vertex.clear();
                        edge.clear();
                        triangle.clear();
                    }

                    /**
                     * Flush mesh: clear underlying structures and release memory
                     */
                    inline void     flush()
                    {
                        vertex.flush();
                        edge.flush();
                        triangle.flush();
                    }

                    /**
                     * Swap internal mesh contents with another context
                     * @param dst destination context to perform swap
                     */
                    inline void     swap(mesh_t *dst)
                    {
                        vertex.swap(&dst->vertex);
                        edge.swap(&dst->edge);
                        triangle.swap(&dst->triangle);
                    }

                    /**
                     * Add object to context
                     * @param obj object to add
                     * @param oid unique id to identify the object
                     * @param material material that describes behaviour of reflected rays
                     * @return status of operation
                     */
                    inline status_t add_object(Object3D *obj, ssize_t oid, rt::material_t *material)
                    {
                        return add_object(obj, oid, obj->matrix(), material);
                    }

                    /**
                     * Add object to context
                     * @param obj object to add
                     * @param oid unique id to identify the object
                     * @param transform transformation matrix to apply to object
                     * @param material material that describes behaviour of reflected rays
                     * @return status of operation
                     */
                    status_t        add_object(Object3D *obj, ssize_t oid, const dsp::matrix3d_t *transform, rt::material_t *material);

                    /**
                     * Remove conflicts between triangles, does not modify the 'itag' field of
                     * triangle, so it can be used to identify objects of the scene
                     *
                     * @return status of operation
                     */
                    status_t        solve_conflicts();

                    /**
                     * Check consistency of the context: that all stored pointers are valid
                     * @return true if context is in valid state
                     */
                    bool            validate();

                    /**
                     * Copy all data from the source mesh
                     * @param src source mesh to copy
                     * @return status of operation
                     */
                    status_t        copy(mesh_t *src);
            } mesh_t;
        } // namespace rt
    } // namespace dspu
} // namespace lsp


#endif /* LSP_PLUG_IN_DSP_UNITS_3D_RT_MESH_H_ */
