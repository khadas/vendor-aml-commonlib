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

#ifndef AML_WPA_WIFI_UTIL_H
#define AML_WPA_WIFI_UTIL_H
#include "wpa_ctrl.h"


#ifdef __cplusplus
extern "C" {
#endif

// Default network
#define DEFAULT_WPA_SUPL_CTRL "/var/run/wpa_supplicant/wlan0"
#define DEFAULT_NETWORK_ID 0
#define WPA_SUPPLICANT_CONF_PATH "/etc/wpa_supplicant.conf"

// Return codes
#define RETURN_OK 0
#define RETURN_ERR -1

/*
    Return values for WPA Supplicant command execution
*/
#define WPA_SUP_CMD_SUCCESSFUL 0
#define WPA_SUP_CMD_FAILED -1 // Return value in case WPA command failed
#define WPA_SUP_CMD_TIMEOUT -2 // Return value in case WPA command timed out
#define WPA_SUP_CMD_INIT_ERR -3 // Return value in case wifi_init is not done

#define WPA_SUP_TIMEOUT 500000 // 500 msec
#define WPA_SUP_CMD_MAX 256 // Maximum command size that can be sent to WPA Supplicant
#define WPA_SSID_SIZE_MAX 64 // Maximum SSID length
#define WPA_BSSID_SIZE_MAX 64 // Maximum BSSID length
#define WPA_PSK_SIZE_MAX 64 // Maximum PSK length
#define WPA_AP_NUM_MAX 64 // Maximum number of scanned APs to store

#define MAX_RETRY 10

/*
    Wi-Fi status types
*/
typedef enum {
    WPA_WIFI_INVALID = 0,
    WPA_WIFI_SUCCESS,
    WPA_WIFI_CONNECTING,
    WPA_WIFI_DISCONNECTED,
    WPA_WIFI_ERROR_NOT_FOUND,
    WPA_WIFI_ERROR_INVALID_CREDENTIALS,
    WPA_WIFI_ERROR_TIMEOUT_EXPIRED,
    WPA_WIFI_ERROR_DEV_DISCONNECT,
    WPA_WIFI_ERROR_UNKNOWN
} WPA_WIFI_STATUS_TYPE;

/*
    Wi-Fi scan state types
*/
typedef enum {
    WPA_WIFI_SCAN_STATE_IDLE,
    WPA_WIFI_SCAN_STATE_CMD_SENT,
    WPA_WIFI_SCAN_STATE_STARTED,
    WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED,
} WPA_WIFI_SCAN_STATE;

typedef void(* wpa_wifi_connect_callback) (char *ssid, const WPA_WIFI_STATUS_TYPE *cur_status);

typedef struct {
    struct wpa_ctrl *ctrl_handle;

    // Callback function for Wi-Fi connection events
    wpa_wifi_connect_callback connect_callback;

    int monitor_is_stop;
    pthread_t monitor_thread_id;

    // scan state and wifi connect status
    WPA_WIFI_SCAN_STATE cur_scan_status;
    WPA_WIFI_STATUS_TYPE cur_wifi_status;

    pthread_mutex_t sup_lock;

    // Whether we want to save SSID information to configuration file: 1=save, 0=don't save
    int cur_enable_network_id;
    int save_when_connected;

    char cur_connected_ssid[WPA_SSID_SIZE_MAX];
}WPA_WIFI_MANAGER;

typedef enum {
    WPA_WIFI_LEVEL_0,  // rssi <= -100
    WPA_WIFI_LEVEL_1,  // (-100, -88]
    WPA_WIFI_LEVEL_2,  // (-88, -77]
    WPA_WIFI_LEVEL_3,  // (-66, -55]
    WPA_WIFI_LEVEL_4   // rssi >= -55
} WPA_WIFI_SIGNAL_LEVEL;

typedef struct {
    char bssid[WPA_BSSID_SIZE_MAX];  // BSSID
    int frequency;   // Frequency
    int signal_strength; // Signal Strength
    WPA_WIFI_SIGNAL_LEVEL signal_level; // Signal Level
    char flags[256];  // Flags
    char ssid[WPA_SSID_SIZE_MAX];    // SSID
} WPA_WIFI_AP_INFO;

int wpa_wifi_send_scan_cmd();
int wpa_wifi_send_status_cmd();
int wpa_wifi_send_save_config_cmd();
int wpa_wifi_send_disable_cmd(int network_id);
int wpa_wifi_send_enable_cmd(int network_id);

/*
    Only wpa-psk is supported for now, any legal encryption is supported
*/
int wpa_wifi_send_connect_cmd(const char* ssid, const char* password);
int wpa_wifi_send_disconnect_cmd();
int wpa_wifi_send_reconnect_cmd();

int wpa_wifi_get_current_wifi_status(WPA_WIFI_STATUS_TYPE* status);
int wpa_wifi_get_current_scan_status(WPA_WIFI_SCAN_STATE* status);
int wpa_wifi_get_scan_results(WPA_WIFI_AP_INFO wifi_array_to_fill[], const int array_size, int* ap_count, int is_sort_by_strength);
int wpa_wifi_get_current_connected_ssid_and_password(char* ssid, char* password);

int wpa_wifi_init(const char* wpa_supl_ctrl, wpa_wifi_connect_callback connect_callback, int enable_network_id, int save_when_connected);
int wpa_wifi_uninit();

#ifdef __cplusplus
}
#endif

#endif //AML_WPA_WIFI_UTIL_H