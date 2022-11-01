#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "bootloader_message.h"

#define AB_METADATA_MISC_PARTITION_OFFSET 2048

#define MISCBUF_SIZE  2080


/* Magic for the A/B struct when serialized. */
#define AVB_AB_MAGIC "\0AB0"
#define AVB_AB_MAGIC_LEN 4

/* Versioning for the on-disk A/B metadata - keep in sync with avbtool. */
#define AVB_AB_MAJOR_VERSION 1
#define AVB_AB_MINOR_VERSION 0

/* Size of AvbABData struct. */
#define AVB_AB_DATA_SIZE 32

/* Maximum values for slot data */
#define AVB_AB_MAX_PRIORITY 15
#define AVB_AB_MAX_TRIES_REMAINING 7
/* Struct used for recording per-slot metadata.
 *
 * When serialized, data is stored in network byte-order.
 */
typedef struct AvbABSlotData {
  /* Slot priority. Valid values range from 0 to AVB_AB_MAX_PRIORITY,
   * both inclusive with 1 being the lowest and AVB_AB_MAX_PRIORITY
   * being the highest. The special value 0 is used to indicate the
   * slot is unbootable.
   */
  uint8_t priority;

  /* Number of times left attempting to boot this slot ranging from 0
   * to AVB_AB_MAX_TRIES_REMAINING.
   */
  uint8_t tries_remaining;

  /* Non-zero if this slot has booted successfully, 0 otherwise. */
  uint8_t successful_boot;

  /* Reserved for future use. */
  uint8_t reserved[1];
} AvbABSlotData;

/* Struct used for recording A/B metadata.
 *
 * When serialized, data is stored in network byte-order.
 */
typedef struct AvbABData {
  /* Magic number used for identification - see AVB_AB_MAGIC. */
  uint8_t magic[AVB_AB_MAGIC_LEN];

  /* Version of on-disk struct - see AVB_AB_{MAJOR,MINOR}_VERSION. */
  uint8_t version_major;
  uint8_t version_minor;

  /* Padding to ensure |slots| field start eight bytes in. */
  uint8_t reserved1[2];

  /* Per-slot metadata. */
  AvbABSlotData slots[2];

  /* Reserved for future use. */
  uint8_t reserved2[12];

  /* CRC32 of all 28 bytes preceding this field. */
  uint32_t crc32;
}AvbABData;

#ifdef BOOTCTOL_AVB
static void dump_boot_info(AvbABData* info)
{
    printf("info->magic 0x%x 0x%x 0x%x 0x%x\n", info->magic[0], info->magic[1], info->magic[2], info->magic[3]);
    printf("info->version_major = %d\n", info->version_major);
    printf("info->version_minor = %d\n", info->version_minor);
    printf("info->slots[0].priority = %d\n", info->slots[0].priority);
    printf("info->slots[0].tries_remaining = %d\n", info->slots[0].tries_remaining);
    printf("info->slots[0].successful_boot = %d\n", info->slots[0].successful_boot);
    printf("info->slots[1].priority = %d\n", info->slots[1].priority);
    printf("info->slots[1].tries_remaining = %d\n", info->slots[1].tries_remaining);
    printf("info->slots[1].successful_boot = %d\n", info->slots[1].successful_boot);

    printf("info->crc32 = %d\n", info->crc32);
}

