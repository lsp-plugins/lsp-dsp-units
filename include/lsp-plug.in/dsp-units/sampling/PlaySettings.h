/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 18 нояб. 2022 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYSETTINGS_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYSETTINGS_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/sampling/types.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * This is a helper class to specify sample playback settings.
         * Specify different options of playing sample.
         */
        class LSP_DSP_UNITS_PUBLIC PlaySettings
        {
            protected:
                size_t              nID;                // The identifier of the sample
                size_t              nChannel;           // The audio channel of the sample
                float               fVolume;            // The volume of the sample
                bool                bReverse;           // Reverse playback
                size_t              nDelay;             // The delay before the playback should start (samples)
                size_t              nStart;             // The start position of the playback
                sample_loop_t       nLoopMode;          // Sample loop mode
                size_t              nLoopStart;         // Start of the loop (samples)
                size_t              nLoopEnd;           // End of the loop (samples)
                sample_crossfade_t  nLoopXFadeType;     // Loop crossfade type
                size_t              nLoopXFadeLength;   // Length of the crossfade between different sample sections

            public:
                static const PlaySettings default_settings;

            public:
                PlaySettings();
                ~PlaySettings();

                void                construct();
                void                destroy();

            public:
                /**
                 * Get sample identifier
                 * @return sample identifier
                 */
                inline size_t       sample_id() const       { return nID;           }

                /**
                 * Get sample channel
                 * @return sample channel
                 */
                inline size_t       sample_channel() const  { return nChannel;      }

                /**
                 * Get the volume of the sample
                 * @return the volume of the sample
                 */
                inline float        volume() const          { return fVolume;       }

                /**
                 * Get the reverse flag
                 * @return reverse playback flag
                 */
                inline bool         reverse() const         { return bReverse;      }

                /**
                 * Get the delay before the playback starts
                 * @return the delay in samples before the playback starts
                 */
                inline size_t       delay() const           { return nDelay;        }

                /**
                 * Get the start point of the playback
                 * @return the start point of the playback in samples
                 */
                inline size_t       start() const           { return nStart;        }

                /**
                 * Get the loop mode of the sample
                 * @return loop mode of the sample
                 */
                inline sample_loop_t loop_mode() const      { return nLoopMode;     }

                /**
                 * Get the start point of the repetition loop
                 * @return the start point of the repetition loop in samples
                 */
                inline size_t       loop_start() const      { return nLoopStart;    }

                /**
                 * Get the end point of the repetition loop
                 * @return the end point of the repetition loop in samples
                 */
                inline size_t       loop_end() const        { return nLoopEnd;      }

                /**
                 * Get type of the crossfade between sample segments in the loop
                 * @return tyoe of the crossfade between sample segments in the loop
                 */
                inline sample_crossfade_t loop_xfade_type() const   { return nLoopXFadeType; }

                /**
                 * Get the length of the crossfade between different parts of the sample
                 * @return the length of the crossfade betweeen different parts of the sample in samples
                 */
                inline size_t       loop_xfade_length() const       { return nLoopXFadeLength;  }

            public:
                /**
                 * Set sample identifier
                 * @param id sample identifier
                 */
                inline void         set_sample_id(size_t id)
                {
                    nID             = id;
                }

                /**
                 * Set sample channel
                 * @param channel sample channel
                 */
                inline void         set_sample_channel(size_t channel)
                {
                    nChannel        = channel;
                }

                /**
                 * Set sample to play
                 * @param sample_id sample identifier
                 * @param channel channel identifier
                 */
                inline void         set_channel(size_t sample_id, size_t channel)
                {
                    nID             = sample_id;
                    nChannel        = channel;
                }

                /**
                 * Set the playback volume of the sample
                 * @param volume playback volume of the sample
                 */
                inline void         set_volume(float volume)
                {
                    fVolume         = volume;
                }

                /**
                 * Set the playback delay
                 * @param delay the delay before the playback starts in samples
                 */
                inline void         set_delay(size_t delay)
                {
                    nDelay          = delay;
                }

                /**
                 * Set start position of the playback
                 * @param position start position of the playback in samples
                 */
                inline void         set_start(size_t position)
                {
                    nStart          = position;
                }

                /**
                 * Set start position of the playback
                 * @param position start position of the playback in samples
                 * @param reverse reverse playback flag
                 */
                inline void         set_start(size_t position, bool reverse)
                {
                    nStart          = position;
                    bReverse        = reverse;
                }

                /**
                 * Set the reversive playback flag
                 * @param reverse reverse playback flag
                 */
                inline void         set_reverse(bool reverse)
                {
                    bReverse        = reverse;
                }

                /**
                 * Set primary playback settings
                 * @param start start position of the playback in samples
                 * @param delay the delay before the playback starts in samples
                 * @param volume playback volume of the sample
                 */
                inline void         set_playback(size_t start, size_t delay, float volume)
                {
                    fVolume         = volume;
                    nDelay          = delay;
                    nStart          = start;
                }

                /**
                 * Set primary playback settings
                 * @param start start position of the playback in samples
                 * @param delay the delay before the playback starts in samples
                 * @param volume playback volume of the sample
                 * @param reverse reverse playback flag
                 */
                inline void         set_playback(size_t start, size_t delay, float volume, bool reverse)
                {
                    fVolume         = volume;
                    nDelay          = delay;
                    nStart          = start;
                    bReverse        = reverse;
                }

                /**
                 * Set-up loop mode
                 * @param mode loop mode
                 */
                inline void         set_loop_mode(sample_loop_t mode)
                {
                    nLoopMode       = mode;
                }

                /**
                 * Set-up start position of the loop range
                 * @param start start position of the loop range in samples
                 */
                inline void         set_loop_start(size_t start)
                {
                    nLoopStart      = start;
                }

                /**
                 * Set-up end position of the loop range
                 * @param end end position of the loop range in samples
                 */
                inline void         set_loop_end(size_t end)
                {
                    nLoopEnd        = end;
                }

                /**
                 * Set-up loop range
                 * @param start start position of the loop range in samples
                 * @param end end position of the loop range in samples
                 */
                inline void         set_loop_range(size_t start, size_t end)
                {
                    nLoopStart      = start;
                    nLoopEnd        = end;
                }

                /**
                 * Set-up loop range
                 * @param mode loop mode
                 * @param start start position of the loop range in samples
                 * @param end end position of the loop range in samples
                 */
                inline void         set_loop_range(sample_loop_t mode, size_t start, size_t end)
                {
                    nLoopMode       = mode;
                    nLoopStart      = start;
                    nLoopEnd        = end;
                }

                /**
                 * Set the crossfade type between different parts of the sample
                 * @param type type of crossfade between different parts of the sample
                 */
                inline void         set_loop_xfade_type(sample_crossfade_t type)
                {
                    nLoopXFadeType  = type;
                }

                /**
                 * Set the crossfade length between different parts of the sample
                 * @param length the crossfade length between different parts of the sample in samples
                 */
                inline void         set_loop_xfade_length(size_t length)
                {
                    nLoopXFadeLength= length;
                }

                /**
                 * Set the crossfade settings for the loop
                 * @param type type of crossfade between different parts of the sample
                 * @param length the crossfade length between different parts of the sample in samples
                 */
                inline void         set_loop_xfade(sample_crossfade_t type, size_t length)
                {
                    nLoopXFadeType  = type;
                    nLoopXFadeLength= length;
                }
        };

    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYSETTINGS_H_ */
