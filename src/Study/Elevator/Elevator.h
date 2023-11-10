#pragma once

#include "Database/Database.h"

namespace GRAPE::Schema {
    class Elevator {
    public:
        Elevator();
        void elevate(const Database& Db, int CurrentVersion) const;
    private:
        typedef std::vector<std::string_view> ElevatorQueries;
        std::map<int, ElevatorQueries> m_ElevatorQueries;
    };
}
