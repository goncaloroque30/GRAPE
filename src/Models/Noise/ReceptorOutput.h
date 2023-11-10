// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Noise.h"

namespace GRAPE {
    /**
    * @brief Stores a vector of receptors obtained from the output of a ReceptorSet.
    */
    class ReceptorOutput {
    public:
        // Constructors and Destructor
        explicit ReceptorOutput(std::size_t Size = 0);
        ReceptorOutput(const ReceptorOutput&) = delete;
        ReceptorOutput(ReceptorOutput&&) = default;
        ReceptorOutput& operator=(const ReceptorOutput&) = delete;
        ReceptorOutput& operator=(ReceptorOutput&&) = default;
        ~ReceptorOutput() = default;

        // Access Data
        [[nodiscard]] const Receptor& receptor(std::size_t Index) const { return m_Receptors.at(Index); }
        const Receptor& operator()(std::size_t Index) const { return m_Receptors.at(Index); }
        [[nodiscard]] const auto& receptors() const { return m_Receptors; }
        [[nodiscard]] std::size_t size() const { return m_Receptors.size(); }
        [[nodiscard]] bool empty() const { return m_Receptors.empty(); }
        [[nodiscard]] auto begin() const { return m_Receptors.begin(); }
        [[nodiscard]] auto end() const { return m_Receptors.end(); }

        // Change Data
        void addReceptor(const Receptor& Recept);
        void addReceptor(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl);
    private:
        std::vector<Receptor> m_Receptors;
    };
}
