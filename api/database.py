"""
SQLite Database Layer for AILEE-Core
Provides persistent storage for transactions and anchor events
"""

import logging
import os
from contextlib import asynccontextmanager
from typing import Optional, List, Dict, Any, Tuple

import aiosqlite

logger = logging.getLogger(__name__)

# Database connection pool
_db_pool: Optional[aiosqlite.Connection] = None


# Schema version for migrations
CURRENT_SCHEMA_VERSION = 1


async def init_database(db_path: str) -> None:
    """
    Initialize the database connection pool and schema
    
    Args:
        db_path: Path to SQLite database file
    """
    global _db_pool
    
    # Ensure directory exists
    db_dir = os.path.dirname(db_path)
    if db_dir and not os.path.exists(db_dir):
        os.makedirs(db_dir, exist_ok=True)
        logger.info(f"Created database directory: {db_dir}")
    
    # Connect to database
    _db_pool = await aiosqlite.connect(db_path)
    _db_pool.row_factory = aiosqlite.Row
    
    # Enable WAL mode for better concurrency
    await _db_pool.execute("PRAGMA journal_mode=WAL")
    await _db_pool.execute("PRAGMA synchronous=NORMAL")
    
    logger.info(f"Connected to database: {db_path}")
    
    # Initialize schema
    await _init_schema()
    
    logger.info("Database initialized successfully")


async def _init_schema() -> None:
    """Initialize database schema with migrations"""
    
    # Create schema_version table if it doesn't exist
    await _db_pool.execute("""
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER PRIMARY KEY,
            applied_at TEXT NOT NULL
        )
    """)
    await _db_pool.commit()
    
    # Get current schema version
    cursor = await _db_pool.execute("SELECT MAX(version) as version FROM schema_version")
    row = await cursor.fetchone()
    current_version = row[0] if row[0] is not None else 0
    
    logger.info(f"Current schema version: {current_version}")
    
    # Apply migrations
    if current_version < 1:
        await _apply_migration_v1()
        current_version = 1
    
    logger.info(f"Schema is up to date (version {current_version})")


async def _apply_migration_v1() -> None:
    """Apply migration version 1: Create transactions and anchor_events tables"""
    from datetime import datetime, timezone
    
    logger.info("Applying migration v1...")
    
    # Create transactions table
    await _db_pool.execute("""
        CREATE TABLE IF NOT EXISTS transactions (
            tx_hash TEXT PRIMARY KEY,
            from_address TEXT NOT NULL,
            to_address TEXT NOT NULL,
            amount INTEGER NOT NULL,
            data TEXT,
            status TEXT NOT NULL,
            block_height INTEGER,
            timestamp TEXT NOT NULL
        )
    """)
    
    # Create indexes for transactions
    await _db_pool.execute("""
        CREATE INDEX IF NOT EXISTS idx_from_address ON transactions(from_address)
    """)
    await _db_pool.execute("""
        CREATE INDEX IF NOT EXISTS idx_to_address ON transactions(to_address)
    """)
    await _db_pool.execute("""
        CREATE INDEX IF NOT EXISTS idx_status ON transactions(status)
    """)
    await _db_pool.execute("""
        CREATE INDEX IF NOT EXISTS idx_block_height ON transactions(block_height)
    """)
    
    # Create anchor_events table
    await _db_pool.execute("""
        CREATE TABLE IF NOT EXISTS anchor_events (
            anchor_height INTEGER PRIMARY KEY,
            state_root TEXT NOT NULL,
            bitcoin_txid TEXT NOT NULL,
            timestamp TEXT NOT NULL
        )
    """)
    
    # Create index for anchor_events
    await _db_pool.execute("""
        CREATE INDEX IF NOT EXISTS idx_bitcoin_txid ON anchor_events(bitcoin_txid)
    """)
    
    # Record migration
    await _db_pool.execute(
        "INSERT INTO schema_version (version, applied_at) VALUES (?, ?)",
        (1, datetime.now(timezone.utc).isoformat())
    )
    
    await _db_pool.commit()
    
    logger.info("Migration v1 applied successfully")


async def close_database() -> None:
    """Close the database connection"""
    global _db_pool
    
    if _db_pool:
        await _db_pool.close()
        _db_pool = None
        logger.info("Database connection closed")


def get_db() -> aiosqlite.Connection:
    """
    Get the database connection
    
    Returns:
        Database connection
        
    Raises:
        RuntimeError: If database is not initialized
    """
    if _db_pool is None:
        raise RuntimeError("Database not initialized. Call init_database() first.")
    return _db_pool


# Transaction operations

