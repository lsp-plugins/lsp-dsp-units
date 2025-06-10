/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 июн. 2025 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_ADSRENVELOPE_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_ADSRENVELOPE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * ADSR envelope calculator.
         * Supports additional Hold and Break points working as AHDBSSR
         * (Attack, Hold, Decay, Break, Slope, Sustain, Release) curve
         */
        class ADSREnvelope
        {
            public:
                enum function_t
                {
                    ADSR_LINE
                };

            protected:
                enum flags_t
                {
                    F_USE_HOLD          = 1 << 0,
                    F_USE_BREAK         = 1 << 1,
                    F_RECONFIGURE       = 1 << 2,
                };

                enum part_t
                {
                    P_ATTACK,
                    P_DECAY,
                    P_SLOPE,
                    P_RELEASE,

                    P_TOTAL
                };

                struct line_func_t
                {
                    float               fK1;
                    float               fK2;
                    float               fB2;
                };

                struct curve_t
                {
                    float               fEnd;
                    float               fCurve;
                    function_t          enFunction;
                    union {
                        line_func_t         sLine;
                    };
                };

            protected:
                curve_t             vCurve[P_TOTAL];
                float               fHoldTime;
                float               fBreakLevel;
                float               fSustainLevel;
                uint32_t            nFlags;

            protected:
                void                set_param(float & param, float value);
                void                set_function(function_t & func, function_t value);

            public:
                ADSREnvelope();
                ~ADSREnvelope();

                ADSREnvelope(const ADSREnvelope &) = delete;
                ADSREnvelope(ADSREnvelope &&) = delete;
                ADSREnvelope & operator = (const ADSREnvelope &) = delete;
                ADSREnvelope & operator = (ADSREnvelope &&) = delete;

                /**
                 * Construct envelope
                 */
                void                construct();

                /**
                 * Destroy envelope
                 */
                void                destroy();

            public:
                inline void         set_attack_time(float value)        { set_param(vCurve[P_ATTACK].fEnd, value);      }
                inline void         set_hold_time(float value)          { set_param(fHoldTime, value);                  }
                inline void         set_decay_time(float value)         { set_param(vCurve[P_DECAY].fEnd, value);       }
                inline void         set_slope_time(float value)         { set_param(vCurve[P_SLOPE].fEnd, value);       }
                inline void         set_relese_time(float value)        { set_param(vCurve[P_RELEASE].fEnd, value);     }

                inline void         set_attack_curve(float value)       { set_param(vCurve[P_ATTACK].fCurve, value);    }
                inline void         set_decay_curve(float value)        { set_param(vCurve[P_DECAY].fCurve, value);     }
                inline void         set_slope_curve(float value)        { set_param(vCurve[P_SLOPE].fCurve, value);     }
                inline void         set_release_curve(float value)      { set_param(vCurve[P_RELEASE].fCurve, value);   }

                inline void         set_attack_function(float value)    { set_param(vCurve[P_ATTACK].fCurve, value);    }
                inline void         set_decay_function(float value)     { set_param(vCurve[P_DECAY].fCurve, value);     }
                inline void         set_slope_function(float value)     { set_param(vCurve[P_SLOPE].fCurve, value);     }
                inline void         set_release_function(float value)   { set_param(vCurve[P_RELEASE].fCurve, value);   }

                inline void         set_break_level(float value)        { set_param(fBreakLevel, value);                }
                inline void         set_sustain_level(float value)      { set_param(fSustainLevel, value);              }

                inline float        attack_time() const                 { return vCurve[P_ATTACK].fEnd;                 }
                inline float        hold_time() const                   { return fHoldTime;                             }
                inline float        decay_time() const                  { return vCurve[P_DECAY].fEnd;                  }
                inline float        slope_time() const                  { return vCurve[P_SLOPE].fEnd;                  }
                inline float        release_time() const                { return vCurve[P_RELEASE].fEnd;                }

                inline float        attack_curve() const                { return vCurve[P_ATTACK].fCurve;               }
                inline float        decay_curve() const                 { return vCurve[P_DECAY].fCurve;                }
                inline float        slope_curve() const                 { return vCurve[P_SLOPE].fCurve;                }
                inline float        release_curve() const               { return vCurve[P_RELEASE].fCurve;              }

                inline function_t   attack_function() const             { return vCurve[P_ATTACK].enFunction;           }
                inline function_t   decay_function() const              { return vCurve[P_DECAY].enFunction;            }
                inline function_t   slope_function() const              { return vCurve[P_SLOPE].enFunction;            }
                inline function_t   release_function() const            { return vCurve[P_RELEASE].enFunction;          }

                inline float        break_level() const                 { return fBreakLevel;                           }
                inline float        sustain_level() const               { return fSustainLevel;                         }

                inline float        hold_enabled() const                { return nFlags & F_USE_HOLD;                   }
                inline float        break_enabled() const               { return nFlags & F_USE_BREAK;                  }

            public:
                /**
                 * Update settings if needed
                 */
                void            update_settings();

                /**
                 * Compute ADSR point
                 * @param value normalized time point [0..1]
                 * @return computed ADSR value or 0 if outside the [0..1] range
                 */
                float           compute(float value);

                /**
                 * Generate part of the ADSR curve and store to buffer
                 * @param dst pointer to store generated values
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to generate
                 */
                void            generate(float *dst, float start, float step, size_t count);

                /**
                 * Apply curve to the buffer
                 * @param dst destination buffer to modify
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to apply
                 */
                void            apply(float *dst, float start, float step, size_t count);

                /**
                 * Apply curve to the buffer
                 * @param dst destination buffer to store result
                 * @param src source buffer to read
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to apply
                 */
                void            apply(float *dst, const float *src, float start, float step, size_t count);

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_ADSRENVELOPE_H_ */
