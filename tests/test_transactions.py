"""
Tests for transaction endpoints
"""

import pytest
from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)


def test_submit_transaction():
    """Test submitting a valid transaction"""
    response = client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "bob",
            "amount": 1000,
            "data": "Test payment"
        }
    )
    
    assert response.status_code == 200
    data = response.json()
    assert "tx_hash" in data
    assert data["from_address"] == "alice"
    assert data["to_address"] == "bob"
    assert data["amount"] == 1000
    assert data["status"] == "pending"
    assert "timestamp" in data


def test_submit_transaction_same_address():
    """Test that submitting to same address fails"""
    response = client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "alice",
            "amount": 100
        }
    )
    
    assert response.status_code == 400
    assert "same address" in response.json()["detail"]


def test_submit_transaction_invalid_amount():
    """Test that invalid amount fails validation"""
    response = client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "bob",
            "amount": 0
        }
    )
    
    assert response.status_code == 422  # Pydantic validation error


def test_list_transactions():
    """Test listing all transactions"""
    # Submit a transaction first
    client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "bob",
            "amount": 500
        }
    )
    
    # List transactions
    response = client.get("/transactions/list")
    assert response.status_code == 200
    data = response.json()
    assert "transactions" in data
    assert "total_count" in data
    assert "page" in data
    assert "page_size" in data


def test_get_transaction_by_hash():
    """Test getting a transaction by hash"""
    # Submit a transaction
    submit_response = client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "charlie",
            "amount": 750
        }
    )
    tx_hash = submit_response.json()["tx_hash"]
    
    # Get the transaction
    response = client.get(f"/transactions/hash/{tx_hash}")
    assert response.status_code == 200
    data = response.json()
    assert data["tx_hash"] == tx_hash
    assert data["from_address"] == "alice"
    assert data["to_address"] == "charlie"
    assert data["amount"] == 750


def test_get_nonexistent_transaction():
    """Test getting a nonexistent transaction"""
    response = client.get("/transactions/hash/nonexistent123")
    assert response.status_code == 404
    assert "not found" in response.json()["detail"]


def test_get_transactions_by_address():
    """Test getting transactions for an address"""
    # Submit transactions
    client.post(
        "/transactions/submit",
        json={
            "from_address": "alice",
            "to_address": "bob",
            "amount": 100
        }
    )
    client.post(
        "/transactions/submit",
        json={
            "from_address": "bob",
            "to_address": "charlie",
            "amount": 200
        }
    )
    
    # Get Alice's transactions
    response = client.get("/transactions/address/alice")
    assert response.status_code == 200
    data = response.json()
    assert "transactions" in data
    # Alice should have at least one transaction
    assert data["total_count"] >= 1
    
    # Get Bob's transactions (sender and receiver)
    response = client.get("/transactions/address/bob")
    assert response.status_code == 200
    data = response.json()
    # Bob should have at least two transactions (received from alice, sent to charlie)
    assert data["total_count"] >= 2


def test_transaction_pagination():
    """Test transaction list pagination"""
    # Submit multiple transactions
    for i in range(5):
        client.post(
            "/transactions/submit",
            json={
                "from_address": f"user{i}",
                "to_address": f"user{i+1}",
                "amount": 100 * (i + 1)
            }
        )
    
    # Get first page
    response = client.get("/transactions/list?page=1&page_size=2")
    assert response.status_code == 200
    data = response.json()
    assert data["page"] == 1
    assert data["page_size"] == 2
    assert len(data["transactions"]) <= 2
    
    # Get second page
    response = client.get("/transactions/list?page=2&page_size=2")
    assert response.status_code == 200
    data = response.json()
    assert data["page"] == 2


def test_transaction_hash_deterministic():
    """Test that transaction hash is deterministic"""
    tx_data = {
        "from_address": "alice",
        "to_address": "bob",
        "amount": 1000,
        "data": "Test"
    }
    
    # Submit same transaction twice should produce different hashes
    # (because timestamp is different)
    response1 = client.post("/transactions/submit", json=tx_data)
    response2 = client.post("/transactions/submit", json=tx_data)
    
    # Hashes should be different due to different timestamps
    assert response1.json()["tx_hash"] != response2.json()["tx_hash"]
