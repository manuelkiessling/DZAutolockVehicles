#ifdef MuchCarKey
class AutolockVehicles_MuchCarKey : AutolockVehicles_KeyModBase
{
    override string GetName() { return "MUCHCARKEY"; }
    
    override AutolockVehicles_State GetVehicleState(Transport vehicle)
    {
        CarScript car;
        car = CarScript.Cast(vehicle);
        if(!car) return AutolockVehicles_State.ERROR;

        if(!car.m_HasCKAssigned) return AutolockVehicles_State.UNASSIGNED;
        if(car.m_IsCKLocked) return AutolockVehicles_State.LOCKED;
        return AutolockVehicles_State.UNLOCKED;
    }

    override void LockVehicle(Transport vehicle)
    {
        CarScript car;
        car = CarScript.Cast(vehicle);
        if(!car) return;

        car.m_IsCKLocked = true;
        car.SynchronizeValues();
        car.ResetLifetime();
			
    }
}
#endif