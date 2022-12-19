/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_VERSION_H_
#define LSP_PLUG_IN_DSP_UNITS_VERSION_H_

// Define version of headers
#define LSP_DSP_UNITS_MAJOR         1
#define LSP_DSP_UNITS_MINOR         0
#define LSP_DSP_UNITS_MICRO         8

#if defined(LSP_DSP_UNITS_PUBLISHER)
    #define LSP_DSP_UNITS_PUBLIC        LSP_EXPORT_MODIFIER
#elif defined(LSP_DSP_UNITS_BUILTIN) || defined(LSP_IDE_DEBUG)
    #define LSP_DSP_UNITS_PUBLIC
#else
    #define LSP_DSP_UNITS_PUBLIC        LSP_IMPORT_MODIFIER
#endif

#endif /* LSP_PLUG_IN_DSP_UNITS_VERSION_H_ */
