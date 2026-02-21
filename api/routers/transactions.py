"""
Transaction Router
Transaction submission and query endpoints for L2 blockchain
"""

import hashlib
import logging
from datetime import datetime, timezone
from typing import List, Optional
from fastapi import APIRouter, HTTPException, Query, Depends
from pydantic import BaseModel, Field

from api.l2_client import get_ailee_client
from api.auth import verify_api_key
from api import database as db

router = APIRouter()
logger = logging.getLogger(__name__)


class TransactionInput(BaseModel):
    """Transaction input model for submission"""
    from_address: str = Field(
        ..., 
        description="Sender address/peer ID",
        min_length=1,
        max_length=256
    )
    to_address: str = Field(
        ..., 
        description="Recipient address/peer ID",
        min_length=1,
        max_length=256
    )
    amount: int = Field(
        ..., 
        description="Amount to transfer (in satoshis)",
        gt=0
    )
    data: Optional[str] = Field(
        None,
        description="Optional transaction data/memo",
        max_length=1000
    )


class TransactionResponse(BaseModel):
    """Response model for submitted transaction"""
    tx_hash: str = Field(..., description="Transaction hash")
    from_address: str = Field(..., description="Sender address")
    to_address: str = Field(..., description="Recipient address")
    amount: int = Field(..., description="Amount transferred")
    status: str = Field(..., description="Transaction status")
    block_height: Optional[int] = Field(None, description="Block height (if confirmed)")
    timestamp: str = Field(..., description="Transaction timestamp (ISO 8601)")


class TransactionQuery(BaseModel):
    """Transaction query response model"""
    tx_hash: str = Field(..., description="Transaction hash")
    from_address: str = Field(..., description="Sender address")
    to_address: str = Field(..., description="Recipient address")
    amount: int = Field(..., description="Amount transferred")
    status: str = Field(..., description="Transaction status")
    block_height: Optional[int] = Field(None, description="Block height (if confirmed)")
    data: Optional[str] = Field(None, description="Transaction data/memo")
    timestamp: str = Field(..., description="Transaction timestamp (ISO 8601)")


class TransactionListResponse(BaseModel):
    """Response model for transaction list"""
    transactions: List[TransactionQuery]
    total_count: int = Field(..., description="Total number of transactions", ge=0)
    page: int = Field(..., description="Current page number", ge=1)
    page_size: int = Field(..., description="Number of items per page", ge=1)


def compute_transaction_hash(tx_data: dict) -> str:
    """
    Compute a deterministic hash for a transaction
    
    Args:
        tx_data: Transaction data dictionary
        
    Returns:
        Hexadecimal hash string
    """
    # Create deterministic string from transaction data
    tx_string = f"{tx_data['from_address']}:{tx_data['to_address']}:{tx_data['amount']}:{tx_data.get('data', '')}:{tx_data['timestamp']}"
    return hashlib.sha256(tx_string.encode('utf-8')).hexdigest()


@router.post("/submit", response_model=TransactionResponse)
async def submit_transaction(
    tx: TransactionInput,
    api_key: str = Depends(verify_api_key)
):
    """
    Submit Transaction to L2 Blockchain (Protected)
    
    **Authentication Required**: This endpoint requires a valid API key.
    
    Include the API key in the Authorization header:
    ```
    Authorization: Bearer YOUR_API_KEY
    ```
    
    Submits a transaction to the AILEE Layer-2 blockchain for processing.
    The transaction will be validated and included in the next block.
    
    Args:
        tx: Transaction details (from, to, amount, optional data)
        api_key: API key from Authorization header (validated)
        
    Returns:
        Transaction confirmation with hash and status
    """
    try:
        # Validate addresses are different
        if tx.from_address == tx.to_address:
            raise HTTPException(
                status_code=400,
                detail="Cannot send transaction to the same address"
            )
        
        # Create transaction data
        timestamp = datetime.now(timezone.utc).isoformat()
        tx_data = {
            "from_address": tx.from_address,
            "to_address": tx.to_address,
            "amount": tx.amount,
            "data": tx.data,
            "timestamp": timestamp,
            "status": "pending"
        }
        
        # Compute transaction hash
        tx_hash = compute_transaction_hash(tx_data)
        tx_data["tx_hash"] = tx_hash
        
        # Submit transaction to C++ node's mempool via the shared client so
        # that SSL verification settings and availability tracking are applied.
        client = get_ailee_client()
        try:
            cpp_response = await client.submit_transaction({
                "from_address": tx.from_address,
                "to_address": tx.to_address,
                "amount": tx.amount,
                "data": tx.data or "",
                "tx_hash": tx_hash,
            })
            if cpp_response is not None:
                logger.info(f"Transaction submitted to C++ mempool: {tx_hash[:16]}...")
            else:
                logger.warning(f"C++ mempool unavailable; transaction stored locally only")
        except Exception as e:
            logger.warning(f"Failed to submit to C++ mempool (will retry): {e}")
        
        # Persist transaction to database (critical for production)
        # Database failure must NOT break mempool acceptance
        try:
            await db.insert_transaction(tx_data)
            logger.debug(f"Transaction persisted to database: {tx_hash[:16]}...")
        except Exception as e:
            logger.error(f"Failed to persist transaction to database: {e}", exc_info=True)
            # Return 500 if database write fails
            raise HTTPException(
                status_code=500,
                detail="Failed to persist transaction"
            )
        
        # Get current block height from C++ node for response
        block_height = None
        cpp_state = await client.get_l2_state()
        if cpp_state:
            block_height = cpp_state.get("block_height", 0)
            # Transaction will be included in next block
            block_height += 1
        
        logger.info(f"Transaction submitted: {tx_hash[:16]}... from {tx.from_address} to {tx.to_address} amount {tx.amount}")
        
        return TransactionResponse(
            tx_hash=tx_hash,
            from_address=tx.from_address,
            to_address=tx.to_address,
            amount=tx.amount,
            status="pending",
            block_height=block_height,
            timestamp=timestamp
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Failed to submit transaction: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to submit transaction"
        )


