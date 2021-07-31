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

#include <private/plugins/phase_detector.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <lsp-plug.in/shared/id_colors.h>

#define TRACE_PORT(p)       lsp_trace("  port id=%s", (p)->metadata()->id);

namespace lsp
{
    namespace plugins
    {
        //---------------------------------------------------------------------
        // Plugin factory
        static const meta::plugin_t *plugins[] =
        {
            &meta::phase_detector
        };

        static plug::Module *phase_detector_factory(const meta::plugin_t *meta)
        {
            return new phase_detector(meta);
        }

        static plug::Factory factory(phase_detector_factory, plugins, 1);

        //---------------------------------------------------------------------
        // Implementation
        phase_detector::phase_detector(const meta::plugin_t *meta):
            Module(meta)
        {
            fTimeInterval       = meta::phase_detector_metadata::DETECT_TIME_DFL;
            fReactivity         = meta::phase_detector_metadata::REACT_TIME_DFL;

            vFunction           = NULL;
            vAccumulated        = NULL;
            vNormalized         = NULL;

            nMaxVectorSize      = 0;
            nVectorSize         = 0;
            nFuncSize           = 0;

            nGapSize            = 0;
            nMaxGapSize         = 0;
            nGapOffset          = 0;

            nBest               = 0;
            nWorst              = 0;
            nSelected           = 0;

            vA.nSize            = 0;
            vA.pData            = NULL;
            vB.nSize            = 0;
            vB.pData            = NULL;

            fTau                = 0.0f;
            fSelector           = meta::phase_detector_metadata::SELECTOR_DFL;
            bBypass             = false;

            vIn[0]              = NULL;
            vIn[1]              = NULL;
            vOut[0]             = NULL;
            vOut[1]             = NULL;
            pBypass             = NULL;
            pReset              = NULL;
            pSelector           = NULL;
            pTime               = NULL;
            pReactivity         = NULL;

            for (size_t i=0; i<MK_COUNT; ++i)
            {
                meters_t *vm        = &vMeters[i];
                vm->pTime           = NULL;
                vm->pSamples        = NULL;
                vm->pDistance       = NULL;
                vm->pValue          = NULL;
            }
            pFunction           = NULL;

            pIDisplay           = NULL;
        }

        phase_detector::~phase_detector()
        {
            drop_buffers();
        }

        void phase_detector::init(plug::IWrapper *wrapper)
        {
            Module::init(wrapper);

            // Bind ports
            lsp_trace("Binding ports");
            size_t port_id      = 0;

            // Bind audio ports
            for (size_t i=0; i<2; ++i)
            {
                TRACE_PORT(vPorts[port_id]);
                vIn[i]      = vPorts[port_id++];
            }
            for (size_t i=0; i<2; ++i)
            {
                TRACE_PORT(vPorts[port_id]);
                vOut[i]     = vPorts[port_id++];
            }

            // Bind controls
            lsp_trace("Binding controls");
            TRACE_PORT(vPorts[port_id]);
            pBypass     = vPorts[port_id++];
            TRACE_PORT(vPorts[port_id]);
            pReset      = vPorts[port_id++];
            TRACE_PORT(vPorts[port_id]);
            pTime       = vPorts[port_id++];
            TRACE_PORT(vPorts[port_id]);
            pReactivity = vPorts[port_id++];
            TRACE_PORT(vPorts[port_id]);
            pSelector   = vPorts[port_id++];

            // Bind meters
            lsp_trace("Binding meters");
            for (size_t i=0; i<MK_COUNT; ++i)
            {
                meters_t *vm = &vMeters[i];

                TRACE_PORT(vPorts[port_id]);
                vm->pTime       = vPorts[port_id++];
                TRACE_PORT(vPorts[port_id]);
                vm->pSamples    = vPorts[port_id++];
                TRACE_PORT(vPorts[port_id]);
                vm->pDistance   = vPorts[port_id++];
                TRACE_PORT(vPorts[port_id]);
                vm->pValue      = vPorts[port_id++];
            }

            TRACE_PORT(vPorts[port_id]);
            pFunction   = vPorts[port_id++];
        }

