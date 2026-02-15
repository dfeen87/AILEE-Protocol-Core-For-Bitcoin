// SPDX-License-Identifier: MIT
// ILedger Interface - Abstract interface for ledger operations

#ifndef AILEE_ECON_ILEDGER_H
#define AILEE_ECON_ILEDGER_H

#include <string>
#include <vector>
#include <cstdint>

namespace ailee {
namespace econ {

// Forward declarations for ledger snapshot structures
struct BalanceEntry {
    std::string address;
    std::uint64_t balance;
};

struct EscrowEntry {
    std::string escrowId;
    std::uint64_t amount;
    std::string sender;
    std::string recipient;
};

struct LedgerSnapshot {
    std::vector<BalanceEntry> balances;
    std::vector<EscrowEntry> escrows;
    std::uint64_t totalSupply;
    std::uint64_t blockHeight;
};

/**
 * ILedger - Interface for ledger state management
 * Provides abstraction over different ledger implementations
 */
class ILedger {
public:
    virtual ~ILedger() = default;
    
    /**
     * Get current ledger snapshot
     * @return LedgerSnapshot containing current state
     */
    virtual LedgerSnapshot snapshot() const = 0;
    
    /**
     * Get balance for an address
     * @param address The address to query
     * @return Balance in satoshis
     */
    virtual std::uint64_t getBalance(const std::string& address) const = 0;
    
    /**
     * Credit an address
     * @param address Address to credit
     * @param amount Amount in satoshis
     * @return true if successful
     */
    virtual bool credit(const std::string& address, std::uint64_t amount) = 0;
    
    /**
     * Debit an address
     * @param address Address to debit
     * @param amount Amount in satoshis
     * @return true if successful (false if insufficient balance)
     */
    virtual bool debit(const std::string& address, std::uint64_t amount) = 0;
};

} // namespace econ
} // namespace ailee

#endif // AILEE_ECON_ILEDGER_H
