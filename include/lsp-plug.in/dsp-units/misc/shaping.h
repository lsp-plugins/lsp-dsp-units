/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 Sept 2025.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
	namespace dspu
	{
		namespace shaping
		{

			typedef struct sinusoidal_t
			{
				float slope;  // 0 < slope < M_PI / 2.0f
			} sinusoidal_t;

			typedef struct polynomial_t
			{
				float shape;  // 0 < shape <= 1
			} polynomial_t;

			union shaping_t
			{
				sinusoidal_t sinusoidal;
				polynomial_t polynomial;
			};

			LSP_DSP_UNITS_PUBLIC
			float sinusoidal(shaping_t *params, float value);

			LSP_DSP_UNITS_PUBLIC
			float polynomial(shaping_t *params, float value);

		} /* namespace shaping */
	} /* dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_ */
