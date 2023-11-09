// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    struct EmissionValues {
        EmissionValues() = default;
        EmissionValues(double HCIn, double COIn, double NOxIn, double nvPMIn, double nvPMCountIn) : HC(HCIn), CO(COIn), NOx(NOxIn), nvPM(nvPMIn), nvPMNumber(nvPMCountIn) {}

        EmissionValues& operator+=(const EmissionValues& Vals) {
            HC += Vals.HC;
            CO += Vals.CO;
            NOx += Vals.NOx;
            nvPM += Vals.nvPM;
            nvPMNumber += Vals.nvPMNumber;
            return *this;
        }

        double HC = 0.0;
        double CO = 0.0;
        double NOx = 0.0;
        double nvPM = 0.0;
        double nvPMNumber = 0.0;
    };

    struct EmissionsSegmentOutput {
        std::size_t Index = 0;
        double Fuel = 0.0;
        EmissionValues Emissions;
    };

    /**
    * @brief Contains the fuel and emissions of each segment of an operation as well as the totals.
    */
    class EmissionsOperationOutput {
    public:
        /**
        * @brief Initializes with no segments and all totals set to 0.
        */
        EmissionsOperationOutput() = default;

        /**
        * @return The list of segment outputs.
        */
        [[nodiscard]] const auto& segmentOutput() const { return m_SegOutputs; }

        /**
        * @return The total fuel consumption of an operation.
        */
        [[nodiscard]] double totalFuel() const { return m_Fuel; }

        /**
        * @return EmissionValues containing the total emissions of an operation.
        */
        [[nodiscard]] const EmissionValues& totalEmissions() const { return m_EmissionValues; }

        /**
        * @brief Clears the segment output and sets the total values.
        */
        void setTotals(double Fuel, const EmissionValues& EmiVals) {
            clearSegmentOutput();
            m_Fuel = Fuel;
            m_EmissionValues = EmiVals;
        }

        /**
        * @brief Adds a segment fuel and emissions to the list and updates the totals.
        */
        void addSegmentOutput(const EmissionsSegmentOutput& SegOut) {
            m_SegOutputs.emplace_back(SegOut);
            m_Fuel += SegOut.Fuel;
            m_EmissionValues += SegOut.Emissions;
        }

        /**
        * @brief Clears the segment output list. The totals are left unchanged.
        */
        void clearSegmentOutput(bool Shrink = false) { m_SegOutputs.clear(); if (Shrink) m_SegOutputs.shrink_to_fit(); }

    private:
        std::vector<EmissionsSegmentOutput> m_SegOutputs{};

        double m_Fuel = 0.0;
        EmissionValues m_EmissionValues;
    };
}
