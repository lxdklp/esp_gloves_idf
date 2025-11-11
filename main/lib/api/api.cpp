#include <iostream>
#include <cstring>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "soc/rtc.h"
#include "api.h"
#include "mpu/mpu.h"
#include "wifi/wifi.h"
// 软件版本定义
#define SOFTWARE_VERSION "1.0.0"
#define SOFTWARE_NAME "ESP Gloves"

static httpd_handle_t server = NULL;

// root
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* response = "esp gloves";
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    std::cout << "收到 GET 请求 / : " << response << std::endl;
    return ESP_OK;
}

// status
static esp_err_t status_get_handler(httpd_req_t *req)
{
    const char* response = "{\"status\": \"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    std::cout << "收到 GET 请求 /v1/status : " << response << std::endl;
    return ESP_OK;
}

// mpu
static esp_err_t mpu_get_handler(httpd_req_t *req)
{
    int *data = mpu();
    char response[128];
    // 格式化成 JSON
    snprintf(
        response, sizeof(response),
        "{"
        "\"mpu1\": [%d, %d, %d],"
        "\"mpu2\": [%d, %d, %d],"
        "\"mpu3\": [%d, %d, %d]"
        "}",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    std::cout << "收到 GET 请求 /v1/mpu : " << response << std::endl;
    return ESP_OK;
}


// info
static esp_err_t info_get_handler(httpd_req_t *req)
{
    // 获取网络信息
    const NetworkInfo* net_info = get_network_info();
    // 获取硬件信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    // 获取 CPU 频率
    rtc_cpu_freq_config_t cpu_freq_config;
    rtc_clk_cpu_freq_get_config(&cpu_freq_config);
    uint8_t cpu_freq = cpu_freq_config.freq_mhz;
    // 获取 RAM 信息
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t min_free_heap = esp_get_minimum_free_heap_size();
    // 获取 Flash 信息
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    const esp_partition_t* app_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    uint32_t app_size = app_partition ? app_partition->size : 0;
    const esp_partition_t* data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    uint32_t data_size = data_partition ? data_partition->size : 0;
    uint32_t total_used = app_size + data_size;
    uint32_t total_available = flash_size - total_used;
    // 响应
    char response[1024];
    // 格式化成 JSON
    snprintf(
        response, sizeof(response),
        "{"
        "\"network\":{"
        "\"mac\":\"%s\","
        "\"ssid\":\"%s\","
        "\"ip\":\"%s\","
        "\"netmask\":\"%s\","
        "\"gateway\":\"%s\","
        "\"dns1\":\"%s\","
        "\"dns2\":\"%s\""
        "},"
        "\"software\":{"
        "\"name\":\"%s\","
        "\"version\":\"%s\","
        "\"idf_version\":\"%s\""
        "},"
        "\"hardware\":{"
        "\"chip\":\"ESP32-C3\","
        "\"cores\":%d,"
        "\"cpu_freq\":%d,"
        "\"revision\":%d,"
        "\"flash\":{"
        "\"total\":%lu,"
        "\"used\":%lu,"
        "\"available\":%lu,"
        "\"app_partition\":%lu,"
        "\"data_partition\":%lu"
        "},"
        "\"ram\":{"
        "\"free_heap\":%lu,"
        "\"min_free_heap\":%lu"
        "},"
        "\"features\":{"
        "\"wifi\":%s,"
        "\"bt\":%s,"
        "\"ble\":%s"
        "}}}",
        net_info->mac,
        net_info->ssid,
        net_info->ip,
        net_info->netmask,
        net_info->gateway,
        net_info->dns1,
        net_info->dns2,
        SOFTWARE_NAME,
        SOFTWARE_VERSION,
        esp_get_idf_version(),
        chip_info.cores,
        cpu_freq,
        chip_info.revision,
        flash_size / 1024,
        total_used / 1024,
        total_available / 1024,
        app_size / 1024,
        data_size / 1024,
        free_heap / 1024,
        min_free_heap / 1024,
        (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "true" : "false",
        (chip_info.features & CHIP_FEATURE_BT) ? "true" : "false",
        (chip_info.features & CHIP_FEATURE_BLE) ? "true" : "false"
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    std::cout << "收到 GET 请求 /v1/info" << std::endl;
    return ESP_OK;
}

// teapot
static esp_err_t teapot_get_handler(httpd_req_t *req)
{
    const char* response = "I'm a teapot";
    httpd_resp_set_status(req, "418 I'm a teapot");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    std::cout << "收到 GET 请求 /teapot" << std::endl;
    return ESP_OK;
}

// API 路由
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t status_uri = {
    .uri       = "/v1/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t mpu1_uri = {
    .uri       = "/v1/mpu",
    .method    = HTTP_GET,
    .handler   = mpu_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t teapot_uri = {
    .uri       = "/teapot",
    .method    = HTTP_GET,
    .handler   = teapot_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t info_uri = {
    .uri       = "/v1/info",
    .method    = HTTP_GET,
    .handler   = info_get_handler,
    .user_ctx  = NULL
};

// 启动 HTTP 服务器
esp_err_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_open_sockets = 7;
    config.backlog_conn = 5;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.keep_alive_enable = false;
    config.keep_alive_idle = 0;
    config.keep_alive_interval = 0;
    config.keep_alive_count = 0;
    std::cout << "尝试在 " << config.server_port << " 端口启动 HTTP 服务器" << std::endl;
    if (httpd_start(&server, &config) == ESP_OK) {
        // 注册 URI 处理器
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &status_uri);
        httpd_register_uri_handler(server, &mpu1_uri);
        httpd_register_uri_handler(server, &info_uri);
        httpd_register_uri_handler(server, &teapot_uri);
        std::cout << "HTTP 服务器启动成功" << std::endl;
        return ESP_OK;
    }
    std::cout << "HTTP 服务器启动失败" << std::endl;
    return ESP_FAIL;
}

// 停止 HTTP 服务器
esp_err_t stop_http_server(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        std::cout << "HTTP 服务器已停止" << std::endl;
    }
    return ESP_OK;
}
