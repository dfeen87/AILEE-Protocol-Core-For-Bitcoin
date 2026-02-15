# AILEE-Core API Security Guide

## Overview

This document describes the security architecture of the AILEE-Core REST API, including authentication requirements, endpoint protection, and deployment best practices.

---

## API Authentication

### Protected vs Public Endpoints

The AILEE-Core API uses **API key authentication** to protect write operations while keeping read operations publicly accessible.

#### Protected Endpoints (Require API Key)

These endpoints **require** a valid API key in the `Authorization` header:

- `POST /transactions/submit` - Submit new transactions

All future write endpoints (`POST`, `PUT`, `DELETE`) will also require authentication.

#### Public Endpoints (No Authentication Required)

These endpoints are **publicly accessible** without authentication:

- `GET /health` - Health check
- `GET /status` - Node status
- `GET /metrics` - System metrics
- `GET /l2/state` - Layer-2 state snapshot
- `GET /l2/anchors` - Anchor history (paginated)
- `GET /l2/anchors/{height}` - Get anchor by height
- `GET /transactions/hash/{tx_hash}` - Get transaction by hash
- `GET /transactions/address/{address}` - Get transactions by address
- `GET /transactions/list` - List all transactions
- `GET /docs` - OpenAPI documentation (Swagger UI)
- `GET /redoc` - Alternative API documentation
- `GET /` - Root endpoint (web dashboard or API info)

---

## API Key Management

### Environment Variable

The API key is configured via the `AILEE_API_KEY` environment variable:

```bash
export AILEE_API_KEY=your-secure-api-key-here
```

**Important:**
- The API key should be at least 32 characters long
- Use a cryptographically secure random value
- Never commit the API key to version control
- Never hardcode the API key in source code

### Key Generation

If `AILEE_API_KEY` is not set, the server will:

1. Generate a secure random key (32 bytes = 64 hex characters)
2. Log the key **once** on startup
3. Store it in memory for the current session only
4. The key will be **lost** when the server restarts

**For production deployments**, you must set `AILEE_API_KEY` to ensure the key persists across restarts.

### Generating a Secure Key

Generate a secure API key using one of these methods:

**Using OpenSSL:**
```bash
openssl rand -hex 32
```

**Using Python:**
```python
import secrets
print(secrets.token_hex(32))
```

**Using Node.js:**
```javascript
require('crypto').randomBytes(32).toString('hex')
```

---

## Setting API Key on Fly.io

For deployments on Fly.io, set the API key as a secret:

### Set the Secret

```bash
# Generate a secure key
API_KEY=$(openssl rand -hex 32)

# Set it as a Fly.io secret
fly secrets set AILEE_API_KEY=$API_KEY -a ailee-protocol-core-for-bitcoin

# Save the key securely (e.g., in a password manager)
echo "Your API key: $API_KEY"
```

### Verify the Secret

```bash
fly secrets list -a ailee-protocol-core-for-bitcoin
```

### Update the Secret

```bash
# Generate a new key
NEW_API_KEY=$(openssl rand -hex 32)

# Update the secret (this will redeploy the app)
fly secrets set AILEE_API_KEY=$NEW_API_KEY -a ailee-protocol-core-for-bitcoin
```

---

## Using the API with Authentication

### Example: Submit Transaction

**With curl:**
```bash
curl -X POST http://localhost:8000/transactions/submit \
  -H "Authorization: Bearer YOUR_API_KEY_HERE" \
  -H "Content-Type: application/json" \
  -d '{
    "from_address": "alice",
    "to_address": "bob",
    "amount": 1000,
    "data": "Payment for services"
  }'
```

**With Python (requests):**
```python
import requests

api_key = "your-api-key-here"
url = "http://localhost:8000/transactions/submit"

headers = {
    "Authorization": f"Bearer {api_key}",
    "Content-Type": "application/json"
}

data = {
    "from_address": "alice",
    "to_address": "bob",
    "amount": 1000,
    "data": "Payment for services"
}

response = requests.post(url, headers=headers, json=data)
print(response.json())
```

**With JavaScript (fetch):**
```javascript
const apiKey = 'your-api-key-here';
const url = 'http://localhost:8000/transactions/submit';

const response = await fetch(url, {
  method: 'POST',
  headers: {
    'Authorization': `Bearer ${apiKey}`,
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    from_address: 'alice',
    to_address: 'bob',
    amount: 1000,
    data: 'Payment for services'
  })
});

const data = await response.json();
console.log(data);
```

---

## Error Responses

### 401 Unauthorized

Returned when the API key is missing or invalid:

```json
{
  "detail": "Invalid or missing API key"
}
```

