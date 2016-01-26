#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include "Configuration.h"

void Config_ResetDefault();

#ifndef DISABLE_M503
void Config_PrintSettings();
#else
static FORCE_INLINE void Config_PrintSettings() {}
#endif

#ifdef EEPROM_SETTINGS
void Config_StoreSettings();
void Config_RetrieveSettings();
#else
static FORCE_INLINE void Config_StoreSettings() {}
static FORCE_INLINE void Config_RetrieveSettings() { Config_ResetDefault(); Config_PrintSettings(); }
#endif

#endif//CONFIG_STORE_H
