#include "provisioning_portal.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "app_log.h"
#include "captive_dns_server.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "provisioning_form.h"
#include "settings_store.h"
#include "sticky_rtc.h"
#include "time_sync.h"
#include "wifi_service.h"

namespace {

constexpr char kTag[] = "setup_portal";
constexpr size_t kMaxBodyLength = 1024U;
constexpr uint16_t kSetupServerPort = 80U;
constexpr uint32_t kSetupConnectTimeoutMs = 16000U;
constexpr uint32_t kSetupTimeSyncTimeoutMs = 12000U;

constexpr char kSetupPage[] = R"html(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Sticky Salah Wi-Fi</title><style>
:root{color-scheme:light;--ink:#171713;--paper:#f3efe5;--line:#c8bfae;--accent:#174d3b}*{box-sizing:border-box}body{margin:0;background:var(--paper);color:var(--ink);font:16px system-ui,sans-serif}main{max-width:640px;margin:auto;padding:28px 18px 48px}h1{margin:0 0 8px;font:700 34px Georgia,serif}.card{margin-top:20px;padding:22px;border:1px solid var(--line);border-radius:16px;background:#fff;box-shadow:0 8px 28px #3b302010}.head{display:flex;align-items:center;justify-content:space-between;gap:12px}label{display:block;margin:16px 0 6px;font-weight:650}input{width:100%;padding:12px;border:1px solid #aaa;border-radius:8px;font:inherit}.list{max-height:300px;overflow:auto;border:1px solid #aaa;border-radius:9px}.network-option{display:flex;justify-content:space-between;gap:12px;width:100%;margin:0;padding:12px;border:0;border-bottom:1px solid #ddd;background:#fff;color:var(--ink);font:inherit;text-align:left}.network-option[aria-selected=true]{background:#e2eee8;color:var(--accent)}.name{overflow:hidden;font-weight:650;text-overflow:ellipsis;white-space:nowrap}.meta,.empty,small{color:#666;font-size:13px}.empty{padding:14px}.primary,.secondary{padding:13px;border-radius:9px;font:700 16px system-ui}.primary{width:100%;margin-top:22px;border:0;background:var(--accent);color:#fff}.secondary{border:1px solid var(--accent);background:#fff;color:var(--accent)}.status{min-height:24px;font-weight:650}.error{color:#a12619}.done{padding:26px;border:1px solid var(--accent);border-radius:16px;background:#e2eee8}</style></head><body><main><h1>Sticky Salah</h1><p>Connect the device to Wi-Fi.</p><form id="wifi" class="card" method="post" action="/save_wifi" novalidate><div class="head"><label id="network_label">Nearby Wi-Fi networks</label><button id="rescan" class="secondary" type="button">Scan again</button></div><div id="network_list" class="list" role="listbox" aria-labelledby="network_label" aria-busy="true"><p class="empty">Scanning nearby networks...</p></div><label for="ssid">Wi-Fi name</label><input id="ssid" name="ssid" type="text" maxlength="32" autocomplete="off" placeholder="Select above or enter a network name"><small>Select a scanned network or type a hidden network name.</small><label for="password">Wi-Fi password</label><input id="password" name="password" type="password" maxlength="64" autocomplete="new-password"><p id="status" class="status" role="status"></p><button id="submit" class="primary" type="submit">Check and save</button></form><section id="done" class="done" hidden><h2>Wi-Fi connected</h2><p>Continue setup on the Sticky screen. The device will restart automatically.</p></section></main><script>
const q=id=>document.getElementById(id);function status(m,e=false){q('status').textContent=m;q('status').className='status '+(e?'error':'')}function sync(){for(const o of q('network_list').querySelectorAll('.network-option'))o.setAttribute('aria-selected',String(o.dataset.ssid===q('ssid').value))}function selectNetwork(s){q('ssid').value=s;sync()}function message(m){const p=document.createElement('p');p.className='empty';p.textContent=m;q('network_list').replaceChildren(p)}function addNetworkOption(i){const b=document.createElement('button'),n=document.createElement('span'),m=document.createElement('span');b.type='button';b.className='network-option';b.dataset.ssid=i.ssid;b.setAttribute('role','option');n.className='name';n.textContent=i.ssid;m.className='meta';m.textContent=`${i.rssi} dBm · ${i.secure?'Locked':'Open'}`;b.append(n,m);b.addEventListener('click',()=>selectNetwork(i.ssid));q('network_list').append(b)}async function scanNetworks(){q('rescan').disabled=true;message('Scanning nearby networks...');try{const r=await fetch('/api/wifi/scan',{cache:'no-store'}),d=await r.json();if(!r.ok||!Array.isArray(d.networks))throw Error();q('network_list').replaceChildren();for(const i of d.networks)addNetworkOption(i);if(!d.networks.length)message('No visible networks found. Enter the name below.');sync();status(`${d.networks.length} networks found.`)}catch(e){message('Scan failed. Enter the Wi-Fi name below.');status('Wi-Fi scan failed. Manual entry is still available.',true)}finally{q('rescan').disabled=false}}q('ssid').addEventListener('input',sync);q('rescan').addEventListener('click',scanNetworks);q('wifi').addEventListener('submit',async e=>{e.preventDefault();if(!q('ssid').value.trim()){status('Choose or enter a Wi-Fi network name.',true);q('ssid').focus();return}q('submit').disabled=true;status('Connecting and checking internet access...');try{const r=await fetch('/save_wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(new FormData(e.target))}),d=await r.json();if(!r.ok||!d.ok)throw Error(d.message);q('wifi').hidden=true;q('done').hidden=false}catch(x){status(x.message||'Wi-Fi connection failed.',true);q('submit').disabled=false}});scanNetworks();
</script></body></html>)html";

struct PortalContext {
    std::atomic<bool> saved{false};
    SalahSettings settings = {};
};

esp_err_t send_json_result(httpd_req_t *request, bool ok, const char *message)
{
    char response[320] = {};
    const int length = std::snprintf(
        response, sizeof(response), "{\"ok\":%s,\"message\":\"%s\"}",
        ok ? "true" : "false", message == nullptr ? "" : message);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(response)) {
        return ESP_ERR_INVALID_SIZE;
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, response, length);
}

bool json_escape(const char *source, char *output, size_t capacity)
{
    if (source == nullptr || output == nullptr || capacity == 0U) return false;
    size_t written = 0U;
    for (size_t index = 0U; source[index] != '\0'; ++index) {
        const unsigned char value = static_cast<unsigned char>(source[index]);
        const char *escape = nullptr;
        if (value == '"') escape = "\\\"";
        else if (value == '\\') escape = "\\\\";
        else if (value == '\n') escape = "\\n";
        else if (value == '\r') escape = "\\r";
        else if (value == '\t') escape = "\\t";
        if (escape != nullptr) {
            if (written + 2U >= capacity) return false;
            output[written++] = escape[0];
            output[written++] = escape[1];
        } else if (value < 0x20U) {
            if (written + 6U >= capacity) return false;
            const int length = std::snprintf(
                output + written, capacity - written, "\\u%04X", value);
            if (length != 6) return false;
            written += 6U;
        } else {
            if (written + 1U >= capacity) return false;
            output[written++] = static_cast<char>(value);
        }
    }
    output[written] = '\0';
    return true;
}

bool receive_form(httpd_req_t *request, char *body, size_t capacity)
{
    if (request == nullptr || body == nullptr || capacity == 0U ||
        request->content_len <= 0 ||
        request->content_len >= static_cast<int>(capacity)) return false;
    size_t received = 0U;
    while (received < static_cast<size_t>(request->content_len)) {
        const int result = httpd_req_recv(
            request, body + received,
            static_cast<size_t>(request->content_len) - received);
        if (result <= 0) return false;
        received += static_cast<size_t>(result);
    }
    body[received] = '\0';
    return true;
}

esp_err_t wifi_scan_get_handler(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=GET path=%s", request->uri);
    WifiScanList list = {};
    if (wifi_service_scan_networks(list) != ESP_OK) {
        return send_json_result(request, false,
                                "Nearby Wi-Fi networks could not be scanned.");
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    esp_err_t result = httpd_resp_send_chunk(
        request, "{\"networks\":[", HTTPD_RESP_USE_STRLEN);
    bool first = true;
    for (size_t index = 0U; result == ESP_OK && index < list.count; ++index) {
        char ssid[200] = {};
        char item[280] = {};
        if (!json_escape(list.items[index].ssid, ssid, sizeof(ssid))) continue;
        const int length = std::snprintf(
            item, sizeof(item),
            "%s{\"ssid\":\"%s\",\"rssi\":%d,\"secure\":%s}",
            first ? "" : ",", ssid,
            static_cast<int>(list.items[index].rssi),
            list.items[index].secure ? "true" : "false");
        if (length <= 0 || static_cast<size_t>(length) >= sizeof(item)) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }
        result = httpd_resp_send_chunk(request, item, length);
        first = false;
    }
    if (result == ESP_OK) {
        result = httpd_resp_send_chunk(request, "]}", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_send_chunk(request, nullptr, 0U);
    return result;
}

esp_err_t send_setup_page(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=GET path=%s", request->uri);
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, kSetupPage, HTTPD_RESP_USE_STRLEN);
}

esp_err_t wifi_save_handler(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=POST path=%s", request->uri);
    auto *context = static_cast<PortalContext *>(request->user_ctx);
    if (context == nullptr) return send_json_result(request, false, "Invalid Wi-Fi form.");
    char body[kMaxBodyLength + 1U] = {};
    if (!receive_form(request, body, sizeof(body))) {
        return send_json_result(request, false, "The Wi-Fi form could not be read.");
    }
    SalahSettings wifi = {};
    char error[128] = {};
    if (!provisioning_wifi_form_parse(body, wifi, error, sizeof(error))) {
        return send_json_result(request, false, error);
    }
    if (wifi_service_validate_setup_connection(wifi, kSetupConnectTimeoutMs) != ESP_OK) {
        return send_json_result(request, false,
            "The selected Wi-Fi network could not be reached. Check its password and try again.");
    }
    SalahSettings next = context->settings;
    std::snprintf(next.wifi_ssid, sizeof(next.wifi_ssid), "%s", wifi.wifi_ssid);
    std::snprintf(next.wifi_password, sizeof(next.wifi_password), "%s", wifi.wifi_password);
    if (!next.configured) next.location_confirmed = false;
    StickyRtcDateTime utc_time = {};
    if (time_sync_from_network(0, utc_time, kSetupTimeSyncTimeoutMs) != ESP_OK) {
        return send_json_result(request, false,
            "Wi-Fi connected, but internet time could not be reached. Check the network connection.");
    }
    if (settings_store_save(next) != ESP_OK) {
        return send_json_result(request, false,
                                "The device could not save the Wi-Fi settings.");
    }
    context->settings = next;
    const esp_err_t result = send_json_result(
        request, true, "Wi-Fi saved. Continue setup on the Sticky screen.");
    if (result == ESP_OK) context->saved.store(true, std::memory_order_release);
    return result;
}

esp_err_t health_get_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_sendstr(request, "ok");
}

bool portal_self_test()
{
    const int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (socket_fd < 0) return false;
    const timeval timeout = {2, 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(kSetupServerPort);
    address.sin_addr.s_addr = inet_addr("192.168.4.1");
    bool healthy = connect(socket_fd, reinterpret_cast<sockaddr *>(&address),
                           sizeof(address)) == 0;
    constexpr char kRequest[] = "GET /health HTTP/1.0\r\nHost: 192.168.4.1\r\n\r\n";
    if (healthy) {
        healthy = send(socket_fd, kRequest, sizeof(kRequest) - 1U, 0) ==
                  static_cast<ssize_t>(sizeof(kRequest) - 1U);
    }
    char response[160] = {};
    if (healthy) {
        const ssize_t received = recv(socket_fd, response, sizeof(response) - 1U, 0);
        healthy = received > 0 && std::strstr(response, "200 OK") != nullptr;
    }
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    return healthy;
}

}  // namespace

esp_err_t provisioning_portal_run(const SalahSettings *existing_settings,
                                  SalahSettings &saved_settings)
{
    app_log_register_tag(kTag);
    PortalContext context = {};
    settings_store_defaults(context.settings);
    if (existing_settings != nullptr && settings_wifi_valid(*existing_settings)) {
        context.settings = *existing_settings;
    }
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = kSetupServerPort;
    config.stack_size = 8192U;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server = nullptr;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), kTag,
                        "start setup web server");

    httpd_uri_t wifi_save_uri = {};
    wifi_save_uri.uri = "/save_wifi";
    wifi_save_uri.method = HTTP_POST;
    wifi_save_uri.handler = wifi_save_handler;
    wifi_save_uri.user_ctx = &context;
    esp_err_t result = httpd_register_uri_handler(server, &wifi_save_uri);
    httpd_uri_t wifi_scan_uri = {};
    wifi_scan_uri.uri = "/api/wifi/scan";
    wifi_scan_uri.method = HTTP_GET;
    wifi_scan_uri.handler = wifi_scan_get_handler;
    wifi_scan_uri.user_ctx = &context;
    if (result == ESP_OK) result = httpd_register_uri_handler(server, &wifi_scan_uri);
    httpd_uri_t health_uri = {};
    health_uri.uri = "/health";
    health_uri.method = HTTP_GET;
    health_uri.handler = health_get_handler;
    health_uri.user_ctx = &context;
    if (result == ESP_OK) result = httpd_register_uri_handler(server, &health_uri);
    httpd_uri_t page_uri = {};
    page_uri.uri = "/*";
    page_uri.method = HTTP_GET;
    page_uri.handler = send_setup_page;
    page_uri.user_ctx = &context;
    if (result == ESP_OK) result = httpd_register_uri_handler(server, &page_uri);
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }
    result = captive_dns_server_start();
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!portal_self_test()) {
        STICKY_LOGW(kTag,
                    "portal=self_test address=http://192.168.4.1/health result=failed");
        captive_dns_server_stop();
        httpd_stop(server);
        return ESP_ERR_INVALID_RESPONSE;
    }
    STICKY_LOGI(kTag,
                "portal=self_test address=http://192.168.4.1/health result=ok");
    STICKY_LOGI(kTag, "portal=ready address=http://192.168.4.1/ result=ok");
    while (!context.saved.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(750));
    captive_dns_server_stop();
    httpd_stop(server);
    saved_settings = context.settings;
    return ESP_OK;
}
