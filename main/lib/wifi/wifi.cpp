#include <iostream>
#include <cstring>
#include <lwip/ip4_addr.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "wifi.h"

// WiFi 参数
static const char* WIFI_SSID = "IoT";
static const char* WIFI_PASSWORD = "1145141919810";
static const int MAXIMUM_RETRY = 5;

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

// 保存网络信息
static NetworkInfo g_network_info = {};

// WiFi 事件位
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// WiFi 事件处理函数
static void wifi_event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            std::cout << "正在连接 Wi-Fi (" << s_retry_num << "/" << MAXIMUM_RETRY << ")" << std::endl;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            std::cout << "连接 Wi-Fi 失败" << std::endl;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(g_network_info.mac, sizeof(g_network_info.mac),
                "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        // 保存网络信息
        std::strncpy(g_network_info.ssid, WIFI_SSID, sizeof(g_network_info.ssid) - 1);
        std::strncpy(g_network_info.ip, ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.ip), sizeof(g_network_info.ip) - 1);
        std::strncpy(g_network_info.netmask, ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.netmask), sizeof(g_network_info.netmask) - 1);
        std::strncpy(g_network_info.gateway, ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.gw), sizeof(g_network_info.gateway) - 1);
        esp_netif_t* netif = (esp_netif_t*)arg;
        esp_netif_dns_info_t dns_info;
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
            std::strncpy(g_network_info.dns1, ip4addr_ntoa((const ip4_addr_t*)&dns_info.ip.u_addr.ip4), sizeof(g_network_info.dns1) - 1);
        }
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
            std::strncpy(g_network_info.dns2, ip4addr_ntoa((const ip4_addr_t*)&dns_info.ip.u_addr.ip4), sizeof(g_network_info.dns2) - 1);
        }
        // 打印网络信息
        std::cout << "========== Wi-Fi 连接成功 ==========" << std::endl;
        std::cout << "SSID: " << g_network_info.ssid << std::endl;
        std::cout << "MAC:  " << g_network_info.mac << std::endl;
        std::cout << "IP:   " << g_network_info.ip << std::endl;
        std::cout << "子网掩码:  " << g_network_info.netmask << std::endl;
        std::cout << "网关:      " << g_network_info.gateway << std::endl;
        std::cout << "主 DNS:    " << g_network_info.dns1 << std::endl;
        std::cout << "备用 DNS:  " << g_network_info.dns2 << std::endl;
        std::cout << "====================================" << std::endl;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// WiFi 初始化
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, "esp_gloves"));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        sta_netif,
        &instance_any_id
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        sta_netif,
        &instance_got_ip
    ));
    // WiFi 配置
    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    std::cout << "WiFi 初始化完成" << std::endl;
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );
    if (bits & WIFI_CONNECTED_BIT) {
        std::cout << "连接 " << WIFI_SSID << " 成功" << std::endl;
    } else if (bits & WIFI_FAIL_BIT) {
        std::cout << "连接 " << WIFI_SSID << " 失败" << std::endl;
    } else {
        std::cout << "未预期的事件" << std::endl;
    }
}

// 获取网络信息
const NetworkInfo* get_network_info(void)
{
    return &g_network_info;
}
