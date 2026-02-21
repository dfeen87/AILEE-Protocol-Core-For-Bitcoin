# AILEE Testnet Launch Guide

This guide describes how to deploy the AILEE-Core Protocol to Fly.io for the Testnet launch.

## Prerequisites

1.  **Fly.io CLI**: Install via `curl -L https://fly.io/install.sh | sh`
2.  **Docker**: Required for local builds (optional if using Fly's remote builder).
3.  **Bitcoin Testnet Node**: You need a ZMQ endpoint from a Bitcoin Testnet node (e.g., `tcp://<ip>:28332`).

## Configuration

The project is pre-configured for testnet in `fly.toml`.

Key Environment Variables (set in `fly.toml` or via secrets):

*   `AILEE_ENV`: set to `"testnet"`.
*   `AILEE_NETWORK`: set to `"bitcoin-testnet"`.
*   `AILEE_BITCOIN_ZMQ_ENDPOINT`: **CRITICAL**. You must point this to a valid Bitcoin Testnet ZMQ publisher. The default is `tcp://127.0.0.1:28332`, which only works if you run a sidecar Bitcoin node. For deployment, use your provider's endpoint.

## Deployment Steps

1.  **Login to Fly.io**:
    ```bash
    fly auth login
    ```

2.  **Initialize App (First Time Only)**:
    ```bash
    fly launch --no-deploy
    ```
    *   Choose a unique app name (e.g., `ailee-testnet-01`).
    *   Select your preferred region.

3.  **Set Secrets (Optional/Recommended)**:
    If your ZMQ endpoint requires authentication or is sensitive:
    ```bash
    fly secrets set AILEE_BITCOIN_ZMQ_ENDPOINT="tcp://your-secure-node:28332"
    ```

4.  **Deploy**:
    ```bash
    fly deploy
    ```

5.  **Verify Deployment**:
    *   Visit `https://<your-app-name>.fly.dev/` to see the Testnet Dashboard.
    *   Check logs: `fly logs`

## Accessing the Dashboard

The web dashboard is available at the root URL of your deployed application.

*   **Status**: Shows "Online" if the node is running.
*   **Network**: Should display `bitcoin-testnet`.
*   **Metrics**: Real-time throughput and transaction counts.

## Troubleshooting

*   **Node Offline**: Check `fly logs`. If the ZMQ connection fails, the node might log an error but keep running.
*   **Reorg Warnings**: If connected to a volatile testnet, you might see "Deep reorg detected" logs. This is normal and proves the security module is working.
