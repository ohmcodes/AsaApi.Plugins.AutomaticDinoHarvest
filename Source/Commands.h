

void AddOrRemoveCommands(bool addCmd = true)
{

	// Owner

	const FString AddHarvester = AutomaticDinoHarvest::config["Commands"]["AddHarvesterCMD"].get<std::string>().c_str();
	if (!AddHarvester.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(AddHarvester, &AddHarvesterCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(AddHarvester);
		}
	}

	const FString RemoveHarvester = AutomaticDinoHarvest::config["Commands"]["RemoveHarvesterCMD"].get<std::string>().c_str();
	if (!RemoveHarvester.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(RemoveHarvester, &RemoveHarvesterCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(RemoveHarvester);
		}
	}

	const FString StopAllHarvester = AutomaticDinoHarvest::config["Commands"]["StopAllHarvesterCMD"].get<std::string>().c_str();
	if (!StopAllHarvester.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(StopAllHarvester, &StopAllHarvesterCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(StopAllHarvester);
		}
	}

	const FString GetAllHarvestingDinos = AutomaticDinoHarvest::config["Commands"]["GetAllHarvestingDinosCMD"].get<std::string>().c_str();
	if (!GetAllHarvestingDinos.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(GetAllHarvestingDinos, &GetAllHarvestingDinosCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(GetAllHarvestingDinos);
		}
	}

	
	// Admin

	const FString AdminSAH = AutomaticDinoHarvest::config["Commands"]["AdminSAHCMD"].get<std::string>().c_str();
	if (!AdminSAH.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(AdminSAH, &AdminStopAllHarvestCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(AdminSAH);
		}
	}

	const FString AdminGAHD = AutomaticDinoHarvest::config["Commands"]["AdminGAHDCMD"].get<std::string>().c_str();
	if (!AdminGAHD.IsEmpty())
	{
		if (addCmd)
		{
			AsaApi::GetCommands().AddChatCommand(AdminGAHD, &AdminGetAllHarvestingDinosCallBack);
		}
		else
		{
			AsaApi::GetCommands().RemoveChatCommand(AdminGAHD);
		}
	}
}