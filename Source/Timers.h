
void TimerCallback()
{
	int interval = AutomaticDinoHarvest::config["General"].value("AutoHarvestInterval", 1);

	if (AutomaticDinoHarvest::counter % interval == 0)
	{
		DoHarvestAttack();
	}

	AutomaticDinoHarvest::counter++;
}


void SetTimers(bool addTmr = true)
{
	if (addTmr)
	{
		AsaApi::GetCommands().AddOnTimerCallback("AutomaticDinoHarvestTimerTick", &TimerCallback);
	}
	else
	{
		AsaApi::GetCommands().RemoveOnTimerCallback("AutomaticDinoHarvestTimerTick");
	}
}