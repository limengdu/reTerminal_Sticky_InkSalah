#include "provisioning_portal.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "aladhan_client.h"
#include "app_log.h"
#include "captive_dns_server.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "location_catalog.h"
#include "provisioning_form.h"
#include "sticky_rtc.h"
#include "time_sync.h"
#include "wifi_service.h"

namespace {

constexpr char kTag[] = "setup_portal";
constexpr size_t kMaxBodyLength = 1024U;
constexpr uint16_t kSetupServerPort = 80U;
constexpr uint32_t kSetupConnectTimeoutMs = 16000U;
constexpr uint32_t kSetupTimeSyncTimeoutMs = 12000U;

constexpr char kSetupPage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sticky Salah Setup</title><style>
:root{color-scheme:light;--ink:#151515;--paper:#f4f0e8;--line:#c9c1b2;--accent:#174d3b}
*{box-sizing:border-box}body{margin:0;background:var(--paper);color:var(--ink);font:16px system-ui,sans-serif}
main{max-width:660px;margin:auto;padding:28px 18px 48px}h1{font:700 34px Georgia,serif;margin:0 0 8px}
p{line-height:1.5}.card{background:#fff;border:1px solid var(--line);border-radius:16px;padding:22px;margin-top:20px;box-shadow:0 8px 28px #3b302010}
label{display:block;font-weight:650;margin:16px 0 6px}input,select{width:100%;padding:12px;border:1px solid #aaa;border-radius:8px;background:#fff;font:inherit}
.row{display:grid;grid-template-columns:1fr 1fr;gap:14px}.section-head{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-top:16px}.section-head label{margin:0}
.secondary{width:auto;margin:0;padding:12px 14px;border:1px solid var(--accent);background:white;color:var(--accent)}
.network-list{max-height:220px;margin-top:8px;overflow-y:auto;border:1px solid #aaa;border-radius:9px;background:#fff}
.network-option{display:flex;align-items:center;justify-content:space-between;gap:12px;width:100%;margin:0;padding:12px;border:0;border-bottom:1px solid #ddd;border-radius:0;background:#fff;color:var(--ink);font:inherit;text-align:left}.network-option:last-child{border-bottom:0}.network-option[aria-selected="true"]{background:#e2eee8;color:var(--accent)}
.network-name{overflow:hidden;font-weight:650;text-overflow:ellipsis;white-space:nowrap}.network-meta{flex:none;color:#666;font-size:13px}.network-empty{margin:0;padding:14px;color:#666}
.check{display:flex;gap:10px;align-items:center;font-weight:500}.check input{width:auto}
button{width:100%;margin-top:22px;padding:13px;border:0;border-radius:9px;background:var(--accent);color:white;font:700 17px system-ui}
button:disabled{opacity:.55}.hidden{display:none}.status{min-height:24px;margin:16px 0 0;font-weight:650}.error{color:#a12619}.success{color:var(--accent)}
small{display:block;color:#5b5b5b;margin-top:6px}@media(max-width:520px){.row{grid-template-columns:1fr}}
</style></head><body><main><h1>Sticky Salah</h1><p>Choose a nearby Wi-Fi network and a verified prayer timetable location.</p>
<form id="setup" class="card" method="post" action="/save">
<div class="section-head"><label id="network_label">Nearby Wi-Fi networks</label><button id="rescan" class="secondary" type="button">Scan again</button></div>
<div id="network_list" class="network-list" role="listbox" aria-labelledby="network_label" aria-busy="true"><p class="network-empty">Scanning nearby networks...</p></div>
<label for="ssid">Wi-Fi name</label><input id="ssid" name="ssid" type="text" maxlength="32" required autocomplete="off" placeholder="Select above or enter a network name"><small>Select a network above to fill this field, or type a name manually.</small>
<label for="password">Wi-Fi password</label><input id="password" name="password" type="password" maxlength="64" autocomplete="new-password">
<div class="row"><div><label for="country">Country</label><select id="country" name="country" required><option value="">Loading countries...</option></select></div>
<div><label for="city_select">City</label><select id="city_select" required><option value="">Choose a country first</option></select><input id="city" name="city" type="hidden"><input id="custom_city" class="hidden" maxlength="47" placeholder="Enter another city"></div></div>
<label for="utc_offset">UTC offset</label><select id="utc_offset" name="utc_offset" required></select><small>Detected from this phone. You can adjust it before saving.</small>
<div class="row"><div><label for="method">Calculation authority</label><select id="method" name="method">
<option value="20">Indonesia - Kemenag</option><option value="17">Malaysia - JAKIM</option>
<option value="11">Singapore - MUIS</option><option value="4">Makkah - Umm Al-Qura</option>
<option value="3" selected>Muslim World League</option><option value="2">North America - ISNA</option>
<option value="5">Egypt Survey Authority</option><option value="1">Karachi Islamic Sciences</option>
<option value="8">Gulf Region</option><option value="9">Kuwait</option><option value="10">Qatar</option>
<option value="13">Turkey - Diyanet</option><option value="14">Russia</option><option value="15">Moonsighting Committee</option>
<option value="16">Dubai</option><option value="18">Tunisia</option><option value="19">Algeria</option>
<option value="21">Morocco</option><option value="22">Portugal - Lisbon</option><option value="23">Jordan Awqaf</option>
<option value="12">France - UOIF</option><option value="7">Tehran Geophysics</option><option value="0">Shia Ithna-Ashari</option>
</select></div>
<div><label for="school">Asr school</label><select id="school" name="school"><option value="0">Standard</option><option value="1">Hanafi</option></select></div></div>
<label class="check"><input name="sound" type="checkbox">Play a gentle reminder at prayer time</label>
<p id="status" class="status" role="status"></p><button id="submit" type="submit">Check and save</button></form></main><script>
const q=id=>document.getElementById(id),other='__other__';let countries=[];
function offsetText(minutes){const sign=minutes>=0?'+':'-',value=Math.abs(minutes);return sign+String(Math.floor(value/60)).padStart(2,'0')+':'+String(value%60).padStart(2,'0')}
function loadOffsets(){const detected=-new Date().getTimezoneOffset();for(let value=-720;value<=840;value+=15){const option=new Option(offsetText(value),offsetText(value));option.selected=value===detected;q('utc_offset').add(option)}}
function setStatus(message,error=false){q('status').textContent=message;q('status').className='status '+(error?'error':'')}
function syncSelectedNetwork(){for(const option of q('network_list').querySelectorAll('.network-option'))option.setAttribute('aria-selected',String(option.dataset.ssid===q('ssid').value))}
function selectNetwork(ssid){q('ssid').value=ssid;syncSelectedNetwork()}
function networkMessage(message){const item=document.createElement('p');item.className='network-empty';item.textContent=message;q('network_list').replaceChildren(item)}
function addNetworkOption(item){const option=document.createElement('button'),name=document.createElement('span'),meta=document.createElement('span');option.type='button';option.className='network-option';option.dataset.ssid=item.ssid;option.setAttribute('role','option');option.setAttribute('aria-selected','false');name.className='network-name';name.textContent=item.ssid;meta.className='network-meta';meta.textContent=`${item.rssi} dBm · ${item.secure?'Locked':'Open'}`;option.append(name,meta);option.addEventListener('click',()=>selectNetwork(item.ssid));q('network_list').append(option)}
async function scanNetworks(){q('rescan').disabled=true;q('network_list').setAttribute('aria-busy','true');networkMessage('Scanning nearby networks...');setStatus('Scanning nearby Wi-Fi networks...');try{const response=await fetch('/api/wifi/scan',{cache:'no-store'}),data=await response.json();if(!response.ok||!Array.isArray(data.networks))throw new Error('Invalid scan response');q('network_list').replaceChildren();for(const item of data.networks)addNetworkOption(item);if(data.networks.length===0)networkMessage('No visible networks found. Enter the Wi-Fi name below.');syncSelectedNetwork();setStatus(`${data.networks.length} networks found.`)}catch(error){networkMessage('Wi-Fi scan failed. Enter the Wi-Fi name below or scan again.');setStatus('Wi-Fi scan failed. You can still enter the network name manually.',true)}finally{q('network_list').setAttribute('aria-busy','false');q('rescan').disabled=false}}
function cityChanged(){const custom=q('city_select').value===other;q('custom_city').classList.toggle('hidden',!custom);q('city').value=custom?q('custom_city').value:q('city_select').value}
function countryChanged(){const option=q('country').selectedOptions[0],code=option?.dataset.code,country=countries.find(item=>item.code===code);q('city_select').replaceChildren(new Option('Choose a city',''));for(const city of country?.cities||[])q('city_select').add(new Option(city,city));q('city_select').add(new Option('Other city',other));q('method').value=String(country?.method??3);cityChanged()}
async function loadLocations(){try{const data=await fetch('/api/locations',{cache:'no-store'}).then(r=>r.json());countries=data.countries;q('country').replaceChildren(new Option('Choose a country',''));for(const item of countries){const option=new Option(item.name,item.name);option.dataset.code=item.code;q('country').add(option)}const region=(navigator.language||'').split('-').pop().toUpperCase(),preferred=countries.find(item=>item.code===region)||countries.find(item=>item.code==='ID');q('country').value=preferred?.name||'';countryChanged()}catch(error){setStatus('The country list could not be loaded. Reload this page.',true)}}
q('ssid').addEventListener('input',syncSelectedNetwork);q('rescan').addEventListener('click',scanNetworks);q('country').addEventListener('change',countryChanged);q('city_select').addEventListener('change',cityChanged);q('custom_city').addEventListener('input',cityChanged);
q('setup').addEventListener('submit',async event=>{event.preventDefault();cityChanged();if(!q('ssid').value||!q('city').value){setStatus('Enter a Wi-Fi network name, country, and city.',true);return}q('submit').disabled=true;setStatus('Checking Wi-Fi and prayer time data. This can take about a minute...');try{const response=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(new FormData(event.target))});const result=await response.json();if(!result.ok)throw new Error(result.message);q('setup').innerHTML='<h2>Setup complete</h2><p>Wi-Fi and prayer time data were verified. Sticky Salah is restarting.</p>'}catch(error){setStatus(error.message||'Setup could not be verified. Try again.',true);q('submit').disabled=false}});
loadOffsets();loadLocations();scanNetworks();
</script></body></html>)html";

struct PortalContext {
    std::atomic<bool> saved{false};
    SalahSettings settings = {};
};

esp_err_t send_json_result(httpd_req_t *request,
                           bool ok,
                           const char *message)
{
    char response[256] = {};
    const int length = std::snprintf(
        response,
        sizeof(response),
        "{\"ok\":%s,\"message\":\"%s\"}",
        ok ? "true" : "false",
        message == nullptr ? "" : message);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(response)) {
        return ESP_ERR_INVALID_SIZE;
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, response, length);
}

bool json_escape(const char *source, char *output, size_t capacity)
{
    if (source == nullptr || output == nullptr || capacity == 0U) {
        return false;
    }
    size_t written = 0U;
    for (size_t index = 0U; source[index] != '\0'; ++index) {
        const unsigned char value = static_cast<unsigned char>(source[index]);
        const char *escape = nullptr;
        if (value == '"') {
            escape = "\\\"";
        } else if (value == '\\') {
            escape = "\\\\";
        } else if (value == '\n') {
            escape = "\\n";
        } else if (value == '\r') {
            escape = "\\r";
        } else if (value == '\t') {
            escape = "\\t";
        }
        if (escape != nullptr) {
            if (written + 2U >= capacity) {
                return false;
            }
            output[written++] = escape[0];
            output[written++] = escape[1];
        } else if (value < 0x20U) {
            if (written + 6U >= capacity) {
                return false;
            }
            const int length = std::snprintf(
                output + written, capacity - written, "\\u%04X", value);
            if (length != 6) {
                return false;
            }
            written += 6U;
        } else {
            if (written + 1U >= capacity) {
                return false;
            }
            output[written++] = static_cast<char>(value);
        }
    }
    output[written] = '\0';
    return true;
}

esp_err_t wifi_scan_get_handler(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=GET path=%s", request->uri);
    WifiScanList list = {};
    const esp_err_t scan_result = wifi_service_scan_networks(list);
    if (scan_result != ESP_OK) {
        return send_json_result(
            request, false, "Nearby Wi-Fi networks could not be scanned.");
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    esp_err_t result = httpd_resp_send_chunk(
        request, "{\"networks\":[", HTTPD_RESP_USE_STRLEN);
    for (size_t index = 0U; result == ESP_OK && index < list.count; ++index) {
        char ssid[200] = {};
        char item[280] = {};
        if (!json_escape(list.items[index].ssid, ssid, sizeof(ssid))) {
            continue;
        }
        const int length = std::snprintf(
            item,
            sizeof(item),
            "%s{\"ssid\":\"%s\",\"rssi\":%d,\"secure\":%s}",
            index == 0U ? "" : ",",
            ssid,
            static_cast<int>(list.items[index].rssi),
            list.items[index].secure ? "true" : "false");
        if (length <= 0 || static_cast<size_t>(length) >= sizeof(item)) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }
        result = httpd_resp_send_chunk(request, item, length);
    }
    if (result == ESP_OK) {
        result = httpd_resp_send_chunk(
            request, "]}", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_send_chunk(request, nullptr, 0U);
    return result;
}

esp_err_t locations_get_handler(httpd_req_t *request)
{
    STICKY_LOGI(kTag, "portal=request method=GET path=%s", request->uri);
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    esp_err_t result = httpd_resp_send_chunk(
        request, "{\"countries\":[", HTTPD_RESP_USE_STRLEN);
    for (size_t index = 0U;
         result == ESP_OK && index < location_country_count();
         ++index) {
        const LocationCountry *country = location_country_at(index);
        if (country == nullptr) {
            continue;
        }
        char header[240] = {};
        const int header_length = std::snprintf(
            header,
            sizeof(header),
            "%s{\"code\":\"%s\",\"name\":\"%s\",\"method\":%u,\"cities\":[",
            index == 0U ? "" : ",",
            country->code,
            country->api_name,
            static_cast<unsigned>(country->recommended_method));
        if (header_length <= 0 ||
            static_cast<size_t>(header_length) >= sizeof(header)) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }
        result = httpd_resp_send_chunk(request, header, header_length);
        const char *cursor = country->cities;
        bool first_city = true;
        while (result == ESP_OK && cursor != nullptr && *cursor != '\0') {
            const char *separator = std::strchr(cursor, '|');
            const size_t city_length = separator == nullptr
                                           ? std::strlen(cursor)
                                           : static_cast<size_t>(separator - cursor);
            char city[96] = {};
            if (city_length + 4U >= sizeof(city)) {
                result = ESP_ERR_INVALID_SIZE;
                break;
            }
            const int city_size = std::snprintf(
                city,
                sizeof(city),
                "%s\"%.*s\"",
                first_city ? "" : ",",
                static_cast<int>(city_length),
                cursor);
            result = httpd_resp_send_chunk(request, city, city_size);
            first_city = false;
            cursor = separator == nullptr ? nullptr : separator + 1;
        }
        if (result == ESP_OK) {
            result = httpd_resp_send_chunk(
                request, "]}", HTTPD_RESP_USE_STRLEN);
        }
    }
    if (result == ESP_OK) {
        result = httpd_resp_send_chunk(
            request, "]}", HTTPD_RESP_USE_STRLEN);
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
        return send_json_result(request, false, error);
    }
    const esp_err_t wifi_result = wifi_service_validate_setup_connection(
        parsed, kSetupConnectTimeoutMs);
    if (wifi_result != ESP_OK) {
        return send_json_result(
            request,
            false,
            "The selected Wi-Fi network could not be reached. Check its password and try again.");
    }
    StickyRtcDateTime local_time = {};
    const esp_err_t time_result = time_sync_from_network(
        parsed.utc_offset_minutes,
        local_time,
        kSetupTimeSyncTimeoutMs);
    if (time_result != ESP_OK) {
        return send_json_result(
            request,
            false,
            "Wi-Fi connected, but internet time could not be reached. Check the network connection.");
    }
    const esp_err_t location_result = aladhan_validate_location(parsed);
    if (location_result != ESP_OK) {
        return send_json_result(
            request,
            false,
            location_result == ESP_ERR_INVALID_RESPONSE
                ? "The selected city could not be verified in this country. Choose another city or check the spelling."
                : "The prayer time service could not be reached. Try again in a moment.");
    }
    const esp_err_t save_result = settings_store_save(parsed);
    if (save_result != ESP_OK) {
        return send_json_result(
            request, false, "The device could not save the setup.");
    }
    context->settings = parsed;
    const esp_err_t response_result = send_json_result(
        request, true, "Wi-Fi and prayer time data were verified.");
    if (response_result == ESP_OK) {
        context->saved.store(true, std::memory_order_release);
    }
    return response_result;
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
    config.stack_size = 8192U;
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

    httpd_uri_t wifi_scan_uri = {};
    wifi_scan_uri.uri = "/api/wifi/scan";
    wifi_scan_uri.method = HTTP_GET;
    wifi_scan_uri.handler = wifi_scan_get_handler;
    wifi_scan_uri.user_ctx = &context;
    result = httpd_register_uri_handler(server, &wifi_scan_uri);
    if (result != ESP_OK) {
        httpd_stop(server);
        return result;
    }

    httpd_uri_t locations_uri = {};
    locations_uri.uri = "/api/locations";
    locations_uri.method = HTTP_GET;
    locations_uri.handler = locations_get_handler;
    locations_uri.user_ctx = &context;
    result = httpd_register_uri_handler(server, &locations_uri);
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
    STICKY_LOGI(kTag,
                "portal=ready address=http://192.168.4.1/ result=ok");
    while (!context.saved.load(std::memory_order_acquire)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(750));
    captive_dns_server_stop();
    httpd_stop(server);
    saved_settings = context.settings;
    return ESP_OK;
}
