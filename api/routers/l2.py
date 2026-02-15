"""
Layer-2 Router
Layer-2 state snapshot and anchor history endpoints
"""

from datetime import datetime, timezone, timedelta
from typing import List, Optional
from fastapi import APIRouter, Query
from pydantic import BaseModel, Field

from api.l2_client import get_ailee_client

router = APIRouter()


class L2StateSnapshot(BaseModel):
    """Layer-2 state snapshot model"""
    state_root: str = Field(..., description="Deterministic state root hash")
    block_height: int = Field(..., description="Current L2 block height", ge=0)
    total_transactions: int = Field(..., description="Total transaction count", ge=0)
    last_anchor_height: int = Field(..., description="Last Bitcoin anchor height", ge=0)
    timestamp: str = Field(..., description="Snapshot timestamp (ISO 8601)")


class AnchorRecord(BaseModel):
    """Bitcoin anchor record model"""
    anchor_id: str = Field(..., description="Unique anchor identifier")
    l2_state_root: str = Field(..., description="L2 state root at anchor")
    bitcoin_height: int = Field(..., description="Bitcoin block height", ge=0)
    anchor_hash: str = Field(..., description="Bitcoin anchor commitment hash")
    timestamp: str = Field(..., description="Anchor timestamp (ISO 8601)")
    tx_count: int = Field(..., description="Transactions anchored", ge=0)


class L2StateResponse(BaseModel):
    """Response model for L2 state"""
    state: L2StateSnapshot
    health: str = Field(..., description="L2 state health status")


class AnchorsResponse(BaseModel):
    """Response model for anchor history"""
    anchors: List[AnchorRecord]
    total_count: int = Field(..., description="Total number of anchors", ge=0)
    latest_height: int = Field(..., description="Latest Bitcoin height", ge=0)


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
        # Return real state from C++ node
        # The C++ node returns: {"layer": "Layer-2", "protocol": "AILEE-Core", ...}
        # We need to adapt it to our expected format
        return {
            "state": {
                "state_root": "",  # C++ node doesn't expose this in current format
                "block_height": 0,
                "total_transactions": 0,
                "last_anchor_height": 0,
                "timestamp": datetime.now(timezone.utc).isoformat()
            },
            "health": cpp_state.get("ledger", {}).get("status", "unknown")
        }
    
    # C++ node not available - return API-only mode status
    return {
        "state": {
            "state_root": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "block_height": 0,
            "total_transactions": 0,
            "last_anchor_height": 0,
            "timestamp": datetime.now(timezone.utc).isoformat()
        },
        "health": "api_only_mode"
    }


@router.get("/anchors", response_model=AnchorsResponse)
async def get_anchor_history(
    limit: int = Query(
        default=10,
        ge=1,
        le=100,
        description="Number of anchors to return"
    ),
    offset: int = Query(
        default=0,
        ge=0,
        description="Offset for pagination"
    )
):
    """
    Get Bitcoin Anchor History
    
    Returns the history of Bitcoin anchor commitments from C++ AILEE-Core node
    
    Query Parameters:
    - limit: Number of anchors to return (1-100, default: 10)
    - offset: Pagination offset (default: 0)
    
    Returns:
        List of anchor records with pagination info
    """
    client = get_ailee_client()
    
    # Get latest anchor from C++ node
    cpp_anchor = await client.get_latest_anchor()
    
    if cpp_anchor and cpp_anchor.get("last_anchor_hash"):
        # Build anchor record from latest
        anchors = [
            AnchorRecord(
                anchor_id="latest",
                l2_state_root="",
                bitcoin_height=0,
                anchor_hash=cpp_anchor["last_anchor_hash"],
                timestamp=datetime.now(timezone.utc).isoformat(),
                tx_count=0
            )
        ]
        
        return AnchorsResponse(
            anchors=anchors,
            total_count=1,
            latest_height=0
        )
    
    # C++ node not available or no anchors
    return AnchorsResponse(
        anchors=[],
        total_count=0,
        latest_height=0
    )
