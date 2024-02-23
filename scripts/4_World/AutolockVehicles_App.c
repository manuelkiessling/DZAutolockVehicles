/*
- On player disconnect
-- Player not in a vehicle? EXIT
-- Player not driver of that vehicle? EXIT
-- Goto StartTimer with settings.AutolockDelay_PlayerDisconnect

- On vehicle spawn (restart, receipt thrown, parked out, ..)
-- Goto StartTimer with settings.AutolockDelay_Startup

- Vehicle deleted or destroyed
-- Stop timer

- Vehicle engine start, vehicle deleted or destroyed
-- Start measuring distance between player and car
-- If distance > ProximityLock_DistanceMeters, goto LockVehicle

- StartTimer
-- Vehicle has no lock? EXIT
-- Vehicle is locked already? EXIT
-- Start the actual timer

- On timer finish
-- Goto LockVehicle

- LockVehicle
-- Vehicle has no lock? EXIT
-- Vehicle is locked already? EXIT
-- LOCK!

*/

class AutolockVehicles_App
{
	protected static ref AutolockVehicles_App s_Instance;
	static const string m_AppName = "AutolockVehicles";

	ref AutolockVehicles_Logger m_Logger;
	ref AutolockVehicles_Settings m_Settings;
	    
	string m_TimerTargetFunctionName = "LockVehicle";

	ref map<string, ref AutolockVehicles_KeyModBase> m_KeyMods;
	ref AutolockVehicles_KeyModBase m_KeyMod;

	ref map<int, ref Timer> m_Timers;

	void AutolockVehicles_App()
	{
		m_Logger = new AutolockVehicles_Logger(m_AppName);
		m_Settings = new AutolockVehicles_Settings(m_AppName);
		
		m_Timers = new map<int, ref Timer>;
		m_KeyMods = new map<string, ref AutolockVehicles_KeyModBase>();

		AddKeyMod("CUSTOM", new AutolockVehicles_Custom());

		#ifdef CarLock
		#ifndef CARLOCKDISABLE
			AddKeyMod("TRADERPLUSCARLOCK", new AutolockVehicles_TraderPlusCarLock());
		#endif
		#endif

		#ifdef MuchCarKey
			AddKeyMod("MUCHCARKEY", new AutolockVehicles_MuchCarKey());
		#endif

		#ifdef Trader
			AddKeyMod("TRADER", new AutolockVehicles_Trader());
		#endif

		#ifdef EXPANSIONMODVEHICLE
			AddKeyMod("EXPANSION", new AutolockVehicles_Expansion());
		#endif

		s_Instance = this;

		if (!m_Settings.IsLoaded())
		{
			Print("[" + m_AppName + "] Settings incomplete. Please check " + m_Logger.m_LogFile);
			Print("[" + m_AppName + "] If it didn't exist at all, an empty one was created right now.");
			return;
		}

		Print("[" + m_AppName + "] Ready. Logging commences in " + m_Logger.m_LogFile);
		AutolockVehicles_App.GetInstance().m_Logger.Log("Ready.");
	}

	static AutolockVehicles_App GetInstance()
	{
		if (!s_Instance) new AutolockVehicles_App();
		return s_Instance;
	}

	void AddKeyMod(string name, AutolockVehicles_KeyModBase keyMod)
	{
		if(!m_KeyMods) m_KeyMods = new map<string, ref AutolockVehicles_KeyModBase>();
		m_KeyMods.Set(name, keyMod);
	}

