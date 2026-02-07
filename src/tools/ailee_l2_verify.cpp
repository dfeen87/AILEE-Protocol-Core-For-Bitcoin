#include "L2State.h"

#include <iostream>
#include <string>

namespace {

void printUsage(const char* exe) {
    std::cout << "Usage: " << exe << " --snapshot <path>\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string snapshotPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--snapshot" && i + 1 < argc) {
            snapshotPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (snapshotPath.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    std::string err;
    auto snapshotOpt = ailee::l2::loadLatestSnapshotFromFile(snapshotPath, &err);
    if (!snapshotOpt.has_value()) {
        std::cerr << "Failed to load snapshot: " << err << "\n";
        return 1;
    }

    const auto& snapshot = snapshotOpt.value();
    const std::string computedRoot = ailee::l2::computeL2StateRoot(snapshot);
    std::cout << "Computed L2StateRoot: " << computedRoot << "\n";

    bool ok = true;
    if (snapshot.anchor.has_value()) {
        std::string anchorErr;
        if (!ailee::l2::validateAnchorCommitment(snapshot.anchor.value(), computedRoot, &anchorErr)) {
            std::cerr << "Anchor verification failed: " << anchorErr << "\n";
            ok = false;
        } else {
            std::cout << "Anchor verified: " << snapshot.anchor->hash << "\n";
        }
    } else {
        std::cerr << "No anchor commitment present in snapshot.\n";
        ok = false;
    }

    if (snapshot.anchor.has_value()) {
        const std::string& anchorHash = snapshot.anchor->hash;
        for (const auto& pegout : snapshot.bridge.pegouts) {
            if (pegout.anchorCommitmentHash.empty()) {
                std::cerr << "Peg-out " << pegout.pegId << " missing anchor commitment hash.\n";
                ok = false;
                continue;
            }
            if (pegout.anchorCommitmentHash != anchorHash) {
                std::cerr << "Peg-out " << pegout.pegId
                          << " anchor hash mismatch. expected=" << anchorHash
                          << " got=" << pegout.anchorCommitmentHash << "\n";
                ok = false;
            }
        }
    }

    if (!ok) {
        return 2;
    }

    std::cout << "Verification OK.\n";
    return 0;
}