        void phase_detector::destroy()
        {
            drop_buffers();
            Module::destroy();
        }

        size_t phase_detector::fill_gap(const float *a, const float *b, size_t count)
        {
            lsp_assert(a != NULL);
            lsp_assert(b != NULL);
            lsp_assert(vA.pData != NULL);
            lsp_assert(vB.pData != NULL);

            size_t fill         = nMaxGapSize - nGapSize;

            if (fill <= 0)
            {
                if (nGapOffset < nGapSize)
                    return 0;

                lsp_assert((nGapSize + vA.nSize) <= (nMaxVectorSize * 3));
                lsp_assert((nGapSize + vB.nSize) <= (nMaxVectorSize * 4));

                dsp::copy(vA.pData, &vA.pData[nGapSize], vA.nSize);
                dsp::copy(vB.pData, &vB.pData[nGapSize], vB.nSize);
                nGapSize            = 0;
                nGapOffset          = 0;
                fill                = nMaxGapSize;
            }

            if (count < fill)
                fill                = count;

            lsp_assert((nGapSize + vA.nSize + fill) <= (nMaxVectorSize * 3));
            lsp_assert((nGapSize + vB.nSize + fill) <= (nMaxVectorSize * 4));

            dsp::copy(&vA.pData[vA.nSize + nGapSize], a, fill);
            dsp::copy(&vB.pData[vB.nSize + nGapSize], b, fill);
            nGapSize           += fill;

            return fill;
        }

        void phase_detector::clear_buffers()
        {
            lsp_debug("force buffer clear");
            lsp_assert(vA.pData != NULL);
            lsp_assert(vB.pData != NULL);
            lsp_assert(vFunction != NULL);
            lsp_assert(vAccumulated != NULL);
            lsp_assert(vNormalized != NULL);

            dsp::fill_zero(vA.pData, nMaxVectorSize * 3);
            dsp::fill_zero(vB.pData, nMaxVectorSize * 4);
            dsp::fill_zero(vFunction, nMaxVectorSize * 2);
            dsp::fill_zero(vAccumulated, nMaxVectorSize * 2);
            dsp::fill_zero(vNormalized, nMaxVectorSize * 2);
        }

        void phase_detector::drop_buffers()
        {
            // Drop previously used buffers
            if (vA.pData != NULL)
            {
                delete []   vA.pData;
                vA.pData    = NULL;
            }
            if (vB.pData != NULL)
            {
                delete []   vB.pData;
                vB.pData    = NULL;
            }
            if (vFunction != NULL)
            {
                delete []   vFunction;
                vFunction   = NULL;
            }
            if (vAccumulated != NULL)
            {
                delete []   vAccumulated;
                vAccumulated= NULL;
            }
            if (vNormalized != NULL)
            {
                delete []   vNormalized;
                vNormalized = NULL;
            }
            if (pIDisplay != NULL)
            {
                pIDisplay->destroy();
                pIDisplay   = NULL;
            }
        }

        bool phase_detector::set_time_interval(float interval, bool force)
        {
            lsp_debug("interval = %.3f", interval);

            // Calculate parameters
            if ((!force) && (fTimeInterval == interval))
                return false;

            // Re-calculate buffers
            fTimeInterval   = interval;
            nVectorSize     = (size_t(dspu::millis_to_samples(fSampleRate, interval)) >> 2) << 2; // Make number of samples multiple of SSE register size
            nFuncSize       = nVectorSize << 1;
            vA.nSize        = nFuncSize;
            vB.nSize        = nFuncSize + nVectorSize;
            nMaxGapSize     = (nMaxVectorSize * 3) - nFuncSize; // Size of A buffer - size of function
            nGapSize        = 0;
            nGapOffset      = 0;

            // Yep, clear all buffers
            return true;
        }

        void phase_detector::set_reactive_interval(float interval)
        {
            lsp_debug("reactivity = %.3f", interval);

            // Calculate Reduction
            fReactivity     = interval;
            fTau            = 1.0f - expf(logf(1.0 - M_SQRT1_2) / dspu::seconds_to_samples(fSampleRate, interval));
        }

