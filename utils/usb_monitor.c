#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#define UEVENT_BUFFER_SIZE 2048
#define MIN_SIZE           30
#define USB_STATE_FILE     "/sys/class/android_usb/android0/state"
#define USB_CONFIG_ADB_CMD     "echo ff400000.dwc2_a > /sys/kernel/config/usb_gadget/amlogic/UDC"
#define USB_CONFIG_ADB_UDC     "/sys/kernel/config/usb_gadget/amlogic/UDC"
#define USB_CONFIG_ADB_UDC_VAL_FILE     "/etc/adb_udc_file"
#define ERROR_GET_CONTENT  -1
#define ERROR_USB_CONFIG_ADB  -2
#define SUCCESS            0
#define MAX_CUSTOMIZED_PATH_LENGTH 64
static char CUSTOMIZED_UDC_FILE_PATH[MAX_CUSTOMIZED_PATH_LENGTH];

int UDC_config()
{
    char UdcValue[128] = "ff400000.dwc2_a";
    char UdcValue_check[MIN_SIZE] = {0};
    FILE *fp = NULL;
    fp = fopen(CUSTOMIZED_UDC_FILE_PATH, "r");
    if (fp != NULL)
    {
        fgets(UdcValue, 128, fp);
        fclose(fp);
    }
    fp = NULL;

    fp = fopen(USB_CONFIG_ADB_UDC, "wb");
    if (fp != NULL)
    {
      //printf("udc config data!\n");
      if ( fputs("none", fp) < 0)
        printf("udc config data failure\n");
      fclose(fp);
    }
    fp = NULL;
    fp = fopen(USB_CONFIG_ADB_UDC, "wb");
    if (fp != NULL)
    {
      //printf("udc config data!\n");
      if ( fputs(UdcValue, fp) < 0)
        printf("udc config data failure\n");
      fclose(fp);
    }
    fp = NULL;

    fp = fopen(USB_CONFIG_ADB_UDC, "wb");
    if (fp != NULL)
    {
      //printf("udc config data!\n");
      if ( fputs(UdcValue, fp) < 0)
        printf("udc config data failure\n");
      fclose(fp);
    }
    fp = NULL;
    return 0;
}

int usb_configure()
{
    /*open android0 state node*/
    char FileName[MIN_SIZE] = {0};
    int flag = -1, ret = -1;
    FILE *fp = fopen(USB_STATE_FILE, "r");
    if (fp != NULL)
    {
        if (fgets(FileName, MIN_SIZE, fp) == NULL)
        {
            printf("get android0 state  content failure!\n");
            fclose(fp);
            ret = ERROR_GET_CONTENT;
        }
        else
        {
            //printf("content: %s\n", FileName);
            if(!strncmp("DISCONNECTED", (const char*)FileName, 12))
            {
                usleep(200*1000);   //Delay 200ms
                static time_t last = 0;
                time_t now;
                time(&now);
                if (now - last >= 1) {
                  UDC_config();
                } else {
                  printf("filter disconnected event in short time\n");
                }
                last = now;
                //system("echo ff400000.dwc2_a > /sys/kernel/config/usb_gadget/amlogic/UDC");
                ret = SUCCESS;
                //printf("config usb successfull!\n");
            }
             fclose(fp);
        }
    }
    else
        ret = ERROR_USB_CONFIG_ADB;
    fp = NULL;
    return ret;
}

void usage()
{
    printf("\tusb_monitor usage: \n");
    printf("\t./usb_monitor $PATH\n");
    printf("\t$PATH is OPTIONAL\n\tif no $PATH is specified, the default path /etc/adb_udc_file will be used\n");
    return;
}

int parse_customized_udc_file_path(int argc, char *argv[])
{
    int ret = 0;
    if (argc == 1) {
        strncpy(CUSTOMIZED_UDC_FILE_PATH, USB_CONFIG_ADB_UDC_VAL_FILE, sizeof(CUSTOMIZED_UDC_FILE_PATH));
        CUSTOMIZED_UDC_FILE_PATH[MAX_CUSTOMIZED_PATH_LENGTH - 1] = '\0';
        printf("select UDC file path: %s\n", CUSTOMIZED_UDC_FILE_PATH);
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h")) {
            if (strlen(argv[1]) > (MAX_CUSTOMIZED_PATH_LENGTH -1)) {
                ret = -1;
                printf("udc file path length %d exceeds max file path length %d\n", strlen(argv[1]), MAX_CUSTOMIZED_PATH_LENGTH-1);
            } else {
                strncpy(CUSTOMIZED_UDC_FILE_PATH, argv[1], sizeof(CUSTOMIZED_UDC_FILE_PATH));
                CUSTOMIZED_UDC_FILE_PATH[MAX_CUSTOMIZED_PATH_LENGTH - 1] = '\0';
                printf("select UDC file path: %s\n", CUSTOMIZED_UDC_FILE_PATH);
            }
        } else {
            ret = -1;
            usage();
        }
    } else {
        ret = -1;
        usage();
    }
    return ret;
}

int main(int argc, char *argv[])
{
    struct sockaddr_nl client;
    struct timeval tv;
    int Usbmonitor, rcvlen, ret;
    fd_set fds;
    int buffersize = 1024;

    ret = parse_customized_udc_file_path(argc, argv);
    if (ret) {
        return -1;
    }

    Usbmonitor = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (0 > Usbmonitor) {
      return -1;
    }
    memset(&client, 0, sizeof(client));
    client.nl_family = AF_NETLINK;
    client.nl_pid = getpid();
    client.nl_groups = 1; /* receive broadcast message*/
    ret = setsockopt(Usbmonitor, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
    if (0 > ret) {
      return -1;
    }
    ret = bind(Usbmonitor, (struct sockaddr*)&client, sizeof(client));
    if (0 > ret) {
      return -1;
    }

    while (1) {
        char buf[UEVENT_BUFFER_SIZE] = { 0 };
        FD_ZERO(&fds);
        FD_SET(Usbmonitor, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(Usbmonitor + 1, &fds, NULL, NULL, &tv);
        if (ret < 0) {
            continue;
        }
        if (!(ret > 0 && FD_ISSET(Usbmonitor, &fds))) {
              continue;
        }
        /* receive data */
        rcvlen = recv(Usbmonitor, &buf, sizeof(buf), 0);
        if (rcvlen > 0 && strstr(buf,"usb") ) {
          //printf("%s\n", buf);
          usb_configure();
        }
    }
    close(Usbmonitor);
    return 0;
}
