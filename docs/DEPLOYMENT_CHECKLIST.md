# Pre-Deployment Checklist - Bitcoin Layer-2 on Fly.io

**Status:** System suspended ✅  
**Goal:** Deploy production-ready Bitcoin Layer-2 protocol  
**Timeline:** Complete all items before unsuspending

---

## Phase 1: Pre-Deployment Security (Complete BEFORE deploying)

### 1.1 Secrets Management ✓

**CRITICAL:** All secrets MUST be configured via Fly.io secrets, NEVER in code.

```bash
# Generate secure secrets
JWT_SECRET=$(openssl rand -base64 64)
BITCOIN_RPC_PASSWORD=$(openssl rand -base64 32)

# Set secrets on Fly.io (these are encrypted and never visible in code)
fly secrets set \
  AILEE_JWT_SECRET="$JWT_SECRET" \
  BITCOIN_RPC_USER="your-bitcoin-rpc-username" \
  BITCOIN_RPC_PASSWORD="$BITCOIN_RPC_PASSWORD" \
  -a ailee-protocol-core-for-bitcoin

# Verify secrets are set (shows keys only, not values)
fly secrets list -a ailee-protocol-core-for-bitcoin
```

**Checklist:**
- [ ] JWT secret generated (min 64 characters)
- [ ] Bitcoin RPC credentials obtained from secure source
- [ ] All secrets set via `fly secrets set` (NEVER committed to git)
- [ ] Verified secrets list shows all required keys
- [ ] Local `.env` file contains NO real secrets (examples only)

### 1.2 Bitcoin RPC Setup ✓

**Decision Required:** Mainnet or Testnet?

**For Testnet (Recommended for initial launch):**
```bash
fly secrets set \
  BITCOIN_RPC_URL="https://your-testnet-node.example.com:18332" \
  BITCOIN_RPC_USER="testnet-rpc-user" \
  BITCOIN_RPC_PASSWORD="testnet-rpc-password" \
  BITCOIN_NETWORK="testnet" \
  -a ailee-protocol-core-for-bitcoin
```

**For Mainnet (Only after extensive testing):**
```bash
# Mainnet requires multiple redundant nodes!
fly secrets set \
  BITCOIN_RPC_URL="https://primary-node.example.com:8332" \
  BITCOIN_RPC_FAILOVER_URLS="https://backup1.example.com:8332,https://backup2.example.com:8332" \
  BITCOIN_RPC_USER="mainnet-rpc-user" \
  BITCOIN_RPC_PASSWORD="mainnet-rpc-password" \
  BITCOIN_NETWORK="mainnet" \
  -a ailee-protocol-core-for-bitcoin
```

**Bitcoin Node Options:**
1. **Self-hosted:** Full control, but requires infrastructure
2. **QuickNode:** Reliable, paid service ($49-299/month)
3. **Blockstream:** Esplora API (free tier available)
4. **GetBlock:** Multiple blockchain APIs ($49+/month)

**Checklist:**
- [ ] Network decision made (testnet vs mainnet)
- [ ] Primary Bitcoin RPC node configured
- [ ] Failover nodes configured (minimum 2 for mainnet)
- [ ] RPC credentials secured
- [ ] TLS/HTTPS enabled for all RPC connections
- [ ] Connection tested from development environment

### 1.3 Persistent Storage ✓

**CRITICAL:** Bitcoin Layer-2 needs persistent storage for state!

```bash
# Create 10GB persistent volume
fly volumes create ailee_data \
  --region iad \
  --size 10 \
  -a ailee-protocol-core-for-bitcoin

# Verify volume created
fly volumes list -a ailee-protocol-core-for-bitcoin

# Volume will be mounted at /data in container
```

**Checklist:**
- [ ] Volume created (minimum 10GB)
- [ ] Volume in same region as app (iad)
- [ ] Backup strategy defined (see Phase 3)
- [ ] Mount point configured in fly.toml

### 1.4 Environment Configuration ✓

Update `.env.example` for production deployment guide:

