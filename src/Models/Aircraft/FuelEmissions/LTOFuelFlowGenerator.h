// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "LTO.h"

#include "Base/BaseModels.h"

namespace GRAPE {
    /**
    * @brief Calculates fuel flow based on the LTO Phases
    */
    class LTOFuelFlowGenerator {
    public:
        explicit LTOFuelFlowGenerator(const LTOEngine& LTOEng);

        /**
        * @brief Gets the LTOPhase from the FlightPhase and returns the respective fuel flow.
        */
        [[nodiscard]] double fuelFlow(FlightPhase Phase) const;
    private:
        std::array<double, 4> m_CorrectedFuelFlows{};
    };
}
