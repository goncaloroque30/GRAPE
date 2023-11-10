// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseSingleEventOutput.h"

namespace GRAPE {
    NoiseSingleEventOutput::NoiseSingleEventOutput(std::size_t Size) {
        m_Values.reserve(Size);
    }

    void NoiseSingleEventOutput::fill(std::size_t Size, double Value) {
        m_Values.assign(Size, std::make_pair(Value, Value));
    }

    void NoiseSingleEventOutput::setValues(std::size_t Index, double Lamax, double Sel) {
        GRAPE_ASSERT(Index < size());

        m_Values.at(Index) = std::make_pair(Lamax, Sel);
    }

    void NoiseSingleEventOutput::addValues(double LaMax, double Sel) {
        m_Values.emplace_back(LaMax, Sel);
    }

    void NoiseSingleEventOutput::clear() {
        m_Values.clear();
        m_Values.shrink_to_fit();
    }
}