```bash
# .env.example - Template for production (NOT actual secrets!)
AILEE_ENV=production
AILEE_NODE_ID=ailee-node-production
AILEE_LOG_LEVEL=info

# API Server
AILEE_HOST=0.0.0.0
AILEE_PORT=8080

# Authentication (REQUIRED for production)
AILEE_JWT_ENABLED=true
# Set via: fly secrets set AILEE_JWT_SECRET="..."

# Rate Limiting (Adjust based on expected load)
AILEE_RATE_LIMIT_ENABLED=true
AILEE_RATE_LIMIT_REQUESTS=100
AILEE_RATE_LIMIT_WINDOW=60

# TLS (Fly.io provides automatic HTTPS)
AILEE_TLS_ENABLED=false  # Fly.io handles TLS termination

# Bitcoin RPC (Set via fly secrets)
# BITCOIN_RPC_URL, BITCOIN_RPC_USER, BITCOIN_RPC_PASSWORD
```

**Checklist:**
- [ ] Production environment variables documented
- [ ] JWT authentication enabled
- [ ] Rate limiting configured
- [ ] Logging level appropriate (info/warning)
- [ ] No secrets in .env.example

---

## Phase 2: Monitoring & Observability (Setup BEFORE deployment)

### 2.1 Uptime Monitoring ✓

**Options:**
1. **UptimeRobot** (Free tier: 50 monitors, 5-min intervals)
2. **Pingdom** (Free trial, then $10/month)
3. **Better Uptime** (Free tier: 10 monitors)

**Setup:**
```bash
# After deployment, monitor these endpoints:
# - https://ailee-protocol-core-for-bitcoin.fly.dev/health (every 1 min)
# - https://ailee-protocol-core-for-bitcoin.fly.dev/status (every 5 min)

# Configure alerts to email/Slack/PagerDuty
```

**Checklist:**
- [ ] Uptime monitoring service selected
- [ ] Health endpoint monitoring configured
- [ ] Alert destinations configured (email/Slack)
- [ ] Escalation policy defined
- [ ] Test alerts sent and received

### 2.2 Log Aggregation ✓

**Options:**
1. **Fly.io built-in logs** (Basic, 7-day retention)
2. **Papertrail** (Free tier: 100MB/month, 2-day search)
3. **Logtail** (Better Datadog, $10/month)

**Setup with Papertrail:**
```bash
# Get Papertrail syslog endpoint
# e.g., logs7.papertrailapp.com:12345

# Configure Fly.io to send logs
fly secrets set \
  FLY_LOG_DESTINATION="syslog://logs7.papertrailapp.com:12345" \
  -a ailee-protocol-core-for-bitcoin
```

**Checklist:**
- [ ] Log aggregation service configured
- [ ] Log retention meets compliance needs (90+ days)
- [ ] Search and filter tested
- [ ] Alert rules configured for errors
- [ ] Security audit logs separated

### 2.3 Metrics & Dashboards ✓

**Fly.io Metrics:**
```bash
# View built-in metrics
fly dashboard -a ailee-protocol-core-for-bitcoin

# Monitor:
# - CPU usage
# - Memory usage
# - HTTP request rate
# - Response times
# - Error rates
```

**Checklist:**
- [ ] Fly.io dashboard bookmarked
- [ ] Key metrics identified (CPU, memory, requests, errors)
- [ ] Baseline metrics documented
- [ ] Alert thresholds defined
- [ ] On-call rotation established

---

## Phase 3: Backup & Disaster Recovery

### 3.1 Backup Strategy ✓

**What to Backup:**
1. L2 state database (in `/data` volume)
2. Configuration (in git, but verify)
3. Secrets (document recovery process)

**Automated Backups:**
```bash
# Option 1: Fly.io volume snapshots (manual)
fly volumes snapshots list ailee_data -a ailee-protocol-core-for-bitcoin
fly volumes snapshots create ailee_data -a ailee-protocol-core-for-bitcoin

# Option 2: Application-level backups (recommended)
# Add cron job in container to backup to S3/B2/GCS
```

**Checklist:**
- [ ] Backup frequency defined (hourly/daily)
- [ ] Backup destination configured (S3/B2/GCS)
- [ ] Backup encryption enabled
- [ ] Backup retention policy defined (30 days minimum)
- [ ] Restore procedure documented and tested
- [ ] Recovery Time Objective (RTO) defined
- [ ] Recovery Point Objective (RPO) defined

### 3.2 Disaster Recovery Plan ✓

**Create file:** `docs/INCIDENT_RESPONSE.md`

**Checklist:**
- [ ] Incident response runbook created
- [ ] Emergency contacts documented
- [ ] System halt procedure defined
- [ ] Data recovery procedure documented
- [ ] Communication templates prepared
- [ ] Post-mortem template created

---

## Phase 4: Security Hardening

### 4.1 Authentication ✓

**REQUIRED for production:**

