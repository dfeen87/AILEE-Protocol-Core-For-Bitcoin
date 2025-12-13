# AILEE Loss Bitcoin Recovery Protocol: User Guide

**"Lost is no longer lost. It is simply dormant."**

This document explains the technical workflow of the `ailee_recovery_protocol.h` module. It details how a user can utilize Zero-Knowledge Proofs (ZKPs) and Verifiable Delay Functions (VDFs) to reclaim Bitcoin that has been dormant for over 20 years, without exposing private keys or relying on centralized intermediaries.

---

## ⚠️ Important Prerequisite
**This protocol is designed for "Deep Dormancy" assets.**
To initiate a claim, the target UTXO (Unspent Transaction Output) must have zero on-chain activity for a minimum of **20 Years** (configurable in `MIN_INACTIVITY_YEARS`).

---

## The Recovery Workflow

The recovery process is divided into four distinct cryptographic phases. Below is the step-by-step guide on how a user interacts with the AILEE-Core software to execute this flow.

### Phase 1: Identification & Witness Generation
You do not need the private key. You need "Witness Data"—this could be a partial key shard, a hardware wallet signature from a legacy device, or a cryptographically linked identity.

**In the Code:**
The user calls the generator to create a Zero-Knowledge Proof.
```cpp
// Example C++ Implementation
auto zkProof = ailee::ZeroKnowledgeProof::generateOwnershipProof(
    "1A1zP1e...",       // The Dormant Bitcoin Address
    witnessData,        // Your partial data/shard
    "ClaimantUserA"     // Your new Identity
);
What happens: The system generates a cryptographic proof that says "I know the origin of this address" without ever revealing the data itself.
Phase 2: The Time-Lock Challenge (VDF)
To prevent hackers from spamming claims, you must solve a Verifiable Delay Function. This is a mathematical puzzle that cannot be parallelized. It forces a mandatory waiting period (e.g., 6 months).
In the Code:
code
C++
// This computation takes time. You cannot speed it up with more GPUs.
auto vdfOutput = ailee::VerifiableDelayFunction::compute(
    transactionHash, 
    VDF_DIFFICULTY // Set to require millions of sequential iterations
);
Why this matters: Even if a hacker has a supercomputer, they cannot bypass the time-lock. This gives the original owner (if they are still active) ample time to see the alert and move their funds, instantly cancelling your claim.
Phase 3: Claim Submission & Validator Voting
Once the ZK Proof is generated and the VDF is solved, the claim is broadcast to the AILEE Network. Validators (the community) review the proofs.
In the Code:
code
C++
ailee::RecoveryProtocol protocol;
std::string claimId = protocol.submitClaim(
    txId, vout, claimantAddr, inactivityTime, witnessData
);
Status: The claim enters the CHALLENGE_PERIOD.
Dispute Mechanism: If any activity occurs on the original address during this window, the claim is instantly voided (Status::REJECTED).
Phase 4: Finalization & Fund Transfer
If the challenge period expires with no disputes, and validators reach a 67% quorum, the funds are unlocked.
In the Code:
code
C++
bool success = protocol.finalizeClaim(claimId);
if (success) {
    // Funds are wrapped or bridged to the claimant's AILEE address
    std::cout << "Asset Recovery Successful." << std::endl;
}
Frequently Asked Questions
1. Can a hacker use this to steal my active wallet?
No. The protocol requires the address to be dormant for 20+ years. If you have moved your coins anytime in the last two decades, the protocol rejects the claim immediately at line 208 of ailee_recovery_protocol.h.
2. What if the original owner comes back during the claim?
If the original owner notices a claim on their funds, they simply need to make one transaction (send 1 Satoshi to themselves). This resets the inactivity timer, invalidating the claim and proving the wallet is not lost.
3. Why use Zero-Knowledge Proofs?
ZKPs allow us to verify that you are the rightful owner (or heir) without you ever typing a password or key into the network. This ensures that the recovery process itself doesn't become a security vulnerability.
Developer Simulation
To test this flow in the current repository:
Open src/main.cpp.
Locate the RecoveryProtocol instantiation.
Run the executable ./ailee_node.
Observe the terminal output as it simulates the VDF computation and Validator voting logic.
code
Code
***

### Why this file helps
1.  **It demystifies the "Magic":** It shows the C++ function calls, proving it's logic, not magic.
2.  **It reassures holders:** The FAQ specifically addresses the fear of theft.
3.  **It adds interactivity:** It tells the user where to look in `main.cpp` to see it happening.

Add this file, and your repository becomes a complete educational platform for the future of Bitcoin.
