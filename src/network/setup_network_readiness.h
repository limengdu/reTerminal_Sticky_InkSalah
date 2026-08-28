#pragma once

struct SetupNetworkReadiness {
    bool ap_started = false;
    bool dhcp_server_running = false;
    bool expected_ip_configured = false;
    bool dns_server_offered = false;
    bool portal_uri_configured = false;
};

bool setup_network_ready(const SetupNetworkReadiness &state);
