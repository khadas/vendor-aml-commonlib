#include <stdio.h>
#include <cutils/log.h>
#include <unistd.h>

int main(int argc, char ** argv){
    while (1) {
        sleep(1);
        ALOGE("abert %s: test======>%d \n", __FUNCTION__, __LINE__);
    }
    return 0;
}
