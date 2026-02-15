// SPDX-License-Identifier: MIT
// AILEE Web Server Implementation - REST API for global web integration

#include "AILEEWebServer.h"
#include "Orchestrator.h"
#include "Ledger.h"
#include "L2State.h"
#include "third_party/httplib.h"
#include "nlohmann/json.hpp"

#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

namespace ailee {

// Implementation class using PIMPL idiom
class AILEEWebServer::Impl {
public:
    Impl(const WebServerConfig& config) : config_(config) {
        server_ = std::make_unique<httplib::Server>();
        setupRoutes();
    }

    ~Impl() {
        stop();
    }

    bool start() {
        if (running_) return false;
        
        running_ = true;
        server_thread_ = std::thread([this]() {
            std::cout << "[WebServer] Starting on " << config_.host 
                      << ":" << config_.port << std::endl;
            
            if (config_.enable_ssl && !config_.ssl_cert_path.empty() 
                && !config_.ssl_key_path.empty()) {
                // SSL/TLS support - Note: cpp-httplib may require compilation with SSL support
                // For now, fall back to regular HTTP with a warning
                std::cout << "[WebServer] WARNING: SSL configured but not fully implemented yet" << std::endl;
                std::cout << "[WebServer] Falling back to HTTP" << std::endl;
                server_->listen(config_.host.c_str(), config_.port);
            } else {
                // Standard HTTP
                server_->listen(config_.host.c_str(), config_.port);
            }
            running_ = false;
        });
        
        return true;
    }

    void stop() {
        if (server_) {
            server_->stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        running_ = false;
    }

    bool isRunning() const {
        return running_;
    }

    void setNodeStatusCallback(std::function<NodeStatus()> callback) {
        status_callback_ = callback;
    }

    void setOrchestratorRef(Orchestrator* orch) {
        orchestrator_ = orch;
    }

    void setLedgerRef(Ledger* ledger) {
        ledger_ = ledger;
    }

private:
    void setupRoutes() {
        // CORS middleware
        if (config_.enable_cors) {
            server_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-API-Key");
                
                if (req.method == "OPTIONS") {
                    res.status = 200;
                    return httplib::Server::HandlerResponse::Handled;
                }
                return httplib::Server::HandlerResponse::Unhandled;
            });
        }

        // API key authentication middleware (optional)
        if (!config_.api_key.empty()) {
            server_->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
                if (req.path.find("/api/") == 0) {
                    auto api_key = req.get_header_value("X-API-Key");
                    if (api_key != config_.api_key) {
                        res.status = 401;
                        json error_response = {
                            {"error", "Unauthorized"},
                            {"message", "Invalid or missing API key"}
                        };
                        res.set_content(error_response.dump(), "application/json");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                }
                return httplib::Server::HandlerResponse::Unhandled;
            });
        }