**Common causes:**
- No `Authorization` header provided
- Invalid API key value
- Using the wrong authentication scheme (must be `Bearer`)

**Solution:**
- Verify the API key is correct
- Ensure the header format is: `Authorization: Bearer YOUR_KEY`

### 500 Internal Server Error

Returned when there's a server-side error (e.g., database failure):

```json
{
  "detail": "Failed to persist transaction"
}
```

**Note:** Stack traces are never returned to clients for security reasons.

---

## Rate Limiting

All endpoints (both protected and public) are subject to rate limiting:

- **Default limit:** 100 requests per 60 seconds per IP address
- **Status code:** 429 Too Many Requests

Configure rate limiting via environment variables:
```bash
export AILEE_RATE_LIMIT_ENABLED=true
export AILEE_RATE_LIMIT_REQUESTS=100
export AILEE_RATE_LIMIT_WINDOW=60
```

---

## Security Best Practices

### For API Operators

1. **Always set `AILEE_API_KEY`** in production
2. **Use Fly.io secrets** (never environment variables in `fly.toml`)
3. **Rotate keys periodically** (e.g., every 90 days)
4. **Monitor access logs** for suspicious activity
5. **Use HTTPS** (automatically enabled on Fly.io)
6. **Keep the API key confidential** - treat it like a password
7. **Use different keys** for different environments (dev, staging, production)

### For API Consumers

1. **Store the API key securely** (environment variables, secrets manager)
2. **Never commit the key** to version control
3. **Never log the key** in application logs
4. **Use HTTPS** for all API requests
5. **Handle 401 errors** gracefully (retry with correct credentials)
6. **Implement exponential backoff** for rate limit errors (429)

---

## Database Security

### SQLite Persistent Storage

Transactions and anchor events are stored in SQLite at:

```
/data/ailee.db
```

**Security considerations:**

1. **File permissions:** Database file is owned by the `ailee` user
2. **No external access:** SQLite is not exposed via network
3. **Backup:** Fly.io volumes should be backed up regularly
4. **No RocksDB exposure:** C++ core persistence remains isolated

### Volume Persistence

The database is stored on a Fly.io volume to survive restarts:

```toml
[mounts]
  source = "ailee_data"
  destination = "/data"
```

**Create the volume:**
```bash
fly volumes create ailee_data --region iad --size 1 -a ailee-protocol-core-for-bitcoin
```

---

## Monitoring and Auditing

### Security Audit Log

All authentication attempts are logged to:

```
/app/logs/security-audit.log
```

**Logged events:**
- Invalid API key attempts
- Rate limit violations
- Database errors
- Startup/shutdown events

### Access Logs

API access logs are available via:

```bash
fly logs -a ailee-protocol-core-for-bitcoin
```

**Monitor for:**
- Repeated 401 errors (potential brute force)
- Unusual traffic patterns
- High rate of 500 errors

---

## Incident Response

### Compromised API Key

If you suspect the API key has been compromised:

1. **Immediately rotate the key:**
   ```bash
   fly secrets set AILEE_API_KEY=$(openssl rand -hex 32)
   ```

2. **Monitor logs** for unauthorized usage:
   ```bash
   fly logs --region all -a ailee-protocol-core-for-bitcoin | grep "401"
   ```

3. **Review transactions** submitted during the compromise window

4. **Update all legitimate clients** with the new key

### Database Corruption

If the database becomes corrupted:

1. **Stop the application:**
   ```bash
   fly scale count 0 -a ailee-protocol-core-for-bitcoin
   ```

2. **Backup the volume** (if possible)

3. **Delete and recreate the database:**
   ```bash
   fly ssh console -a ailee-protocol-core-for-bitcoin
   rm /data/ailee.db
   ```

4. **Restart the application:**
   ```bash
   fly scale count 1 -a ailee-protocol-core-for-bitcoin
   ```

**Note:** This will erase all transaction history. Implement regular backups to prevent data loss.

---

## Compliance

### Data Privacy

- **No PII storage:** The API does not store personally identifiable information
- **Public blockchain data:** All transactions are public and permanent
- **No encryption at rest:** SQLite database is stored unencrypted

### Regulatory Considerations

Operators should consider:

- **KYC/AML requirements** for financial transactions
- **Data residency** requirements (configure Fly.io region)
- **Audit trail** requirements (security audit log)
- **Backup and retention** policies

---

## Support and Questions

For security-related questions or to report vulnerabilities:

- **Email:** dfeen87@gmail.com
- **Do not open public issues** for security vulnerabilities

See `SECURITY.md` in the root directory for the full vulnerability disclosure policy.
