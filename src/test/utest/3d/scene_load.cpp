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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/dsp-units/3d/Scene3D.h>
#include <lsp-plug.in/io/InMemoryStream.h>

UTEST_BEGIN("dspu.3d", scene_load)

    void test_load_from_obj()
    {
        dspu::Scene3D s;
        dspu::Object3D *o;

        static const char *data =
            "# Quad test\n"
            "# (C) Linux Studio Plugins Project\n"
            "o Quad 1\n"
            "v -2 -2 -1\n"
            "v 2 -2 -1\n"
            "v 2 2 -1\n"
            "v -2 2 -1\n"
            "vn 0 0 1\n"
            "f 1//1 2//1 3//1 4//1\n"
            "\n"
            "o Quad 2\n"
            "v -2 -2 -2\n"
            "v 2 -2 -2\n"
            "v 2 2 -2\n"
            "v -2 2 -2\n"
            "vn 0 0 1\n"
            "f 5//2 6//2 7//2 8//2\n";

        io::InMemoryStream is;
        is.wrap(data, strlen(data));
        UTEST_ASSERT(s.load(&is, WRAP_CLOSE) == STATUS_OK);

        // Validate scene
        UTEST_ASSERT(s.num_objects() == 2);
        UTEST_ASSERT(s.num_vertexes() == 8);
        UTEST_ASSERT(s.num_edges() == 10);
        UTEST_ASSERT(s.num_triangles() == 4);
        UTEST_ASSERT(s.num_normals() == 2);

        // Validate object 1
        o = s.object(0);
        UTEST_ASSERT(o != NULL);
        UTEST_ASSERT(o->get_name() != NULL);
        UTEST_ASSERT(strcmp(o->get_name(), "Quad 1") == 0);
        UTEST_ASSERT(o->num_triangles() == 2);

        // Validate object 2
        o = s.object(1);
        UTEST_ASSERT(o != NULL);
        UTEST_ASSERT(o->get_name() != NULL);
        UTEST_ASSERT(strcmp(o->get_name(), "Quad 2") == 0);
        UTEST_ASSERT(o->num_triangles() == 2);
    }

    UTEST_MAIN
    {
        test_load_from_obj();
    }

UTEST_END


