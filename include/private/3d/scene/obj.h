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

#ifndef PRIVATE_3D_SCENE_OBJ_H_
#define PRIVATE_3D_SCENE_OBJ_H_

#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/io/IInSequence.h>
#include <lsp-plug.in/dsp-units/3d/Scene3D.h>
#include <lsp-plug.in/fmt/obj/IObjHandler.h>
#include <lsp-plug.in/fmt/obj/PushParser.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Handler of the Wavefont OBJ file format
         */
        class ObjSceneHandler: public obj::IObjHandler
        {
            private:
                ObjSceneHandler(const ObjSceneHandler &);
                ObjSceneHandler & operator = (const ObjSceneHandler &);

            protected:
                typedef struct vertex_t
                {
                    obj_vertex_t   *p;
                    obj_normal_t   *n;
                    ssize_t         ip;
                    ssize_t         in;
                } vertex_t;

            protected:
                Scene3D        *pScene;
                Object3D       *pObject;
                ssize_t         nFaceID;

            public:
                explicit ObjSceneHandler(Scene3D *scene)
                {
                    pScene      = scene;
                    pObject     = NULL;
                    nFaceID     = 0;
                }

                virtual ~ObjSceneHandler()
                {
                }

            public:
                virtual status_t begin_object(const char *name)
                {
                    if (pObject != NULL)
                        return STATUS_BAD_STATE;

                    LSPString sname;
                    if (!sname.set_utf8(name))
                        return STATUS_NO_MEM;

                    pObject = pScene->add_object(&sname);
                    return (pObject != NULL) ? STATUS_OK : STATUS_NO_MEM;
                }

                virtual status_t begin_object(const LSPString *name)
                {
                    if (pObject != NULL)
                        return STATUS_BAD_STATE;
                    pObject = pScene->add_object(name);
                    return (pObject != NULL) ? STATUS_OK : STATUS_NO_MEM;
                }

                virtual status_t end_object()
                {
                    if (pObject == NULL)
                        return STATUS_BAD_STATE;

                    pObject->post_load();
                    pObject = NULL;
                    return STATUS_OK;
                }

                virtual status_t end_of_data()
                {
                    if (pScene == NULL)
                        return STATUS_BAD_STATE;
                    pScene->postprocess_after_loading();
                    return STATUS_OK;
                }

                virtual ssize_t add_vertex(float x, float y, float z, float w)
                {
                    dsp::point3d_t p;
                    p.x     = x;
                    p.y     = y;
                    p.z     = z;
                    p.w     = w;
                    return pScene->add_vertex(&p);
                }

                virtual ssize_t add_normal(float nx, float ny, float nz, float nw)
                {
                    dsp::vector3d_t n;
                    n.dx    = nx;
                    n.dy    = ny;
                    n.dz    = nz;
                    n.dw    = nw;
                    return pScene->add_normal(&n);
                }

                virtual status_t add_face(const obj::index_t *vv, const obj::index_t *vn, const obj::index_t *vt, size_t n)
                {
                    if ((pObject == NULL) || (n < 3))
                        return STATUS_BAD_STATE;

                    lltl::darray<vertex_t> vertex;
                    vertex_t *vx        = vertex.append_n(n);
                    if (vx == NULL)
                        return STATUS_NO_MEM;

                    // Prepare structure
                    for (size_t i=0; i<n; ++i)
                    {
                        vx[i].ip            = vv[i];
                        vx[i].p             = (vx[i].ip >= 0) ? pScene->vertex(vx[i].ip) : NULL;
                        if (vx[i].p == NULL)
                            return STATUS_BAD_STATE;
                        vx[i].in            = vn[i];
                        vx[i].n             = (vx[i].in >= 0) ? pScene->normal(vx[i].in) : NULL;
                    }

                    ssize_t face_id     = nFaceID++;

                    // Calc default normals for vertexes without normals
                    vertex_t *v1, *v2, *v3;
                    obj_normal_t on;
                    v1 = vertex.uget(0);
                    v2 = vertex.uget(1);
                    v3 = vertex.uget(2);

                    dsp::calc_normal3d_p3(&on, v1->p, v2->p, v3->p);
                    for (size_t i=0; i<n; ++i)
                    {
                        v1 = &vx[i];
                        if (v1->n == NULL)
                            v1->n = &on;
                    }

                    // Triangulation algorithm
                    size_t index = 0;
                    float ck = 0.0f;

                    while (n > 3)
                    {
                        v1 = vertex.uget(index % n);
                        v2 = vertex.uget((index+1) % n);
                        v3 = vertex.uget((index+2) % n);

    //                    lsp_trace(
    //                        "analyzing triangle (%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f)",
    //                        v1->p->x, v1->p->y, v1->p->z,
    //                        v2->p->x, v2->p->y, v2->p->z,
    //                        v3->p->x, v3->p->y, v3->p->z
    //                    );

                        // Check that it is an ear
                        ck = dsp::check_triplet3d_p3n(v1->p, v2->p, v3->p, v1->n);
                        if (ck < 0.0f)
                        {
                            index = (index + 1) % n;
                            continue;
                        }
                        else if (ck == 0.0f)
                        {
                            size_t longest = dsp::longest_edge3d_p3(v1->p, v2->p, v3->p);
                            size_t remove = (longest + 2) % 3;

                            // Need to eliminate point that lies on the line
                            if (!vertex.remove((index + remove) % n))
                                return STATUS_BAD_STATE;

                            // Rollback index and decrement counter
                            n--;
                            index = (index > 0) ? index - 1 : n-1;
                            continue;
                        }

                        // Now ensure that there are no other points inside the triangle
                        int found = 0;
                        for (size_t i=0; i<n; ++i)
                        {
                            vertex_t *vx = vertex.uget(i);
                            if ((vx->ip == v1->ip) || (vx->ip == v2->ip) || (vx->ip == v3->ip))
                                continue;

                            ck  = dsp::check_point3d_on_triangle_p3p(v1->p, v2->p, v3->p, vx->p);
                            if (ck >= 0.0f)
                            {
    //                            lsp_trace("point (%8.3f, %8.3f, %8.3f) has failed", vx->p->x, vx->p->y, vx->p->z);
                                found ++;
                                break;
                            }
                        }

                        if (found)
                        {
                            index = (index + 1) % n;
                            continue;
                        }

                        // It's an ear, there are no points inside, can emit triangle and remove the middle point
    //                    lsp_trace(
    //                        "emit triangle (%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f)",
    //                        v1->p->x, v1->p->y, v1->p->z,
    //                        v2->p->x, v2->p->y, v2->p->z,
    //                        v3->p->x, v3->p->y, v3->p->z
    //                    );
                        status_t result = pObject->add_triangle(face_id, v1->ip, v2->ip, v3->ip, v1->in, v2->in, v3->in);
                        if (result != STATUS_OK)
                            return result;

                        // Remove the middle point
                        if (!vertex.remove((index + 1) % n))
                            return STATUS_BAD_STATE;

                        if (index >= (--n))
                            index = 0;
                    }

                    // Add last triangle
                    v1 = vertex.uget(0);
                    v2 = vertex.uget(1);
                    v3 = vertex.uget(2);

                    ck = dsp::check_triplet3d_p3n(v1->p, v2->p, v3->p, v1->n);
                    if (ck != 0.0f)
                    {
    //                    lsp_trace(
    //                        "emit triangle (%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f):(%8.3f, %8.3f, %8.3f)",
    //                        v1->p->x, v1->p->y, v1->p->z,
    //                        v2->p->x, v2->p->y, v2->p->z,
    //                        v3->p->x, v3->p->y, v3->p->z
    //                    );
                        status_t result = (ck < 0.0f) ?
                            pObject->add_triangle(face_id, v1->ip, v3->ip, v2->ip, v1->in, v3->in, v2->in) :
                            pObject->add_triangle(face_id, v1->ip, v2->ip, v3->ip, v1->in, v2->in, v3->in);
                        if (result != STATUS_OK)
                            return result;
                    }

                    return STATUS_OK;
                }
        };

        status_t load_scene_from_obj(dspu::Scene3D *scene, io::IInStream *is, const char *charset)
        {
            obj::PushParser pp;
            ObjSceneHandler handler(scene);
            return pp.parse_data(&handler, is, WRAP_NONE, charset);
        }

        status_t load_scene_from_obj(dspu::Scene3D *scene, io::IInSequence *is)
        {
            obj::PushParser pp;
            ObjSceneHandler handler(scene);
            return pp.parse_data(&handler, is, WRAP_NONE);
        }
    }
}

#endif /* PRIVATE_3D_SCENE_OBJ_H_ */
