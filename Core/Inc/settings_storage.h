#ifndef SETTINGS_STORAGE_H
#define SETTINGS_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "settings.h"

uint8_t Settings_Storage_Load(SettingsState *state);
uint8_t Settings_Storage_Save(const SettingsState *state);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_STORAGE_H */
