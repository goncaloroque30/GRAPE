// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Aircraft/Doc29/Doc29Performance.h"
#include "Aircraft/Doc29/Doc29Noise.h"

namespace GRAPE {
    class Doc29Panel : public Panel {
    public:
        // Constructors & Destructor
        Doc29Panel() : Panel("Doc29") {}

        // Change Data
        void select(Doc29Performance& Doc29Acft);
        void select(Doc29ProfileArrival& Doc29Prof);
        void select(Doc29ProfileDeparture& Doc29Prof);
        void select(Doc29Noise& Doc29Ns);

        void deselect(Doc29Performance& Doc29Acft);
        void deselect(Doc29ProfileArrival& Doc29Prof);
        void deselect(Doc29ProfileDeparture& Doc29Prof);
        void deselect(Doc29Noise& Doc29Ns);

        void clearAircraftSelection();
        void clearNoiseSelection();
        void clearSelection();

        // Status Checks
        [[nodiscard]] bool isSelected(Doc29Performance& Doc29Acft) const;
        [[nodiscard]] bool isSelected(Doc29ProfileArrival& Doc29Prof) const;
        [[nodiscard]] bool isSelected(Doc29ProfileDeparture& Doc29Prof) const;
        [[nodiscard]] bool isSelected(Doc29Noise& Doc29Ns) const;

        // Panel Interface
        void reset() override;
        void onNoiseRunStart() override;
        void imGuiDraw() override;

    private:
        // Selection
        std::vector<Doc29Performance*> m_SelectedDoc29Aircraft;
        std::vector<Doc29ProfileArrival*> m_SelectedDoc29ProfileArrivals;
        std::vector<Doc29ProfileDeparture*> m_SelectedDoc29ProfileDepartures;
        std::vector<Doc29Noise*> m_SelectedDoc29Noises;

        NpdData* m_SelectedNpdData = nullptr;
        OperationType m_SelectedNpdOp = OperationType::Arrival;
        NoiseSingleMetric m_SelectedMetric = NoiseSingleMetric::Lamax;

        std::function<void()> m_Action;

    private:
        void drawDoc29AircraftNode(const std::string& Doc29AcftId, Doc29Performance& Doc29Acft);
        void drawDoc29NoiseNode(const std::string& Doc29NsId, Doc29Noise& Doc29Ns);

        void drawSelectedDoc29Aircraft();
        void drawSelectedDoc29AircraftThrust() const;
        void drawSelectedDoc29AircraftAerodynamicCoefficients();
        void drawSelectedDoc29Noise();
        [[nodiscard]] bool drawSelectedDoc29NoiseMetric() const;
    };
}
