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

#ifndef LSP_PLUG_IN_DSP_UNITS_3D_OBJECT3D_H_
#define LSP_PLUG_IN_DSP_UNITS_3D_OBJECT3D_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/3d/types.h>
#include <lsp-plug.in/dsp-units/3d/Scene3D.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace dspu
    {
        class Scene3D;

        /** One scene object in the 3D space
         *
         */
        class Object3D
        {
            private:
                Object3D(const Object3D &);
                Object3D & operator = (const Object3D &);

            protected:
                LSPString                       sName;
                lltl::parray<obj_triangle_t>    vTriangles;

                dsp::matrix3d_t                 sMatrix;
                bool                            bVisible;
                Scene3D                        *pScene;
                obj_boundbox_t                  sBoundBox;
                dsp::point3d_t                  sCenter;

                friend class Scene3D;

            protected:
                /** Default constructor
                 *
                 */
                explicit Object3D(Scene3D *scene, const LSPString *name);

                /** Destructor
                 *
                 */
                ~Object3D();

            protected:
                void calc_bound_box(const obj_vertex_t *v);
                obj_edge_t *register_edge(obj_vertex_t *v0, obj_vertex_t *v1);

            public:
                /** Destroy object's contents
                 *
                 */
                void destroy();

                /**
                 * Compute additional parameters after loading
                 */
                void post_load();

                /** Add triangle
                 *
                 * @param face_id unique face identifier
                 * @param v1 index of vertex 1, non-negative
                 * @param v2 index of vertex 2, non-negative
                 * @param v3 index of vertex 3, non-negative
                 * @param vn1 index of normal 1, can be negative
                 * @param vn2 index of normal 2, can be negative
                 * @param vn3 index of normal 3, can be negative
                 * @return status of operation
                 */
                status_t add_triangle(
                        ssize_t face_id,
                        ssize_t v1, ssize_t v2, ssize_t v3,
                        ssize_t vn1=-1, ssize_t vn2=-1, ssize_t vn3=-1
                    );

                /** Add triangle
                 *
                 * @param vv array of three vertex indexes
                 * @param vn array of three normal indexes
                 * @return status of operation
                 */
                status_t add_triangle(ssize_t *vv, ssize_t *vn);

                /**
                 * Return number of triangles
                 * @return number of triangles
                 */
                inline size_t num_triangles() const { return vTriangles.size(); }

                /**
                 * Get triangle
                 * @param index triangle index
                 * @return pointer to triangle or NULL
                 */
                inline obj_triangle_t *triangle(size_t index) { return vTriangles.get(index); }

                /** Add triangle
                 *
                 * @param vv array of three vertex indexes
                 * @return status of operation
                 */
                status_t add_triangle(ssize_t *vv);

                /** Check if object is visible
                 *
                 * @return true if object is visible
                 */
                inline bool is_visible() const
                {
                    return bVisible;
                }

                /** Set object visible
                 *
                 * @param visible visibility flag
                 */
                inline void set_visible(bool visible)
                {
                    bVisible = visible;
                }

                /**
                 * Get name of object
                 * @param name pointer to store name
                 * @return true on success
                 */
                inline bool get_name(LSPString *name) const { return name->set(&sName); }

                /**
                 * Get name of object in UTF-8 character set
                 * @return name of object in UTF-8 character set
                 */
                inline const char *get_name() const { return sName.get_utf8(); }

                /**
                 * Get bounding box
                 * @return pointer to bounding box
                 */
                inline obj_boundbox_t *bound_box() { return &sBoundBox; }

                /**
                 * Get bounding box (const version)
                 * @return pointer to bounding box
                 */
                inline const obj_boundbox_t *bound_box() const { return &sBoundBox; }

                /**
                 * Get bounding box
                 * @return pointer to bounding box
                 */
                inline dsp::point3d_t *center() { return &sCenter; }

                /**
                 * Get bounding box (const version)
                 * @return pointer to bounding box
                 */
                inline const dsp::point3d_t *center() const { return &sCenter; }

                /**
                 * Get the scene the object relates to
                 * @return the scene the object relates to
                 */
                inline Scene3D *scene() { return pScene; }

                /** Get object transformation matrix
                 *
                 * @return object transformation matrix
                 */
                inline dsp::matrix3d_t *matrix() { return &sMatrix; }

                /**
                 * Compute bounding box
                 */
                void calc_bound_box();
        };
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_3D_OBJECT3D_H_ */
