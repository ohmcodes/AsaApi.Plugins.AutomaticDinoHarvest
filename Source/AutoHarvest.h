



AActor* GetTargetActor(AShooterPlayerController* pc)
{
	ACharacter* character = pc->CharacterField().Get();

	if (!character) return nullptr;

	APrimalCharacter* primalCharacter = static_cast<APrimalCharacter*>(character);

	if (!primalCharacter) return nullptr;

	AActor* actor = primalCharacter->GetAimedActor(ECollisionChannel::ECC_GameTraceChannel2, 0i64, 0.0, 0.0, 0i64, 0i64, false, false, false, 0i64);

	if (!actor) return nullptr;

	return actor;
}


TArray<HarvesterData>& GetHarvestingDino(AShooterPlayerController* pc)
{
	if (pc->IsInTribe())
	{
		return AutomaticDinoHarvest::tribeHarvestingDinos;
	}
	else
	{
		return AutomaticDinoHarvest::personalHarvestingDinos;
	}
}

bool CountPlayerHarvester(AShooterPlayerController* pc, nlohmann::json command)
{
	int MaxHarvester = pc->IsInTribe() ? command.value("MaxTribeHarvestingDino", 0) : command.value("MaxPersonalHarvestingDino", 0);
	int count = 0;

	TArray<HarvesterData> harvestingDinos = GetHarvestingDino(pc);

	for (const HarvesterData& harvester : harvestingDinos)
	{
		if (pc->IsInTribe() && harvester.TribeID == pc->TargetingTeamField())
		{
			count++;
		}
		else if (!pc->IsInTribe() && harvester.PlayerID == pc->LinkedPlayerIDField())
		{
			count++;
		}
	}

	if (MaxHarvester <= count)
	{
		return false;
	}

	return true;
}


bool CheckExcludedDino(APrimalDinoCharacter* dino, nlohmann::json command)
{
	FString dinoName;
	dino->GetDescriptiveName(&dinoName);

	std::vector<std::string> excludedDinos = AutomaticDinoHarvest::config["General"]["ExcludedDinos"].get<std::vector<std::string>>();

	for (std::string exc : excludedDinos)
	{
		if (dinoName.Contains(FString(exc.c_str())))
			return true;
	}

	return false;
}


int GetAttackIndex(FString* param, APrimalDinoCharacter* dino)
{
	TArray<FString> parsed;
	param->ParseIntoArray(parsed, L" ", false);

	int paramAttackIndex = 0;

	// eg: /ah 2 </command> <attack_index>
	if (parsed.IsValidIndex(1))
	{
		paramAttackIndex = FCString::Atoi(*parsed[1]);

		// TODO: clean up
		Log::GetLog()->info("paramAttackIndex {}", paramAttackIndex);
	}

	nlohmann::json listedDinos = AutomaticDinoHarvest::config["General"]["DinoAttackIndex"].get<nlohmann::json>();

	if (!listedDinos.is_null())
	{
		//Log::GetLog()->info("listedDinos {}", to_string(listedDinos));

		std::vector<int> attackIndexes;

		for (auto it = listedDinos.begin(); it != listedDinos.end(); ++it)
		{
			if (dino->DescriptiveNameField().Contains(FString(it.key().c_str())))
			{
				attackIndexes = (it.value()).get<std::vector<int>>();

				//Log::GetLog()->info("attackIndexes {}", attackIndexes.size());
				break;
			}
		}

		if (attackIndexes.size() > 0)
		{
			//Log::GetLog()->info("has attackIndexes {}", attackIndexes.size());

			bool isParamValid = false;
			for (int ai : attackIndexes)
			{
				// valid to config list
				if (paramAttackIndex == ai)
				{
					isParamValid = true;
					break;
				}
			}
			// not valid set default 0
			//Log::GetLog()->info("not valid {}", paramAttackIndex);
			paramAttackIndex = isParamValid ? paramAttackIndex : 0;
		}

	}

	
	// Do dino check
	for (int i = 0; i < dino->AttackInfosField().Num(); i++)
	{
		if (paramAttackIndex == i)
		{
			//Log::GetLog()->info("dino attackindex {} paramAttackIndex {}", i, paramAttackIndex);
			return paramAttackIndex;
		}
	}

	// default 0
	return 0;

}


