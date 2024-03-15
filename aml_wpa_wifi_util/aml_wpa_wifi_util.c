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
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>

#include "aml_wpa_wifi_util.h"
#include "aml_log.h"

#define MAX_LINE_LENGTH 256
static WPA_WIFI_AP_INFO wpa_wifi_array[WPA_AP_NUM_MAX];
static WPA_WIFI_MANAGER g_wpa_manager;

static int sorted_by_ssid_dictionary(const void *a, const void *b) {
    const WPA_WIFI_AP_INFO *ap_info_a = (const WPA_WIFI_AP_INFO *)a;
    const WPA_WIFI_AP_INFO *ap_info_b = (const WPA_WIFI_AP_INFO *)b;

    return strcasecmp(ap_info_a->ssid, ap_info_b->ssid);
}

static int sorted_by_signal_strength(const void *a, const void *b) {
    const WPA_WIFI_AP_INFO *ap_info_a = (const WPA_WIFI_AP_INFO *)a;
    const WPA_WIFI_AP_INFO *ap_info_b = (const WPA_WIFI_AP_INFO *)b;

    return ap_info_b->signal_strength  - ap_info_a->signal_strength ;
}

static int parse_scan_results(char *res_buf, int is_sort_by_strength) {
    int count = 0, token;
    char signal_str[64];
    char *start_ptr, *end_ptr;

    if (!res_buf)
        return RETURN_ERR;

#define SCAN_HEADER "bssid / frequency / signal level / flags / ssid"
    if (strncmp(res_buf, SCAN_HEADER, sizeof(SCAN_HEADER) - 1) != 0) {
        AML_LOGE("Scan result header mismatch, result format might have changed, [%s]\n", res_buf);
        return RETURN_ERR;
    }
    // Skip first line (Header)
    if ((start_ptr = strchr(res_buf, '\n')) == NULL)
        return RETURN_ERR;
    start_ptr++;
// get next token separated by delimeter, copy a string from current position upto position of 'end'
#define COPY_NEXT_TOKEN(output, start, end, delim)                               \
    {                                                                            \
        if (token > 0) {                                                         \
            end = strchr(start, delim);                                          \
            if (!end) {                                                          \
                break;                                                           \
            }                                                                     \
        }                                                                         \
        size_t length = end - start;                                             \
        if (length >= sizeof(output)) {                                          \
            break;                                                               \
        }                                                                         \
        memcpy(output, start, length);                                           \
        output[length] = '\0';                                                   \
        start = end + 1;                                                         \
        token++;                                                                  \
    }

    // Parse scan results
    while ((end_ptr = strchr(start_ptr, '\t')) != NULL) {
        token = 0;

        /* BSSID */
        COPY_NEXT_TOKEN(wpa_wifi_array[count].bssid, start_ptr, end_ptr, '\t');

        /* Frequency */
        COPY_NEXT_TOKEN(signal_str, start_ptr, end_ptr, '\t');
        wpa_wifi_array[count].frequency = atoi(signal_str);

        /* Signal Strength */
        COPY_NEXT_TOKEN(signal_str, start_ptr, end_ptr, '\t');

        /* Signal Level */
        wpa_wifi_array[count].signal_strength  = atoi(signal_str);
        if (wpa_wifi_array[count].signal_strength <= -88) {
            wpa_wifi_array[count].signal_level = WPA_WIFI_LEVEL_0;
        } else if (wpa_wifi_array[count].signal_strength <= -77) {
            wpa_wifi_array[count].signal_level = WPA_WIFI_LEVEL_1;
        } else if (wpa_wifi_array[count].signal_strength <= -66) {
            wpa_wifi_array[count].signal_level = WPA_WIFI_LEVEL_2;
        } else if (wpa_wifi_array[count].signal_strength <= -55) {
            wpa_wifi_array[count].signal_level = WPA_WIFI_LEVEL_3;
        } else {
            wpa_wifi_array[count].signal_level = WPA_WIFI_LEVEL_4;
        }

        /* Flags (Encryption Mode) */
        COPY_NEXT_TOKEN(wpa_wifi_array[count].flags, start_ptr, end_ptr, '\t');

        /* SSID */
        COPY_NEXT_TOKEN(wpa_wifi_array[count].ssid, start_ptr, end_ptr, '\n');

        if (count >= WPA_AP_NUM_MAX - 1) {
            AML_LOGW("WARNING - Scan reached Maximum supported AP (%d), stopping...\n", WPA_AP_NUM_MAX);
            break;
        }
        count++;
    }

    // Sort by strength or by ssid
    if (is_sort_by_strength) {
        qsort(wpa_wifi_array, count, sizeof(WPA_WIFI_AP_INFO), sorted_by_signal_strength);
    } else {
        qsort(wpa_wifi_array, count, sizeof(WPA_WIFI_AP_INFO), sorted_by_ssid_dictionary);
    }

    AML_LOGI("Total scan AP count=%d\n", count);
    return count;
}

