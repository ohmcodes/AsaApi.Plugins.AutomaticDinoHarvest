#pragma once

#include "json.hpp"

#include "Database/DatabaseFactory.h"

namespace AutomaticDinoHarvest
{
	inline nlohmann::json config;
	inline bool isDebug{ false };

	inline int counter = 0;

	inline std::unique_ptr<IDatabaseConnector> pluginTemplateDB;

	inline std::unique_ptr<IDatabaseConnector> permissionsDB;

	inline std::unique_ptr<IDatabaseConnector> pointsDB;

	inline TArray<HarvesterData> tribeHarvestingDinos;

	inline TArray<HarvesterData> personalHarvestingDinos;

}