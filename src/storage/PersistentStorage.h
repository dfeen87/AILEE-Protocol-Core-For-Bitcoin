// SPDX-License-Identifier: MIT  
// PersistentStorage.h — Production persistence layer using RocksDB
// Stores nodes, tasks, proofs, and telemetry with ACID guarantees

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace ailee::storage {

/**
 * Persistent storage layer for AILEE-Core components.
 * 
 * Storage Schema:
 * - nodes/{pubkey} → NodeState (serialized)
 * - tasks/{task_id} → TaskRecord
 * - proofs/{proof_hash} → ZKProof
 * - telemetry/{node_id}/{timestamp} → TelemetrySample
 */
class PersistentStorage {
public:
    struct Config {
        std::string dbPath = "./data/ailee.db";
        size_t maxOpenFiles = 1000;
        size_t writeBufferSizeMB = 64;
        size_t blockCacheSizeMB = 512;
        bool createIfMissing = true;
    };
    
    explicit PersistentStorage(const Config& config = Config{});
    ~PersistentStorage();
    
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool remove(const std::string& key);
    bool exists(const std::string& key);
};

} // namespace ailee::storage
