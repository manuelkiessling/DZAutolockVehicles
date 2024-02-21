#ifdef LoggingTools_Server_Vehicles
#ifndef CARLOCKDISABLE
#ifdef CarLock
class FOGAutolockVehicles_Settings
{
	bool DoDebug = true;
	int UseKeyMod = 0;
	int AutolockDelay_Startup = 5;
	int AutolockDelay_PlayerDisconnect = 5;
	int ProximityLock_DistanceMeters = 10;
	
	[NonSerialized()]
	private bool m_IsLoaded;

	void FOGAutolockVehicles_Settings(string AppName)
	{
		string ConfigPath = "$profile:FOG";
		MakeDirectory(ConfigPath);

		string FOGConfigFile = ConfigPath + "/" + AppName + ".json";
		this.m_IsLoaded = this.Load(FOGConfigFile) ;
	}

	bool Load(string FOGConfigFile)
	{
		if (FileExist(FOGConfigFile))
		{
			JsonFileLoader<FOGAutolockVehicles_Settings>.JsonLoadFile(FOGConfigFile, this);
			JsonFileLoader<FOGAutolockVehicles_Settings>.JsonSaveFile(FOGConfigFile, this);
			return true;
		}

		JsonFileLoader<FOGAutolockVehicles_Settings>.JsonSaveFile(FOGConfigFile, this);
		return false;
	}

	bool IsLoaded()
	{
		return this.m_IsLoaded;
	}

	bool GetDoDebug()
	{
		return DoDebug;
	}
}
#endif
#endif
#endif