static int extract_psk_from_wpa_conf(const char *filename, const char *target_ssid, char *output_psk) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        AML_LOGE("Error opening %s file\n", filename);
        return RETURN_ERR;
    }

    char line[MAX_LINE_LENGTH];
    char current_ssid[WPA_SSID_SIZE_MAX];
    char current_psk[WPA_PSK_SIZE_MAX];

    output_psk[0] = '\0';

    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        if (strstr(line, "network={") != NULL) {
            current_ssid[0] = '\0';
            current_psk[0] = '\0';

            while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
                if (strstr(line, "ssid=") != NULL) {
                    sscanf(line, " ssid=\"%[^\"]\"", current_ssid);
                }
                else if (strstr(line, "psk=") != NULL) {
                    sscanf(line, " psk=\"%[^\"]\"", current_psk);
                }
                else if (strstr(line, "}") != NULL) {
                    break;
                }
            }
            if (strcmp(current_ssid, target_ssid) == 0) {
                strcpy(output_psk, current_psk);
                return RETURN_OK;
            }
        }
    }
    fclose(file);
    return RETURN_ERR;
}

static int send_wpa_cli_command(char *reply, size_t reply_len, const char *cmd, ...) {
    char cmd_buf[WPA_SUP_CMD_MAX];
    int ret;

    va_list argp;
    va_start(argp, cmd);
    vsnprintf(cmd_buf, sizeof(cmd_buf), cmd, argp);
    va_end(argp);

   /* clear previous result string. */
    memset(reply, 0, reply_len);

    if (!g_wpa_manager.ctrl_handle) {
        AML_LOGE("%s: cmd=%s, error=init_err\n",__func__,cmd_buf);
        return WPA_SUP_CMD_INIT_ERR;
    }

    ret = wpa_ctrl_request(g_wpa_manager.ctrl_handle, cmd_buf, strlen(cmd_buf), reply, &reply_len, NULL);

    if (ret == WPA_SUP_CMD_TIMEOUT) {
        AML_LOGE("%s : cmd=%s, error=timeout \n", __func__, cmd_buf);
        return WPA_SUP_CMD_TIMEOUT;
    } else if (ret < 0) {
        AML_LOGE("%s : cmd=%s, error=failed \n", __func__, cmd_buf);
        return WPA_SUP_CMD_FAILED;
    }
    AML_LOGD("send wpa cmd is %s \n", cmd_buf);
    return WPA_SUP_CMD_SUCCESSFUL;
}

