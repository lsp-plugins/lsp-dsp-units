/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 авг. 2021 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_TRIGGER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_TRIGGER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum trg_mode_t
        {
            TRG_MODE_SINGLE,
            TRG_MODE_MANUAL,
            TRG_MODE_REPEAT,
            TRG_MODE_MAX
        };

        enum trg_type_t
        {
            TRG_TYPE_NONE,
            TRG_TYPE_SIMPLE_RISING_EDGE,
            TRG_TYPE_SIMPLE_FALLING_EDGE,
            TRG_TYPE_ADVANCED_RISING_EDGE,
            TRG_TYPE_ADVANCED_FALLING_EDGE,
            TRG_TYPE_MAX
        };

        enum trg_state_t
        {
            TRG_STATE_WAITING,
            TRG_STATE_ARMED,
            TRG_STATE_FIRED,
            TRG_STATE_MAX
        };

        class Trigger
        {
            protected:

                // To use with Manual and Single Mode
                typedef struct trg_locks_t
                {
                    bool    bSingleLock;
                    bool    bManualAllow;
                    bool    bManualLock;
                } trg_locks_t;

                typedef struct simple_trg_t
                {
                    float   fThreshold;
                } simple_trg_t;

                typedef struct advanced_trg_t
                {
                    float   fThreshold;
                    float   fHysteresis;
                    float   fLowerThreshold;
                    float   fUpperThreshold;
                    bool    bDisarm;
                } advanced_trg_t;

            private:
                Trigger & operator = (const Trigger &);

            private:
                float           fPrevious;

                trg_mode_t      enTriggerMode;
                trg_type_t      enTriggerType;
                trg_state_t     enTriggerState;

                size_t          nTriggerHold;
                size_t          nTriggerHoldCounter;

                trg_locks_t     sLocks;

                simple_trg_t    sSimpleTrg;
                advanced_trg_t  sAdvancedTrg;

                bool            bSync;

            public:
                explicit Trigger();
                ~Trigger();

                void            construct();
                void            destroy();

            protected:
                inline void set_simple_trg_threshold(float threshold)
                {
                    sSimpleTrg.fThreshold = threshold;
                }

                inline void update_advanced_trg()
                {
                    sAdvancedTrg.fLowerThreshold = sAdvancedTrg.fThreshold - sAdvancedTrg.fHysteresis;
                    sAdvancedTrg.fUpperThreshold = sAdvancedTrg.fThreshold + sAdvancedTrg.fHysteresis;
                }

                inline void set_advanced_trg_threshold(float threshold)
                {
                    sAdvancedTrg.fThreshold = threshold;

                    update_advanced_trg();
                }

                inline void set_advanced_trg_hysteresis(float hysteresis)
                {
                    if (hysteresis < 0.0f)
                        hysteresis = -hysteresis;

                    sAdvancedTrg.fHysteresis = hysteresis;

                    update_advanced_trg();
                }

            public:

                /** Check that trigger needs settings update.
                 *
                 * @return true if trigger needs settings update.
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** This method should be called if needs_update() returns true.
                 * before calling process() methods.
                 *
                 */
                void update_settings();

                /** Set the trigger mode.
                 *
                 * @param mode trigger mode.
                 */
                inline void set_trigger_mode(trg_mode_t mode)
                {
                    if ((mode < TRG_MODE_SINGLE) || (mode >= TRG_MODE_MAX) || (enTriggerMode == mode))
                        return;

                    enTriggerMode = mode;
                    bSync = true;
                }

                /** Reset the single trigger.
                 *
                 */
                inline void reset_single_trigger()
                {
                    sLocks.bSingleLock = false;
                    bSync = true;
                }

                /** Activate the manual trigger.
                 *
                 */
                inline void activate_manual_trigger()
                {
                    sLocks.bManualAllow = true;
                    sLocks.bManualLock = false;
                    bSync = true;
                }

                /** Set the post-trigger samples. The trigger can be allowed to fire only after the post-trigger samples have elapsed.
                 *
                 * @param nSamples number of post-trigger samples.
                 */
                inline void set_trigger_hold_samples(size_t nSamples)
                {
                    if (nSamples == nTriggerHold)
                        return;

                    nTriggerHold = nSamples;
                    nTriggerHoldCounter = 0;
                }

                /** Set the trigger type.
                 *
                 * @param type trigger type.
                 */
                inline void set_trigger_type(trg_type_t type)
                {
                    if ((type < TRG_TYPE_NONE) || (type >= TRG_TYPE_MAX) || (enTriggerType == type))
                        return;

                    enTriggerType = type;
                    bSync = true;
                }

                /** Set the trigger threshold.
                 *
                 * @param trigger threshold.
                 */
                inline void set_trigger_threshold(float threshold)
                {
                    set_simple_trg_threshold(threshold);
                    set_advanced_trg_threshold(threshold);
                    bSync = true;
                }

                /** Set the trigger hysteresis.
                 *
                 * @param hysteresis hysteresis.
                 */
                inline void set_trigger_hysteresis(float hysteresis)
                {
                    set_advanced_trg_hysteresis(hysteresis);
                    bSync = true;
                }

                /** Return he trigger state.
                 *
                 * @return trigger state.
                 */
                inline trg_state_t get_trigger_state() const
                {
                    return enTriggerState;
                }

                /** Feed a single sample to the trigger. Query the trigger status afterwards.
                 *
                 */
                void single_sample_processor(float value);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}




#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_TRIGGER_H_ */
