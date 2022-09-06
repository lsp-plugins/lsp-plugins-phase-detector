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

#ifndef PRIVATE_META_PHASE_DETECTOR_H_
#define PRIVATE_META_PHASE_DETECTOR_H_

#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/const.h>

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Phase detector metadata
        struct phase_detector_metadata
        {
            static constexpr float DETECT_TIME_MIN          =   1.0f;
            static constexpr float DETECT_TIME_MAX          =   50.0f;
            static constexpr float DETECT_TIME_DFL          =   10.0f;
            static constexpr float DETECT_TIME_STEP         =   0.0025f;
            static constexpr float DETECT_TIME_RANGE_MAX    =   100.0f;
            static constexpr float DETECT_TIME_RANGE_MIN    =   - 100.0f;

            static constexpr size_t MESH_POINTS             =   256;

            static constexpr float REACT_TIME_MIN           =   0.000;
            static constexpr float REACT_TIME_MAX           =  10.000;
            static constexpr float REACT_TIME_DFL           =   1.000;
            static constexpr float REACT_TIME_STEP          =   0.0025;

            static constexpr float SELECTOR_MIN             =   -100.0f;
            static constexpr float SELECTOR_MAX             =   100.0f;
            static constexpr float SELECTOR_DFL             =   0.0f;
            static constexpr float SELECTOR_STEP            =   0.1f;

            static constexpr float SAMPLES_MIN              =   - 50.0f /* DETECT_TIME_MAX [ms] */ * 0.001 /* [s/ms] */ * MAX_SAMPLE_RATE /* [ samples / s ] */;
            static constexpr float SAMPLES_MAX              =   + 50.0f /* DETECT_TIME_MAX [ms] */ * 0.001 /* [s/ms] */ * MAX_SAMPLE_RATE /* [ samples / s ] */;
            static constexpr float DISTANCE_MIN             =   - 50.0f /* DETECT_TIME_MAX [ms] */ * 0.001 /* [s/ms] */ * MAX_SOUND_SPEED /* [ m / s] */ * 100 /* c / m */;
            static constexpr float DISTANCE_MAX             =   + 50.0f /* DETECT_TIME_MAX [ms] */ * 0.001 /* [s/ms] */ * MAX_SOUND_SPEED /* [ m / s] */ * 100 /* c / m */;
            static constexpr float TIME_MIN                 =   - 50.0f /* DETECT_TIME_MAX [ms] */;
            static constexpr float TIME_MAX                 =   + 50.0f /* DETECT_TIME_MAX [ms] */;
            static constexpr float VALUE_MIN                =   -1.0f;
            static constexpr float VALUE_MAX                =   +1.0f;
        };

        extern const plugin_t phase_detector;
    }
}

#endif /* PRIVATE_META_PHASE_DETECTOR_H_ */
