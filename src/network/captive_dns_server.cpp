#include "captive_dns_server.h"

#include <atomic>

#include "app_log.h"
#include "captive_dns_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

namespace {

constexpr char kTag[] = "captive_dns";
constexpr uint16_t kDnsPort = 53U;
constexpr EventBits_t kReadyBit = BIT0;
constexpr EventBits_t kFailedBit = BIT1;
constexpr EventBits_t kStoppedBit = BIT2;
constexpr uint8_t kPortalIp[] = {192U, 168U, 4U, 1U};

EventGroupHandle_t s_events = nullptr;
TaskHandle_t s_task = nullptr;
std::atomic<bool> s_running{false};

void dns_task(void *)
{
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        xEventGroupSetBits(s_events, kFailedBit | kStoppedBit);
        s_task = nullptr;
        vTaskDelete(nullptr);
    }
    const timeval timeout = {0, 250000};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(kDnsPort);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(socket_fd,
             reinterpret_cast<sockaddr *>(&address),
             sizeof(address)) != 0) {
        close(socket_fd);
        xEventGroupSetBits(s_events, kFailedBit | kStoppedBit);
        s_task = nullptr;
        vTaskDelete(nullptr);
    }

    xEventGroupSetBits(s_events, kReadyBit);
    while (s_running.load(std::memory_order_acquire)) {
        uint8_t query[256] = {};
        sockaddr_in client = {};
        socklen_t client_length = sizeof(client);
        const int received = recvfrom(
            socket_fd,
            query,
            sizeof(query),
            0,
            reinterpret_cast<sockaddr *>(&client),
            &client_length);
        if (received <= 0) {
            continue;
        }
        uint8_t response[272] = {};
        const size_t response_length = captive_dns_build_response(
            query,
            static_cast<size_t>(received),
            kPortalIp,
            response,
            sizeof(response));
        if (response_length > 0U) {
            sendto(socket_fd,
                   response,
                   response_length,
                   0,
                   reinterpret_cast<sockaddr *>(&client),
                   client_length);
        }
    }
    close(socket_fd);
    s_task = nullptr;
    xEventGroupSetBits(s_events, kStoppedBit);
    vTaskDelete(nullptr);
}

}  // namespace

esp_err_t captive_dns_server_start()
{
    app_log_register_tag(kTag);
    if (s_running.load(std::memory_order_acquire)) {
        return ESP_OK;
    }
    if (s_events == nullptr) {
        s_events = xEventGroupCreate();
        if (s_events == nullptr) {
            return ESP_ERR_NO_MEM;
        }
    }
    xEventGroupClearBits(s_events, kReadyBit | kFailedBit | kStoppedBit);
    s_running.store(true, std::memory_order_release);
    if (xTaskCreate(dns_task, "captive_dns", 4096U, nullptr, 5U, &s_task) !=
        pdPASS) {
        s_running.store(false, std::memory_order_release);
        return ESP_ERR_NO_MEM;
    }
    const EventBits_t bits = xEventGroupWaitBits(
        s_events,
        kReadyBit | kFailedBit,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(2000));
    if ((bits & kReadyBit) == 0U) {
        s_running.store(false, std::memory_order_release);
        return ESP_FAIL;
    }
    STICKY_LOGI(kTag, "dns=redirect address=192.168.4.1 port=53 result=ok");
    return ESP_OK;
}

void captive_dns_server_stop()
{
    if (!s_running.exchange(false, std::memory_order_acq_rel) ||
        s_events == nullptr) {
        return;
    }
    xEventGroupWaitBits(s_events,
                        kStoppedBit,
                        pdFALSE,
                        pdFALSE,
                        pdMS_TO_TICKS(1000));
}