HarvesterData* FindHarvester(AShooterPlayerController* pc, APrimalDinoCharacter* dino)
{
	unsigned int DinoID1 = dino->DinoID1Field();
	unsigned int DinoID2 = dino->DinoID2Field();

	TArray<HarvesterData> harvestingDino = GetHarvestingDino(pc);

	return harvestingDino.FindByPredicate([&](const HarvesterData& harvester)
		{
			return harvester.DinoID1 == DinoID1 && harvester.DinoID2 == DinoID2;
		}
	);
}

TArray<FString> GetResources(FString* param)
{
	TArray<FString> parsed;
	param->ParseIntoArray(parsed, L" ", false);

	TArray<FString> resources = { "any" };

	// eg: /ah 2 stone </command> <attack_index> <resources>
	// eg: /ah 2 stone,wood,metal </command> <attack_index> <resources>(separated by comma)
	if (!parsed.IsValidIndex(2)) return resources;

	resources.Empty();

	if (parsed[2].Contains(","))
	{
		parsed[2].ParseIntoArray(resources, L",", true);
	}
	else
	{
		resources.Add(parsed[2]);
	}

	return resources;
}

void AddHarvester(AShooterPlayerController* pc, APrimalDinoCharacter* dino, FString* param)
{
	int AttackIndex = GetAttackIndex(param, dino);
	TArray<FString> resources = GetResources(param);

	HarvesterData harvester;

	harvester.AttackIndex = AttackIndex;
	harvester.DinoID1 = dino->DinoID1Field();
	harvester.DinoID2 = dino->DinoID2Field();
	harvester.TribeID = pc->TargetingTeamField();
	harvester.PlayerID = pc->LinkedPlayerIDField();
	harvester.EosID = pc->GetEOSId();
	harvester.Resources = resources;
	
	TArray<HarvesterData>& harvesterData = GetHarvestingDino(pc);

	harvesterData.Add(harvester);
}


bool FindItemInList(FString item, TArray<FString> item_list)
{
	for (FString iList : item_list)
	{
		if (item.Contains(iList))
		{
			return true;
		}
	}

	return false;
}

void RemoveHarvesterByObject(HarvesterData hdino, TArray<HarvesterData> harvester_list)
{
	harvester_list.RemoveAll([hdino](const HarvesterData& harvester)
		{
			return harvester.DinoID1 == hdino.DinoID1 && harvester.DinoID2 == hdino.DinoID2;
		}
	);
}

bool RemoveHarvesterByDino(APrimalDinoCharacter* dino, TArray<HarvesterData>& harvester_list)
{
	int32 removedDino = harvester_list.RemoveAll([dino](const HarvesterData& harvester)
		{
			return harvester.DinoID1 == dino->DinoID1Field() && harvester.DinoID2 == dino->DinoID2Field();
		}
	);

	if (removedDino != INDEX_NONE) return true;

	return false;
}

void RemoveAllHarvestingDinosByTribeID(AShooterPlayerController* pc)
{
	TArray<HarvesterData>& harvester_list = GetHarvestingDino(pc);

	harvester_list.RemoveAll([&](const HarvesterData& harvester)
		{
			return harvester.TribeID == pc->TargetingTeamField();
		}
	);
}

