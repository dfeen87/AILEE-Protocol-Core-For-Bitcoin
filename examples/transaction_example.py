#!/usr/bin/env python3
"""
Example: Submit and Query Transactions on AILEE Blockchain

This example demonstrates how to:
1. Submit transactions to the blockchain
2. Query transactions by hash
3. Query transactions by address
4. List all transactions
"""

import requests
import json
import time

# API endpoint (change this to your server URL)
API_URL = "http://localhost:8080"


def submit_transaction(from_addr, to_addr, amount, data=None):
    """Submit a transaction to the blockchain"""
    url = f"{API_URL}/transactions/submit"
    payload = {
        "from_address": from_addr,
        "to_address": to_addr,
        "amount": amount
    }
    if data:
        payload["data"] = data
    
    response = requests.post(url, json=payload)
    response.raise_for_status()
    return response.json()


def get_transaction(tx_hash):
    """Get a transaction by its hash"""
    url = f"{API_URL}/transactions/hash/{tx_hash}"
    response = requests.get(url)
    response.raise_for_status()
    return response.json()


def get_transactions_by_address(address, page=1, page_size=10):
    """Get all transactions for an address"""
    url = f"{API_URL}/transactions/address/{address}"
    params = {"page": page, "page_size": page_size}
    response = requests.get(url, params=params)
    response.raise_for_status()
    return response.json()


def list_transactions(page=1, page_size=10, status=None):
    """List all transactions"""
    url = f"{API_URL}/transactions/list"
    params = {"page": page, "page_size": page_size}
    if status:
        params["status"] = status
    response = requests.get(url, params=params)
    response.raise_for_status()
    return response.json()


def main():
    print("=" * 80)
    print("AILEE Blockchain - Transaction Example")
    print("=" * 80)
    print()
    
    # 1. Submit transactions
    print("1. Submitting transactions...")
    print("-" * 80)
    
    tx1 = submit_transaction("alice", "bob", 1000, "Payment for services")
    print(f"✓ Transaction 1: {tx1['tx_hash'][:16]}...")
    print(f"  From: {tx1['from_address']} → To: {tx1['to_address']}")
    print(f"  Amount: {tx1['amount']} satoshis")
    print()
    
    tx2 = submit_transaction("alice", "charlie", 500)
    print(f"✓ Transaction 2: {tx2['tx_hash'][:16]}...")
    print(f"  From: {tx2['from_address']} → To: {tx2['to_address']}")
    print(f"  Amount: {tx2['amount']} satoshis")
    print()
    
    tx3 = submit_transaction("bob", "charlie", 300, "Partial payment")
    print(f"✓ Transaction 3: {tx3['tx_hash'][:16]}...")
    print(f"  From: {tx3['from_address']} → To: {tx3['to_address']}")
    print(f"  Amount: {tx3['amount']} satoshis")
    print()
    
    time.sleep(0.5)  # Small delay to ensure transactions are stored
    
    # 2. Query transaction by hash
    print("\n2. Querying transaction by hash...")
    print("-" * 80)
    tx = get_transaction(tx1['tx_hash'])
    print(f"Transaction: {tx['tx_hash'][:16]}...")
    print(f"  From: {tx['from_address']} → To: {tx['to_address']}")
    print(f"  Amount: {tx['amount']} satoshis")
    print(f"  Status: {tx['status']}")
    print(f"  Data: {tx['data']}")
    print()
    
    # 3. Query transactions by address
    print("\n3. Querying transactions by address...")
    print("-" * 80)
    
    print("Alice's transactions:")
    alice_txs = get_transactions_by_address("alice")
    for tx in alice_txs['transactions']:
        direction = "sent" if tx['from_address'] == "alice" else "received"
        print(f"  • {tx['tx_hash'][:16]}... ({direction})")
        print(f"    {tx['from_address']} → {tx['to_address']}: {tx['amount']} satoshis")
    print(f"Total: {alice_txs['total_count']} transactions")
    print()
    
    print("Charlie's transactions:")
    charlie_txs = get_transactions_by_address("charlie")
    for tx in charlie_txs['transactions']:
        direction = "sent" if tx['from_address'] == "charlie" else "received"
        print(f"  • {tx['tx_hash'][:16]}... ({direction})")
        print(f"    {tx['from_address']} → {tx['to_address']}: {tx['amount']} satoshis")
    print(f"Total: {charlie_txs['total_count']} transactions")
    print()
    
    # 4. List all transactions
    print("\n4. Listing all transactions...")
    print("-" * 80)
    all_txs = list_transactions(page_size=10)
    print(f"Total transactions in blockchain: {all_txs['total_count']}")
    print(f"Showing page {all_txs['page']} (max {all_txs['page_size']} per page)")
    print()
    for i, tx in enumerate(all_txs['transactions'], 1):
        print(f"{i}. {tx['tx_hash'][:16]}...")
        print(f"   {tx['from_address']} → {tx['to_address']}: {tx['amount']} satoshis")
        if tx['data']:
            print(f"   Data: {tx['data']}")
        print(f"   Status: {tx['status']} | Time: {tx['timestamp']}")
        print()
    
    print("=" * 80)
    print("Example completed successfully!")
    print("=" * 80)


if __name__ == "__main__":
    try:
        main()
    except requests.exceptions.ConnectionError:
        print("ERROR: Could not connect to AILEE API server")
        print(f"Make sure the server is running at {API_URL}")
        print("\nStart the server with:")
        print("  uvicorn api.main:app --host 0.0.0.0 --port 8080")
    except requests.exceptions.HTTPError as e:
        print(f"ERROR: HTTP {e.response.status_code}")
        print(e.response.text)
    except Exception as e:
        print(f"ERROR: {e}")
