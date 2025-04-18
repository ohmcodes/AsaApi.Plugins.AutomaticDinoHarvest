
#include <fstream>



bool Points(FString eos_id, int cost, bool check_points = false)
{
	if (cost == -1)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("Cost is -1");
		}
		return false;
	}

	if (cost == 0)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("Cost is 0");
		}

		return true;
	}

	nlohmann::json config = AutomaticDinoHarvest::config["PointsDBSettings"];

	if (config.value("Enabled", false) == false)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("Points system is disabled");
		}

		return true;
	}

	std::string tablename = config.value("TableName", "ArkShopPlayers");
	std::string unique_id = config.value("UniqueIDField", "EosId");
	std::string points_field = config.value("PointsField", "Points");
	std::string totalspent_field = config.value("TotalSpentField", "TotalSpent");

	if (tablename.empty() || unique_id.empty() || points_field.empty())
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("DB Fields are empty");
		}
		return false;
	}

	std::string escaped_eos_id = AutomaticDinoHarvest::pointsDB->escapeString(eos_id.ToString());

	std::string query = fmt::format("SELECT * FROM {} WHERE {}='{}'", tablename, unique_id, escaped_eos_id);

	std::vector<std::map<std::string, std::string>> results;

	if (!AutomaticDinoHarvest::pointsDB->read(query, results))
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("Error reading points db");
		}

		return false;
	}

	if (results.size() <= 0)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("No record found");
		}
		return false;
	}

	int points = std::atoi(results[0].at(points_field).c_str());

	if (check_points)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->warn("Player got {} points", points);
		}

		if (points >= cost) return true;
	}
	else
	{
		int amount = points - cost;

		std::vector<std::pair<std::string, std::string>> data;

		data.push_back({ points_field, std::to_string(amount) });

		if (totalspent_field != "")
		{
			int total_spent = std::atoi(results[0].at(totalspent_field).c_str());
			std::string total_ts = std::to_string(total_spent + cost);

			data.push_back({totalspent_field, total_ts});
		}

		std::string condition = fmt::format("{}='{}'", unique_id, escaped_eos_id);

		if (AutomaticDinoHarvest::pointsDB->update(tablename, data, condition))
		{
			if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
			{
				Log::GetLog()->info("{} Points DB updated", amount);
			}

			return true;
		}
	}

	return false;
}

nlohmann::json GetCommandString(const std::string permission, const std::string command)
{
	if (permission.empty()) return {};
	if (command.empty()) return {};

	nlohmann::json config_obj = AutomaticDinoHarvest::config["PermissionGroups"];
	nlohmann::json perm_obj = config_obj[permission];
	nlohmann::json command_obj = perm_obj["Commands"];
	nlohmann::json setting_obj = command_obj[command];

	return setting_obj;
}

TArray<FString> GetPlayerPermissions(FString eos_id)
{
	TArray<FString> PlayerPerms = { "Default" };

	std::string escaped_eos_id = AutomaticDinoHarvest::permissionsDB->escapeString(eos_id.ToString());

	std::string tablename = AutomaticDinoHarvest::config["PermissionsDBSettings"].value("TableName", "Players");

	std::string condition = AutomaticDinoHarvest::config["PermissionsDBSettings"].value("UniqueIDField", "EOS_Id");

	std::string query = fmt::format("SELECT * FROM {} WHERE {}='{}';", tablename, condition, escaped_eos_id);

	std::vector<std::map<std::string, std::string>> results;
	if (!AutomaticDinoHarvest::permissionsDB->read(query, results))
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Permissions", false) == true)
		{
			Log::GetLog()->warn("Error reading permissions DB");
		}

		return PlayerPerms;
	}

	if (results.size() <= 0) return PlayerPerms;

	std::string permsfield = AutomaticDinoHarvest::config["PermissionsDBSettings"].value("PermissionGroupField","PermissionGroups");

	FString playerperms = FString(results[0].at(permsfield));

	if (AutomaticDinoHarvest::config["Debug"].value("Permissions", false) == true)
	{
		Log::GetLog()->info("current player perms {}", playerperms.ToString());
	}

	playerperms.ParseIntoArray(PlayerPerms, L",", true);

	return PlayerPerms;
}

FString GetPriorPermByEOSID(FString eos_id)
{
	TArray<FString> player_groups = GetPlayerPermissions(eos_id);

	const nlohmann::json permGroups = AutomaticDinoHarvest::config["PermissionGroups"];

	std::string defaultGroup = "Default";
	int minPriority = INT_MAX;
	nlohmann::json result;
	FString selectedPerm = "Default";

	for (const FString& param : player_groups)
	{
		if (permGroups.contains(param.ToString()))
		{
			int priority = static_cast<int>(permGroups[param.ToString()]["Priority"]);
			if (priority < minPriority)
			{
				minPriority = priority;
				result = permGroups[param.ToString()];
				selectedPerm = param;
			}
		}
	}

	if (result.is_null() && permGroups.contains(defaultGroup))
	{
		if(!permGroups[defaultGroup].is_null())
			result = permGroups[defaultGroup];

		result = {};
	}

	if (AutomaticDinoHarvest::config["Debug"].value("Permissions", false) == true)
	{
		Log::GetLog()->info("Selected Permission {}", selectedPerm.ToString());
	}

	return selectedPerm;
}

