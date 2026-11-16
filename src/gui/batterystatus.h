#ifndef BATTERYSTATUS_H
#define BATTERYSTATUS_H

enum BatteryStatus {
    BATT_STATUS_UNKNOWN,
    BATT_STATUS_CHARGING,
    BATT_STATUS_DISCHARGING,
    BATT_STATUS_CHARGED,
    BATT_STATUS_INVALID, // Used for safety checks
};

// Index into the battery level names (1 critical .. 4 high); 0 means not connected
inline int batteryLevelIndex(unsigned int bat) {
    if(bat > 50)
        return 4;
    if(bat > 30)
        return 3;
    if(bat > 15)
        return 2;
    return 1;
}

#endif // BATTERYSTATUS_H
