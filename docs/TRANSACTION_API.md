# Transaction API Documentation

## Overview

The AILEE-Core blockchain now supports transaction submission and querying through a RESTful API. This document describes the available transaction endpoints.

## Endpoints

### Submit Transaction

Submit a new transaction to the blockchain for processing.

**Endpoint:** `POST /transactions/submit`

**Request Body:**
```json
{
  "from_address": "alice",
  "to_address": "bob",
  "amount": 1000,
  "data": "Optional transaction memo"
}
```

**Parameters:**
- `from_address` (string, required): Sender address/peer ID (1-256 characters)
- `to_address` (string, required): Recipient address/peer ID (1-256 characters)
- `amount` (integer, required): Amount to transfer in satoshis (must be > 0)
- `data` (string, optional): Transaction memo or data (max 1000 characters)

**Response:**
```json
{
  "tx_hash": "df9f96690eefa7d6ff1bf870427749f2c4d58a3cc87f907b98bb17c35920ff9b",
  "from_address": "alice",
  "to_address": "bob",
  "amount": 1000,
  "status": "pending",
  "block_height": null,
  "timestamp": "2026-02-15T08:41:13.390820+00:00"
}
```

**Example:**
```bash
curl -X POST http://localhost:8080/transactions/submit \
  -H "Content-Type: application/json" \
  -d '{
    "from_address": "alice",
    "to_address": "bob",
    "amount": 1000,
    "data": "Payment for services"
  }'
```

**Validation:**
- `from_address` and `to_address` must be different
- `amount` must be greater than 0
- All required fields must be provided

---

### Get Transaction by Hash

Retrieve a specific transaction by its hash.

**Endpoint:** `GET /transactions/hash/{tx_hash}`

**Parameters:**
- `tx_hash` (path parameter): The transaction hash to query

**Response:**
```json
{
  "tx_hash": "df9f96690eefa7d6ff1bf870427749f2c4d58a3cc87f907b98bb17c35920ff9b",
  "from_address": "alice",
  "to_address": "bob",
  "amount": 1000,
  "status": "pending",
  "block_height": null,
  "data": "Payment for services",
  "timestamp": "2026-02-15T08:41:13.390820+00:00"
}
```

**Example:**
```bash
curl http://localhost:8080/transactions/hash/df9f96690eefa7d6ff1bf870427749f2c4d58a3cc87f907b98bb17c35920ff9b
```

**Error Response (404):**
```json
{
  "detail": "Transaction not found: {tx_hash}"
}
```

---

### Get Transactions by Address

Retrieve all transactions for a specific address (sent or received).

**Endpoint:** `GET /transactions/address/{address}`

**Parameters:**
- `address` (path parameter): The address to query
- `page` (query parameter, optional): Page number (default: 1, min: 1)
- `page_size` (query parameter, optional): Items per page (default: 10, min: 1, max: 100)

**Response:**
```json
{
  "transactions": [
    {
      "tx_hash": "df9f96690eefa7d6ff1bf870427749f2c4d58a3cc87f907b98bb17c35920ff9b",
      "from_address": "alice",
      "to_address": "bob",
      "amount": 1000,
      "status": "pending",
      "block_height": null,
      "data": "Payment for services",
      "timestamp": "2026-02-15T08:41:13.390820+00:00"
    }
  ],
  "total_count": 1,
  "page": 1,
  "page_size": 10
}
```

**Example:**
```bash
# Get all transactions for Alice
curl http://localhost:8080/transactions/address/alice

# Get first page with 5 items
curl http://localhost:8080/transactions/address/alice?page=1&page_size=5
```

---

### List All Transactions

Retrieve a paginated list of all transactions in the blockchain.

**Endpoint:** `GET /transactions/list`

**Query Parameters:**
- `page` (optional): Page number (default: 1, min: 1)
- `page_size` (optional): Items per page (default: 10, min: 1, max: 100)
- `status` (optional): Filter by status (pending, confirmed, failed)

**Response:**
```json
{
  "transactions": [
    {
      "tx_hash": "5b3eb1d8093824c85b2262a388d48053af7d6feeb736ff99e9aa1133fd4ea130",
      "from_address": "alice",
      "to_address": "charlie",
      "amount": 500,
      "status": "pending",
      "block_height": null,
      "data": null,
      "timestamp": "2026-02-15T08:41:18.953642+00:00"
    },
    {
      "tx_hash": "df9f96690eefa7d6ff1bf870427749f2c4d58a3cc87f907b98bb17c35920ff9b",
      "from_address": "alice",
      "to_address": "bob",
      "amount": 1000,
      "status": "pending",
      "block_height": null,
      "data": "Payment for services",
      "timestamp": "2026-02-15T08:41:13.390820+00:00"
    }
  ],
  "total_count": 2,
  "page": 1,
  "page_size": 10
}
```

**Example:**
```bash
# Get all transactions
curl http://localhost:8080/transactions/list

# Get second page with 20 items
curl http://localhost:8080/transactions/list?page=2&page_size=20

# Filter by status
curl http://localhost:8080/transactions/list?status=pending
```

---

## Transaction Hash

Transaction hashes are computed deterministically using SHA-256 based on:
- `from_address`
- `to_address`
- `amount`
- `data` (if provided)
- `timestamp`

This ensures each transaction has a unique identifier.

## Transaction Status

Currently supported statuses:
- `pending`: Transaction submitted but not yet confirmed in a block
- `confirmed`: Transaction included in a block (future enhancement)
- `failed`: Transaction failed validation or execution (future enhancement)

## Rate Limiting

All transaction endpoints are subject to the API's rate limiting:
- 100 requests per 60 seconds (default configuration)

## Error Codes

- `400 Bad Request`: Invalid transaction data (e.g., same address for sender and receiver)
- `404 Not Found`: Transaction not found
- `422 Unprocessable Entity`: Validation error in request body
- `429 Too Many Requests`: Rate limit exceeded
- `500 Internal Server Error`: Server error

## Interactive Documentation

Visit `/docs` on your API server for interactive Swagger UI documentation where you can test all endpoints directly in your browser.

## Example Workflow

```bash
# 1. Submit a transaction
TX_HASH=$(curl -s -X POST http://localhost:8080/transactions/submit \
  -H "Content-Type: application/json" \
  -d '{
    "from_address": "alice",
    "to_address": "bob",
    "amount": 1000,
    "data": "Payment for services"
  }' | jq -r '.tx_hash')

# 2. Get the transaction details
curl http://localhost:8080/transactions/hash/$TX_HASH

# 3. List all transactions for Alice
curl http://localhost:8080/transactions/address/alice

# 4. List all pending transactions
curl http://localhost:8080/transactions/list?status=pending
```

## Future Enhancements

The following features are planned for future releases:
- Transaction signatures and verification
- Integration with C++ BlockProducer for on-chain inclusion
- Transaction confirmation tracking
- Transaction fees and gas limits
- Smart contract support
- Batch transaction submission