// Owner
void AddHarvesterCallBack(AShooterPlayerController* pc, FString* param, int, int)
{
	Log::GetLog()->warn("Function: {}", __FUNCTION__);

	// TODO
	int current_dinos = 0;

	if (AutomaticDinoHarvest::config["Debug"].value("MaxGlobalHarvestingDino", 100) <= current_dinos)
	{
		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Orange, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("AddHarvesterMaxMSG", "The server reached maximum dino harvester.").c_str());
	}

	// permissions check
	FString perms = GetPriorPermByEOSID(pc->GetEOSId());
	nlohmann::json command = GetCommandString(perms.ToString(), "AddHarvesterCMD");

	if (command.is_null() || (!command.is_null() && command.value("Enabled", false) == false))
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Permissions", false) == true)
		{
			Log::GetLog()->info("{} No permissions. Command: {}", pc->GetCharacterName().ToString(), __FUNCTION__);
		}

		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Red, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("PermErrorMSG", "You don't have permission to use this command.").c_str());

		return;
	}

	// points checking
	if (Points(pc->GetEOSId(), command.value("Cost", 0), true) == false)
	{
		if (AutomaticDinoHarvest::config["Debug"].value("Points", false) == true)
		{
			Log::GetLog()->info("{} don't have points. Command: {}", pc->GetCharacterName().ToString(), __FUNCTION__);
		}

		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Red, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("PointsErrorMSG", "Not enough points.").c_str());

		return;
	}

	AActor* actor = GetTargetActor(pc);

	if (!actor) return;

	if (!actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) return;

	APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

	if (!dino) return;

	if (dino->bIsMounted().Get()) return;

	if (dino->IsDead()) return;

	int dinoTeam = dino->TargetingTeamField();
	int playerTeam = pc->TargetingTeamField();

	if (dinoTeam != playerTeam) return;

	if (CheckExcludedDino(dino, command))
	{
		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Orange, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("AddHarvesterExcludedMSG", "Dino is forbidden for autohavesting.").c_str());
		return;
	}

	if (!CountPlayerHarvester(pc, command))
	{
		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Orange, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("AddHarvesterMaxPersonTribeMSG", "You have reached the maximum dino harvester.").c_str());

		return;
	}

	HarvesterData* existingHarvester = FindHarvester(pc, dino);

	if (existingHarvester) return;

	// points deductions
	Points(pc->GetEOSId(), command.value("Cost", 0));

	AddHarvester(pc, dino, param);

	AsaApi::GetApiUtils().SendNotification(pc, FColorList::Green, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("AddedHarvesterMSG", "{} has been added to automatic harvesting dinos.").c_str(), dino->DescriptiveNameField().ToString());
}

void RemoveHarvesterCallBack(AShooterPlayerController* pc, FString* param, int, int)
{
	Log::GetLog()->warn("Function: {}", __FUNCTION__);

	AActor* actor = GetTargetActor(pc);

	if (!actor) return;

	if (!actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) return;

	APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

	if (!dino) return;

	TArray<HarvesterData>& harvester_list = GetHarvestingDino(pc);

	if (RemoveHarvesterByDino(dino, harvester_list))
	{
		AsaApi::GetApiUtils().SendNotification(pc, FColorList::Orange, 1.3f, 15.0f, nullptr, AutomaticDinoHarvest::config["Messages"].value("RemovedHarvesterMSG", "Auto harvesting for this dino has been stopped").c_str());
	}

}

void StopAllHarvesterCallBack(AShooterPlayerController* pc, FString* param, int, int)
{
	Log::GetLog()->warn("Function: {}", __FUNCTION__);

	RemoveAllHarvestingDinosByTribeID(pc);
}

void GetAllHarvestingDinosCallBack(AShooterPlayerController* pc, FString* param, int, int)
{

}


// Admin
void AdminStopAllHarvestCallBack(AShooterPlayerController* pc, FString* param, int, int)
{
	AutomaticDinoHarvest::tribeHarvestingDinos.Empty();

	AutomaticDinoHarvest::personalHarvestingDinos.Empty();

	AsaApi::GetApiUtils().SendNotification(pc, FColorList::Orange, 1.3f, 15.0f, nullptr, "All Harvesting dinos has been stoped.");
}

void AdminGetAllHarvestingDinosCallBack(AShooterPlayerController* pc, FString* param, int, int)
{

}





