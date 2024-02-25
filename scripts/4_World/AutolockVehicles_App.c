/*
- On vehicle unlock:
-- enable_proximity_autolock not set? EXIT
-- Save vehicle in player.LastUnlockedVehicle
-- Save player in vehicle.LastPlayerUnlocked
-- GOTO ProximityProcess

- ProximityProcess (on the client):
-- Continuously measure the distance between the player and the vehicle
-- If the player leaves the proximity defined by proximity_lock_distance_meters:
--- Player doesn't have a LastUnlockedVehicle? EXIT
--- LastUnlockedVehicle was last unlocked by a different player? EXIT
--- GOTO LockVehicle

- On player disconnect:
-- enable_disconnect_autolock_timer not set? EXIT
-- Player doesn't have a LastUnlockedVehicle? EXIT
-- LastUnlockedVehicle was last unlocked by a different player? EXIT
-- GOTO StartTimer with settings.autolock_delay_player_disconnect_minutes

- On vehicle spawn (restart, vehicle bought, parked out, ..)
-- enable_startup_autolock_timer not set? EXIT
-- GOTO StartTimer with settings.autolock_delay_startup_minutes

- On vehicle engine start, vehicle deleted or destroyed
-- Remove any timers on this vehicle

- StartTimer
-- Vehicle has no lock? EXIT
-- Vehicle is locked already? EXIT
-- Start the actual timer
-- On timer finish: GOTO LockVehicle

- LockVehicle
-- Vehicle has no lock? EXIT
-- Vehicle is locked already? EXIT
-- lock_only_when_all_doors_are_closed set, but not all doors are closed? EXIT
-- enable_close_doors_on_autolock set? Close all doors
-- enable_engine_off_on_autolock set? Engine off
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
		if(!m_KeyMod) m_KeyMod = m_KeyMods.Get(EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.use_key_mod));
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
			m_Logger.Log("use_key_mod is set to " + m_Settings.use_key_mod + " (" + EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.use_key_mod) + ") but the required mod doesn't seem to be installed");
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
			m_Logger.Log("Vehicle state ERROR - use_key_mod set to 0, but no custom KeyMod implemented?", AutolockVehicles_LogLevel.CRITICAL);
			return;
		}

		int AutolockDelay;

		switch(mode)
		{
			case AutolockVehicles_TimerMode.STARTUP:
			{
				if(!m_Settings.enable_startup_autolock_timer) return;
				AutolockDelay = m_Settings.autolock_delay_startup_minutes;
				break;
			}
			case AutolockVehicles_TimerMode.PLAYERDISCONNECT:
			{
				if(!m_Settings.enable_disconnect_autolock_timer) return;
				AutolockDelay = m_Settings.autolock_delay_player_disconnect_minutes;
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

	void StartProximityWatcher(PlayerBase player, CarScript car)
	{
		if( !player || !car ) return;

		player.m_AutolockVehicles_LastUnlockedVehicle = car;
		car.m_AutolockVehicles_LastPlayerUnlocked = player;

		if(!m_Settings.enable_proximity_autolock) return;

		RemoveAutolockTimer(car, "StartProximityWatcher");

		string networkId = AutolockVehicles_Helper.GetNetworkID(car);

		Param2<string, int> carParam = new Param2<string, int>(networkId, m_Settings.proximity_lock_distance_meters);
		GetGame().RPCSingleParam(player, AutolockVehicles_RPC.START_PROXIMITY, carParam, true, player.GetIdentity());
	}

	void RemoveAutolockTimer(CarScript car, string Reason)
	{
		if(!GetGame().IsServer()) return;
		if(!car) return;

		m_Logger.Log("Removing autolock timer (" + Reason + ")");
		m_Timers.Remove(car.GetID());
	}

	void OnPlayerDisconnect(PlayerBase player)
	{
		if(!player) return;
		if(!player.GetIdentity()) return;
		if(!m_Settings.enable_disconnect_autolock_timer) return;
		
		string playerId = player.GetIdentity().GetPlainId();
		m_Logger.Log("Starting OnDisconnect autolock, player:" + playerId);

		if(!player.m_AutolockVehicles_LastUnlockedVehicle)
		{
			m_Logger.Log("Player doesn't have a currently unlocked vehicle assigned, exiting");
			return;
		}
	  
	  	if(player.m_AutolockVehicles_LastUnlockedVehicle.m_AutolockVehicles_LastPlayerUnlocked.GetID() != player.GetID())
		{
			m_Logger.Log("Vehicle wasn't unlocked last by player, exiting");
			return;
		}

		StartAutolockTimer(player.m_AutolockVehicles_LastUnlockedVehicle, AutolockVehicles_TimerMode.PLAYERDISCONNECT);
	}

	array<string> GetDoorAnimSources(CarScript car)
	{
		if(!car) return null;

		array<Selection> selections = new array<Selection>();
		array<string> doorAnimSources = new array<string>();

		LOD lod = car.GetLODByName("geometryView");
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
			m_Logger.Log("use_key_mod is set to " + m_Settings.use_key_mod + " (" + EnumTools.EnumToString(AutolockVehicles_KeyMod, m_Settings.use_key_mod) + ") but the required mod doesn't seemto be installed, not locking");
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

		if(m_Settings.lock_only_when_all_doors_are_closed && !AreAllDoorsClosed(car))
		{
			m_Logger.Log("lock_only_when_all_doors_are_closed set, but not all doors are closed, not locking");
			return;
		}

		m_Logger.Log("Locking vehicle");

		if(m_Settings.enable_close_doors_on_autolock) CloseAllDoors(car);
		if(m_Settings.enable_engine_off_on_autolock) car.EngineStop();
		keyMod.LockVehicle(car);
	}
}