        // Serve the web dashboard
        server_->Get("/", [](const httplib::Request&, httplib::Response& res) {
            std::ifstream file("web/index.html");
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_content(buffer.str(), "text/html");
            } else {
                // Fallback to API documentation
                json response = {
                    {"name", "AILEE Protocol Core API"},
                    {"version", "1.0.0"},
                    {"description", "REST API for AILEE Bitcoin Layer-2 Protocol"},
                    {"endpoints", {
                        {"/api/status", "Get node status and health"},
                        {"/api/metrics", "Get system metrics"},
                        {"/api/l2/state", "Get Layer-2 state information"},
                        {"/api/orchestration/tasks", "Get orchestration task list"},
                        {"/api/anchors/latest", "Get latest Bitcoin anchor"},
                        {"/api/health", "Health check endpoint"}
                    }},
                    {"documentation", "https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin"}
                };
                res.set_content(response.dump(), "application/json");
            }
        });

        // Health check endpoint
        server_->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            json response = {
                {"status", "healthy"},
                {"timestamp", static_cast<double>(ms)}
            };
            res.set_content(response.dump(), "application/json");
        });

        // Node status endpoint
        server_->Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    json response = {
                        {"running", status.running},
                        {"version", status.version},
                        {"uptime_seconds", status.uptime_seconds},
                        {"network", status.network},
                        {"statistics", {
                            {"total_transactions", status.total_transactions},
                            {"total_blocks", status.total_blocks},
                            {"current_tps", status.current_tps},
                            {"pending_tasks", status.pending_tasks}
                        }},
                        {"last_anchor_hash", status.last_anchor_hash}
                    };
                    res.set_content(response.dump(), "application/json");
                } catch (const std::exception& e) {
                    res.status = 500;
                    json error = {{"error", e.what()}};
                    res.set_content(error.dump(), "application/json");
                }
            } else {
                json response = {
                    {"status", "running"},
                    {"message", "Status callback not configured"}
                };
                res.set_content(response.dump(), "application/json");
            }
        });

        // Metrics endpoint
        server_->Get("/api/metrics", [this](const httplib::Request&, httplib::Response& res) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            json metrics = {
                {"timestamp", static_cast<double>(ms)},
                {"node", {
                    {"type", "AILEE-Core"},
                    {"layer", "Bitcoin Layer-2"}
                }}
            };
            
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    metrics["performance"] = {
                        {"current_tps", status.current_tps},
                        {"pending_tasks", status.pending_tasks}
                    };
                } catch (...) {
                    // Ignore errors for metrics
                }
            }
            
            res.set_content(metrics.dump(), "application/json");
        });

        // Layer-2 state endpoint
        server_->Get("/api/l2/state", [this](const httplib::Request&, httplib::Response& res) {
            json state = {
                {"layer", "Layer-2"},
                {"protocol", "AILEE-Core"},
                {"description", "Bitcoin-anchored Layer-2 state"}
            };
            
            if (ledger_) {
                // Cast to ILedger to access snapshot() method
                auto* iledger = dynamic_cast<ailee::econ::ILedger*>(ledger_);
                if (iledger) {
                    try {
                        // Get ledger snapshot
                        auto ledgerSnapshot = iledger->snapshot();
                        
                        // Create a minimal L2StateSnapshot with just ledger data
                        // Note: Full snapshot with bridge/orchestration requires adding
                        // SidechainBridge reference to WebServer (currently not passed in)
                        ailee::l2::L2StateSnapshot snapshot;
                        auto now = std::chrono::system_clock::now();
                        snapshot.snapshotTimestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()).count();
                        snapshot.ledger = ledgerSnapshot;
                        
                        // Compute state root from snapshot
                        std::string stateRoot = ailee::l2::computeL2StateRoot(snapshot);
                        
                        // Build response with ledger snapshot details
                        state["state_root"] = stateRoot;
                        state["timestamp_ms"] = snapshot.snapshotTimestampMs;
                        state["ledger"] = {
                            {"status", "active"},
                            {"type", "federated"},
                            {"balance_count", ledgerSnapshot.balances.size()},
                            {"escrow_count", ledgerSnapshot.escrows.size()}
                        };
                        
                        // Add balance information (count only for performance)
                        std::uint64_t totalBalance = 0;
                        for (const auto& bal : ledgerSnapshot.balances) {
                            totalBalance += bal.balance;
                        }
                        state["ledger"]["total_balance"] = totalBalance;
                        
                        // Add escrow information (count only)
                        std::uint64_t totalEscrow = 0;
                        for (const auto& esc : ledgerSnapshot.escrows) {
                            totalEscrow += esc.amount;
                        }
                        state["ledger"]["total_escrow"] = totalEscrow;
                        
                    } catch (const std::exception& e) {
                        // If snapshot fails, return basic status
                        state["ledger"] = {
                            {"status", "active"},
                            {"type", "federated"},
                            {"error", e.what()}
                        };
                    }
                } else {
                    // Ledger doesn't support ILedger interface
                    state["ledger"] = {
                        {"status", "active"},
                        {"type", "federated"}
                    };
                }
            }
            
            res.set_content(state.dump(), "application/json");
        });

        // Orchestration tasks endpoint
        server_->Get("/api/orchestration/tasks", [this](const httplib::Request&, httplib::Response& res) {
            json tasks = {
                {"tasks", json::array()},
                {"total", 0}
            };
            
            if (orchestrator_) {
                // If orchestrator is available, we could query tasks
                // For now, return empty list
                tasks["status"] = "available";
            } else {
                tasks["status"] = "not_configured";
            }
            
            res.set_content(tasks.dump(), "application/json");
        });

        // Latest anchor endpoint
        server_->Get("/api/anchors/latest", [this](const httplib::Request&, httplib::Response& res) {
            json anchor = {
                {"message", "Bitcoin anchoring is active"}
            };
            
            if (status_callback_) {
                try {
                    NodeStatus status = status_callback_();
                    anchor["last_anchor_hash"] = status.last_anchor_hash;
                } catch (...) {
                    anchor["last_anchor_hash"] = "N/A";
                }
            }
            
            res.set_content(anchor.dump(), "application/json");
        });

        // Submit task endpoint (POST)
        server_->Post("/api/orchestration/submit", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json request_body = json::parse(req.body);
                
                // Validate request
                if (!request_body.contains("task_type") || !request_body.contains("task_data")) {
                    res.status = 400;
                    json error = {
                        {"error", "Invalid request"},
                        {"message", "task_type and task_data are required"}
                    };
                    res.set_content(error.dump(), "application/json");
                    return;
                }
                
                // Generate unique task ID with timestamp and counter
                static std::atomic<uint64_t> task_counter{0};
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                uint64_t counter = task_counter.fetch_add(1);
                
                std::ostringstream task_id_stream;
                task_id_stream << "task_" << ms << "_" << counter;
                
                // For now, just acknowledge the task
                json response = {
                    {"status", "accepted"},
                    {"task_id", task_id_stream.str()},
                    {"message", "Task submitted successfully"}
                };
                
                res.status = 202; // Accepted
                res.set_content(response.dump(), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 400;
                json error = {
                    {"error", "Invalid request"},
                    {"message", e.what()}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        // 404 handler
        server_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
            json error = {
                {"error", "Not Found"},
                {"message", "The requested endpoint does not exist"},
                {"status_code", res.status}
            };
            res.set_content(error.dump(), "application/json");
        });
    }

    WebServerConfig config_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    
    // Callbacks and references
    std::function<NodeStatus()> status_callback_;
    Orchestrator* orchestrator_ = nullptr;
    Ledger* ledger_ = nullptr;
};

// Public API implementation
AILEEWebServer::AILEEWebServer(const WebServerConfig& config) 
    : pImpl(std::make_unique<Impl>(config)), config_(config) {
}

AILEEWebServer::~AILEEWebServer() = default;

bool AILEEWebServer::start() {
    running_ = true;
    return pImpl->start();
}

void AILEEWebServer::stop() {
    running_ = false;
    pImpl->stop();
}

bool AILEEWebServer::isRunning() const {
    return pImpl->isRunning();
}

void AILEEWebServer::setNodeStatusCallback(std::function<NodeStatus()> callback) {
    pImpl->setNodeStatusCallback(callback);
}

void AILEEWebServer::setOrchestratorRef(Orchestrator* orch) {
    pImpl->setOrchestratorRef(orch);
}

void AILEEWebServer::setLedgerRef(Ledger* ledger) {
    pImpl->setLedgerRef(ledger);
}

} // namespace ailee
