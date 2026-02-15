#!/bin/bash
# Install Git hooks to prevent committing secrets
# This is critical for public repositories!

set -e

REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

# Colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Installing AILEE-Core Git Security Hooks ===${NC}"
echo ""
echo -e "${YELLOW}⚠️  This repository is PUBLIC - these hooks prevent secret leaks!${NC}"
echo ""

# Create hooks directory if it doesn't exist
mkdir -p "$HOOKS_DIR"

# Install pre-commit hook
echo "Installing pre-commit hook..."
cat > "$HOOKS_DIR/pre-commit" << 'HOOK_EOF'
#!/bin/bash
# AILEE-Core Pre-Commit Hook - Prevent Secret Leaks
# This repository is PUBLIC - secrets must never be committed!

RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "🔍 Scanning for secrets before commit..."

# Get list of files being committed
FILES=$(git diff --cached --name-only --diff-filter=ACM)

# Flag to track if secrets found
SECRETS_FOUND=0

# Check for common secret patterns
SECRET_PATTERNS=(
    "password.*=.*['\"].*['\"]"
    "api[_-]?key.*=.*['\"].*['\"]"
    "secret.*=.*['\"].*['\"]"
    "token.*=.*['\"].*['\"]"
    "private[_-]?key.*=.*['\"].*['\"]"
    "aws[_-]?secret"
    "BEGIN.*PRIVATE.*KEY"
    "-----BEGIN RSA PRIVATE KEY-----"
    "-----BEGIN EC PRIVATE KEY-----"
    "-----BEGIN OPENSSH PRIVATE KEY-----"
)

# Check each file
for FILE in $FILES; do
    # Skip if file doesn't exist (deleted files)
    if [ ! -f "$FILE" ]; then
        continue
    fi
    
    # Skip binary files
    if file "$FILE" | grep -q "binary"; then
        continue
    fi
    
    # Check for forbidden file types
    case "$FILE" in
        *.key|*.pem|*.p12|*.pfx|*.jks)
            if [[ "$FILE" != *"public"* ]]; then
                echo -e "${RED}❌ BLOCKED: Private key file detected: $FILE${NC}"
                echo "   Private keys must NEVER be committed to this public repository!"
                SECRETS_FOUND=1
            fi
            ;;
        .env|secrets.yaml|secrets.yml)
            echo -e "${RED}❌ BLOCKED: Secret file detected: $FILE${NC}"
            echo "   Use .env.example or secrets.example.yaml instead!"
            SECRETS_FOUND=1
            ;;
    esac
    
    # Check file content for secret patterns
    for PATTERN in "${SECRET_PATTERNS[@]}"; do
        if git diff --cached "$FILE" | grep -iE "$PATTERN" > /dev/null; then
            # Check if it's using environment variable syntax (safe)
            if git diff --cached "$FILE" | grep -iE "$PATTERN" | grep -E '\$\{.*\}|process\.env\.' > /dev/null; then
                # This is safe - using environment variables
                continue
            fi
            
            echo -e "${RED}❌ BLOCKED: Potential secret detected in $FILE${NC}"
            echo "   Pattern matched: $PATTERN"
            echo "   Matched lines:"
            git diff --cached "$FILE" | grep -iE "$PATTERN" | sed 's/^/   /'
            SECRETS_FOUND=1
        fi
    done
done

# Check for high-entropy strings (potential API keys/secrets)
for FILE in $FILES; do
    if [ ! -f "$FILE" ]; then
        continue
    fi
    
    # Skip certain file types
    case "$FILE" in
        *.md|*.txt|*.json|*.yaml|*.yml)
            # Check for long base64-like strings
            if git diff --cached "$FILE" | grep -E '[A-Za-z0-9+/]{40,}' | grep -v '${' | grep -v 'example' > /dev/null; then
                echo -e "${YELLOW}⚠️  Warning: High-entropy string in $FILE${NC}"
                echo "   This might be an API key or secret. Please review:"
                git diff --cached "$FILE" | grep -E '[A-Za-z0-9+/]{40,}' | head -n 3 | sed 's/^/   /'
                echo ""
                echo "   If this is a secret, STOP and remove it!"
                echo "   If this is safe (example, documentation), you can proceed."
                echo ""
                read -p "   Is this safe to commit? (yes/no): " RESPONSE
                if [ "$RESPONSE" != "yes" ]; then
                    SECRETS_FOUND=1
                fi
            fi
            ;;
    esac