@router.get("/hash/{tx_hash}", response_model=TransactionQuery)
async def get_transaction(tx_hash: str):
    """
    Get Transaction by Hash
    
    Retrieves a transaction by its hash from the L2 blockchain.
    
    Args:
        tx_hash: Transaction hash to query
        
    Returns:
        Transaction details if found
    """
    try:
        # Query transaction from database
        tx_data = await db.get_transaction_by_hash(tx_hash)
        
        if tx_data:
            return TransactionQuery(
                tx_hash=tx_data["tx_hash"],
                from_address=tx_data["from_address"],
                to_address=tx_data["to_address"],
                amount=tx_data["amount"],
                status=tx_data["status"],
                block_height=tx_data.get("block_height"),
                data=tx_data.get("data"),
                timestamp=tx_data["timestamp"]
            )
        
        raise HTTPException(
            status_code=404,
            detail=f"Transaction not found: {tx_hash}"
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Failed to get transaction: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to get transaction"
        )


@router.get("/address/{address}", response_model=TransactionListResponse)
async def get_transactions_by_address(
    address: str,
    page: int = Query(default=1, ge=1, description="Page number"),
    page_size: int = Query(default=10, ge=1, le=100, description="Items per page")
):
    """
    Get Transactions by Address
    
    Retrieves all transactions for a specific address (sent or received).
    
    Args:
        address: Address to query
        page: Page number for pagination
        page_size: Number of items per page
        
    Returns:
        List of transactions involving the address
    """
    try:
        # Query transactions from database
        transactions_data, total_count = await db.get_transactions_by_address(
            address, page, page_size
        )
        
        # Convert to response model
        transactions = [
            TransactionQuery(
                tx_hash=tx["tx_hash"],
                from_address=tx["from_address"],
                to_address=tx["to_address"],
                amount=tx["amount"],
                status=tx["status"],
                block_height=tx.get("block_height"),
                data=tx.get("data"),
                timestamp=tx["timestamp"]
            )
            for tx in transactions_data
        ]
        
        return TransactionListResponse(
            transactions=transactions,
            total_count=total_count,
            page=page,
            page_size=page_size
        )
        
    except Exception as e:
        logger.error(f"Failed to get transactions for address: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to get transactions"
        )


@router.get("/list", response_model=TransactionListResponse)
async def list_transactions(
    page: int = Query(default=1, ge=1, description="Page number"),
    page_size: int = Query(default=10, ge=1, le=100, description="Items per page"),
    status: Optional[str] = Query(default=None, description="Filter by status (pending, confirmed, failed)")
):
    """
    List All Transactions
    
    Retrieves a paginated list of all transactions in the L2 blockchain.
    
    Args:
        page: Page number for pagination
        page_size: Number of items per page
        status: Optional status filter
        
    Returns:
        Paginated list of transactions
    """
    try:
        # Query transactions from database
        transactions_data, total_count = await db.get_transactions(
            page, page_size, status
        )
        
        # Convert to response model
        transactions = [
            TransactionQuery(
                tx_hash=tx["tx_hash"],
                from_address=tx["from_address"],
                to_address=tx["to_address"],
                amount=tx["amount"],
                status=tx["status"],
                block_height=tx.get("block_height"),
                data=tx.get("data"),
                timestamp=tx["timestamp"]
            )
            for tx in transactions_data
        ]
        
        return TransactionListResponse(
            transactions=transactions,
            total_count=total_count,
            page=page,
            page_size=page_size
        )
        
    except Exception as e:
        logger.error(f"Failed to list transactions: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to list transactions"
        )
