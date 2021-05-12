/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-phase-detector
 * Created on: 12 мая 2021 г.
 *
 * lsp-plugins-phase-detector is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-phase-detector is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-phase-detector. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_PLUGINS_PHASE_DETECTOR_H_
#define PRIVATE_PLUGINS_PHASE_DETECTOR_H_

#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <private/meta/phase_detector.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Phase detector plugin implementation
         */
        class phase_detector: public plug::Module
        {
            private:
                phase_detector & operator = (const phase_detector &);

            protected:
                typedef struct buffer_t
                {
                    float      *pData;
                    size_t      nSize;
                } buffer_t;

                typedef struct meters_t
                {
                    plug::IPort        *pTime;
                    plug::IPort        *pSamples;
                    plug::IPort        *pDistance;
                    plug::IPort        *pValue;
                } meters_t;

                enum meter_kind_t
                {
                    MK_BEST,
                    MK_SEL,
                    MK_WORST,

                    MK_COUNT
                };

            protected:
                float               fTimeInterval;
                float               fReactivity;

                float              *vFunction;
                float              *vAccumulated;
                float              *vNormalized;

                size_t              nMaxVectorSize;
                size_t              nVectorSize;
                size_t              nFuncSize;

                size_t              nGapSize;
                size_t              nMaxGapSize;
                size_t              nGapOffset;

                buffer_t            vA, vB;

                float               fTau;
                float               fSelector;
                bool                bBypass;

                plug::IPort        *vIn[2];             // Inputs
                plug::IPort        *vOut[2];            // Outputs
                plug::IPort        *pBypass;            // Bypass switch
                plug::IPort        *pReset;             // Reset button
                plug::IPort        *pSelector;          // Selector knob
                plug::IPort        *pTime;              // Time
                plug::IPort        *pReactivity;        // Reactivity
                meters_t            vMeters[MK_COUNT];  // Output meters
                plug::IPort        *pFunction;          // Output function

                // TODO
//                float_buffer_t     *pIDisplay;      // Inline display buffer

            protected:
                size_t              fill_gap(const float *a, const float *b, size_t count);
                void                clear_buffers();
                bool                set_time_interval(float interval, bool force);
                void                set_reactive_interval(float interval);
                void                drop_buffers();
                static void         dump_buffer(dspu::IStateDumper *v, const buffer_t *buf, const char *label);

            public:
                explicit            phase_detector(const meta::plugin_t *meta);
                virtual            ~phase_detector();

                virtual void        init(plug::IWrapper *wrapper);
                void                destroy();

            public:

                virtual void        update_sample_rate(long sr);
                virtual void        update_settings();
                virtual void        process(size_t samples);
                virtual bool        inline_display(plug::ICanvas *cv, size_t width, size_t height);
                virtual void        dump(dspu::IStateDumper *v) const;
        };
    }
}



#endif /* PRIVATE_PLUGINS_PHASE_DETECTOR_H_ */
