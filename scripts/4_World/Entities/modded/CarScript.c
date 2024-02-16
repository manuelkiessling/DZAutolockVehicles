#ifdef LoggingTools_Server_Vehicles
#ifndef CARLOCKDISABLE
#ifdef CarLock

modded class CarScript
{
    ref FOGAutolockVehicles_App m_App;

    /* override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if ( !super.OnStoreLoad(ctx, version))
            return false;
        
        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
        m_App.StartAutolockTimer(this, FOGAutolockVehicles_TimerMode.STARTUP);

        return true;
    } */

    override void DeferredInit()
    {
        super.DeferredInit();
        
        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
        m_App.m_Logger.DebugLog("Initializing vehicle " + m_LoggingTools_CarLock_VehicleId);

        m_App.StartAutolockTimer(this, FOGAutolockVehicles_TimerMode.STARTUP);
    }
    
    override void OnEngineStop()
	{
		super.OnEngineStop();

        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
		m_App.StartAutolockTimer(this, FOGAutolockVehicles_TimerMode.ENGINESTOP);
	}

    override void OnEngineStart()
	{
		super.OnEngineStart();

        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
		m_App.RemoveAutolockTimer("OnEngineStart");
	}

    override void EEKilled(Object killer)
	{
        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
		m_App.RemoveAutolockTimer("EEKilled");

		super.EEKilled(killer);
	}

	override void EEDelete(EntityAI parent)
	{
        if(!m_App) m_App = FOGAutolockVehicles_App.GetInstance();
        m_App.RemoveAutolockTimer("EEDelete");

		super.EEDelete(parent);
	}
}

#endif
#endif
#endif