```python
# Update api/config.py to enforce JWT in production
if settings.env == "production" and not settings.jwt_enabled:
    raise ValueError("JWT authentication MUST be enabled in production")
```

**Checklist:**
- [ ] JWT authentication enabled
- [ ] JWT secret is 64+ characters
- [ ] Token expiration configured (60 minutes recommended)
- [ ] Refresh token strategy implemented
- [ ] API key system implemented (optional, for service-to-service)

### 4.2 Rate Limiting ✓

**Current:** 100 requests/60 seconds per IP

**Adjust for production load:**
```bash
fly secrets set \
  AILEE_RATE_LIMIT_REQUESTS=500 \
  AILEE_RATE_LIMIT_WINDOW=60 \
  -a ailee-protocol-core-for-bitcoin
```

**Checklist:**
- [ ] Rate limits tested under load
- [ ] Limits documented for API users
- [ ] Different limits for authenticated users (optional)
- [ ] Rate limit headers returned (X-RateLimit-*)
- [ ] 429 error responses tested

### 4.3 Input Validation ✓

**Checklist:**
- [ ] All API endpoints validate input
- [ ] SQL injection protection (N/A - no SQL)
- [ ] XSS protection (CORS configured)
- [ ] Request size limits enforced
- [ ] Malformed JSON handled gracefully

### 4.4 Audit Logging ✓

**Security audit logs configured:**

```python
# All security events logged to /var/log/ailee/security-audit.log
# - Authentication attempts
# - API key usage
# - Bitcoin RPC calls
# - Transaction broadcasts
# - Configuration changes
```

**Checklist:**
- [ ] Security audit logger integrated
- [ ] All security events logged
- [ ] Logs are tamper-evident (hash chain)
- [ ] Log rotation configured
- [ ] Logs sent to centralized system

---

## Phase 5: Testing (In Staging Environment)

### 5.1 Create Staging Environment ✓

```bash
# Deploy to staging first
fly apps create ailee-protocol-staging

# Copy secrets to staging
fly secrets set --app ailee-protocol-staging \
  AILEE_JWT_SECRET="staging-secret" \
  BITCOIN_RPC_URL="https://testnet..." \
  ...

# Deploy to staging
fly deploy --app ailee-protocol-staging
```

**Checklist:**
- [ ] Staging environment created
- [ ] Staging uses testnet (NOT mainnet)
- [ ] All features tested in staging
- [ ] Load testing completed
- [ ] Failover testing completed
- [ ] Recovery testing completed

### 5.2 Security Testing ✓

**Checklist:**
- [ ] OWASP Top 10 testing completed
- [ ] Dependency scanning (npm audit, pip audit)
- [ ] Secret scanning (gitleaks)
- [ ] Rate limiting tested
- [ ] Authentication bypass tested
- [ ] TLS configuration tested (SSL Labs)

### 5.3 Load Testing ✓

```bash
# Use tool like Apache Bench or k6
ab -n 10000 -c 100 https://ailee-protocol-staging.fly.dev/health

# Or k6
k6 run load-test.js
```

**Checklist:**
- [ ] Load test scenarios defined
- [ ] Expected load + 50% tested
- [ ] Sustained load tested (1+ hour)
- [ ] Spike testing completed
- [ ] Memory leaks checked
- [ ] Performance metrics documented

---

## Phase 6: Documentation

### 6.1 Deployment Documentation ✓

**Checklist:**
- [ ] Deployment guide created
- [ ] Configuration guide updated
- [ ] API documentation current (OpenAPI/Swagger)
- [ ] Architecture diagrams created
- [ ] Dependencies documented

### 6.2 Operational Documentation ✓

**Checklist:**
- [ ] Runbooks created (deployment, rollback, scaling)
- [ ] Troubleshooting guide created
- [ ] Monitoring guide created
- [ ] Backup/restore procedures documented
- [ ] Incident response plan documented

### 6.3 User Documentation ✓

**Checklist:**
- [ ] API documentation published
- [ ] Getting started guide created
- [ ] Code examples provided
- [ ] FAQ created
- [ ] Support channels documented

---

## Phase 7: Legal & Compliance

### 7.1 Legal Review ✓

**Checklist:**
- [ ] Terms of Service created
- [ ] Privacy Policy created
- [ ] Regulatory compliance reviewed
- [ ] Money transmitter license (if required)
- [ ] KYC/AML requirements reviewed

### 7.2 Insurance ✓

