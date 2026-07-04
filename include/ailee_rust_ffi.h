#include <stddef.h>
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// FFI declarations for Halo2 Rust prover

typedef struct {
    const unsigned char* commitment_ptr;
    size_t commitment_len;
    const unsigned char* proof_ptr;
    size_t proof_len;
} Halo2ProofOutput;

int generate_halo2_proof_ffi(const char* task_id, const char* computation_hash, Halo2ProofOutput** out_proof);
int verify_halo2_proof_ffi(const unsigned char* proof_data, size_t proof_len, const char* computation_hash);
void free_halo2_proof_ffi(Halo2ProofOutput* proof_ptr);

// Network FFI functions for libp2p via rust-libp2p
int init_network_ffi();
int broadcast_message_ffi(const char* topic, const unsigned char* payload, size_t payload_len);
int subscribe_topic_ffi(const char* topic);

#ifdef __cplusplus
}
#endif
