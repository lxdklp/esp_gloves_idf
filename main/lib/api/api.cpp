#include <cstring>
#include <format>
#include <string>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "soc/rtc.h"
#include "api.hpp"
#include "mpu/mpu.hpp"
#include "wifi/wifi.hpp"

#define SOFTWARE_VERSION "1.0.0"
#define SOFTWARE_NAME "ESP Gloves"

static const char *TAG = "API";

static httpd_handle_t server = NULL;

// root
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const std::string response = "esp gloves";
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response.c_str(), response.length());
    ESP_LOGI(TAG, "收到 GET 请求 / : %s", response.c_str());
    return ESP_OK;
}

// status
static esp_err_t status_get_handler(httpd_req_t *req)
{
    const std::string response = "{\"status\": \"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response.c_str(), response.length());
    ESP_LOGI(TAG, "收到 GET 请求 /v1/status : %s", response.c_str());
    return ESP_OK;
}

// mpu
static esp_err_t mpu_get_handler(httpd_req_t *req)
{
    int *data = mpu();
    // 使用 std::format 构建 JSON
    std::string response = std::format(
        "{{"
        "\"mpu1\": [{}, {}, {}],"
        "\"mpu2\": [{}, {}, {}],"
        "\"mpu3\": [{}, {}, {}]"
        "}}",
        data[0], data[1], data[2],
        data[3], data[4], data[5],
        data[6], data[7], data[8]
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response.c_str(), response.length());
    ESP_LOGI(TAG, "收到 GET 请求 /v1/mpu");
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
    uint32_t total_heap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    // 获取 Flash 信息
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    const esp_partition_t* app_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    uint32_t app_size = app_partition ? app_partition->size : 0;
    const esp_partition_t* data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    uint32_t data_size = data_partition ? data_partition->size : 0;
    uint32_t total_used = app_size + data_size;
    uint32_t total_available = flash_size - total_used;
    
    // 使用 std::format 构建 JSON
    std::string response = std::format(
        "{{"
        "\"network\":{{"
        "\"mac\":\"{}\","
        "\"ssid\":\"{}\","
        "\"ip\":\"{}\","
        "\"netmask\":\"{}\","
        "\"gateway\":\"{}\","
        "\"dns1\":\"{}\","
        "\"dns2\":\"{}\""
        "}},"
        "\"software\":{{"
        "\"name\":\"{}\","
        "\"version\":\"{}\","
        "\"idf_version\":\"{}\""
        "}},"
        "\"hardware\":{{"
        "\"chip\":\"ESP32-C3\","
        "\"cores\":{},"
        "\"cpu_freq\":{},"
        "\"revision\":{},"
        "\"flash\":{{"
        "\"total\":{},"
        "\"used\":{},"
        "\"available\":{},"
        "\"app_partition\":{},"
        "\"data_partition\":{}"
        "}},"
        "\"ram\":{{"
        "\"total\":{},"
        "\"free_heap\":{},"
        "\"min_free_heap\":{}"
        "}},"
        "\"features\":{{"
        "\"wifi\":{},"
        "\"bt\":{},"
        "\"ble\":{}"
        "}}}}}}",
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
        total_heap / 1024,
        free_heap / 1024,
        min_free_heap / 1024,
        (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "true" : "false",
        (chip_info.features & CHIP_FEATURE_BT) ? "true" : "false",
        (chip_info.features & CHIP_FEATURE_BLE) ? "true" : "false"
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response.c_str(), response.length());
    ESP_LOGI(TAG, "收到 GET 请求 /v1/info");
    return ESP_OK;
}

// teapot
static esp_err_t teapot_get_handler(httpd_req_t *req)
{
    const std::string response = "I'm a teapot";
    httpd_resp_set_status(req, "418 I'm a teapot");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, response.c_str(), response.length());
    ESP_LOGI(TAG, "收到 GET 请求 /teapot");
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
    ESP_LOGI(TAG, "尝试在 %d 端口启动 HTTP 服务器", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // 注册 URI 处理器
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &status_uri);
        httpd_register_uri_handler(server, &mpu1_uri);
        httpd_register_uri_handler(server, &info_uri);
        httpd_register_uri_handler(server, &teapot_uri);
        ESP_LOGI(TAG, "HTTP 服务器启动成功");
        return ESP_OK;
    }
    ESP_LOGE(TAG, "HTTP 服务器启动失败");
    return ESP_FAIL;
}

// 停止 HTTP 服务器
esp_err_t stop_http_server(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "HTTP 服务器已停止");
    }
    return ESP_OK;
}