int wpa_wifi_send_scan_cmd() {
    char result[128];
    int ret;
    int result_len = sizeof(result)-1;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    if (g_wpa_manager.cur_scan_status == WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED) {
        AML_LOGD("Scan is finish, please firstly get the last scan results \n");
        pthread_mutex_unlock(&g_wpa_manager.sup_lock);
        return RETURN_OK;
    }
    ret = send_wpa_cli_command(result, result_len, "SCAN");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (ret) {
        AML_LOGE("send wpa cmd SCAN fail\n");
        return ret;
    }
    while (strstr(result, "FAIL-BUSY") != NULL) {
        result_len = sizeof(result)-1;
        AML_LOGD("scan command returned %s .. waiting \n", result);
        pthread_mutex_lock(&g_wpa_manager.sup_lock);
        sleep(1);
        send_wpa_cli_command(result, result_len, "SCAN");
        pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    }

    AML_LOGI("send wpa cmd SCAN successfully, return msg is %s\n", result);
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    g_wpa_manager.cur_scan_status = WPA_WIFI_SCAN_STATE_CMD_SENT;
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    return ret;
}

int wpa_wifi_send_status_cmd() {
    char result[512];
    int ret;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, "STATUS");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd STATUS successfully, scurrent status is :\n %s\n", result);
    } else {
        AML_LOGE("send wpa cmd STATUS fail\n");
    }
    return ret;
}

int wpa_wifi_send_save_config_cmd() {
    char result[64];
    int ret;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, "SAVE_CONFIG");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd SAVE_CONFIG successfully\n");
    } else {
        AML_LOGE("send wpa cmd SAVE_CONFIG fail\n");
    }
    return ret;
}

int wpa_wifi_send_enable_cmd(int network_id) {
    char result[64];
    int ret;
    char cmd[256];
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "ENABLE_NETWORK %d", network_id);
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, cmd);
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd ENABLE_NETWORK successfully\n");
    } else {
        AML_LOGE("send wpa cmd ENABLE_NETWORK fail\n");
    }
    return ret;
}

int wpa_wifi_send_disable_cmd(int network_id) {
    char result[64];
    int ret;
    char cmd[256];
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "DISABLE_NETWORK %d", network_id);
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, cmd);
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd DISABLE_NETWORK successfully\n");
    } else {
        AML_LOGE("send wpa cmd DISABLE_NETWORK fail\n");
    }
    return ret;
}

int wpa_wifi_send_disconnect_cmd() {
    char result[64];
    int ret;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, "DISCONNECT");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd DISCONNECT successfully\n");
        pthread_mutex_lock(&g_wpa_manager.sup_lock);
        g_wpa_manager.cur_wifi_status = WPA_WIFI_DISCONNECTED;
        pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    } else {
        AML_LOGE("send wpa cmd DISCONNECT fail\n");
    }
    return ret;
}

int wpa_wifi_send_reconnect_cmd() {
    char result[64];
    int ret;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, "RECONNECT");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("send wpa cmd RECONNECT successfully\n");
    } else {
        AML_LOGE("send wpa cmd RECONNECT fail\n");
    }
    return ret;
}

int wpa_wifi_send_connect_cmd(const char* ssid, const char* password) {
    char result[64];
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    g_wpa_manager.cur_wifi_status = WPA_WIFI_INVALID;
    send_wpa_cli_command(result, sizeof(result)-1, "REMOVE_NETWORK %d", g_wpa_manager.cur_enable_network_id);
    send_wpa_cli_command(result, sizeof(result)-1, "ADD_NETWORK");

    // set ssid
    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d ssid \"%s\"", g_wpa_manager.cur_enable_network_id, ssid);
    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d key_mgmt WPA-PSK", g_wpa_manager.cur_enable_network_id);
    // set psk
    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d psk \"%s\"", g_wpa_manager.cur_enable_network_id, password);

    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d pairwise CCMP TKIP", g_wpa_manager.cur_enable_network_id);
    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d group CCMP TKIP", g_wpa_manager.cur_enable_network_id);
    send_wpa_cli_command(result, sizeof(result)-1, "SET_NETWORK %d proto RSN", g_wpa_manager.cur_enable_network_id);

    send_wpa_cli_command(result, sizeof(result)-1, "ENABLE_NETWORK %d", g_wpa_manager.cur_enable_network_id);
    send_wpa_cli_command(result, sizeof(result)-1, "REASSOCIATE");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);

    return RETURN_OK;
}