	AutolockVehicles_KeyModBase GetKeyMod()
	{
		if(!m_KeyMods) m_KeyMods = new map<string, ref AutolockVehicles_KeyModBase>();
		if(!m_KeyMod) m_KeyMod = m_KeyMods.Get(EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.UseKeyMod));
		return m_KeyMod;
	}

	void StartAutolockTimer(CarScript car, AutolockVehicles_TimerMode mode)
    {
		if(!car)
        {
            m_Logger.Log("No vehicle, exiting");
            return;
        }
       
		AutolockVehicles_KeyModBase keyMod = GetKeyMod();
		if(!keyMod)
        {
            m_Logger.Log("UseKeyMod is set to " + m_Settings.UseKeyMod + " (" + EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.UseKeyMod) + ") but the required mod doesn't seem to be installed");
            return;
        }

		if(keyMod.GetVehicleState(car) == AutolockVehicles_State.UNASSIGNED)
        {
            m_Logger.Log("Car doesn't even have a lock, no timer will be started, exiting");
            return;
        }

        if(keyMod.GetVehicleState(car) == AutolockVehicles_State.LOCKED)
        {
            m_Logger.Log("Car already locked, no timer will be started, exiting");
            return;
        }

		if(keyMod.GetVehicleState(car) == AutolockVehicles_State.ERROR)
        {
            m_Logger.Log("Vehicle state ERROR - UseKeyMod set to 0, but no custom KeyMod implemented?", AutolockVehicles_LogLevel.CRITICAL);
            return;
        }

        int AutolockDelay;

        switch(mode)
        {
            case AutolockVehicles_TimerMode.STARTUP:
            {
				if(!m_Settings.enableStartupAutolock) return;
                AutolockDelay = m_Settings.AutolockDelay_Startup;
                break;
            }
			case AutolockVehicles_TimerMode.PLAYERDISCONNECT:
            {
				if(!m_Settings.enableDisconnectAutolock) return;
                AutolockDelay = m_Settings.AutolockDelay_PlayerDisconnect;
                break;
            }
            default:
            {
                m_Logger.Log("Unknown AutolockVehicles_TimerMode:" + mode);
                return;
            }
        }

        m_Logger.Log("Starting autolock timer, mode:" + typename.EnumToString(AutolockVehicles_TimerMode, mode) + ", minutes:" + AutolockDelay);

        Param1<CarScript> carParam = new Param1<CarScript>(car);
		Timer timer = new Timer();
		timer.Run((AutolockDelay * 60), this, m_TimerTargetFunctionName, carParam, false);
		m_Timers.Set(car.GetID(), timer);
    }

	void RemoveAutolockTimer(CarScript car, string Reason)
    {
        if(!GetGame().IsServer()) return;
		if(!car) return;

        m_Logger.Log("Removing autolock timer, (" + Reason + ")");
        m_Timers.Remove(car.GetID());
    }

	void OnPlayerDisconnect(PlayerBase player)
	{
		if(!player) return;
		if(!player.GetIdentity()) return;
		if(!m_Settings.enableDisconnectAutolock) return;
		
		string playerId = player.GetIdentity().GetPlainId();
        m_Logger.Log("Starting OnDisconnect autolock, player:" + playerId);

		if(!player.m_AutolockVehicles_CurrentUnlockedVehicle)
        {
            m_Logger.Log("Player doesn't have a currently unlocked vehicle assigned, exiting");
            return;
        }
      
        StartAutolockTimer(player.m_AutolockVehicles_CurrentUnlockedVehicle, AutolockVehicles_TimerMode.PLAYERDISCONNECT);
	}

	array<string> GetDoorAnimSources(CarScript car)
	{
		if(!car) return null;

		array<Selection> selections = new array<Selection>();
		array<string> doorAnimSources = new array<string>();

		LOD lod = car.GetLODByName(LOD.NAME_GEOMETRY);
		lod.GetSelections(selections);

		foreach(Selection selection : selections)
		{
			if(selection.GetName().Substring(0, 6) != "doors_") continue;
			string animSource = car.GetAnimSourceFromSelection(selection.GetName());
			if(animSource == "") continue;
			doorAnimSources.Insert(animSource);
		}

		return doorAnimSources;
	}

	void CloseAllDoors(CarScript car)
	{
		if(!car) return;
		
		array<string> doorAnimSources = GetDoorAnimSources(car);

		foreach(string animSource : doorAnimSources)
		{
			car.SetAnimationPhase(animSource, 0.0);
		}
	}

	bool AreAllDoorsClosed(CarScript car)
	{
		if(!car) return false;

		array<string> doorAnimSources = GetDoorAnimSources(car);

		foreach(string animSource : doorAnimSources)
		{
			if(car.GetAnimationPhase(animSource) > 0.0) return false;
		}

		return true;
	}

	void LockVehicle(CarScript car)
	{
		m_Logger.Log("Starting LockVehicle");

		if(!car)
        {
            m_Logger.Log("No car, exiting");
            return;
        }

		AutolockVehicles_KeyModBase keyMod = GetKeyMod();
		if(!keyMod)
        {
            m_Logger.Log("UseKeyMod is set to " + m_Settings.UseKeyMod + " (" + EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.UseKeyMod) + ") but the required mod doesn't seemto be installed, not locking");
            return;
        }

		if(keyMod.GetVehicleState(car) == AutolockVehicles_State.UNASSIGNED)
        {
            m_Logger.Log("Car doesn't even have a lock, not locking");
            return;
        }

        if(keyMod.GetVehicleState(car) == AutolockVehicles_State.LOCKED)
        {
            m_Logger.Log("Car already locked, not locking");
            return;
        }

		if(m_Settings.lockOnlyWhenAllDoorsAreClosed && !AreAllDoorsClosed(car))
		{
			m_Logger.Log("lockOnlyWhenAllDoorsAreClosed set, but not all doors are closed, not locking");
			return;
		}

		m_Logger.Log("Locking vehicle");

		if(m_Settings.enableCloseDoors) CloseAllDoors(car);
		if(m_Settings.enableEngineOff) car.EngineStop();
		keyMod.LockVehicle(car);
	}
}
