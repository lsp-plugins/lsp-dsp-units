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

#ifndef PRIVATE_3D_SCENE_OBJ_H_
#define PRIVATE_3D_SCENE_OBJ_H_

#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/io/IInSequence.h>
#include <lsp-plug.in/io/InMemoryStream.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/3d/Scene3D.h>
#include <lsp-plug.in/fmt/obj/Decompressor.h>
#include <lsp-plug.in/fmt/obj/IObjHandler.h>
#include <lsp-plug.in/fmt/obj/PushParser.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr float ONE_THIRD      = 1.0f / 3.0f;

        /**
         * Handler of the Wavefront OBJ file format
         */
        class ObjSceneHandler: public obj::IObjHandler
        {
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

            protected:
//                static size_t   calc_intersections(lltl::darray<vertex_t> & vertex, size_t start, const dsp::vector3d_t *plane)
//                {
//                    size_t intersections = 0;
//                    const size_t count = vertex.size();
//                    const size_t limit = count - 3;
//
//                    for (size_t i=0; i<limit; ++i)
//                    {
//                        const dsp::point3d_t *p1 = vertex.uget((start + i) % count)->p;
//                        const dsp::point3d_t *p2 = vertex.uget((start + i + 1) % count)->p;
//
//                        const size_t mask = dsp::colocation_x2_v1p2(plane, p1, p2);
//                        switch (mask)
//                        {
//                            case 0x02: // 0010
//                            case 0x08: // 1000
//                                ++intersections;
//                                break;
//                            default:
//                                break;
//                        }
//                    }
//
//                    return intersections;
//                }

                static bool     check_points_in_triangle(
                    lltl::darray<vertex_t> & vertex,
                    const vertex_t *p1,
                    const vertex_t *p2,
                    const vertex_t *p3)
                {
                    const size_t count = vertex.size();

                    for (size_t i=0; i < count; ++i)
                    {
                        vertex_t *px = vertex.uget(i);
                        if ((px->ip == p1->ip) || (px->ip == p2->ip) || (px->ip == p3->ip))
                            continue;

                        const float ck      = dsp::check_point3d_on_triangle_p3p(p1->p, p2->p, p3->p, px->p);
                        if (ck >= 0.0f)
                            return true;
                    }

                    return false;
                }

                static bool     check_point_in_poly(const dsp::point3d_t *p, lltl::darray<vertex_t> & vertex)
                {
                    const size_t count = vertex.size();

                    const vertex_t *p1 = vertex.uget(0);
                    const vertex_t *p2 = vertex.uget(1);
                    const vertex_t *p3 = vertex.uget(2);

                    float ck    = dsp::check_point3d_on_triangle_p3p(p1->p, p2->p, p3->p, p);
                    bool inside = (ck >= 0.0f);

                    for (size_t i=3; i < count; ++i)
                    {
                        p2          = p3;
                        p3          = vertex.uget(i);

                        ck          = dsp::check_point3d_on_triangle_p3p(p1->p, p2->p, p3->p, p);
                        if (ck >= 0.0f)
                            inside      = !inside;
                    }

                    return inside;
                }

                static bool     compute_normal(dsp::vector3d_t *n, lltl::darray<vertex_t> & vertex)
                {
                    // For triangle do simple stuff
                    const size_t count = vertex.size();
                    const vertex_t *p1;
                    const vertex_t *p2 = vertex.uget(0);
                    const vertex_t *p3 = vertex.uget(1);

                    if (count <= 3)
                    {
                        p1              = vertex.uget(2);
                        dsp::calc_normal3d_p3(n, p2->p, p3->p, p1->p);
                        return true;
                    }

                    for (size_t i=0; i<count; ++i)
                    {
                        p1              = p2;
                        p2              = p3;
                        p3              = vertex.uget((i + 2) % count);

                        if (check_points_in_triangle(vertex, p1, p2, p3))
                            continue;

                        // Compute middle point and test it
                        dsp::point3d_t p;
                        p.x     = (p1->p->x + p2->p->x + p3->p->x) * ONE_THIRD;
                        p.y     = (p1->p->y + p2->p->y + p3->p->y) * ONE_THIRD;
                        p.z     = (p1->p->z + p2->p->z + p3->p->z) * ONE_THIRD;

                        if (check_point_in_poly(&p, vertex))
                        {
                            dsp::calc_normal3d_p3(n, p1->p, p2->p, p3->p);
                            return true;
                        }
                    }

                #ifdef LSP_TRACE
                    for (size_t i=0; i<count; ++i)
                    {
                        const vertex_t *p = vertex.uget(i);
                        lsp_trace("vertex[%d] = {%f, %f, %f}",
                            int(i), p->p->x, p->p->y, p->p->z);
                    }

                    fprintf(stderr, "x;y;z;\n");
                    for (size_t i=0; i<count; ++i)
                    {
                        const vertex_t *p = vertex.uget(i);
                        fprintf(stderr, "%f;%f;%f\n",
                            p->p->x, p->p->y, p->p->z);
                    }
                #endif

                    return false;
                }

            public:
                explicit ObjSceneHandler(Scene3D *scene)
                {
                    pScene      = scene;
                    pObject     = NULL;
                    nFaceID     = 0;
                }
                ObjSceneHandler(const ObjSceneHandler &) = delete;
                ObjSceneHandler(ObjSceneHandler &&) = delete;

                virtual ~ObjSceneHandler() override
                {
                }

                ObjSceneHandler & operator = (const ObjSceneHandler &) = delete;
                ObjSceneHandler & operator = (ObjSceneHandler &&) = delete;

            public:
                virtual status_t begin_object(const char *name) override
                {
                    if (pObject != NULL)
                        return STATUS_BAD_STATE;

                    LSPString sname;
                    if (!sname.set_utf8(name))
                        return STATUS_NO_MEM;

                    pObject = pScene->add_object(&sname);
                    return (pObject != NULL) ? STATUS_OK : STATUS_NO_MEM;
                }

                virtual status_t begin_object(const LSPString *name) override
                {
                    if (pObject != NULL)
                        return STATUS_BAD_STATE;
                    pObject = pScene->add_object(name);
                    return (pObject != NULL) ? STATUS_OK : STATUS_NO_MEM;
                }

                virtual status_t end_object() override
                {
                    if (pObject == NULL)
                        return STATUS_BAD_STATE;

                    pObject->post_load();
                    pObject = NULL;
                    return STATUS_OK;
                }

                virtual status_t end_of_data() override
                {
                    if (pScene == NULL)
                        return STATUS_BAD_STATE;
                    pScene->postprocess_after_loading();
                    return STATUS_OK;
                }

                virtual ssize_t add_vertex(float x, float y, float z, float w) override
                {
                    dsp::point3d_t p;
                    p.x     = x;
                    p.y     = y;
                    p.z     = z;
                    p.w     = w;
                    return pScene->add_vertex(&p);
                }

                virtual ssize_t add_normal(float nx, float ny, float nz, float nw) override
                {
                    dsp::vector3d_t n;
                    n.dx    = nx;
                    n.dy    = ny;
                    n.dz    = nz;
                    n.dw    = nw;
                    return pScene->add_normal(&n);
                }

                virtual ssize_t add_face(const obj::index_t *vv, const obj::index_t *vn, const obj::index_t *vt, size_t count) override
                {
                    if ((pObject == NULL) || (count < 3))
                        return -STATUS_BAD_STATE;

                    lltl::darray<vertex_t> vertex;
                    vertex_t *vx        = vertex.append_n(count);
                    if (vx == NULL)
                        return -STATUS_NO_MEM;

                    // Prepare structure, eliminate duplicate sequential points
                    size_t added        = 0;
                    for (size_t i=0; i<count; ++i)
                    {
                        vertex_t *vc        = &vx[added];
                        vc->ip              = vv[i];
                        vc->p               = (vc->ip >= 0) ? pScene->vertex(vc->ip) : NULL;
                        if (vc->p == NULL)
                            return -STATUS_BAD_STATE;
                        if (added > 0)
                        {
                            const vertex_t *vp  = &vx[added - 1];
                            if (vp->ip == vc->ip)
                                continue;
                            const float distance = dsp::calc_sqr_distance_p2(vp->p, vc->p);
                            if (distance < 1e-12f)
                            {
                                lsp_trace("square distance between {%f, %f, %f} and {%f %f %f} is %g",
                                    vp->p->x, vp->p->y, vp->p->z,
                                    vc->p->x, vc->p->y, vc->p->z,
                                    distance);
                                continue;
                            }
                        }
                        vc->in              = vn[i];
                        vc->n               = (vc->in >= 0) ? pScene->normal(vc->in) : NULL;

                        ++added;
                    }
                    if (added < 3)
                    {
                    #ifdef LSP_TRACE
                        lsp_trace("Invalid geometry:");
                        for (size_t i=0; i<count; ++i)
                        {
                            const obj_vertex_t *p = pScene->vertex(vv[i]);
                            lsp_trace("vertex[%d] = {%f, %f, %f}",
                                int(i), p->x, p->y, p->z);
                        }
                    #endif
                        return (count >= 3) ? STATUS_OK : -STATUS_CORRUPTED;
                    }
                    if (added < count)
                    {
                        vertex.pop_n(count - added);
                        count               = added;
                    }
                    ssize_t face_id     = nFaceID++;

                    // Calc default normals for vertexes without normals
                    vertex_t *v1, *v2, *v3;
                    obj_normal_t on;
                    obj_normal_t *pon = NULL;

                    v1 = vertex.uget(0);
                    v2 = vertex.uget(1);
                    v3 = vertex.uget(2);

                    // Ensure that we have at least one normal specified
                    for (size_t i=0; i<count; ++i)
                    {
                        v1 = &vx[i];
                        if (v1->n != NULL)
                        {
                            pon         = v1->n;
                            break;
                        }
                    }

                    // Ensure that all vertices have normals
                    for (size_t i=0; i<count; ++i)
                    {
                        v1 = &vx[i];
                        if (v1->n == NULL)
                        {
                            if (pon == NULL)
                            {
                                if (!compute_normal(&on, vertex))
                                    return - STATUS_CORRUPTED;
                                pon     = &on;
                            }
                            v1->n   = pon;
                        }
                    }

                    // Triangulation algorithm
                    size_t index = 0;
                    float ck = 0.0f;
                    bool dump = false;

                    for (size_t n=count; n > 3; )
                    {
                    #ifdef LSP_TRACE
                        if (dump)
                        {
                            for (size_t i=0; i<count; ++i)
                            {
                                const obj_vertex_t *p = pScene->vertex(vv[i]);
                                lsp_trace("vertex[%d] = {%f, %f, %f}",
                                    int(i), p->x, p->y, p->z);
                            }

                            if (pon)
                                lsp_trace("normal = {%f, %f, %f}",
                                    pon->dx, pon->dy, pon->dz);

                            fprintf(stderr, "x;y;z;\n");
                            for (size_t i=0; i<count; ++i)
                            {
                                const obj_vertex_t *p = pScene->vertex(vv[i]);
                                fprintf(stderr, "%f;%f;%f\n",
                                    p->x, p->y, p->z);
                            }

                            for (size_t i=0; i<n; ++i)
                            {
                                const vertex_t *p = vertex.uget(i);
                                lsp_trace("vertex[%d] = {%f, %f, %f}",
                                    int(i), p->p->x, p->p->y, p->p->z);
                            }

                            fprintf(stderr, "x;y;z;\n");
                            for (size_t i=0; i<n; ++i)
                            {
                                const vertex_t *p = vertex.uget(i);
                                fprintf(stderr, "%f;%f;%f\n",
                                    p->p->x, p->p->y, p->p->z);
                            }
                        }
                    #endif

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
                        if (ck < -1e-6f)
                        {
                            index = (index + 1) % n;
                            continue;
                        }
                        else if (ck <= 1e-6f)
                        {
                            size_t longest = dsp::longest_edge3d_p3(v1->p, v2->p, v3->p);
                            size_t remove = (longest + 2) % 3;

                            // Need to eliminate point that lies on the line
                            if (!vertex.remove((index + remove) % n))
                                return -STATUS_BAD_STATE;

                            // Rollback index and decrement counter
                            n--;
                            index = (index > 0) ? index - 1 : n-1;
                            continue;
                        }

                        // Now ensure that there are no other points inside of the triangle
                        const bool found = check_points_in_triangle(vertex, v1, v2, v3);
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
                            return -result;

                        // Remove the middle point
                        if (!vertex.remove((index + 1) % n))
                            return -STATUS_BAD_STATE;

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
                            return -result;
                    }

                    return face_id;
                }
        };

        status_t load_scene_from_obj(dspu::Scene3D *scene, io::IInStream *is, const char *charset)
        {
            // Load to memory
            io::OutMemoryStream oms;
            const wssize_t count = is->sink(&oms);
            if (count < 0)
                return status_t(-count);

            status_t res = STATUS_OK;
            ObjSceneHandler handler(scene);

            // Try to load compressed object file first
            {
                io::InMemoryStream ims;
                ims.wrap(oms.data(), oms.size());

                obj::Decompressor dp;
                if ((res = dp.parse_data(&handler, &ims)) == STATUS_OK)
                    return res;
                if ((res != STATUS_BAD_FORMAT) &&
                    (res != STATUS_UNSUPPORTED_FORMAT))
                    return res;
            }

            // Try to load non-compressed object file
            {
                io::InMemoryStream ims;
                ims.wrap(oms.data(), oms.size());

                obj::PushParser pp;
                if ((res = pp.parse_data(&handler, &ims, WRAP_NONE, charset)) == STATUS_OK)
                    return res;
            }

            return res;
        }

    } /* namespace dspu */
} /* namespace lsp */

#endif /* PRIVATE_3D_SCENE_OBJ_H_ */
