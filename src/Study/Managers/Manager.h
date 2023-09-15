// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Constraints.h"

#include "Database/Database.h"

namespace GRAPE {
    struct Manager {
        explicit Manager(const Database& Db, Constraints& Blocks) : m_Db(Db), m_Blocks(Blocks) {}

        [[nodiscard]] auto& constraints() const { return m_Blocks; }
        virtual ~Manager() = default;

    protected:
        const Database& m_Db;
        Constraints& m_Blocks;
    };
}
