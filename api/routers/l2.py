"""
Layer-2 Router
Layer-2 state snapshot and anchor history endpoints
"""

from datetime import datetime, timezone, timedelta
from typing import List, Optional
from fastapi import APIRouter, Query
from pydantic import BaseModel, Field

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


def generate_mock_state_root() -> str:
    """
    Generate a mock deterministic state root
    In production, this would query the actual C++ AILEE-Core state
    """
    import hashlib
    timestamp = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    return hashlib.sha256(f"L2STATE:{timestamp}".encode()).hexdigest()


def generate_mock_anchors(limit: int = 10, offset: int = 0) -> List[AnchorRecord]:
    """
    Generate mock anchor history
    In production, this would query the actual anchor database
    """
    import hashlib
    
    anchors = []
    current_bitcoin_height = 830000  # Mock current Bitcoin height
    
    # Generate enough anchors to satisfy limit + offset
    total_to_generate = limit + offset
    
    for i in range(total_to_generate):
        height = current_bitcoin_height - (i * 144)  # One anchor per day approximately
        timestamp = datetime.now(timezone.utc) - timedelta(days=i)
        
        anchor_id = f"anchor-{height}"
        state_root = hashlib.sha256(f"STATE:{height}".encode()).hexdigest()
        anchor_hash = hashlib.sha256(f"ANCHOR:{height}".encode()).hexdigest()
        
        anchors.append(AnchorRecord(
            anchor_id=anchor_id,
            l2_state_root=state_root,
            bitcoin_height=height,
            anchor_hash=anchor_hash,
            timestamp=timestamp.isoformat(),
            tx_count=100 + (i * 10)
        ))
    
    return anchors


@router.get("/state", response_model=L2StateResponse)
async def get_l2_state():
    """
    Get Current Layer-2 State Snapshot
    
    Returns the current Layer-2 state snapshot including:
    - Deterministic state root
    - Block height
    - Transaction counts
    - Last anchor information
    
    This endpoint is:
    - Read-only (no state modification)
    - Deterministic (consistent snapshots)
    - Safe (no side effects)
    
    Returns:
        Current L2 state snapshot with health status
    """
    # Generate deterministic state snapshot
    state_root = generate_mock_state_root()
    
    snapshot = L2StateSnapshot(
        state_root=state_root,
        block_height=12345,
        total_transactions=567890,
        last_anchor_height=830000,
        timestamp=datetime.now(timezone.utc).isoformat()
    )
    
    return L2StateResponse(
        state=snapshot,
        health="operational"
    )


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
    
    Returns the history of Bitcoin anchor commitments for the Layer-2 state.
    Each anchor represents a deterministic commitment of L2 state to Bitcoin.
    
    Query Parameters:
    - limit: Number of anchors to return (1-100, default: 10)
    - offset: Pagination offset (default: 0)
    
    This endpoint is:
    - Read-only (no state modification)
    - Paginated (for efficient large datasets)
    - Safe (no side effects)
    
    Returns:
        List of anchor records with pagination info
    """
    # Get mock anchor history with proper pagination
    all_anchors = generate_mock_anchors(limit, offset)
    
    # Apply offset (anchors are already generated with offset considered)
    anchors = all_anchors[offset:offset + limit] if offset < len(all_anchors) else []
    
    # Total count would come from database in production
    total_count = 1000  # Mock total count
    
    return AnchorsResponse(
        anchors=anchors,
        total_count=total_count,
        latest_height=830000
    )
