#include <stdio.h>
#include <stdlib.h>

#include "bootloader_message.h"

void usage(char *program_name) {
    printf("ab system select!!!\n");
    printf("%s get ---get current slot\n", program_name);
    printf("%s set ---set slot\n", program_name);
    printf("%s success_boot ---set boot success\n", program_name);
    printf("example: \n");
    printf("%s get\n", program_name);
    printf("%s set _a\n", program_name);
    printf("%s set _b\n", program_name);
    printf("%s success_boot\n", program_name);
}

int main(int argc, char **argv) {
    int ret = 0;

    if ((argc != 2) && (argc != 3)) {
        usage(argv[0]);
        return -1;
    }

    if (argc == 3) {
        int slot;

        if (!strcmp(argv[2],"_a")) {
            slot = 0;
        } else if (!strcmp(argv[2],"_b")) {
            slot = 1;
        } else {
            printf("invalid slot: %s\n", argv[2]);
            usage(argv[0]);
            return -1;
        }

        if (!strcmp(argv[1], "set")) {
            ret = set_active_slot(slot);
            if (ret != 0) {
                printf("set active slot: %s failed!\n", argv[2]);
                return -1;
            }
        } else {
            usage(argv[0]);
            return -1;
        }
    }

    if (!strcmp(argv[1], "get")) {
        int active = 0;
        get_active_slot_from_misc(&active);
        printf("active is %d\n", active);
    } else if (!strcmp(argv[1], "success_boot")) {
        ret = set_successful_boot();
        if (ret != 0) {
            printf("set successful boot slot failed!\n");
            return -1;
        }
    }

    return 0;
}
