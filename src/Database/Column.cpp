// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Column.h"

#include "Statement.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <sqlite3.h>
#pragma warning ( pop )

namespace GRAPE {
    Column::Column(const Statement& Stmt, int Index) : m_Stmt(Stmt.m_Stmt), m_Index(Index) { GRAPE_ASSERT(m_Stmt); }

    int Column::getInt() const noexcept { return sqlite3_column_int(m_Stmt, m_Index); }

    double Column::getDouble() const noexcept { return sqlite3_column_double(m_Stmt, m_Index); }

    std::string Column::getString() const noexcept {
        const auto strPtr = reinterpret_cast<const char*>(sqlite3_column_text(m_Stmt, m_Index));
        return strPtr ? strPtr : "";
    }
}
