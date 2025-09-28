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

#include <lsp-plug.in/dsp-units/misc/shaping.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/misc/quickmath.h>

namespace lsp
{
	namespace dspu
	{
		namespace shaping
		{

			LSP_DSP_UNITS_PUBLIC
			float sinusoidal(shaping_t *params, float value)
			{
				params->sinusoidal.slope = lsp_limit(params->sinusoidal.slope, 1e-3f, 0.5f * M_PI);

				float radius = M_PI / (2.0f * params->sinusoidal.slope);

				if (value >= radius)
					return 1.0f;

				if (value <= -radius)
					return -1.0f;

				return quick_sinf(params->sinusoidal.slope * value);
			}

			LSP_DSP_UNITS_PUBLIC
			float polynomial(shaping_t *params, float value)
			{
				params->polynomial.shape = lsp_limit(params->polynomial.shape, 1e-3f, 1.0f);

				float radius = 1.0f - params->polynomial.shape;

				if (value >= 1.0f)
					return 1.0f;

				if (value <= -1.0f)
					return -1.0f;

				if ((value <= radius) || (value >= -radius))
					return (2.0f * value) / (2.0f - params->polynomial.shape);

				float value2 = value * value;
				float c_m_1_2 = (params->polynomial.shape - 1.0f) * (params->polynomial.shape - 1.0f);

				if ((value > radius))
					return (value2 - 2 * value + c_m_1_2) / (params->polynomial.shape * (params->polynomial.shape - 2.0f));

				return (value2 + 2 * value + c_m_1_2) / (params->polynomial.shape * (2.0f - params->polynomial.shape));
			}

		} /* namespace shaping */
	} /* dspu */
} /* namespace lsp */
