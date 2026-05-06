#include "settings_storage.h"

#include <stddef.h>
#include <string.h>

#define SETTINGS_STORAGE_FLASH_ADDRESS 0x0807F800U
#define SETTINGS_STORAGE_FLASH_BANK FLASH_BANK_2
#define SETTINGS_STORAGE_FLASH_PAGE 127U
#define SETTINGS_STORAGE_MAGIC 0x53424845U
#define SETTINGS_STORAGE_VERSION 1U

typedef struct
{
  uint32_t magic;
  uint16_t version;
  uint16_t length;
  uint32_t sequence;
  SettingsState state;
  uint32_t crc;
} SettingsStorageRecord;

static uint32_t Settings_Storage_CalculateCrc(const uint8_t *data, uint32_t length)
{
  uint32_t index;
  uint32_t crc = 2166136261UL;

  for (index = 0U; index < length; index++)
  {
    crc ^= data[index];
    crc *= 16777619UL;
  }

  return crc;
}

static uint8_t Settings_Storage_IsRecordValid(const SettingsStorageRecord *record)
{
  uint32_t stored_crc;
  uint32_t calculated_crc;

  if (record->magic != SETTINGS_STORAGE_MAGIC)
  {
    return 0U;
  }

  if (record->version != SETTINGS_STORAGE_VERSION)
  {
    return 0U;
  }

  if (record->length != (uint16_t)sizeof(SettingsState))
  {
    return 0U;
  }

  stored_crc = record->crc;
  calculated_crc = Settings_Storage_CalculateCrc((const uint8_t *)record,
                                                 (uint32_t)offsetof(SettingsStorageRecord, crc));

  return (stored_crc == calculated_crc) ? 1U : 0U;
}

uint8_t Settings_Storage_Load(SettingsState *state)
{
  const SettingsStorageRecord *record = (const SettingsStorageRecord *)SETTINGS_STORAGE_FLASH_ADDRESS;

  if (state == NULL)
  {
    return 0U;
  }

  if (Settings_Storage_IsRecordValid(record) == 0U)
  {
    return 0U;
  }

  *state = record->state;
  return 1U;
}

uint8_t Settings_Storage_Save(const SettingsState *state)
{
  FLASH_EraseInitTypeDef erase = {0};
  SettingsStorageRecord record;
  uint32_t page_error = 0U;
  uint32_t offset;
  uint8_t program_bytes[sizeof(SettingsStorageRecord) + 7U];
  uint32_t program_length = (uint32_t)((sizeof(SettingsStorageRecord) + 7U) & ~7U);
  HAL_StatusTypeDef status;
  uint64_t double_word;

  if (state == NULL)
  {
    return 0U;
  }

  memset(&record, 0, sizeof(record));
  record.magic = SETTINGS_STORAGE_MAGIC;
  record.version = SETTINGS_STORAGE_VERSION;
  record.length = (uint16_t)sizeof(SettingsState);
  record.sequence = 0U;
  record.state = *state;
  record.crc = Settings_Storage_CalculateCrc((const uint8_t *)&record,
                                             (uint32_t)offsetof(SettingsStorageRecord, crc));

  memset(program_bytes, 0xFF, sizeof(program_bytes));
  memcpy(program_bytes, &record, sizeof(record));

  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    return 0U;
  }

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Banks = SETTINGS_STORAGE_FLASH_BANK;
  erase.Page = SETTINGS_STORAGE_FLASH_PAGE;
  erase.NbPages = 1U;

  status = HAL_FLASHEx_Erase(&erase, &page_error);

  for (offset = 0U; status == HAL_OK && offset < program_length; offset += 8U)
  {
    memcpy(&double_word, &program_bytes[offset], sizeof(double_word));
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                               SETTINGS_STORAGE_FLASH_ADDRESS + offset,
                               double_word);
  }

  HAL_FLASH_Lock();

  return (status == HAL_OK) ? 1U : 0U;
}
