// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "AtmosphericAbsorption.h"
#include "Base/BaseModels.h"
#include "ReceptorSets.h"

namespace GRAPE {
    /**
    * @brief Defines the parameters needed by a noise run.
    */
    struct NoiseSpecification {
        std::unique_ptr<ReceptorSet> ReceptSet = std::make_unique<ReceptorPoints>();

        NoiseModel NoiseMdl = NoiseModel::Doc29;
        AtmosphericAbsorption::Type AtmAbsorptionType = AtmosphericAbsorption::Type::SaeArp5534;
        bool SaveSingleMetrics = false;
    };
}
