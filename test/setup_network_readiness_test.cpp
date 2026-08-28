#include <cassert>

#include "setup_network_readiness.h"

int main()
{
    SetupNetworkReadiness state = {};
    assert(!setup_network_ready(state));

    state.ap_started = true;
    assert(!setup_network_ready(state));

    state.dhcp_server_running = true;
    assert(!setup_network_ready(state));

    state.expected_ip_configured = true;
    assert(setup_network_ready(state));
    return 0;
}