/*
    Monitoring thread, sends state messages to wifi service manager
*/
static void* wpa_wifi_event_monitor_thread() {
    char *start;
    char event_buf[512];
    size_t event_buf_len;
    char result_buf[4096];
    while (!g_wpa_manager.monitor_is_stop && g_wpa_manager.ctrl_handle) {
        if (wpa_ctrl_pending(g_wpa_manager.ctrl_handle) > 0) {
            memset(event_buf, 0, sizeof(event_buf));
            memset(result_buf, 0, sizeof(result_buf));

            event_buf_len = sizeof(event_buf) - 1;
            if (0 == wpa_ctrl_recv(g_wpa_manager.ctrl_handle, event_buf, &event_buf_len)) {
                start = strchr(event_buf, '>') + 1;
                if (start == NULL) continue;
                AML_LOGI("Received event (length = %d) message: %s\n", (int)event_buf_len, start);

                if (strstr(start, WPA_EVENT_SCAN_STARTED) != NULL) {
                    AML_LOGI("Scanning started \n");

                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // Only actively triggered scans will Flush the BSS
                    if (g_wpa_manager.cur_scan_status == WPA_WIFI_SCAN_STATE_CMD_SENT) {
                        /* Flush the BSS everytime so that there is no stale information */
                        AML_LOGI("Flushing the BSS now\n");
                        send_wpa_cli_command(result_buf, sizeof(result_buf)-1, "BSS_FLUSH %d", g_wpa_manager.cur_enable_network_id);
                        g_wpa_manager.cur_scan_status = WPA_WIFI_SCAN_STATE_STARTED;
                    }
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                }
                else if (strstr(start, WPA_EVENT_SCAN_RESULTS) != NULL) {
                    AML_LOGI("Scanning results received \n");
                    if (g_wpa_manager.cur_scan_status == WPA_WIFI_SCAN_STATE_STARTED) {
                        pthread_mutex_lock(&g_wpa_manager.sup_lock);
                        g_wpa_manager.cur_scan_status = WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED;
                        pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    }
                }
                else if ((strstr(start, WPS_EVENT_AP_AVAILABLE_PBC) != NULL)){
                    AML_LOGI("WPS Connection in progress\n");
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    g_wpa_manager.cur_wifi_status = WPA_WIFI_CONNECTING;
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                }
                else if (strstr(start, WPS_EVENT_TIMEOUT) != NULL) {
                    AML_LOGI("WPS Connection is timeout\n");
                    char cur_ssid[WPA_SSID_SIZE_MAX];
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // get ssid
                    send_wpa_cli_command(cur_ssid, sizeof(cur_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);
                    g_wpa_manager.cur_wifi_status = WPA_WIFI_ERROR_NOT_FOUND;
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    if (g_wpa_manager.connect_callback) {
                        g_wpa_manager.connect_callback(cur_ssid, &g_wpa_manager.cur_wifi_status);
                    }
                }
                else if (strstr(start, WPS_EVENT_SUCCESS) != NULL) {
                    AML_LOGI("WPS is successful...Associating now\n");
                }
                else if (strstr(start, WPA_EVENT_CONNECTED) != NULL) {
                    AML_LOGI("Authentication completed successfully and data connection enabled\n");
                    char cur_ssid[WPA_SSID_SIZE_MAX];
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // get ssid
                    send_wpa_cli_command(cur_ssid, sizeof(cur_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);
                    if (strcmp(g_wpa_manager.cur_connected_ssid, cur_ssid) != 0) {
                        // mean new ssid connection
                        strcpy(g_wpa_manager.cur_connected_ssid, cur_ssid);
                        AML_LOGD("Update the current connected ssid to %s\n", cur_ssid);
                    }
                    g_wpa_manager.cur_wifi_status = WPA_WIFI_SUCCESS;

                    if (g_wpa_manager.save_when_connected) {
                        send_wpa_cli_command(result_buf, sizeof(result_buf)-1,"SAVE_CONFIG");
                    }
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    if (g_wpa_manager.connect_callback) {
                        g_wpa_manager.connect_callback(cur_ssid, &g_wpa_manager.cur_wifi_status);
                    }
                }
                else if (strstr(start, WPA_EVENT_DISCONNECTED) != NULL) {
                    char cur_ssid[WPA_SSID_SIZE_MAX];
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // get ssid
                    send_wpa_cli_command(cur_ssid, sizeof(cur_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);
                    g_wpa_manager.cur_wifi_status = WPA_WIFI_DISCONNECTED;
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    AML_LOGI("Disconnected from the network:%s\n", cur_ssid);
                    if (g_wpa_manager.connect_callback) {
                        g_wpa_manager.connect_callback(cur_ssid, &g_wpa_manager.cur_wifi_status);
                    }
                }
                else if (strstr(start, WPA_EVENT_TEMP_DISABLED) != NULL){
                    char cur_ssid[WPA_SSID_SIZE_MAX];
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // get ssid
                    send_wpa_cli_command(cur_ssid, sizeof(cur_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);

                    AML_LOGI("Network authentication failure (Incorrect password), ssid is %s\n", cur_ssid);

                    g_wpa_manager.cur_wifi_status = WPA_WIFI_ERROR_INVALID_CREDENTIALS;
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    if (g_wpa_manager.connect_callback) {
                        g_wpa_manager.connect_callback(cur_ssid, &g_wpa_manager.cur_wifi_status);
                    }
                }
                else if (strstr(start, WPA_EVENT_NETWORK_NOT_FOUND) != NULL) {
                    char cur_ssid[WPA_SSID_SIZE_MAX];
                    pthread_mutex_lock(&g_wpa_manager.sup_lock);
                    // get ssid
                    send_wpa_cli_command(cur_ssid, sizeof(cur_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);

                    AML_LOGI("Received a network not found event, ssid is %s\n", cur_ssid);

                    g_wpa_manager.cur_wifi_status = WPA_WIFI_ERROR_NOT_FOUND;
                    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
                    if (g_wpa_manager.connect_callback) {
                        g_wpa_manager.connect_callback(cur_ssid, &g_wpa_manager.cur_wifi_status);
                    }
                }
                else {
                    continue;
                }
            }
        }
        else {
            usleep(WPA_SUP_TIMEOUT);
        }
    }
    return NULL;
}

int wpa_wifi_get_current_wifi_status(WPA_WIFI_STATUS_TYPE* status) {
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    *status = g_wpa_manager.cur_wifi_status;
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    AML_LOGI("get wpa current wifi connection status is %d\n", g_wpa_manager.cur_wifi_status);
    return RETURN_OK;
}

int wpa_wifi_get_current_scan_status(WPA_WIFI_SCAN_STATE* status) {
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    *status = g_wpa_manager.cur_scan_status;
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    AML_LOGI("get wpa current wifi scan status is %d\n", g_wpa_manager.cur_scan_status);
    return RETURN_OK;
}

static int is_ssid_duplicate(WPA_WIFI_AP_INFO wifi_array[], int count, char *ssid) {
    for (int i = 0; i < count; i++) {
        if (strcmp(wifi_array[i].ssid, ssid) == 0) {
            // SSID found in the array, it's a duplicate
            AML_LOGD("ssid [%s] found in the array is a duplicate, the bssid is [%s]\n", ssid, wifi_array[i].bssid);
            return 1;
        }
    }
    return 0;
}

int wpa_wifi_get_scan_results(WPA_WIFI_AP_INFO wifi_array_to_fill[], const int array_size, int* ap_count, int is_sort_by_strength) {
    if (NULL == wifi_array_to_fill) {
        AML_LOGE("Invalid wpa_ctrl connection or wifi_array_to_fill array\n");
        return RETURN_ERR;
    }
    int retry = 0;
    int found_ap_count = 0;
    char long_result_str[4096];
    int result_len=sizeof(long_result_str)-1;
    while ((g_wpa_manager.cur_scan_status != WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED) && (retry++ < 40)) {
        AML_LOGD("Scanning !!!\n");
        usleep(WPA_SUP_TIMEOUT);
    }
    if (g_wpa_manager.cur_scan_status != WPA_WIFI_SCAN_STATE_RESULTS_RECEIVED) {
        AML_LOGE("Getting scan results is out-of-time\n");
        return RETURN_ERR;
    }
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    send_wpa_cli_command(long_result_str, result_len, "SCAN_RESULTS");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);

    // parsing the scan results
    found_ap_count = parse_scan_results(long_result_str, is_sort_by_strength);
    if (RETURN_ERR == found_ap_count) {
        AML_LOGE("Parse scan results fail\n");
        return RETURN_ERR;
    }
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    g_wpa_manager.cur_scan_status = WPA_WIFI_SCAN_STATE_IDLE;
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);

    // extract ssid, which may have duplicates or nulls
    int real_count = 0;
    for (int index = 0; index < found_ap_count && found_ap_count < array_size; ++index) {
        if (strlen(wpa_wifi_array[index].ssid) > 0 && !is_ssid_duplicate(wifi_array_to_fill, real_count, wpa_wifi_array[index].ssid)) {
            strncpy(wifi_array_to_fill[real_count].ssid, wpa_wifi_array[index].ssid, WPA_SSID_SIZE_MAX - 1);
            strncpy(wifi_array_to_fill[real_count].bssid, wpa_wifi_array[index].bssid, WPA_BSSID_SIZE_MAX - 1);
            wifi_array_to_fill[real_count].signal_strength = wpa_wifi_array[index].signal_strength;
            wifi_array_to_fill[real_count].signal_level = wpa_wifi_array[index].signal_level;
            wifi_array_to_fill[real_count].frequency = wpa_wifi_array[index].frequency;
            ++real_count;
        }
    }

    *ap_count = real_count;

    AML_LOGI("The actual ssid scanned was %d, and %d were obtained after de-weighting and de-nulling\n", found_ap_count, real_count);
    return RETURN_OK;
}

int wpa_wifi_get_current_connected_ssid_and_password(char* ssid, char* password) {
    if (ssid == NULL || password == NULL) {
        AML_LOGE("Error: NULL pointer of ssid or password passed.\n");
        return RETURN_ERR;
    }
    char result[512];
    int ret;
    int result_len = sizeof(result)-1;

    // Get current ssid
    if (g_wpa_manager.cur_wifi_status != WPA_WIFI_SUCCESS) {
        AML_LOGW("Current wifi is unconnected \n");
        return RETURN_ERR;
    }
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, result_len, "STATUS");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (ret) {
        AML_LOGE("send wpa cmd STATUS fail\n");
        return ret;
    }
    char* cur_line = strtok(result, "\n");
    while (cur_line != NULL) {
        if (strncmp(cur_line, "ssid=", 5) == 0) {
            char* ssid_start = cur_line + 5;
            char* ssid_end = strchr(ssid_start, '\0');
            if (ssid_end) {
                *ssid_end = '\0'; // Null-terminate the SSID string
                strncpy(ssid, ssid_start, ssid_end - ssid_start + 1);
                break;
            } else {
                AML_LOGE("SSID not found in STATUS command result.\n");
                return RETURN_ERR;
            }
        }
        cur_line = strtok(NULL, "\n");
    }

    if (cur_line == NULL) {
        AML_LOGE("SSID not found in STATUS command result.\n");
        return RETURN_ERR;
    }

    // Read the wpa_supplicant.conf file to find the PSK for the current SSID
    FILE* conf = fopen(WPA_SUPPLICANT_CONF_PATH, "r");
    if (!conf) {
        AML_LOGE("Failed to open wpa_supplicant.conf.\n");
        return RETURN_ERR;
    }

    char line[256];
    int ssid_found = 0;
    while (fgets(line, sizeof(line), conf) != NULL) {
        if (strstr(line, ssid) && strstr(line, "ssid=\"")) {
            ssid_found = 1;
        } else if (ssid_found && strstr(line, "psk=\"")) {
            char* psk_start = strstr(line, "psk=\"") + 5;
            char* psk_end = strchr(psk_start, '\"');
            if (psk_end) {
                *psk_end = '\0';
                strncpy(password, psk_start, psk_end - psk_start + 1);
                fclose(conf);
                return RETURN_OK;
            }
        }
    }

    fclose(conf);
    AML_LOGE("PSK not found for SSID %s.\n", ssid);
    return RETURN_ERR;

}

/*
    Mainly to make sure to get the status of connected, when the wifi is already connected;
    the other statuses don't matter
*/
static int init_cur_wifi_status() {
    char result[512];
    int ret;
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    ret = send_wpa_cli_command(result, sizeof(result)-1, "STATUS");
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);
    if (!ret) {
        AML_LOGI("get wpa wifi STATUS successfully, scurrent status is :\n %s\n", result);
        if (strstr(result, "wpa_state=COMPLETED") != NULL) {
            g_wpa_manager.cur_wifi_status = WPA_WIFI_SUCCESS;
        }
    } else {
        AML_LOGE("get wpa wifi STATUS fail\n");
    }
    return ret;
}

int wpa_wifi_init (const char* wpa_supl_ctrl, wpa_wifi_connect_callback connect_callback, int enable_network_id, int save_when_connected) {
    int retry = 0;
    g_wpa_manager.connect_callback = connect_callback;
    g_wpa_manager.cur_enable_network_id = enable_network_id;
    g_wpa_manager.save_when_connected = save_when_connected;
    g_wpa_manager.monitor_is_stop = 0;
    g_wpa_manager.cur_connected_ssid[0] = '\0';
    g_wpa_manager.ctrl_handle = wpa_ctrl_open(wpa_supl_ctrl);

    while (g_wpa_manager.ctrl_handle == NULL) {
        g_wpa_manager.ctrl_handle = wpa_ctrl_open(wpa_supl_ctrl);
        if (retry++ > MAX_RETRY) break;
        sleep(1);
    }

    init_cur_wifi_status();
    // get current ssid
    pthread_mutex_lock(&g_wpa_manager.sup_lock);
    send_wpa_cli_command(g_wpa_manager.cur_connected_ssid, sizeof(g_wpa_manager.cur_connected_ssid)-1,"GET_NETWORK %d ssid", g_wpa_manager.cur_enable_network_id);
    pthread_mutex_unlock(&g_wpa_manager.sup_lock);

    if (wpa_ctrl_attach(g_wpa_manager.ctrl_handle) != 0) {
        AML_LOGE("wpa_ctrl_attach failed \n");
        return RETURN_ERR;
    }
    if (pthread_create(&g_wpa_manager.monitor_thread_id, NULL, wpa_wifi_event_monitor_thread, NULL) != 0) {
        AML_LOGE("thread creation failed for wpa_wifi_event_monitor_thread() \n");
        return RETURN_ERR;
    }
    return RETURN_OK;
}

int wpa_wifi_uninit(){
    wpa_ctrl_detach(g_wpa_manager.ctrl_handle);
    g_wpa_manager.monitor_is_stop = 1;
    wpa_ctrl_close(g_wpa_manager.ctrl_handle);
    pthread_join(g_wpa_manager.monitor_thread_id, NULL);
    return RETURN_OK;
}