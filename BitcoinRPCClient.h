// BitcoinRPCClient.h
#ifndef BITCOIN_RPC_CLIENT_H
#define BITCOIN_RPC_CLIENT_H

#include <string>

namespace ailee {
    class BitcoinRPCClient {
    public:
        BitcoinRPCClient(const std::string& rpcUser,
                         const std::string& rpcPassword,
                         const std::string& rpcUrl = "http://127.0.0.1:8332");

        bool broadcastCheckpoint(const std::string& hexTx);

    private:
        std::string rpcUser_;
        std::string rpcPassword_;
        std::string rpcUrl_;
    };
}

#endif // BITCOIN_RPC_CLIENT_H