bool AddPlayer(FString eosID, int playerID, FString playerName)
{
	std::vector<std::pair<std::string, std::string>> data = {
		{"EosId", eosID.ToString()},
		{"PlayerId", std::to_string(playerID)},
		{"PlayerName", playerName.ToString()}
	};

	return AutomaticDinoHarvest::pluginTemplateDB->create(AutomaticDinoHarvest::config["PluginDBSettings"]["TableName"].get<std::string>(), data);
}

bool ReadPlayer(FString eosID)
{
	std::string escaped_id = AutomaticDinoHarvest::pluginTemplateDB->escapeString(eosID.ToString());

	std::string query = fmt::format("SELECT * FROM {} WHERE EosId='{}'", AutomaticDinoHarvest::config["PluginDBSettings"]["TableName"].get<std::string>(), escaped_id);

	std::vector<std::map<std::string, std::string>> results;
	AutomaticDinoHarvest::pluginTemplateDB->read(query, results);

	return results.size() <= 0 ? false : true;
}

bool UpdatePlayer(FString eosID, FString playerName)
{
	std::string unique_id = "EosId";

	std::string escaped_id = AutomaticDinoHarvest::pluginTemplateDB->escapeString(eosID.ToString());

	std::vector<std::pair<std::string, std::string>> data = {
		{"PlayerName", playerName.ToString() + "123"}
	};

	std::string condition = fmt::format("{}='{}'", unique_id, escaped_id);

	return AutomaticDinoHarvest::pluginTemplateDB->update(AutomaticDinoHarvest::config["PluginDBSettings"]["TableName"].get<std::string>(), data, condition);
}

bool DeletePlayer(FString eosID)
{
	std::string escaped_id = AutomaticDinoHarvest::pluginTemplateDB->escapeString(eosID.ToString());

	std::string condition = fmt::format("EosId='{}'", escaped_id);

	return AutomaticDinoHarvest::pluginTemplateDB->deleteRow(AutomaticDinoHarvest::config["PluginDBSettings"]["TableName"].get<std::string>(), condition);
}

void ReadConfig()
{
	try
	{
		const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/" + PROJECT_NAME + "/config.json";
		std::ifstream file{config_path};
		if (!file.is_open())
		{
			throw std::runtime_error("Can't open config file.");
		}
		file >> AutomaticDinoHarvest::config;

		Log::GetLog()->info("{} config file loaded.", PROJECT_NAME);

		AutomaticDinoHarvest::isDebug = AutomaticDinoHarvest::config["General"]["Debug"].get<bool>();

		Log::GetLog()->warn("Debug {}", AutomaticDinoHarvest::isDebug);

	}
	catch(const std::exception& error)
	{
		Log::GetLog()->error("Config load failed. ERROR: {}", error.what());
		throw;
	}
}

void LoadDatabase()
{
	//Log::GetLog()->warn("LoadDatabase");
	//AutomaticDinoHarvest::pluginTemplateDB = DatabaseFactory::createConnector(AutomaticDinoHarvest::config["PluginDBSettings"]);

	nlohmann::ordered_json tableDefinition = {};
	if (AutomaticDinoHarvest::config["PluginDBSettings"].value("UseMySQL", true) == true)
	{
		tableDefinition = {
			{"Id", "INT NOT NULL AUTO_INCREMENT"},
			{"EosId", "VARCHAR(50) NOT NULL"},
			{"PlayerId", "VARCHAR(50) NOT NULL"},
			{"PlayerName", "VARCHAR(50) NOT NULL"},
			{"CreateAt", "DATETIME DEFAULT CURRENT_TIMESTAMP"},
			{"PRIMARY", "KEY(Id)"},
			{"UNIQUE", "INDEX EosId_UNIQUE (EosId ASC)"}
		};
	}
	else
	{
		tableDefinition = {
			{"Id","INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT"},
			{"EosId","TEXT NOT NULL UNIQUE"},
			{"PlayerId","TEXT"},
			{"PlayerName","TEXT"},
			{"CreateAt","TIMESTAMP DEFAULT CURRENT_TIMESTAMP"}
		};
	}

	//AutomaticDinoHarvest::pluginTemplateDB->createTableIfNotExist(AutomaticDinoHarvest::config["PluginDBSettings"].value("TableName", ""), tableDefinition);


	// PermissionsDB
	if (AutomaticDinoHarvest::config["PermissionsDBSettings"].value("Enabled", true) == true)
	{
		AutomaticDinoHarvest::permissionsDB = DatabaseFactory::createConnector(AutomaticDinoHarvest::config["PermissionsDBSettings"]);
	}

	// PointsDB (ArkShop)
	if (AutomaticDinoHarvest::config["PointsDBSettings"].value("Enabled", true) == true)
	{
		AutomaticDinoHarvest::pointsDB = DatabaseFactory::createConnector(AutomaticDinoHarvest::config["PointsDBSettings"]);
	}
	
}