done

# Final verdict
if [ $SECRETS_FOUND -eq 1 ]; then
    echo ""
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  COMMIT BLOCKED - Secrets Detected!                       ║${NC}"
    echo -e "${RED}║                                                            ║${NC}"
    echo -e "${RED}║  This is a PUBLIC repository - secrets must never be      ║${NC}"
    echo -e "${RED}║  committed! Anyone on the internet can see this repo.     ║${NC}"
    echo -e "${RED}║                                                            ║${NC}"
    echo -e "${RED}║  Actions:                                                  ║${NC}"
    echo -e "${RED}║  1. Remove secrets from staged files                      ║${NC}"
    echo -e "${RED}║  2. Use environment variables instead                     ║${NC}"
    echo -e "${RED}║  3. See SECURITY_PUBLIC_REPO.md for guidance              ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ No secrets detected - commit allowed${NC}"
exit 0
HOOK_EOF

chmod +x "$HOOKS_DIR/pre-commit"
echo -e "${GREEN}✓${NC} Pre-commit hook installed"

# Install commit-msg hook for additional checks
echo ""
echo "Installing commit-msg hook..."
cat > "$HOOKS_DIR/commit-msg" << 'HOOK_EOF'
#!/bin/bash
# AILEE-Core Commit Message Hook

COMMIT_MSG_FILE=$1
COMMIT_MSG=$(cat "$COMMIT_MSG_FILE")

# Check if commit message contains potential secrets
if echo "$COMMIT_MSG" | grep -iE 'password|secret|key|token' | grep -vE 'add.*password|update.*secret|remove.*key'; then
    echo "⚠️  Warning: Commit message may reference secrets"
    echo "   Ensure no actual secrets are in the message"
fi

# Check for common accidentally committed patterns
if echo "$COMMIT_MSG" | grep -iE 'fuck|shit|damn|xxx|todo.*remove'; then
    echo "⚠️  Warning: Commit message contains informal language or TODOs"
fi
HOOK_EOF

chmod +x "$HOOKS_DIR/commit-msg"
echo -e "${GREEN}✓${NC} Commit-msg hook installed"

# Test the hooks
echo ""
echo "Testing hooks..."
if [ -x "$HOOKS_DIR/pre-commit" ]; then
    echo -e "${GREEN}✓${NC} Pre-commit hook is executable"
else
    echo -e "${RED}✗${NC} Pre-commit hook is not executable"
    exit 1
fi

# Create bypass instructions
echo ""
echo -e "${BLUE}=== Hook Installation Complete ===${NC}"
echo ""
echo "Git hooks installed successfully!"
echo ""
echo "What this protects against:"
echo "  • Committing .env files"
echo "  • Committing private keys (.key, .pem)"
echo "  • Committing secrets.yaml files"
echo "  • Hardcoded passwords, API keys, tokens"
echo "  • High-entropy strings (potential secrets)"
echo ""
echo -e "${YELLOW}⚠️  IMPORTANT: Hooks only run on YOUR machine${NC}"
echo "   Other contributors must also install these hooks!"
echo ""
echo "To bypass hooks (USE WITH EXTREME CAUTION):"
echo "  git commit --no-verify"
echo ""
echo -e "${RED}⚠️  Never bypass hooks for public repository commits!${NC}"
echo ""
echo "Additional protection:"
echo "  • Install gitleaks: brew install gitleaks"
echo "  • Run: gitleaks detect"
echo "  • Enable GitHub secret scanning (free for public repos)"
echo ""
