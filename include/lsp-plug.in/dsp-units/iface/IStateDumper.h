/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 8 июл. 2020 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_IFACE_ISTATEDUMPER_H_
#define LSP_PLUG_IN_DSP_UNITS_IFACE_ISTATEDUMPER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * The interface for dumping DSP module state
         * Allows to dump module state to any format by overriding virtual methods
         */
        class LSP_DSP_UNITS_PUBLIC IStateDumper
        {
            public:
                explicit IStateDumper();
                IStateDumper(const IStateDumper &) = delete;
                IStateDumper(IStateDumper &&) = delete;
                virtual ~IStateDumper();

                IStateDumper &operator = (const IStateDumper &) = delete;
                IStateDumper &operator = (IStateDumper &&) = delete;

            public:
                virtual void begin_object(const char *name, const void *ptr, size_t szof);
                virtual void begin_object(const void *ptr, size_t szof);
                virtual void end_object();

                virtual void begin_array(const char *name, const void *ptr, size_t count);
                virtual void begin_array(const void *ptr, size_t count);
                virtual void end_array();

                virtual void write(const void *value);
                virtual void write(const char *value);
                virtual void write(bool value);
                virtual void write(unsigned char value);
                virtual void write(signed char value);
                virtual void write(unsigned short value);
                virtual void write(signed short value);
                virtual void write(unsigned int value);
                virtual void write(signed int value);
                virtual void write(unsigned long value);
                virtual void write(signed long value);
                virtual void write(unsigned long long value);
                virtual void write(signed long long value);
                virtual void write(float value);
                virtual void write(double value);

                virtual void write(const char *name, const void *value);
                virtual void write(const char *name, const char *value);
                virtual void write(const char *name, bool value);
                virtual void write(const char *name, unsigned char value);
                virtual void write(const char *name, signed char value);
                virtual void write(const char *name, unsigned short value);
                virtual void write(const char *name, signed short value);
                virtual void write(const char *name, unsigned int value);
                virtual void write(const char *name, signed int value);
                virtual void write(const char *name, unsigned long value);
                virtual void write(const char *name, signed long value);
                virtual void write(const char *name, unsigned long long value);
                virtual void write(const char *name, signed long long value);
                virtual void write(const char *name, float value);
                virtual void write(const char *name, double value);

                virtual void writev(const void * const *value, size_t count);
                virtual void writev(const bool *value, size_t count);
                virtual void writev(const unsigned char *value, size_t count);
                virtual void writev(const signed char *value, size_t count);
                virtual void writev(const unsigned short *value, size_t count);
                virtual void writev(const signed short *value, size_t count);
                virtual void writev(const unsigned int *value, size_t count);
                virtual void writev(const signed int *value, size_t count);
                virtual void writev(const unsigned long *value, size_t count);
                virtual void writev(const signed long *value, size_t count);
                virtual void writev(const unsigned long long *value, size_t count);
                virtual void writev(const signed long long *value, size_t count);
                virtual void writev(const float *value, size_t count);
                virtual void writev(const double *value, size_t count);

                virtual void writev(const char *name, const void * const *value, size_t count);
                virtual void writev(const char *name, const bool *value, size_t count);
                virtual void writev(const char *name, const unsigned char *value, size_t count);
                virtual void writev(const char *name, const signed char *value, size_t count);
                virtual void writev(const char *name, const unsigned short *value, size_t count);
                virtual void writev(const char *name, const signed short *value, size_t count);
                virtual void writev(const char *name, const unsigned int *value, size_t count);
                virtual void writev(const char *name, const signed int *value, size_t count);
                virtual void writev(const char *name, const unsigned long *value, size_t count);
                virtual void writev(const char *name, const signed long *value, size_t count);
                virtual void writev(const char *name, const unsigned long long *value, size_t count);
                virtual void writev(const char *name, const signed long long *value, size_t count);
                virtual void writev(const char *name, const float *value, size_t count);
                virtual void writev(const char *name, const double *value, size_t count);

            public:
                template <class T>
                inline void writev(const T * const * value, size_t count)
                {
                    writev(reinterpret_cast<const void * const *>(value), count);
                }

                template <class T>
                inline void writev(const char *name, const T * const * value, size_t count)
                {
                    writev(name, reinterpret_cast<const void * const *>(value), count);
                }

                template <class T>
                inline void write_object(const T *value)
                {
                    if (value != NULL)
                    {
                        begin_object(value, sizeof(T));
                        value->dump(this);
                        end_object();
                    }
                    else
                        write(value);
                }

                template <class T>
                inline void write_object(const char *name, const T *value)
                {
                    if (value != NULL)
                    {
                        begin_object(name, value, sizeof(T));
                        value->dump(this);
                        end_object();
                    }
                    else
                        write(name, value);
                }

                template <class T>
                inline void write_object_array(const T *value, size_t count)
                {
                    if (value != NULL)
                    {
                        begin_array(value, count);
                        {
                            for (size_t i=0; i<count; ++i)
                                write_object(&value[i]);
                        }
                        end_array();
                    }
                    else
                        write(value);
                }

                template <class T>
                inline void write_object_array(const char *name, const T *value, size_t count)
                {
                    if (value != NULL)
                    {
                        begin_array(name, value, count);
                        {
                            for (size_t i=0; i<count; ++i)
                                write_object(&value[i]);
                        }
                        end_array();
                    }
                    else
                        write(name, value);
                }

                template <class T>
                inline void write_object_array(const T * const *value, size_t count)
                {
                    if (value != NULL)
                    {
                        begin_array(value, count);
                        {
                            for (size_t i=0; i<count; ++i)
                                write_object(value[i]);
                        }
                        end_array();
                    }
                    else
                        write(value);
                }

                template <class T>
                inline void write_object_array(const char *name, const T * const *value, size_t count)
                {
                    if (value != NULL)
                    {
                        begin_array(name, value, count);
                        {
                            for (size_t i=0; i<count; ++i)
                                write_object(value[i]);
                        }
                        end_array();
                    }
                    else
                        write(name, value);
                }
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_IFACE_ISTATEDUMPER_H_ */