async def insert_transaction(tx_data: Dict[str, Any]) -> None:
    """
    Insert a transaction into the database
    
    Args:
        tx_data: Transaction data dictionary
        
    Raises:
        Exception: If insert fails
    """
    db = get_db()
    
    await db.execute("""
        INSERT INTO transactions (
            tx_hash, from_address, to_address, amount, data, status, block_height, timestamp
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        tx_data["tx_hash"],
        tx_data["from_address"],
        tx_data["to_address"],
        tx_data["amount"],
        tx_data.get("data"),
        tx_data["status"],
        tx_data.get("block_height"),
        tx_data["timestamp"]
    ))
    
    await db.commit()
    
    logger.debug(f"Inserted transaction: {tx_data['tx_hash'][:16]}...")


async def get_transaction_by_hash(tx_hash: str) -> Optional[Dict[str, Any]]:
    """
    Get a transaction by hash
    
    Args:
        tx_hash: Transaction hash
        
    Returns:
        Transaction data or None if not found
    """
    db = get_db()
    
    cursor = await db.execute(
        "SELECT * FROM transactions WHERE tx_hash = ?",
        (tx_hash,)
    )
    row = await cursor.fetchone()
    
    if row:
        return dict(row)
    return None


async def get_transactions_by_address(
    address: str,
    page: int = 1,
    page_size: int = 10
) -> Tuple[List[Dict[str, Any]], int]:
    """
    Get transactions by address with pagination
    
    Args:
        address: Address to query
        page: Page number (1-indexed)
        page_size: Items per page
        
    Returns:
        Tuple of (transactions list, total count)
    """
    db = get_db()
    
    # Get total count
    cursor = await db.execute(
        "SELECT COUNT(*) as count FROM transactions WHERE from_address = ? OR to_address = ?",
        (address, address)
    )
    row = await cursor.fetchone()
    total_count = row[0]
    
    # Get paginated results
    offset = (page - 1) * page_size
    cursor = await db.execute("""
        SELECT * FROM transactions 
        WHERE from_address = ? OR to_address = ?
        ORDER BY timestamp DESC
        LIMIT ? OFFSET ?
    """, (address, address, page_size, offset))
    
    rows = await cursor.fetchall()
    transactions = [dict(row) for row in rows]
    
    return transactions, total_count


async def get_transactions(
    page: int = 1,
    page_size: int = 10,
    status: Optional[str] = None
) -> Tuple[List[Dict[str, Any]], int]:
    """
    Get all transactions with pagination and optional status filter
    
    Args:
        page: Page number (1-indexed)
        page_size: Items per page
        status: Optional status filter
        
    Returns:
        Tuple of (transactions list, total count)
    """
    db = get_db()
    
    # Build query based on status filter
    if status:
        count_query = "SELECT COUNT(*) as count FROM transactions WHERE status = ?"
        select_query = """
            SELECT * FROM transactions 
            WHERE status = ?
            ORDER BY timestamp DESC
            LIMIT ? OFFSET ?
        """
        params_count = (status,)
    else:
        count_query = "SELECT COUNT(*) as count FROM transactions"
        select_query = """
            SELECT * FROM transactions 
            ORDER BY timestamp DESC
            LIMIT ? OFFSET ?
        """
        params_count = ()
    
    # Get total count
    cursor = await db.execute(count_query, params_count)
    row = await cursor.fetchone()
    total_count = row[0]
    
    # Get paginated results
    offset = (page - 1) * page_size
    if status:
        params_select = (status, page_size, offset)
    else:
        params_select = (page_size, offset)
    
    cursor = await db.execute(select_query, params_select)
    rows = await cursor.fetchall()
    transactions = [dict(row) for row in rows]
    
    return transactions, total_count


# Anchor event operations

async def insert_anchor_event(anchor_data: Dict[str, Any]) -> None:
    """
    Insert an anchor event into the database
    
    Args:
        anchor_data: Anchor event data dictionary
        
    Raises:
        Exception: If insert fails
    """
    db = get_db()
    
    await db.execute("""
        INSERT OR REPLACE INTO anchor_events (
            anchor_height, state_root, bitcoin_txid, timestamp
        ) VALUES (?, ?, ?, ?)
    """, (
        anchor_data["anchor_height"],
        anchor_data["state_root"],
        anchor_data["bitcoin_txid"],
        anchor_data["timestamp"]
    ))
    
    await db.commit()
    
    logger.debug(f"Inserted anchor event at height: {anchor_data['anchor_height']}")


async def get_anchor_by_height(height: int) -> Optional[Dict[str, Any]]:
    """
    Get an anchor event by height
    
    Args:
        height: Anchor height
        
    Returns:
        Anchor event data or None if not found
    """
    db = get_db()
    
    cursor = await db.execute(
        "SELECT * FROM anchor_events WHERE anchor_height = ?",
        (height,)
    )
    row = await cursor.fetchone()
    
    if row:
        return dict(row)
    return None


async def get_anchors(
    page: int = 1,
    page_size: int = 10
) -> Tuple[List[Dict[str, Any]], int]:
    """
    Get all anchor events with pagination
    
    Args:
        page: Page number (1-indexed)
        page_size: Items per page
        
    Returns:
        Tuple of (anchors list, total count)
    """
    db = get_db()
    
    # Get total count
    cursor = await db.execute("SELECT COUNT(*) as count FROM anchor_events")
    row = await cursor.fetchone()
    total_count = row[0]
    
    # Get paginated results (ordered by height DESC - most recent first)
    offset = (page - 1) * page_size
    cursor = await db.execute("""
        SELECT * FROM anchor_events 
        ORDER BY anchor_height DESC
        LIMIT ? OFFSET ?
    """, (page_size, offset))
    
    rows = await cursor.fetchall()
    anchors = [dict(row) for row in rows]
    
    return anchors, total_count
