// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/Doc29/Doc29Noise.h"

namespace GRAPE {
    class Doc29NoiseManager : public Manager {
    public:
        Doc29NoiseManager(const Database& Db, Constraints& Blocks);

        auto& noises() { return c_Doc29Noises; }
        auto& operator()() { return c_Doc29Noises; }
        auto& operator()(const std::string& Doc29NsId) { return c_Doc29Noises(Doc29NsId); }
        [[nodiscard]] auto begin() const { return std::views::values(c_Doc29Noises).begin(); }
        [[nodiscard]] auto end() const { return std::views::values(c_Doc29Noises).end(); }

        std::pair<Doc29Noise&, bool> addNoise(const std::string& Name = "");
        Doc29Noise& addNoiseE(const std::string& Name = "");

        void eraseNoises();
        void eraseNoise(const Doc29Noise& Doc29Ns);

        bool updateKeyNoise(Doc29Noise& Doc29Ns, std::string Id);

        void updateNoise(const Doc29Noise& Doc29Ns) const;
        void updateMetric(const Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NsMetric) const;

        void loadFromFile();

    private:
        GrapeMap<std::string, Doc29Noise> c_Doc29Noises{};

    private:
        void updateNpdData(const Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NsMetric, const NpdData& Npd) const;
    };
}
