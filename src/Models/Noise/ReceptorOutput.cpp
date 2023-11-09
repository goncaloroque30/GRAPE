// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "ReceptorOutput.h"

namespace GRAPE {
    ReceptorOutput::ReceptorOutput(std::size_t Size) {
        m_Receptors.reserve(Size);
    }

    void ReceptorOutput::addReceptor(const Receptor& Recept) {
        m_Receptors.push_back(Recept);
    }

    void ReceptorOutput::addReceptor(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl) {
        m_Receptors.emplace_back(Name, Longitude, Latitude, AltitudeMsl);
    }
}
