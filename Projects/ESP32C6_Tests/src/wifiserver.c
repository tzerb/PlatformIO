#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_http_server.h"

#define WIFI_SSID      "Tzerb"
#define WIFI_PASS      "3wbwr11zgt"
#define LED_PIN        GPIO_NUM_15

static const char *TAG = "webserver";
static bool led_state = false;

// HTML page
static const char *HTML_PAGE = 
    "<!DOCTYPE html><html><head>"
    "<title>XIAO ESP32C6</title>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{font-family:sans-serif;text-align:center;margin-top:50px;}"
    "button{font-size:24px;padding:20px 40px;margin:10px;cursor:pointer;}"
    ".on{background:#4CAF50;color:white;}"
    ".off{background:#f44336;color:white;}"
    "</style></head><body>"
    "<h1>XIAO ESP32C6</h1>"
    "<p>LED is: <strong>%s</strong></p>"
    "<a href='/on'><button class='on'>ON</button></a>"
    "<a href='/off'><button class='off'>OFF</button></a>"
    "</body></html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    char response[800];
    snprintf(response, sizeof(response), HTML_PAGE, led_state ? "ON" : "OFF");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

static esp_err_t on_handler(httpd_req_t *req)
{
    led_state = true;
    gpio_set_level(LED_PIN, 1);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t off_handler(httpd_req_t *req)
{
    led_state = false;
    gpio_set_level(LED_PIN, 0);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_handler };
        httpd_uri_t on = { .uri = "/on", .method = HTTP_GET, .handler = on_handler };
        httpd_uri_t off = { .uri = "/off", .method = HTTP_GET, .handler = off_handler };
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &on);
        httpd_register_uri_handler(server, &off);
        ESP_LOGI(TAG, "Web server started");
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        start_webserver();
    }
}

static void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to %s...", WIFI_SSID);
}
#define YELLOW_LED_PIN  GPIO_NUM_15
void app_main(void)
{
    printf("Started\n");
    gpio_set_level(YELLOW_LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(YELLOW_LED_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(YELLOW_LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(500));    
    // Initialize NVS (required for Wi-Fi)
    nvs_flash_init();

    // Initialize LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);

    // Connect to Wi-Fi
    wifi_init();
}