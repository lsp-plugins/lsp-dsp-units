/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 23 сент. 2016 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/misc/interpolation.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace interpolation
        {
            //-----------------------------------------------------------------
            // Matrix-related operations
            static void swap_rows(double *a, double *b, size_t count)
            {
                for (size_t i=0; i<count; ++i)
                {
                    const double tmp = a[i];
                    a[i]    = b[i];
                    b[i]    = tmp;
                }
            }

            static void sub_row(double *a, const double *b, size_t count)
            {
                const double k = a[0] / b[0];

                a[0] = 0.0f;
                for (size_t i=1; i<count; ++i)
                    a[i]    -= k*b[i];
            }

            static double solve_row(const double *row, const double *c, size_t count)
            {
                double sum  = row[count];
                for (size_t i=1; i<count; ++i)
                    sum        += row[i] * c[i];

                return -sum/row[0];
            }

            void solve_matrix(double *c, double *m, size_t vars)
            {
                const size_t cols = vars + 1;

                // Triangulate matrix
                for (size_t i=0; i<vars; ++i)
                {
                    // Make first value non-zero
                    double *srow = &m[i * cols + i];

                    if (fabs(srow[0]) < 1e-20f)
                    {
                        for (size_t j = i; j<vars; ++j)
                        {
                            double *drow = &m[j * cols + i];
                            if (fabs(drow[0]) >= 1e-20f)
                            {
                                swap_rows(srow, drow, cols - i);
                                break;
                            }
                        }
                    }

                    // Subtract all other rows
                    for (size_t j=i+1; j<vars; ++j)
                    {
                        double *drow = &m[j * cols + i];
                        if (fabs(drow[0]) < 1e-20f)
                            continue;

                        sub_row(drow, srow, cols - i);
                    }
                }

                // Compute polynom parameters
                for (size_t i=vars; i-- > 0; )
                    c[i]        = solve_row(&m[i*cols + i], &c[i], vars - i);
            }

            //-----------------------------------------------------------------

            LSP_DSP_UNITS_PUBLIC
            void hermite_quadratic(float *p, float x0, float y0, float k0, float x1, float k1)
            {
                // y = p[0]*x^2 + p[1]*x + p[2]
                p[0]    = (k0 - k1)*0.5f / (x0 - x1);
                p[1]    = k0 - 2.0f*p[0]*x0;
                p[2]    = y0 - (p[0]*x0 + p[1])*x0;
            }

            LSP_DSP_UNITS_PUBLIC
            void hermite_cubic(float *p, float x0, float y0, float k0, float x1, float y1, float k1)
            {
                // y = p[0]*x^3 + p[1]*x^2 + p[2]*x + p[3]
                // dy/dx = 3*p[0]*x^2 + 2*p[1]*x + p[2]
                double dx    = x1 - x0;
                double dy    = y1 - y0;
                double kx    = dy / dx;
                double xx1   = x1*x1;
                double xx2   = x0 + x1;

                double a     = ((k0 + k1)*dx - 2.0f*dy) / (dx*dx*dx);
                double b     = ((kx - k0) + a*((2.0f*x0-x1)*x0 - xx1))/dx;
                double c     = kx - a*(xx1+xx2*x0) - b*xx2;
                double d     = y0 - x0*(c+x0*(b+x0*a));

                p[0]    = a;
                p[1]    = b;
                p[2]    = c;
                p[3]    = d;
            }

            LSP_DSP_UNITS_PUBLIC
            void hermite_quadro(
                float *p,
                float x0, float y0, float k0,
                float x1, float y1, float k1,
                float x2, float y2)
            {
                double m[5 * 6];

                double X[3] = { x0, x1, x2 };
                double Y[3] = { y0, y1, y2 };
                double K[2] = { k0, k1 };

                for (size_t i=0; i<3; ++i)
                {
                    const double x  = X[i];
                    double *r0      = &m[i * 6];

                    r0[5]   = -Y[i];        // y
                    r0[4]   = 1.0;          // 1
                    r0[3]   = x;            // x
                    r0[2]   = x*x;          // x^2
                    r0[1]   = x*r0[2];      // x^3
                    r0[0]   = r0[2]*r0[2];  // x^4

                    if (i < 2)
                    {
                        double *r1      = &m[(i + 3)*6];

                        r1[5]   = -K[i];        // k
                        r1[4]   = 0.0;          // 0
                        r1[3]   = 1.0;          // 1
                        r1[2]   = 2.0 * x;      // 2*x^1
                        r1[1]   = 3.0 * r0[2];  // 3*x^2
                        r1[0]   = 4.0 * r0[1];  // 4*x^3
                    }
                }

                // Compute polynom parameters and output result
                double c[5];
                solve_matrix(c, m, 5);

                for (size_t i=0; i<5; ++i)
                    p[i]        = c[i];
            }

            LSP_DSP_UNITS_PUBLIC
            void hermite_penta(
                float *p,
                float x0, float y0, float k0,
                float x1, float y1, float k1,
                float x2, float y2, float k2)
            {
                double X[3] = { x0, x1, x2 };
                double Y[3] = { y0, y1, y2 };
                double K[3] = { k0, k1, k2 };

                double m[6 * 7];

                for (size_t i=0; i<3; ++i)
                {
                    const double x  = X[i];

                    double *r0      = &m[i * 7];
                    r0[6]   = -Y[i];        // y
                    r0[5]   = 1.0;          // 1
                    r0[4]   = x;            // x
                    r0[3]   = x*x;          // x^2
                    r0[2]   = x*r0[3];      // x^3
                    r0[1]   = r0[3]*r0[3];  // x^4
                    r0[0]   = r0[2]*r0[3];  // x^5

                    double *r1      = &m[(i + 3)*7];
                    r1[6]   = -K[i];        // k
                    r1[5]   = 0.0;          // 0
                    r1[4]   = 1.0;          // 1
                    r1[3]   = 2.0 * x;      // 2*x^1
                    r1[2]   = 3.0 * r0[3];  // 3*x^2
                    r1[1]   = 4.0 * r0[2];  // 4*x^3
                    r1[0]   = 5.0 * r0[1];  // 5*x^4
                }

                // Compute polynom parameters and output result
                double c[6];
                solve_matrix(c, m, 6);

                for (size_t i=0; i<6; ++i)
                    p[i]        = c[i];
            }

            LSP_DSP_UNITS_PUBLIC
            void exponent(float *p, float x0, float y0, float x1, float y1, float k)
            {
                double e        = exp(k*(x0 - x1));
                p[0]            = (y0 - e*y1) / (1.0 - e);
                p[1]            = (y0 - p[0]) / exp(k*x0);
                p[2]            = k;
            }

            LSP_DSP_UNITS_PUBLIC
            void linear(float *p, float x0, float y0, float x1, float y1)
            {
                p[0]            = (y1 - y0) / (x1 - x0);
                p[1]            = y0 - p[0]*x0;
            }
        } /* namespace interpolation */
    } /* namespace dspu */
} /* namespace lsp */

