/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 июл. 2020 г.
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

#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        IStateDumper::IStateDumper()
        {
        }

        IStateDumper::~IStateDumper()
        {
        }

        void IStateDumper::begin_object(const char *name, const void *ptr, size_t szof)
        {
        }

        void IStateDumper::begin_object(const void *ptr, size_t szof)
        {
        }

        void IStateDumper::end_object()
        {
        }

        void IStateDumper::begin_array(const char *name, const void *ptr, size_t count)
        {
        }

        void IStateDumper::begin_array(const void *ptr, size_t count)
        {
        }

        void IStateDumper::end_array()
        {
        }

        void IStateDumper::write(const void *value)
        {
        }

        void IStateDumper::write(const char *value)
        {
        }

        void IStateDumper::write(bool value)
        {
        }

        void IStateDumper::write(unsigned char value)
        {
        }

        void IStateDumper::write(signed char value)
        {
        }

        void IStateDumper::write(unsigned short value)
        {
        }

        void IStateDumper::write(signed short value)
        {
        }

        void IStateDumper::write(unsigned int value)
        {
        }

        void IStateDumper::write(signed int value)
        {
        }

        void IStateDumper::write(unsigned long value)
        {
        }

        void IStateDumper::write(signed long value)
        {
        }

        void IStateDumper::write(unsigned long long value)
        {
        }

        void IStateDumper::write(signed long long value)
        {
        }

        void IStateDumper::write(float value)
        {
        }

        void IStateDumper::write(double value)
        {
        }

        void IStateDumper::write(const char *name, const void *value)
        {
        }

        void IStateDumper::write(const char *name, const char *value)
        {
        }

        void IStateDumper::write(const char *name, bool value)
        {
        }

        void IStateDumper::write(const char *name, unsigned char value)
        {
        }

        void IStateDumper::write(const char *name, signed char value)
        {
        }

        void IStateDumper::write(const char *name, unsigned short value)
        {
        }

        void IStateDumper::write(const char *name, signed short value)
        {
        }

        void IStateDumper::write(const char *name, unsigned int value)
        {
        }

        void IStateDumper::write(const char *name, signed int value)
        {
        }

        void IStateDumper::write(const char *name, unsigned long value)
        {
        }

        void IStateDumper::write(const char *name, signed long value)
        {
        }

        void IStateDumper::write(const char *name, unsigned long long value)
        {
        }

        void IStateDumper::write(const char *name, signed long long value)
        {
        }

        void IStateDumper::write(const char *name, float value)
        {
        }

        void IStateDumper::write(const char *name, double value)
        {
        }

        void IStateDumper::writev(const void * const *value, size_t count)
        {
        }

        void IStateDumper::writev(const bool *value, size_t count)
        {
        }

        void IStateDumper::writev(const unsigned char *value, size_t count)
        {
        }

        void IStateDumper::writev(const signed char *value, size_t count)
        {
        }

        void IStateDumper::writev(const unsigned short *value, size_t count)
        {
        }

        void IStateDumper::writev(const signed short *value, size_t count)
        {
        }

        void IStateDumper::writev(const unsigned int *value, size_t count)
        {
        }

        void IStateDumper::writev(const signed int *value, size_t count)
        {
        }

        void IStateDumper::writev(const unsigned long *value, size_t count)
        {
        }

        void IStateDumper::writev(const signed long *value, size_t count)
        {
        }

        void IStateDumper::writev(const unsigned long long *value, size_t count)
        {
        }

        void IStateDumper::writev(const signed long long *value, size_t count)
        {
        }


        void IStateDumper::writev(const float *value, size_t count)
        {
        }

        void IStateDumper::writev(const double *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const void * const *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const bool *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const unsigned char *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const signed char *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const unsigned short *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const signed short *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const unsigned int *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const signed int *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const unsigned long *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const signed long *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const unsigned long long *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const signed long long *value, size_t count)
        {
        }

        void IStateDumper::writev(const char *name, const float *value, size_t count)
        {
        }
    
        void IStateDumper::writev(const char *name, const double *value, size_t count)
        {
        }
    } /* namespace dspu */
} /* namespace lsp */
