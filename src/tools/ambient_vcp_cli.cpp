// SPDX-License-Identifier: MIT
// ambient_vcp_cli.cpp — Ambient VCP command-line interface
//
// Maintains node session state locally (via LocalSessionManager) even when
// disconnected from the API endpoint.  An observability server shares the
// *live* node pointer rather than a stale copy, so monitoring always
// reflects current state.

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <string>

#include "../../include/AmbientAI.h"

using namespace ambient;

namespace {
    std::atomic<bool> g_running{true};

    void signalHandler(int) {
        g_running = false;
    }
} // namespace

// ---------------------------------------------------------------------------
// ObservabilityServer
//
// Shares the *live* AmbientNode via shared_ptr so it always reflects the
// current state — fixing the previous bug where a stale clone was monitored.
// ---------------------------------------------------------------------------

class ObservabilityServer {
public:
    explicit ObservabilityServer(std::shared_ptr<AmbientNode> node)
        : node_(std::move(node)) {}

    void printStatus() const {
        const auto state = node_->sessionManager().getState();
        std::cout << "[observability]"
                  << " node="      << state.nodeId
                  << " connected=" << (state.connected ? "yes" : "no")
                  << " token="     << state.sessionToken
                  << " log_entries=" << state.activityLog.size()
                  << "\n";
    }

private:
    // Shared live state — not a stale clone.
    std::shared_ptr<AmbientNode> node_;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void printUsage(const char* exe) {
    std::cout << "Usage: " << exe << " --node-id <id> [--interval <secs>]\n"
              << "\n"
              << "  Maintains ambient VCP node session state locally,\n"
              << "  even when disconnected from the API endpoint.\n"
              << "\n"
              << "Options:\n"
              << "  --node-id <id>      Node public key / identifier (required)\n"
              << "  --interval <secs>   Maintenance tick interval in seconds (default: 30)\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    std::string nodeId;
    int intervalSecs = 30;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--node-id" && i + 1 < argc) {
            nodeId = argv[++i];
        } else if (arg == "--interval" && i + 1 < argc) {
            intervalSecs = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (nodeId.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    NodeId id;
    id.pubkey      = nodeId;
    id.region      = "local";
    id.deviceClass = "cli";

    SafetyPolicy policy;
    // node is a shared_ptr so both the maintenance loop and the observability
    // server operate on the *same* live object.
    auto node = std::make_shared<AmbientNode>(id, policy);
    node->sessionManager().recordActivity("[startup] ambient-vcp-cli started");

    // Observability server receives the live shared_ptr — not a copy of the node.
    ObservabilityServer observability(node);

    std::cout << "[ambient-vcp-cli] Starting maintenance loop for node: " << nodeId << "\n";
    std::cout << "[ambient-vcp-cli] Maintenance interval: " << intervalSecs << "s\n";

    // Maintenance loop: calls runMaintenanceTick() to keep session alive even
    // when the node is offline / disconnected from the remote API endpoint.
    while (g_running) {
        node->sessionManager().runMaintenanceTick();
        observability.printStatus();

        for (int s = 0; s < intervalSecs && g_running; ++s) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    node->sessionManager().recordActivity("[shutdown] ambient-vcp-cli stopped");
    std::cout << "[ambient-vcp-cli] Maintenance loop stopped.\n";
    return 0;
}
