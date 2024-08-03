/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/phase_detector.h>

#define LSP_PLUGINS_PHASE_DETECTOR_VERSION_MAJOR         1
#define LSP_PLUGINS_PHASE_DETECTOR_VERSION_MINOR         0
#define LSP_PLUGINS_PHASE_DETECTOR_VERSION_MICRO         21

#define LSP_PLUGINS_PHASE_DETECTOR_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_PHASE_DETECTOR_VERSION_MAJOR, \
        LSP_PLUGINS_PHASE_DETECTOR_VERSION_MINOR, \
        LSP_PLUGINS_PHASE_DETECTOR_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Phase detector
        static const port_t phase_detector_ports[] =
        {
            // Input audio ports
            AUDIO_INPUT_A,
            AUDIO_INPUT_B,

            // Output audio ports
            AUDIO_OUTPUT_A,
            AUDIO_OUTPUT_B,

            // Input controls
            BYPASS,
            TRIGGER("reset", "Reset"),
            LOG_CONTROL("time", "Time", U_MSEC, phase_detector_metadata::DETECT_TIME),
            LOG_CONTROL("react", "Reactivity", U_SEC, phase_detector_metadata::REACT_TIME),
            CONTROL("sel", "Selector", U_PERCENT, phase_detector_metadata::SELECTOR),

            // Output controls
            METERZ("b_t", "Best time", U_MSEC, phase_detector_metadata::TIME),
            METERZ("b_s", "Best samples", U_SAMPLES, phase_detector_metadata::SAMPLES),
            METERZ("b_d", "Best distance", U_CM, phase_detector_metadata::DISTANCE),
            METERZ("b_v", "Best value", U_NONE, phase_detector_metadata::VALUE),

            METERZ("s_t", "Selected time", U_MSEC, phase_detector_metadata::TIME),
            METERZ("s_s", "Selected samples", U_SAMPLES, phase_detector_metadata::SAMPLES),
            METERZ("s_d", "Selected distance", U_CM, phase_detector_metadata::DISTANCE),
            METERZ("s_v", "Selected value", U_NONE, phase_detector_metadata::VALUE),

            METERZ("w_t", "Worst time", U_MSEC, phase_detector_metadata::TIME),
            METERZ("w_s", "Worst samples", U_SAMPLES, phase_detector_metadata::SAMPLES),
            METERZ("w_d", "Worst distance", U_CM, phase_detector_metadata::DISTANCE),
            METERZ("w_v", "Worst value", U_NONE, phase_detector_metadata::VALUE),

            MESH("f", "Function", 2, phase_detector_metadata::MESH_POINTS),

            PORTS_END
        };

        static const int plugin_classes[]           = { C_ANALYSER, -1 };
        static const int clap_features[]            = { CF_ANALYZER, CF_UTILITY, -1 };

        const meta::bundle_t phase_detector_bundle =
        {
            "phase_detector",
            "Phase Detector",
            B_ANALYZERS,
            "j-rNb409GYg",
            "This plugin allows one to detect phase between two sources. For example, for\ntwo or more microphones set at the different positions and distances from\nthe sound source. The internal algorithm is based on correlation function\ncalculation between two sources. Be aware: because there are many correlation\nfunctions for different phases calculated at one time, the entire analyzing\nprocess can take a lot of CPU resources. You can also reduce CPU utilization\nby lowering the maximum analysis time. The plugin bypasses input signal without\nany modifications, so it can be placed everywhere it's needed."
        };

        MONO_PORT_GROUP_PORT(in_a, "in_a");
        MONO_PORT_GROUP_PORT(in_b, "in_b");
        MONO_PORT_GROUP_PORT(out_a, "out_a");
        MONO_PORT_GROUP_PORT(out_b, "out_b");

        const port_group_t phase_detector_port_groups[] =
        {
            { "in_a",           "Input A",       GRP_MONO,       PGF_IN | PGF_MAIN,         in_a_ports          },
            { "in_b",           "Input B",       GRP_MONO,       PGF_IN,                    in_b_ports          },
            { "out_a",          "Output A",      GRP_MONO,       PGF_OUT | PGF_MAIN,        out_a_ports         },
            { "out_b",          "Output B",      GRP_MONO,       PGF_OUT,                   out_b_ports         },
            PORT_GROUPS_END
        };

        const plugin_t phase_detector =
        {
            "Phasendetektor",
            "Phase Detector",
            "Phase Detector",
            "PD1",
            &developers::v_sadovnikov,
            "phase_detector",
            {
                LSP_LV2_URI("phase_detector"),
                LSP_LV2UI_URI("phase_detector"),
                "jffz",
                LSP_VST3_UID("pd1     jffz"),
                LSP_VST3UI_UID("pd1     jffz"),
                LSP_LADSPA_PHASE_DETECTOR_BASE + 0,
                LSP_LADSPA_URI("phase_detector"),
                LSP_CLAP_URI("phase_detector"),
                LSP_GST_UID("phase_detector"),
            },
            LSP_PLUGINS_PHASE_DETECTOR_VERSION,
            plugin_classes,
            clap_features,
            E_DUMP_STATE | E_INLINE_DISPLAY,
            phase_detector_ports,
            "util/phase_detector.xml",
            NULL,
            phase_detector_port_groups,
            &phase_detector_bundle
        };
    } /* namespace meta */
} /* namespace lsp */


