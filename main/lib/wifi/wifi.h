#pragma once

#include "esp_err.h"

// 网络信息结构体
struct NetworkInfo {
    char ssid[32];
    char mac[18];
    char ip[16];
    char netmask[16];
    char gateway[16];
    char dns1[16];
    char dns2[16];
};

void wifi_init_sta(void);
const NetworkInfo* get_network_info(void);
