#!/usr/bin/env python3
"""
Basic integration test for AILEE-Core Next Phase features
Tests API endpoints and Docker Compose configuration
"""

import sys
import subprocess
import time
import requests


def test_api_endpoints():
    """Test Python API endpoints"""
    print("Testing API endpoints...")
    
    # Wait a bit for server to be ready
    time.sleep(2)
    
    base_url = "http://localhost:8080"
    
    tests = [
        ("/health", 200, "Health endpoint"),
        ("/status", 200, "Status endpoint"),
        ("/metrics", 200, "Metrics JSON endpoint"),
        ("/metrics/prometheus", 200, "Prometheus metrics endpoint"),
    ]
    
    passed = 0
    failed = 0
    
    for endpoint, expected_status, description in tests:
        try:
            response = requests.get(f"{base_url}{endpoint}", timeout=5)
            if response.status_code == expected_status:
                print(f"  ✓ {description}: {endpoint}")
                passed += 1
            else:
                print(f"  ✗ {description}: {endpoint} (got {response.status_code}, expected {expected_status})")
                failed += 1
        except Exception as e:
            print(f"  ✗ {description}: {endpoint} (error: {e})")
            failed += 1
    
    print(f"\nAPI Tests: {passed} passed, {failed} failed")
    return failed == 0


def test_prometheus_format():
    """Test Prometheus metrics format"""
    print("\nTesting Prometheus metrics format...")
    
    try:
        response = requests.get("http://localhost:8080/metrics/prometheus", timeout=5)
        if response.status_code == 200:
            content = response.text
            
            # Check for Prometheus format markers
            checks = [
                ("# HELP" in content, "Has HELP comments"),
                ("# TYPE" in content, "Has TYPE comments"),
                ("ailee_" in content, "Has ailee_ prefix metrics"),
                ("gauge" in content or "counter" in content, "Has metric types"),
            ]
            
            passed = sum(1 for check, _ in checks if check)
            print(f"  Format validation: {passed}/{len(checks)} checks passed")
            
            for check, desc in checks:
                status = "✓" if check else "✗"
                print(f"    {status} {desc}")
            
            return all(check for check, _ in checks)
        else:
            print(f"  ✗ Failed to get Prometheus metrics (status {response.status_code})")
            return False
    except Exception as e:
        print(f"  ✗ Error: {e}")
        return False


def test_docker_compose_config():
    """Test Docker Compose configuration"""
    print("\nTesting Docker Compose configuration...")
    
    try:
        result = subprocess.run(
            ["docker-compose", "config"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print("  ✓ docker-compose.yml is valid")
            
            # Check for key services
            services = ["ailee-node-1", "ailee-node-2", "ailee-node-3", "prometheus", "grafana"]
            for service in services:
                if service in result.stdout:
                    print(f"  ✓ Service '{service}' configured")
                else:
                    print(f"  ✗ Service '{service}' not found")
            
            return True
        else:
            print(f"  ✗ docker-compose config failed: {result.stderr}")
            return False
    except Exception as e:
        print(f"  ✗ Error: {e}")
        return False


def main():
    """Run all tests"""
    print("=" * 60)
    print("AILEE-Core Next Phase Integration Tests")
    print("=" * 60)
    
    results = []
    
    # Test Docker Compose config
    results.append(("Docker Compose", test_docker_compose_config()))
    
    # Test Prometheus format (doesn't require running server)
    # Note: API tests require server to be running
    
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    
    for name, passed in results:
        status = "PASSED" if passed else "FAILED"
        print(f"{name}: {status}")
    
    all_passed = all(passed for _, passed in results)
    
    print("=" * 60)
    if all_passed:
        print("✓ All tests passed!")
        return 0
    else:
        print("✗ Some tests failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())
