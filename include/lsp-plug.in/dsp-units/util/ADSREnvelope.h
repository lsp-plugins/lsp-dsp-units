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
                    ADSR_NONE,
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

                struct gen_none_t
                {
                    float               fT;
                    float               fK;
                    float               fB;
                };

                struct gen_line_t
                {
                    float               fT1;
                    float               fT2;
                    float               fK1;
                    float               fB1;
                    float               fK2;
                    float               fB2;
                };

                union gen_params_t
                {
                    gen_none_t          sNone;
                    gen_line_t          sLine;
                };

                typedef float (*generator_t)(float t, const gen_params_t *params);

                struct curve_t
                {
                    float               fTime;
                    float               fCurve;
                    function_t          enFunction;
                    generator_t         pGenerator;
                    gen_params_t        sParams;
                };

            protected:
                curve_t             vCurve[P_TOTAL];
                float               fHoldTime;
                float               fBreakLevel;
                float               fSustainLevel;
                uint32_t            nFlags;

            protected:
                static float        none_generator(float t, const gen_params_t *params);
                static float        line_generator(float t, const gen_params_t *params);
                static inline float limit_range(float t, float prev);
                static inline void  configure_curve(curve_t *curve, float x0, float x1, float y0, float y1);

            protected:
                void                set_param(float & param, float value);
                void                set_function(function_t & func, function_t value);
                void                set_flag(uint32_t flag, bool set);
                void                set_curve(part_t part, float time, float curve, function_t func);
                inline float        do_process(float value);

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
                inline void         set_attack_time(float value)        { set_param(vCurve[P_ATTACK].fTime, value);         }
                inline void         set_hold_time(float value)          { set_param(fHoldTime, value);                      }
                inline void         set_decay_time(float value)         { set_param(vCurve[P_DECAY].fTime, value);          }
                inline void         set_slope_time(float value)         { set_param(vCurve[P_SLOPE].fTime, value);          }
                inline void         set_relese_time(float value)        { set_param(vCurve[P_RELEASE].fTime, value);        }

                inline void         set_attack_curve(float value)       { set_param(vCurve[P_ATTACK].fCurve, value);        }
                inline void         set_decay_curve(float value)        { set_param(vCurve[P_DECAY].fCurve, value);         }
                inline void         set_slope_curve(float value)        { set_param(vCurve[P_SLOPE].fCurve, value);         }
                inline void         set_release_curve(float value)      { set_param(vCurve[P_RELEASE].fCurve, value);       }

                inline void         set_attack_function(function_t f)   { set_function(vCurve[P_ATTACK].enFunction, f);     }
                inline void         set_decay_function(function_t f)    { set_function(vCurve[P_DECAY].enFunction, f);      }
                inline void         set_slope_function(function_t f)    { set_function(vCurve[P_SLOPE].enFunction, f);      }
                inline void         set_release_function(function_t f)  { set_function(vCurve[P_RELEASE].enFunction, f);    }

                inline void         set_break_level(float value)        { set_param(fBreakLevel, value);                    }
                inline void         set_sustain_level(float value)      { set_param(fSustainLevel, value);                  }

                inline void         set_hold_enabled(bool enabled)      { set_flag(F_USE_HOLD, enabled);                    }
                inline void         set_break_enabled(bool enabled)     { set_flag(F_USE_BREAK, enabled);                   }

                inline void         set_attack(float time, float curve, function_t func)    { set_curve(P_ATTACK, time, curve, func);   }
                void                set_hold(float time, bool enabled);
                inline void         set_decay(float time, float curve, function_t func)     { set_curve(P_DECAY, time, curve, func);    }
                void                set_break(float level, bool enabled);
                inline void         set_slope(float time, float curve, function_t func)     { set_curve(P_SLOPE, time, curve, func);    }
                inline void         set_release(float time, float curve, function_t func)   { set_curve(P_RELEASE, time, curve, func);  }

                inline float        attack_time() const                 { return vCurve[P_ATTACK].fTime;                    }
                inline float        hold_time() const                   { return fHoldTime;                                 }
                inline float        decay_time() const                  { return vCurve[P_DECAY].fTime;                     }
                inline float        slope_time() const                  { return vCurve[P_SLOPE].fTime;                     }
                inline float        release_time() const                { return vCurve[P_RELEASE].fTime;                   }

                inline float        attack_curve() const                { return vCurve[P_ATTACK].fCurve;                   }
                inline float        decay_curve() const                 { return vCurve[P_DECAY].fCurve;                    }
                inline float        slope_curve() const                 { return vCurve[P_SLOPE].fCurve;                    }
                inline float        release_curve() const               { return vCurve[P_RELEASE].fCurve;                  }

                inline function_t   attack_function() const             { return vCurve[P_ATTACK].enFunction;               }
                inline function_t   decay_function() const              { return vCurve[P_DECAY].enFunction;                }
                inline function_t   slope_function() const              { return vCurve[P_SLOPE].enFunction;                }
                inline function_t   release_function() const            { return vCurve[P_RELEASE].enFunction;              }

                inline float        break_level() const                 { return fBreakLevel;                               }
                inline float        sustain_level() const               { return fSustainLevel;                             }

                inline float        hold_enabled() const                { return nFlags & F_USE_HOLD;                       }
                inline float        break_enabled() const               { return nFlags & F_USE_BREAK;                      }

            public:
                /**
                 * Update settings if needed
                 */
                void            update_settings();

                /**
                 * Compute ADSR point for specified value
                 * @param value normalized time point [0..1]
                 * @return computed ADSR value or 0 if outside the [0..1] range
                 */
                float           process(float value);

                /**
                 * Compute ADSR points for specified values
                 * @param dst destination buffer to store result
                 * @param src source values to process
                 * @param count number of values to process
                 */
                void            process(float *dst, const float *src, size_t count);

                /**
                 * Compute amd apply ADSR points for specified values and apply to target buffer
                 * @param dst destination buffer to apply result
                 * @param src source values to process
                 * @param count number of values to process
                 */
                void            process_mul(float *dst, const float *src, size_t count);

                /**
                 * Generate part of the ADSR curve and store to buffer
                 * @param dst pointer to store generated values
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to generate
                 */
                void            generate(float *dst, float start, float step, size_t count);

                /**
                 * Generate and apply curve to the buffer
                 * @param dst destination buffer to modify
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to apply
                 */
                void            generate_mul(float *dst, float start, float step, size_t count);

                /**
                 * Apply curve to the buffer
                 * @param dst destination buffer to store result
                 * @param src source buffer to read
                 * @param start the time of first point
                 * @param step the actual time step
                 * @param count number of points to apply
                 */
                void            generate_mul(float *dst, const float *src, float start, float step, size_t count);

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_ADSRENVELOPE_H_ */
