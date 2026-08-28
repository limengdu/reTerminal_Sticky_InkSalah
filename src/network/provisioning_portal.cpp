#include "provisioning_portal.h"

#include <atomic>
#include <cstring>

#include "app_log.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "provisioning_form.h"
#include "wifi_service.h"

namespace {

constexpr char kTag[] = "setup_portal";
constexpr size_t kMaxBodyLength = 1024U;
constexpr uint16_t kSetupServerPort = 80U;

constexpr char kSetupPage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sticky Salah Setup</title><style>
:root{color-scheme:light;--ink:#151515;--paper:#f4f0e8;--line:#c9c1b2;--accent:#174d3b}
*{box-sizing:border-box}body{margin:0;background:var(--paper);color:var(--ink);font:16px system-ui,sans-serif}
main{max-width:620px;margin:auto;padding:32px 20px 48px}h1{font:700 34px Georgia,serif;margin:0 0 8px}
p{line-height:1.5}.card{background:#fff;border:1px solid var(--line);border-radius:16px;padding:22px;margin-top:24px}
label{display:block;font-weight:650;margin:16px 0 6px}input,select{width:100%;padding:12px;border:1px solid #aaa;border-radius:8px;font:inherit}
.row{display:grid;grid-template-columns:1fr 1fr;gap:14px}.check{display:flex;gap:10px;align-items:center;font-weight:500}.check input{width:auto}
button{width:100%;margin-top:22px;padding:13px;border:0;border-radius:9px;background:var(--accent);color:white;font:700 17px system-ui}
small{color:#5b5b5b}@media(max-width:520px){.row{grid-template-columns:1fr}}
</style></head><body><main><h1>Sticky Salah</h1><p>Connect your display to Wi-Fi and choose the prayer timetable used in your area.</p>
<form class="card" method="post" action="/save">
<label for="ssid">Wi-Fi name</label><input id="ssid" name="ssid" maxlength="32" required>
<label for="password">Wi-Fi password</label><input id="password" name="password" type="password" maxlength="64">
<div class="row"><div><label for="city">City</label><input id="city" name="city" value="Jakarta" maxlength="47" required></div>
<div><label for="country">Country</label><input id="country" name="country" value="Indonesia" maxlength="47" required></div></div>
<label for="utc_offset">UTC offset</label><input id="utc_offset" name="utc_offset" value="+07:00" pattern="[+-][0-9]{2}:[0-9]{2}" required><small>Detected from this phone when the page opens.</small>
<div class="row"><div><label for="method">Calculation authority</label><select id="method" name="method">
<option value="20" selected>Indonesia - Kemenag</option><option value="17">Malaysia - JAKIM</option>
<option value="11">Singapore - MUIS</option><option value="4">Makkah - Umm Al-Qura</option>
<option value="3">Muslim World League</option><option value="2">North America - ISNA</option>
<option value="5">Egypt Survey Authority</option><option value="1">Karachi Islamic Sciences</option>
<option value="8">Gulf Region</option><option value="9">Kuwait</option><option value="10">Qatar</option>
<option value="13">Turkey - Diyanet</option><option value="14">Russia</option><option value="15">Moonsighting Committee</option>
<option value="16">Dubai</option><option value="18">Tunisia</option><option value="19">Algeria</option>
<option value="21">Morocco</option><option value="22">Portugal - Lisbon</option><option value="23">Jordan Awqaf</option>
<option value="12">France - UOIF</option><option value="7">Tehran Geophysics</option><option value="0">Shia Ithna-Ashari</option>
</select></div>
<div><label for="school">Asr school</label><select id="school" name="school"><option value="0">Standard</option><option value="1">Hanafi</option></select></div></div>
<label class="check"><input name="sound" type="checkbox">Play a gentle reminder at prayer time</label>
<button type="submit">Save and start</button></form></main><script>
const minutes=-new Date().getTimezoneOffset(),sign=minutes>=0?'+':'-',absolute=Math.abs(minutes);
document.getElementById('utc_offset').value=sign+String(Math.floor(absolute/60)).padStart(2,'0')+':'+String(absolute%60).padStart(2,'0');
</script></body></html>)html";

constexpr char kSavedPage[] = R"html(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Saved</title><style>body{font:18px system-ui;background:#f4f0e8;padding:40px;color:#151515}main{max-width:520px;margin:auto;background:white;padding:28px;border-radius:16px}h1{font:700 32px Georgia,serif}</style></head><body><main><h1>Setup saved</h1><p>Sticky Salah is restarting. You can close this page and reconnect your phone to its usual Wi-Fi network.</p></main></body></html>)html";

struct PortalContext {
    std::atomic<bool> saved{false};
    SalahSettings settings = {};
};

esp_err_t send_setup_page(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=GET path=%s", request->uri);
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, kSetupPage, HTTPD_RESP_USE_STRLEN);
}

esp_err_t setup_get_handler(httpd_req_t *request)
{
    return send_setup_page(request);
}

esp_err_t setup_save_handler(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=POST path=%s", request->uri);
    auto *context = static_cast<PortalContext *>(request->user_ctx);
    if (context == nullptr || request->content_len <= 0 ||
        request->content_len > static_cast<int>(kMaxBodyLength)) {
        httpd_resp_send_err(request,
                            HTTPD_400_BAD_REQUEST,
                            "Invalid setup form.");
        return ESP_FAIL;
    }
    char body[kMaxBodyLength + 1U] = {};
    size_t received = 0U;
    while (received < static_cast<size_t>(request->content_len)) {
        const int result = httpd_req_recv(
            request,
            body + received,
            static_cast<size_t>(request->content_len) - received);
        if (result <= 0) {
            httpd_resp_send_err(request,
                                HTTPD_500_INTERNAL_SERVER_ERROR,
                                "The setup form could not be read.");
            return ESP_FAIL;
        }
        received += static_cast<size_t>(result);
    }
    body[received] = '\0';

    SalahSettings parsed = {};
    char error[128] = {};
    if (!provisioning_form_parse(body, parsed, error, sizeof(error))) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, error);
        return ESP_FAIL;
    }
    const esp_err_t save_result = settings_store_save(parsed);
    if (save_result != ESP_OK) {
        httpd_resp_send_err(request,
                            HTTPD_500_INTERNAL_SERVER_ERROR,
                            "The device could not save the setup.");
        return ESP_FAIL;
    }
    context->settings = parsed;
    context->saved.store(true, std::memory_order_release);
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, kSavedPage, HTTPD_RESP_USE_STRLEN);
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
    if (socket_fd < 0) {
        return false;
    }
    const timeval timeout = {2, 0};
    setsockopt(socket_fd,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &timeout,
               sizeof(timeout));
    setsockopt(socket_fd,
               SOL_SOCKET,
               SO_SNDTIMEO,
               &timeout,
               sizeof(timeout));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(kSetupServerPort);
    address.sin_addr.s_addr = inet_addr("192.168.4.1");
    bool healthy = connect(socket_fd,
                           reinterpret_cast<sockaddr *>(&address),
                           sizeof(address)) == 0;
    constexpr char kRequest[] =
        "GET /health HTTP/1.0\r\nHost: 192.168.4.1\r\n\r\n";
    if (healthy) {
        healthy = send(socket_fd,
                       kRequest,
                       sizeof(kRequest) - 1U,
                       0) == static_cast<ssize_t>(sizeof(kRequest) - 1U);
    }
    char response[160] = {};
    if (healthy) {
        const ssize_t received = recv(
            socket_fd, response, sizeof(response) - 1U, 0);
        healthy = received > 0 &&
                  std::strstr(response, "200 OK") != nullptr;
    }
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    return healthy;
}

}  // namespace