        void phase_detector::update_sample_rate(long sr)
        {
            lsp_debug("sample_rate = %ld", sr);
            /*
                              +---------+---------+---------+
             A:               | Gap     | A Data  | Lookup  |
                              +---------+---------+---------+
                                   /                       |
                    +---------+---------+---------+---------+
             B:     | Gap     | B Delay | B Data  | Lookup  |
                    +---------+---------+---------+---------+
                               |                      /
                              +---------+---------+
             F:               | Correlation funcs |
                              +---------+---------+
            */

            // Cleanup buffers
            drop_buffers();

            nMaxVectorSize  = dspu::millis_to_samples(fSampleRate, meta::phase_detector_metadata::DETECT_TIME_MAX);
            vA.pData        = new float[nMaxVectorSize * 3];
            vB.pData        = new float[nMaxVectorSize * 4];
            vFunction       = new float[nMaxVectorSize * 2];
            vAccumulated    = new float[nMaxVectorSize * 2];
            vNormalized     = new float[nMaxVectorSize * 2];

            set_time_interval(fTimeInterval, true);
            set_reactive_interval(fReactivity);

            clear_buffers();
        }

        void phase_detector::update_settings()
        {
            lsp_debug("update settings sample_rate = %ld", get_sample_rate());

            bool clear          = false;
            bool old_bypass     = bBypass;

            // Read parameters
            bool bypass         = pBypass->value() >= 0.5f;
            bool reset          = pReset->value() >= 0.5f;
            fSelector           = pSelector->value();

            lsp_trace("bypass = %s, reset = %s, selector=%.3f", bypass ? "true" : "false", reset ? "true" : "false", fSelector);
            bBypass             = bypass || reset;
            if ((old_bypass != bBypass) && (bBypass))
                clear               = true;

            if (set_time_interval(pTime->value(), false))
                clear = true;
            set_reactive_interval(pReactivity->value());

            if (clear)
                clear_buffers();
        }

