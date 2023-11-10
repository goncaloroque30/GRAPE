// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseCalculator.h"

namespace GRAPE {
    NoiseCalculator::NoiseCalculator(const PerformanceSpecification& Spec, const NoiseSpecification& NsSpec, const ReceptorOutput& ReceptOutput) : m_PerfSpec(Spec), m_NsSpec(NsSpec), m_Cs(*m_PerfSpec.CoordSys) {
        m_ReceptorOutput.reserve(ReceptOutput.size());

        std::size_t i = 0;
        for (const auto& recept : ReceptOutput)
            m_ReceptorOutput.emplace_back(recept, i++);
    }

    const Atmosphere& NoiseCalculator::atmosphere(const Operation& Op) const {
        return m_PerfSpec.Atmospheres.atmosphere(Op.Time);
    }

    AtmosphericAbsorption NoiseCalculator::atmosphericAbsorption(const Operation& Op)const {
        const Atmosphere& atm = atmosphere(Op);

        switch (m_NsSpec.AtmAbsorptionType)
        {
        case AtmosphericAbsorption::Type::None: return AtmosphericAbsorption(); break;
        case AtmosphericAbsorption::Type::SaeArp866: return AtmosphericAbsorption(atm.seaLevelTemperature(), atm.relativeHumidity());
        case AtmosphericAbsorption::Type::SaeArp5534: return AtmosphericAbsorption(atm.seaLevelTemperature(), atm.seaLevelPressure(), atm.relativeHumidity()); break;
        default: GRAPE_ASSERT(false); break;
        }

        return AtmosphericAbsorption();
    }
}