**Checklist:**
- [ ] Cyber insurance obtained
- [ ] Professional liability insurance
- [ ] Coverage amounts appropriate for risk

---

## Phase 8: Go-Live Checklist

### 8.1 Final Pre-Launch Checks ✓

**24 Hours Before:**
- [ ] All Phase 1-7 items completed
- [ ] Staging environment stable for 7+ days
- [ ] All team members trained
- [ ] On-call rotation in place
- [ ] Monitoring dashboards ready
- [ ] Incident response plan reviewed

**6 Hours Before:**
- [ ] Final backup of staging data
- [ ] Communication channels tested
- [ ] Status page prepared
- [ ] Rollback plan ready
- [ ] Emergency contacts verified

**1 Hour Before:**
- [ ] Team on standby
- [ ] Monitoring dashboards open
- [ ] Logs streaming
- [ ] Communication channels open

### 8.2 Deployment Steps ✓

```bash
# Step 1: Verify all secrets are set
fly secrets list -a ailee-protocol-core-for-bitcoin

# Step 2: Update fly.toml (already done)
# Verify: min_machines_running = 1
# Verify: memory = '2gb'
# Verify: health checks configured

# Step 3: Deploy
fly deploy -a ailee-protocol-core-for-bitcoin

# Step 4: Verify deployment
fly status -a ailee-protocol-core-for-bitcoin

# Step 5: Check health
curl https://ailee-protocol-core-for-bitcoin.fly.dev/health

# Step 6: Check logs
fly logs -a ailee-protocol-core-for-bitcoin

# Step 7: Monitor for 1 hour before announcing
```

### 8.3 Post-Launch Monitoring ✓

**First 24 Hours:**
- [ ] Monitor every hour
- [ ] Check error rates
- [ ] Verify backups running
- [ ] Check performance metrics
- [ ] Review security logs

**First Week:**
- [ ] Daily health checks
- [ ] Weekly team review
- [ ] User feedback collection
- [ ] Performance optimization

---

## Phase 9: Phased Rollout Strategy

### 9.1 Limited Beta (Recommended)

**Week 1-2: Internal Testing**
- [ ] Deploy to production
- [ ] Internal team testing only
- [ ] Testnet Bitcoin only
- [ ] No public announcement

**Week 3-4: Closed Beta**
- [ ] Invite 10-20 trusted users
- [ ] Testnet Bitcoin only
- [ ] Gather feedback
- [ ] Monitor closely

**Week 5-8: Open Beta (Testnet)**
- [ ] Public announcement
- [ ] Testnet Bitcoin only
- [ ] Cap deposit limits
- [ ] 24/7 monitoring

**Week 9+: Mainnet Decision**
- [ ] Security audit completed
- [ ] Penetration testing completed
- [ ] No critical issues for 30+ days
- [ ] User feedback positive
- [ ] Team consensus

---

## GO/NO-GO Decision

**ALL must be ✅ to proceed with deployment:**

- [ ] All secrets configured via Fly.io secrets
- [ ] Bitcoin RPC nodes configured (minimum 1 for testnet, 3 for mainnet)
- [ ] Persistent storage configured
- [ ] JWT authentication enabled
- [ ] Rate limiting configured
- [ ] Monitoring and alerting operational
- [ ] Backups configured and tested
- [ ] Incident response plan documented
- [ ] Staging environment tested successfully
- [ ] Load testing completed
- [ ] Security testing completed
- [ ] Team trained and on-call rotation set
- [ ] Legal compliance verified
- [ ] Rollback plan ready

**ANY ❌ means NO-GO - fix before deploying!**

---

## Emergency Procedures

### If Something Goes Wrong:

```bash
# Immediately scale to zero
fly scale count 0 -a ailee-protocol-core-for-bitcoin

# Preserve logs
fly logs -a ailee-protocol-core-for-bitcoin > incident-$(date +%Y%m%d-%H%M%S).log

# Notify team
# Follow incident response plan
```

---

## Timeline Estimate

- **Phase 1-2:** 1-2 days (setup)
- **Phase 3-4:** 2-3 days (security & backups)
- **Phase 5:** 3-5 days (testing)
- **Phase 6-7:** 2-3 days (docs & legal)
- **Phase 8-9:** 4-8 weeks (phased rollout)

**Minimum:** 2 weeks of focused work before beta launch  
**Recommended:** 1 month before testnet, 3 months before mainnet

---

**Next Step:** Start with Phase 1 - get all secrets configured properly!