// Function
bool DoHarvestAttack()
{
	int harvestingDinoTribe = 0;
	int harvestingDinoPersonal = 0;
	TArray<HarvesterData> forDeleteTribe;
	TArray<HarvesterData> forDeletePersonal;

	UWorld* world = AsaApi::GetApiUtils().GetWorld();

	if (!world) return false;

	for (HarvesterData& harvester : AutomaticDinoHarvest::tribeHarvestingDinos)
	{
		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(world, harvester.DinoID1, harvester.DinoID2);

		if (!dino)
		{
			Log::GetLog()->warn("invalid dino");

			forDeleteTribe.Add(harvester);
			continue;
		}

		if (dino->bIsMounted().Get()) continue;

		if (dino->IsDead())
		{
			forDeleteTribe.Add(harvester);
			continue;
		}

		if (dino->TargetingTeamField() != harvester.TribeID)
		{
			Log::GetLog()->warn("not tribe dino");
			forDeleteTribe.Add(harvester);
			continue;
		}

		if (dino->AttackInfosField().Num() - 1 < harvester.AttackIndex)
		{
			Log::GetLog()->warn("AttackInfosField {}", dino->AttackInfosField().Num());
			forDeleteTribe.Add(harvester);
			continue;
		}

		dino->DoAttack(harvester.AttackIndex, true, false);

		UPrimalInventoryComponent* dinoInvComp = dino->MyInventoryComponentField();

		if (!dinoInvComp) continue;

		if (harvester.Resources[0] == "any") continue;

		TArray<UPrimalItem*> dinoItems = dinoInvComp->InventoryItemsField();

		for (UPrimalItem* dItem : dinoItems)
		{
			if (!FindItemInList(dItem->DescriptiveNameBaseField(), harvester.Resources))
			{
				dItem->RemoveItemFromInventory(true, false);

				//dinoInvComp->EjectItem(&dItem->ItemIDField(), false, true, false, &dino->GetLocation(), true, nullptr, true, harvester.TribeID);
			}
		}
	}

	// TODO merge as one array tribe and personal
	for (HarvesterData& harvester : AutomaticDinoHarvest::personalHarvestingDinos)
	{
		APrimalDinoCharacter* dino = APrimalDinoCharacter::FindDinoWithID(world, harvester.DinoID1, harvester.DinoID2);

		if (!dino)
		{
			forDeletePersonal.Add(harvester);
			continue;
		}

		if (dino->bIsMounted().Get()) continue;

		if (dino->IsDead())
		{
			forDeletePersonal.Add(harvester);
			continue;
		}

		if (dino->TargetingTeamField() != harvester.TribeID)
		{
			forDeletePersonal.Add(harvester);
			continue;
		}

		if (dino->AttackInfosField().Num() - 1 < harvester.AttackIndex)
		{
			forDeletePersonal.Add(harvester);
			continue;
		}

		dino->DoAttack(harvester.AttackIndex, true, false);

		UPrimalInventoryComponent* dinoInvComp = dino->MyInventoryComponentField();

		if (!dinoInvComp) continue;

		if (harvester.Resources[0] == "any") continue;

		TArray<UPrimalItem*> dinoItems = dinoInvComp->InventoryItemsField();

		for (UPrimalItem* dItem : dinoItems)
		{
			if (!FindItemInList(dItem->DescriptiveNameBaseField(), harvester.Resources))
			{
				dItem->RemoveItemFromInventory(true, false);

				//dinoInvComp->EjectItem(&dItem->ItemIDField(), false, true, false, &dino->GetLocation(), true, nullptr, true, harvester.TribeID);
			}
		}
	}

	for (HarvesterData fd : forDeleteTribe)
	{
		RemoveHarvesterByObject(fd, AutomaticDinoHarvest::tribeHarvestingDinos);
	}

	for (HarvesterData fd : forDeletePersonal)
	{
		RemoveHarvesterByObject(fd, AutomaticDinoHarvest::personalHarvestingDinos);
	}

	if (harvestingDinoTribe > 0 || harvestingDinoPersonal > 0)
	{
		Log::GetLog()->warn("Function: {}", __FUNCTION__);
		return true;
	}

	return false;
}



