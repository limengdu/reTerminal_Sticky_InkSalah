#include "setup_network_readiness.h"

bool setup_network_ready(const SetupNetworkReadiness &state)
{
    return state.ap_started && state.dhcp_server_running &&
           state.expected_ip_configured;
}