static int get_bootloader_message_block(char * miscbuf, int size, AvbABData *info) {
    const char *misc_device = "/dev/misc";
    printf ("read misc for emmc device\n");

    FILE* f = fopen(misc_device, "rb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }

    int count = fread(miscbuf, 1, size, f);
    if (count != size) {
        printf("Failed reading %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }

    memcpy(info, miscbuf + AB_METADATA_MISC_PARTITION_OFFSET, AVB_AB_DATA_SIZE);
    dump_boot_info(info);

    return 0;
}

/* Converts a 32-bit unsigned integer from host to big-endian byte order. */
uint32_t avb_htobe32(uint32_t in) {
  union {
    uint32_t word;
    uint8_t bytes[4];
  } ret;
  ret.bytes[0] = (in >> 24) & 0xff;
  ret.bytes[1] = (in >> 16) & 0xff;
  ret.bytes[2] = (in >> 8) & 0xff;
  ret.bytes[3] = in & 0xff;
  return ret.word;
}

static int set_bootloader_message_block(char * miscbuf, int size, AvbABData *info) {
    const char *misc_device = "/dev/misc";
    printf ("write misc for emmc device\n");

    info->crc32 = avb_htobe32(
      avb_crc32((const uint8_t*)info, sizeof(AvbABData) - sizeof(uint32_t)));

    memcpy(miscbuf+AB_METADATA_MISC_PARTITION_OFFSET, info, AVB_AB_DATA_SIZE);
    dump_boot_info(info);

    FILE* f = fopen(misc_device, "wb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    int count = fwrite(miscbuf, 1, MISCBUF_SIZE, f);
    if (count != MISCBUF_SIZE) {
        printf("Failed writing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    return 0;
}

int get_active_slot_from_misc(int *slot) {
    int ret = 0;
    AvbABData info;
    char miscbuf[MISCBUF_SIZE] = {0};

    ret = get_bootloader_message_block(miscbuf, MISCBUF_SIZE, &info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    if (info.slots[0].priority > info.slots[1].priority)
        *slot = 0;
    else
        *slot = 1;

    return 0;
}

bool boot_info_validate(AvbABData* info)
{
    if (memcmp(info->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN) != 0) {
        printf("Magic %s is incorrect.\n", info->magic);
        return false;
    }
    if (info->version_major > AVB_AB_MAJOR_VERSION) {
        printf("No support for given major version.\n");
        return false;
    }
    return true;
}

int boot_info_set_active_slot(AvbABData* info, int slot)
{
    unsigned int other_slot_number;

    printf ("set active slot: %d\n", slot);
    /* Make requested slot top priority, unsuccessful, and with max tries. */
    info->slots[slot].priority = AVB_AB_MAX_PRIORITY;
    info->slots[slot].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[slot].successful_boot = 0;

    /* Ensure other slot doesn't have as high a priority. */
    other_slot_number = 1 - slot;
    if (info->slots[other_slot_number].priority == AVB_AB_MAX_PRIORITY) {
        info->slots[other_slot_number].priority = AVB_AB_MAX_PRIORITY - 1;
    }

    dump_boot_info(info);

    return 0;
}

void boot_info_reset(AvbABData* info)
{
    memset(info, '\0', sizeof(AvbABData));
    memcpy(info->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN);
    info->version_major = AVB_AB_MAJOR_VERSION;
    info->version_minor = AVB_AB_MINOR_VERSION;
    info->slots[0].priority = AVB_AB_MAX_PRIORITY;
    info->slots[0].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[0].successful_boot = 0;
    info->slots[1].priority = AVB_AB_MAX_PRIORITY - 1;
    info->slots[1].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[1].successful_boot = 0;
}

int set_active_slot(SlotType_e slot) {
    int ret = 0;
    char miscbuf[MISCBUF_SIZE] = {0};
    AvbABData info;

    ret = get_bootloader_message_block(miscbuf, MISCBUF_SIZE, &info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    if (!boot_info_validate(&info)) {
        printf("boot-info is invalid. Resetting.\n");
        boot_info_reset(&info);
    }

    boot_info_set_active_slot(&info, slot);

    ret = set_bootloader_message_block(miscbuf, MISCBUF_SIZE, &info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}

int set_successful_boot() {
    int ret = 0;
    char miscbuf[MISCBUF_SIZE] = {0};
    AvbABData info;
    int slot = 0;

    ret = get_bootloader_message_block(miscbuf, MISCBUF_SIZE, &info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    if (info.slots[1].priority > info.slots[0].priority) {
        slot = 1;
    }

    if (info.slots[slot].successful_boot) {
        return 0;
    }

    info.slots[slot].successful_boot = 1;
    ret = set_bootloader_message_block(miscbuf, MISCBUF_SIZE, &info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}
#endif
