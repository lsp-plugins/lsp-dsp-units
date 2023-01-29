/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 30 окт. 2022 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_INSAMPLESTREAM_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_INSAMPLESTREAM_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/mm/IInAudioStream.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Input audio stream that allows to wrap a sample and perform streaming reads from it.
         */
        class LSP_DSP_UNITS_PUBLIC InSampleStream: public mm::IInAudioStream
        {
            private:
                InSampleStream(const InSampleStream &);
                InSampleStream & operator = (const InSampleStream &);

            protected:
                const Sample   *pSample;
                bool            bDelete;

            protected:
                void            do_close();

            protected:
                virtual ssize_t direct_read(void *dst, size_t nframes, size_t fmt) override;
                virtual size_t select_format(size_t fmt) override;

            public:
                InSampleStream();
                InSampleStream(const Sample *s, bool drop = false);
                virtual ~InSampleStream() override;

            public:
                status_t            wrap(const dspu::Sample *s, bool drop = false);

            public:
                virtual status_t    info(mm::audio_stream_t *dst) const override;
                virtual size_t      sample_rate() const override;
                virtual size_t      channels() const override;
                virtual wssize_t    length() const override;
                virtual size_t      format() const override;

                virtual status_t    close() override;
                virtual wssize_t    skip(wsize_t nframes) override;
                virtual wssize_t    seek(wsize_t nframes) override;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_INSAMPLESTREAM_H_ */