        void phase_detector::process(size_t samples)
        {
            // Store pointers to buffers
            float *in_a         = vIn[0]->buffer<float>();
            float *in_b         = vIn[1]->buffer<float>();
            float *out_a        = vOut[0]->buffer<float>();
            float *out_b        = vOut[1]->buffer<float>();
            plug::mesh_t *mesh  = pFunction->buffer<plug::mesh_t>();

            lsp_assert(in_a != NULL);
            lsp_assert(in_b != NULL);
            lsp_assert(out_a != NULL);
            lsp_assert(out_b != NULL);

            // Bypass the original signal
            dsp::copy(out_a, in_a, samples);
            dsp::copy(out_b, in_b, samples);

            if (bBypass)
            {
                for (size_t i=0; i<MK_COUNT; ++i)
                {
                    meters_t *vm = &vMeters[i];

                    vm->pTime       -> set_value(0.0f);
                    vm->pSamples    -> set_value(0.0f);
                    vm->pDistance   -> set_value(0.0f);
                    vm->pValue      -> set_value(0.0f);
                }

                if ((mesh != NULL) && (mesh->isEmpty()))
                    mesh->data(2, 0);       // Set mesh to empty data

                // Always query drawing
                pWrapper->query_display_draw();
                return;
            }

            // Make calculations
            while (samples > 0)
            {
                ssize_t filled   = fill_gap(in_a, in_b, samples);
                samples -= filled;

                // Subtract values
                while (nGapOffset < nGapSize)
                {
                    // Make assertions
                    lsp_assert((nGapOffset + nFuncSize) <= (nMaxVectorSize * 4));
                    lsp_assert(nGapOffset <= (nMaxVectorSize * 3));
                    lsp_assert((nGapOffset + nVectorSize + nFuncSize) < (nMaxVectorSize * 4));
                    lsp_assert((nGapOffset + nVectorSize) <= (nMaxVectorSize * 3));

                    // Update function peak values
                    // vFunction[i] = vFunction[i] - vB.pData[i + nGapOffset] * vA.pData[nGapOffset] +
                    //                + vB.pData[i + nGapOffset + nVectorSize] * vA.pData[nGapOffset + nVectorSize]
                    dsp::mix_add2(vFunction,
                            &vB.pData[nGapOffset], &vB.pData[nGapOffset + nVectorSize],
                            -vA.pData[nGapOffset], vA.pData[nGapOffset + nVectorSize],
                            nFuncSize);


                    // Accumulate peak function value
                    // vAccumulated[i] = vAccumulated[i] * (1.0f - fTau) + vFunction * fTau
                    dsp::mix2(vAccumulated, vFunction, 1.0f - fTau, fTau, nFuncSize);

                    // Increment gap offset: move to next sample
                    nGapOffset++;
                }
            }

            // Now analyze average function in the time
            size_t best     = nVectorSize, worst = nVectorSize;
            ssize_t sel     = nFuncSize * (1.0 - (fSelector + meta::phase_detector_metadata::SELECTOR_MAX) /
                              (meta::phase_detector_metadata::SELECTOR_MAX - meta::phase_detector_metadata::SELECTOR_MIN));
            if (sel >= ssize_t(nFuncSize))
                sel             = nFuncSize - 1;
            else if (sel < 0)
                sel             = 0;

            dsp::normalize(vNormalized, vAccumulated, nFuncSize);
            dsp::minmax_index(vNormalized, nFuncSize, &worst, &best);

            // Output values
            nBest               = ssize_t(nVectorSize - best);
            nSelected           = ssize_t(nVectorSize - sel);
            nWorst              = ssize_t(nVectorSize - worst);

            vMeters[MK_BEST].pTime      -> set_value(dspu::samples_to_millis(fSampleRate, nBest));
            vMeters[MK_BEST].pSamples   -> set_value(nBest);
            vMeters[MK_BEST].pDistance  -> set_value(dspu::samples_to_centimeters(fSampleRate, LSP_DSP_UNITS_SOUND_SPEED_M_S, nBest));
            vMeters[MK_BEST].pValue     -> set_value(vNormalized[best]);

            vMeters[MK_SEL].pTime       -> set_value(dspu::samples_to_millis(fSampleRate, nSelected));
            vMeters[MK_SEL].pSamples    -> set_value(nSelected);
            vMeters[MK_SEL].pDistance   -> set_value(dspu::samples_to_centimeters(fSampleRate, LSP_DSP_UNITS_SOUND_SPEED_M_S, nSelected));
            vMeters[MK_SEL].pValue      -> set_value(vNormalized[sel]);

            vMeters[MK_WORST].pTime     -> set_value(dspu::samples_to_millis(fSampleRate, nWorst));
            vMeters[MK_WORST].pSamples  -> set_value(nWorst);
            vMeters[MK_WORST].pDistance -> set_value(dspu::samples_to_centimeters(fSampleRate, LSP_DSP_UNITS_SOUND_SPEED_M_S, nWorst));
            vMeters[MK_WORST].pValue    -> set_value(vNormalized[worst]);

            // Output mesh if specified
            if ((mesh != NULL) && (mesh->isEmpty()))
            {
                float *x    = mesh->pvData[0];
                float *y    = mesh->pvData[1];
                float di    = (nFuncSize - 1.0) / meta::phase_detector_metadata::MESH_POINTS;
                float dx    = dspu::samples_to_millis(fSampleRate, di);
                ssize_t ci  = ssize_t(meta::phase_detector_metadata::MESH_POINTS >> 1);

                for (size_t i=0; i<meta::phase_detector_metadata::MESH_POINTS; ++i)
                {
                    *(x++)      = dx * (ci - ssize_t(i));
                    *(y++)      = vNormalized[size_t(i * di)];
                }

                mesh->data(2, meta::phase_detector_metadata::MESH_POINTS);
            }

            // Always query drawing
            if (pWrapper != NULL)
                pWrapper->query_display_draw();
        }

