/*
 * Copyright (C) 2023 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>

#include "aml_wpa_wifi_util.h"

#define INVALID_INPUT -99
#define MAX_TEST_CASE 7
#define INPUT_NUMBER(var, ...) get_input_string(type_int,    var, __VA_ARGS__)
#define INPUT_STRING(var, ...) get_input_string(type_string, var, __VA_ARGS__)
#define INPUT_CHAR(var, ...)   get_input_string(type_char,   var, __VA_ARGS__)

static const char *wpa_wifi_connection_status_strings[] = {
    "WPA_WIFI_INVALID",
    "WPA_WIFI_SUCCESS",
    "WPA_WIFI_CONNECTING",
    "WPA_WIFI_DISCONNECTED",
    "WPA_WIFI_ERROR_NOT_FOUND",
    "WPA_WIFI_ERROR_INVALID_CREDENTIALS",
    "WPA_WIFI_ERROR_TIMEOUT_EXPIRED",
    "WPA_WIFI_ERROR_DEV_DISCONNECT",
    "WPA_WIFI_ERROR_UNKNOWN"
};

static const char *wpa_wifi_scan_status_strings[] = {
    "WPA_WIFI_SCAN_STATE_IDLE",
    "WPA_WIFI_SCAN_STATE_CMD_SENT",
    "WPA_WIFI_SCAN_STATE_STARTED",
    "WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED",
};

static const char *wpa_wifi_signal_level[] = {
    "Level 0, Range rssi <= -88",
    "Level 1, Range (-88, -77]",
    "Level 2, Range (-77, -66]",
    "Level 3, Range (-66, -55]",
    "Level 4, Range rssi > -55"
};

typedef enum _input_types {
    type_int = 0,
    type_string = 1,
    type_char = 2
}INPUT_TYPES_E;

void get_input_string(INPUT_TYPES_E var_type, void *input, const char *prompt, ...)
{
   #define MAX_INPUT 100
   va_list argp;
   char buf[MAX_INPUT];
   va_start(argp, prompt);
   vprintf(prompt, argp);
   va_end(argp);

   fgets(buf, MAX_INPUT, stdin);
   if (var_type == type_int) {
       if (isdigit(buf[0]))
           sscanf(buf,"%d",(int*)input);
       else
           *(int*)input = -1;
   }
   else if (var_type == type_string) {
       sscanf(buf,"%[^\n]",(char*)input);
   }
   else if (var_type == type_char) {
       sscanf(buf,"%c",(char*)input);
   }
   /*add more cases if required...*/
   return;
}

void exit_handler(int sig)
{
    wpa_wifi_uninit();
    printf("signal is %d, exit! \n", sig);
    exit(0);
}

/*
    custom wifi connection status callback function
*/
void demo_wifi_connect_callback(char *ssid, const WPA_WIFI_STATUS_TYPE *cur_status) {
    if (WPA_WIFI_SUCCESS == *cur_status) {
        printf("ssid [%s] connect successfully\n", ssid);
    } else {
        printf("ssid [%s] is connecting or fail, status is %s\n", ssid, wpa_wifi_connection_status_strings[*cur_status]);
    }
}

void help()
{
    printf("\n****************** AMLOGIC WPA WiFi Basic API Test Options ******************\n");
    printf(" [ 1 ]    Connect to ssid                   \n");
    printf(" [ 2 ]    Disconnect current connection     \n");
    printf(" [ 3 ]    Scan and get ssids                \n");
    printf(" [ 4 ]    Reconnect the last ssid           \n");
    printf(" [ 5 ]    Get current wifi connection status\n");
    printf(" [ 6 ]    Get current wifi scan status      \n");
    printf(" [ 7 ]    Get current wifi connection info  \n");
    printf(" [ h ]    Show this list                    \n");
    printf(" [ q ]    Quit                              \n");
    printf("***********************************************************************\n");
}

int main() {
    wpa_wifi_init(DEFAULT_WPA_SUPL_CTRL, demo_wifi_connect_callback, 0, 1);

    char input[16];
    char ssid[WPA_SSID_SIZE_MAX];
    char password[WPA_PSK_SIZE_MAX];
    WPA_WIFI_AP_INFO wifi_ap_array[WPA_AP_NUM_MAX];
    WPA_WIFI_STATUS_TYPE cur_status;
    int ap_num;
    int select;
    int ret;
    int is_ap_sort_by_signal_strength;

    signal(SIGTERM, exit_handler);
    signal(SIGINT,  exit_handler);
    signal(SIGPIPE, exit_handler);

    help();
    while (1) {
        input[0]='\0';
        INPUT_STRING(&input,"\n[ Enter your choice ]: ");
        if (input[0] == 'h') {
            help();
            continue;
        }
        else if (input[0] == 'q') {
            exit_handler(0);
        }
        else {
            if (!isdigit(input[0]))
                select = INVALID_INPUT;
            else
                select = atoi(input);

            if (select == INVALID_INPUT || select < 0 || select > MAX_TEST_CASE) {
                printf("Entered option [%s] is not a valid one, supported are [h, q, 1-%d]\n",input,MAX_TEST_CASE);
                continue;
           }
        }
        switch (select) {
        case 1:
            INPUT_STRING(ssid, "[[ Please enter the SSID ]] \n");
            INPUT_STRING(password, "[[ Please enter PSK for SSID %s ]] \n",ssid);
            wpa_wifi_send_connect_cmd(ssid, password);
            break;
        case 2:
            wpa_wifi_send_disconnect_cmd();
            break;
        case 3:
            INPUT_NUMBER(&is_ap_sort_by_signal_strength, "[[ Please enter whether you want to sort by intensity, 1 means yes, 0 means no ]] \n");
            wpa_wifi_send_scan_cmd();
            ret = wpa_wifi_get_scan_results(wifi_ap_array, WPA_AP_NUM_MAX, &ap_num, is_ap_sort_by_signal_strength);
            if (RETURN_OK == ret) {
                printf("get scan results successfully, ssid number is %d \n", ap_num);
                for (int i = 0; i < ap_num; ++i) {
                    printf("ap %d: ssid is [%s], strength is [%d], level info is: [%s]\n", i, \
                    wifi_ap_array[i].ssid, wifi_ap_array[i].signal_strength, wpa_wifi_signal_level[wifi_ap_array[i].signal_level]);
                }
            } else {
                printf("get scan results fail\n");
            }
            break;
        case 4:
            wpa_wifi_send_reconnect_cmd();
            break;
        case 5:
            wpa_wifi_get_current_wifi_status(&cur_status);
            printf("current wifi connection status is %s\n", wpa_wifi_connection_status_strings[cur_status]);
            break;
        case 6:
            wpa_wifi_get_current_scan_status(&cur_status);
            printf("current wifi scan status is %s\n", wpa_wifi_scan_status_strings[cur_status]);
            break;
        case 7:
            wpa_wifi_send_status_cmd();
            break;
        default:
            printf("Option [%d] not supported \n",select);
            continue;
        }
    }
}