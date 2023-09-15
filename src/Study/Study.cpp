// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Study.h"

#include "GrapeSchema.embed"

namespace GRAPE {
    Study::Study() : Airports(m_Database, Blocks), Doc29Performances(m_Database, Blocks), Doc29Noises(m_Database, Blocks), SFIs(m_Database, Blocks), LTOEngines(m_Database, Blocks), Aircrafts(m_Database, Blocks, Doc29Performances, Doc29Noises, SFIs, LTOEngines, Operations), Operations(m_Database, Blocks, Aircrafts, Airports), Scenarios(m_Database, Blocks, Operations, Jobs) {}

    Study::~Study() {
        Jobs.shutdown();
        close();
    }

    bool Study::open(const std::filesystem::path& Path) {
        std::error_code sysErr;
        permissions(Path, std::filesystem::perms::all, sysErr);
        if (sysErr)
        {
            Log::study()->error("Opening study in '{}'. {}", Path.string(), sysErr.message());
            return false;
        }

        if (!m_Database.open(Path))
            return false;

        try
        {
            if (m_Database.applicationId() != GRAPE_ID)
                throw GrapeException("It is not a GRAPE study file.");

            if (const int version = m_Database.userVersion() != GRAPE_VERSION_NUMBER)
                throw GrapeException(std::format("Unsupported version {}.", version));
        }
        catch (const std::exception& err)
        {
            Log::study()->error("Opening study in '{}'. {}", Path.string(), err.what());
            m_Database.close();
            return false;
        }

        loadFile();
        Log::study()->info("Opened study '{}' in '{}'.", name(), m_Database.path().parent_path().string());
        Operations.Tracks4dLoader.Db = m_Database;
        return true;
    }

    bool Study::create(const std::filesystem::path& Path) {
        std::error_code sysErr;
        if (std::filesystem::exists(Path))
            Log::study()->warn("Creating new study at '{}'. The study already exists and will be overwriten.", Path.string());

        if (!m_Database.create(Path, g_GrapeSchema, sizeof g_GrapeSchema))
            return false;

        Log::study()->info("Created study '{}' in '{}'.", name(), m_Database.path().parent_path().string());
        Operations.Tracks4dLoader.Db = m_Database;
        return true;
    }

    void Study::close() { m_Database.close(); }

    void Study::loadFile() {
        GRAPE_ASSERT(valid(), "No file set for this study");
        Airports.loadFromFile();
        Doc29Performances.loadFromFile();
        Doc29Noises.loadFromFile();
        SFIs.loadFromFile();
        LTOEngines.loadFromFile();
        Aircrafts.loadFromFile();
        Operations.loadFromFile();
        Scenarios.loadFromFile();
    }
}
