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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/dsp-units/misc/shaping.h>
#include <lsp-plug.in/stdlib/math.h>

UTEST_BEGIN("dspu.util", shaping)

	UTEST_MAIN
	{
		dspu::shaping::shaping_t params;

		params.sinusoidal.slope = 0.5f * M_PI;
		UTEST_ASSERT(dspu::shaping::sinusoidal(&params, 0.0f) == 0.0f);
		UTEST_ASSERT(dspu::shaping::sinusoidal(&params, 1.0f) == 1.0f);
		UTEST_ASSERT(dspu::shaping::sinusoidal(&params, -1.0f) == -1.0f);
		UTEST_ASSERT(dspu::shaping::sinusoidal(&params, 2.0f) == 1.0f);
		UTEST_ASSERT(dspu::shaping::sinusoidal(&params, -2.0f) == -1.0f);

		params.polynomial.shape = 0.5f;
		UTEST_ASSERT(dspu::shaping::polynomial(&params, 0.0f) == 0.0f);
		UTEST_ASSERT(dspu::shaping::polynomial(&params, 1.0f) == 1.0f);
		UTEST_ASSERT(dspu::shaping::polynomial(&params, -1.0f) == -1.0f);
		UTEST_ASSERT(dspu::shaping::polynomial(&params, 2.0f) == 1.0f);
		UTEST_ASSERT(dspu::shaping::polynomial(&params, -2.0f) == -1.0f);
	}

UTEST_END;
