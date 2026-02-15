"""
Layer-2 Router
Layer-2 state snapshot and anchor history endpoints
"""

import hashlib
import logging
from datetime import datetime, timezone, timedelta
from typing import List, Optional
from fastapi import APIRouter, Query, HTTPException
from pydantic import BaseModel, Field

from api.l2_client import get_ailee_client
from api import database as db

router = APIRouter()
logger = logging.getLogger(__name__)


class L2StateSnapshot(BaseModel):
    """Layer-2 state snapshot model"""
    state_root: str = Field(..., description="Deterministic state root hash")
    block_height: int = Field(..., description="Current L2 block height", ge=0)
    total_transactions: int = Field(..., description="Total transaction count", ge=0)
    last_anchor_height: int = Field(..., description="Last Bitcoin anchor height", ge=0)
    timestamp: str = Field(..., description="Snapshot timestamp (ISO 8601)")


class AnchorRecord(BaseModel):
    """Bitcoin anchor record model"""
    anchor_height: int = Field(..., description="Anchor block height", ge=0)
    state_root: str = Field(..., description="L2 state root at anchor")
    bitcoin_txid: str = Field(..., description="Bitcoin transaction ID")
    timestamp: str = Field(..., description="Anchor timestamp (ISO 8601)")


class AnchorDetailResponse(BaseModel):
    """Response model for single anchor"""
    anchor_height: int = Field(..., description="Anchor block height", ge=0)
    state_root: str = Field(..., description="L2 state root at anchor")
    bitcoin_txid: str = Field(..., description="Bitcoin transaction ID")
    timestamp: str = Field(..., description="Anchor timestamp (ISO 8601)")


class L2StateResponse(BaseModel):
    """Response model for L2 state"""
    state: L2StateSnapshot
    health: str = Field(..., description="L2 state health status")


class AnchorsResponse(BaseModel):
    """Response model for anchor history"""
    anchors: List[AnchorRecord]
    total_count: int = Field(..., description="Total number of anchors", ge=0)
    page: int = Field(..., description="Current page number", ge=1)
    page_size: int = Field(..., description="Number of items per page", ge=1)


@router.get("/state", response_model=L2StateResponse)
async def get_l2_state():
    """
    Get Current Layer-2 State Snapshot
    
    Returns the current Layer-2 state from C++ AILEE-Core node
    
    Returns:
        Current L2 state snapshot with health status
    """
    client = get_ailee_client()
    
    # Get real L2 state from C++ node
    cpp_state = await client.get_l2_state()
    
    if cpp_state:
        # C++ node now returns: {"layer": "Layer-2", "protocol": "AILEE-Core", 
        # "ledger": {"status": "active", "type": "federated"},
        # "block_height": N, "total_transactions": M, "last_anchor_height": K}
        
        # Extract ledger info
        ledger_info = cpp_state.get("ledger", {})
        health_status = ledger_info.get("status", "active")
        
        # Since the C++ node doesn't yet provide state_root, timestamp_ms, balance_count, etc.,
        # we compute a deterministic state root from available data
        state_data = f"{cpp_state.get('layer', '')}-{cpp_state.get('protocol', '')}-{health_status}"
        state_root = hashlib.sha256(state_data.encode()).hexdigest()
        
        # Get block production metrics from C++ node
        block_height = cpp_state.get("block_height", 0)
        total_transactions = cpp_state.get("total_transactions", 0)
        last_anchor_height = cpp_state.get("last_anchor_height", 0)
        
        # Return structured state from C++ node
        return {
            "state": {
                "state_root": state_root,
                "block_height": block_height,
                "total_transactions": total_transactions,
                "last_anchor_height": last_anchor_height,
                "timestamp": datetime.now(timezone.utc).isoformat()
            },
            "health": health_status
        }
    
    # C++ node not available - return fallback data
    logger.warning("C++ AILEE-Core node unavailable for L2 state")
    
    return {
        "state": {
            "state_root": "",
            "block_height": 0,
            "total_transactions": 0,
            "last_anchor_height": 0,
            "timestamp": datetime.now(timezone.utc).isoformat()
        },
        "health": "cpp_node_unavailable"
    }


@router.get("/anchors", response_model=AnchorsResponse)
async def get_anchor_history(
    page: int = Query(default=1, ge=1, description="Page number"),
    page_size: int = Query(default=10, ge=1, le=100, description="Items per page")
):
    """
    Get Bitcoin Anchor History
    
    Returns the history of Bitcoin anchor commitments stored in the database.
    Results are ordered by anchor_height DESC (most recent first).
    
    Query Parameters:
    - page: Page number for pagination (default: 1, min: 1)
    - page_size: Number of items per page (default: 10, min: 1, max: 100)
    
    Returns:
        Paginated list of anchor records
    """
    try:
        # Query anchors from database
        anchors_data, total_count = await db.get_anchors(page, page_size)
        
        # Convert to response model
        anchors = [
            AnchorRecord(
                anchor_height=anchor["anchor_height"],
                state_root=anchor["state_root"],
                bitcoin_txid=anchor["bitcoin_txid"],
                timestamp=anchor["timestamp"]
            )
            for anchor in anchors_data
        ]
        
        return AnchorsResponse(
            anchors=anchors,
            total_count=total_count,
            page=page,
            page_size=page_size
        )
        
    except Exception as e:
        logger.error(f"Failed to get anchor history: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to get anchor history"
        )


@router.get("/anchors/{height}", response_model=AnchorDetailResponse)
async def get_anchor_by_height(height: int):
    """
    Get Anchor by Height
    
    Returns a specific anchor event by its height.
    
    Args:
        height: Anchor height to query
        
    Returns:
        Anchor event details if found
    """
    try:
        # Query anchor from database
        anchor_data = await db.get_anchor_by_height(height)
        
        if anchor_data:
            return AnchorDetailResponse(
                anchor_height=anchor_data["anchor_height"],
                state_root=anchor_data["state_root"],
                bitcoin_txid=anchor_data["bitcoin_txid"],
                timestamp=anchor_data["timestamp"]
            )
        
        raise HTTPException(
            status_code=404,
            detail=f"Anchor not found at height: {height}"
        )
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Failed to get anchor: {e}", exc_info=True)
        raise HTTPException(
            status_code=500,
            detail="Failed to get anchor"
        )
