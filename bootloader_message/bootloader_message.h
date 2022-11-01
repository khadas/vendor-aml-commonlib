#ifndef _AML_BOOTLOADER_MESSAGE_H
#define _AML_BOOTLOADER_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SLOT_A,
  SLOT_B,
} SlotType_e;

int get_active_slot(SlotType_e *slot);
int set_active_slot(SlotType_e slot);
int get_active_slot_misc(SlotType_e *slot);
int set_successful_boot();

int clear_recovery();
int set_recovery();
int set_recovery_otapath(char *path);
int get_recovery_otapath(char * path);
int clean_recovery_otapath();

int get_inactive_mtd(const char *partitionname);
int get_inactive_devicename(const char *partitionname, SlotType_e slot, char *device);
int get_system_type();
int mtd_scan_partitions();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