esp_err_t provisioning_portal_run(SalahSettings &saved_settings)
{
    app_log_register_tag(kTag);
    PortalContext context = {};
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = kSetupServerPort;
    config.stack_size = 6144U;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server = nullptr;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config),
                        kTag,
                        "start setup web server");

    httpd_uri_t save_uri = {};
    save_uri.uri = "/save";
    save_uri.method = HTTP_POST;
    save_uri.handler = setup_save_handler;
    save_uri.user_ctx = &context;
    esp_err_t result = httpd_register_uri_handler(server, &save_uri);
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }

    httpd_uri_t health_uri = {};
    health_uri.uri = "/health";
    health_uri.method = HTTP_GET;
    health_uri.handler = health_get_handler;
    health_uri.user_ctx = &context;
    result = httpd_register_uri_handler(server, &health_uri);
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }

    httpd_uri_t page_uri = {};
    page_uri.uri = "/*";
    page_uri.method = HTTP_GET;
    page_uri.handler = setup_get_handler;
    page_uri.user_ctx = &context;
    result = httpd_register_uri_handler(server, &page_uri);
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    if (!portal_self_test()) {
        STICKY_LOGW(kTag,
                    "portal=self_test address=http://192.168.4.1/health result=failed");
        httpd_stop(server);
        return ESP_ERR_INVALID_RESPONSE;
    }

    STICKY_LOGI(kTag,
                "portal=self_test address=http://192.168.4.1/health result=ok");
    STICKY_LOGI(kTag,
                "portal=ready address=http://192.168.4.1/ result=ok");
    while (!context.saved.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(750));
    httpd_stop(server);
    saved_settings = context.settings;
    return ESP_OK;
}