        bool phase_detector::inline_display(plug::ICanvas *cv, size_t width, size_t height)
        {
            // Check proportions
            if (height > (M_RGOLD_RATIO * width))
                height  = M_RGOLD_RATIO * width;

            // Init canvas
            if (!cv->init(width, height))
                return false;
            width   = cv->width();
            height  = cv->height();
            float cx    = width >> 1;
            float cy    = height >> 1;

            // Clear background
            cv->set_color_rgb((bBypass) ? CV_DISABLED : CV_BACKGROUND);
            cv->paint();

            // Draw axis
            cv->set_line_width(1.0);
            cv->set_color_rgb(CV_WHITE, 0.5f);
            cv->line(cx, 0, cx, height);
            cv->line(0, cy, width, cy);

            // Allocate buffer: t, f(t)
            pIDisplay           = core::IDBuffer::reuse(pIDisplay, 2, width);
            core::IDBuffer *b   = pIDisplay;
            if (b == NULL)
                return false;

            if (!bBypass)
            {
                float di    = (nFuncSize - 1.0) / width;
                float dy    = cy-2;

                for (size_t i=0; i<width; ++i)
                {
                    b->v[0][i]  = width - i;
                    b->v[1][i]  = cy - dy * vNormalized[size_t(i * di)];
                }

                // Set color and draw
                cv->set_color_rgb(CV_MESH);
                cv->set_line_width(2);
                cv->draw_lines(b->v[0], b->v[1], width);

                // Draw worst meter
                cv->set_line_width(1);
                cv->set_color_rgb(CV_RED);
                ssize_t point   = ssize_t(nVectorSize) - nWorst;
                float x         = width - point/di;
                float y         = cy - dy * vNormalized[point];
                cv->line(x, 0, x, height);
                cv->line(0, y, width, y);

                // Draw best meter
                cv->set_line_width(1);
                cv->set_color_rgb(CV_GREEN);
                point           = ssize_t(nVectorSize) - nBest;
                x               = width - point/di;
                y               = cy - dy * vNormalized[point];
                cv->line(x, 0, x, height);
                cv->line(0, y, width, y);
            }
            else
            {
                for (size_t i=0; i<width; ++i)
                    b->v[0][i]      = i;
                dsp::fill(b->v[1], cy, width);

                // Set color and draw
                cv->set_color_rgb(CV_SILVER);
                cv->set_line_width(2);
                cv->draw_lines(b->v[0], b->v[1], width);
            }

            return true;
        }

        void phase_detector::dump_buffer(dspu::IStateDumper *v, const buffer_t *buf, const char *label)
        {
            v->begin_object(label, v, sizeof(buffer_t));
            {
                v->write("pData", buf->pData);
                v->write("nSize", buf->nSize);
            }
            v->end_object();
        }

        void phase_detector::dump(dspu::IStateDumper *v) const
        {
            v->write("fTimeInterval", fTimeInterval);
            v->write("fReactivity", fReactivity);
            v->write("vFunction", vFunction);
            v->write("vAccumulated", vAccumulated);
            v->write("vNormalized", vNormalized);

            v->write("nMaxVectorSize", nMaxVectorSize);
            v->write("nVectorSize", nVectorSize);
            v->write("nFuncSize", nFuncSize);
            v->write("vNormalized", vNormalized);
            v->write("nMaxGapSize", nMaxGapSize);
            v->write("nGapOffset", nGapOffset);

            v->write("nBest", nBest);
            v->write("nSelected", nSelected);
            v->write("nWorst", nWorst);

            dump_buffer(v, &vA, "vA");
            dump_buffer(v, &vB, "vB");

            v->write("fTau", fTau);
            v->write("fSelector", fSelector);
            v->write("bBypass", bBypass);

            v->writev("vIn", vIn, 2);
            v->writev("vOut", vOut, 2);
            v->write("pBypass", pBypass);
            v->write("pReset", pReset);
            v->write("pSelector", pSelector);
            v->write("pReactivity", pReactivity);
            v->begin_array("vMeters", vMeters, MK_COUNT);
            {
                for (size_t i=0; i<MK_COUNT; ++i)
                {
                    const meters_t *vm = &vMeters[i];
                    v->begin_object(vm, sizeof(meters_t));
                    {
                        v->write("pTime", vm->pTime);
                        v->write("pSamples", vm->pSamples);
                        v->write("pDistance", vm->pDistance);
                        v->write("pValue", vm->pValue);
                    }
                    v->end_object();
                }
            }
            v->end_array();
            v->write("pFunction", pFunction);

            v->write_object("pIDisplay", pIDisplay);
        }

    }
}



