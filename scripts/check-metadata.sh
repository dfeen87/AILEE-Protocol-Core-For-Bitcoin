#!/bin/bash
# check-metadata.sh - Validate metadata consistency across AILEE-Core project files
# Usage: ./scripts/check-metadata.sh

set -e

echo "🔍 AILEE-Core Metadata Consistency Check"
echo "========================================"
echo ""

# Extract versions from different files
CMAKE_VERSION=$(grep "project(AILEE_Core VERSION" CMakeLists.txt | grep -oP '\d+\.\d+\.\d+' || echo "NOT_FOUND")
PYPROJECT_VERSION=$(grep "^version = " pyproject.toml | grep -oP '\d+\.\d+\.\d+' || echo "NOT_FOUND")
CITATION_VERSION=$(grep "^version: " CITATION.cff | grep -oP '\d+\.\d+\.\d+' || echo "NOT_FOUND")
API_VERSION=$(grep -A 1 "app_version" api/config.py | grep "default=" | grep -oP '\d+\.\d+\.\d+' || echo "NOT_FOUND")

echo "📌 Version Numbers:"
echo "  CMakeLists.txt:  $CMAKE_VERSION"
echo "  pyproject.toml:  $PYPROJECT_VERSION"
echo "  CITATION.cff:    $CITATION_VERSION"
echo "  api/config.py:   $API_VERSION"
echo ""

# Check version consistency
VERSION_MATCH=true
if [ "$CMAKE_VERSION" != "$PYPROJECT_VERSION" ] || \
   [ "$PYPROJECT_VERSION" != "$CITATION_VERSION" ] || \
   [ "$CITATION_VERSION" != "$API_VERSION" ]; then
    VERSION_MATCH=false
fi

if [ "$VERSION_MATCH" = true ]; then
    echo "✅ All version numbers match: $CMAKE_VERSION"
else
    echo "❌ Version mismatch detected!"
    echo "   Please update all version numbers to match."
    exit 1
fi
echo ""

# Check for required files
echo "📄 Required Metadata Files:"
REQUIRED_FILES=("CMakeLists.txt" "pyproject.toml" "CITATION.cff" "requirements.txt" "README.md" "LICENSE")
ALL_FILES_EXIST=true

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✅ $file"
    else
        echo "  ❌ $file (missing)"
        ALL_FILES_EXIST=false
    fi
done
echo ""

if [ "$ALL_FILES_EXIST" = false ]; then
    echo "❌ Some required files are missing!"
    exit 1
fi

# Check pyproject.toml syntax
echo "🔧 Validating pyproject.toml:"
if command -v python3 &> /dev/null; then
    if python3 -c "import tomllib; tomllib.load(open('pyproject.toml', 'rb'))" 2>/dev/null || \
       python3 -c "import tomli; tomli.load(open('pyproject.toml', 'rb'))" 2>/dev/null || \
       python3 -c "import toml; toml.load('pyproject.toml')" 2>/dev/null; then
        echo "  ✅ pyproject.toml is valid TOML"
    else
        echo "  ⚠️  Could not validate pyproject.toml (install tomli/toml package)"
    fi
else
    echo "  ⚠️  Python3 not found, skipping validation"
fi
echo ""

# Check CITATION.cff syntax
echo "🔧 Validating CITATION.cff:"
if command -v python3 &> /dev/null; then
    if python3 -c "import yaml; yaml.safe_load(open('CITATION.cff'))" 2>/dev/null; then
        echo "  ✅ CITATION.cff is valid YAML"
    else
        echo "  ⚠️  Could not validate CITATION.cff (install pyyaml package)"
    fi
else
    echo "  ⚠️  Python3 not found, skipping validation"
fi
echo ""

# Check project name consistency
echo "📝 Project Name Consistency:"
PROJECT_NAME="AILEE-Core"
if grep -q "$PROJECT_NAME" README.md && \
   grep -q "$PROJECT_NAME" CITATION.cff && \
   grep -q "ailee-core" pyproject.toml; then
    echo "  ✅ Project name consistent across files"
else
    echo "  ⚠️  Project name variations detected (this may be intentional)"
fi
echo ""

# Summary
echo "========================================"
if [ "$VERSION_MATCH" = true ] && [ "$ALL_FILES_EXIST" = true ]; then
    echo "✅ Metadata consistency check PASSED"
    exit 0
else
    echo "❌ Metadata consistency check FAILED"
    exit 1
fi
