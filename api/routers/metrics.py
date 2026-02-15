"""
Metrics Router
Node performance metrics and telemetry with Prometheus support
"""

from datetime import datetime, timezone
from typing import Dict

from fastapi import APIRouter
from fastapi.responses import PlainTextResponse
from pydantic import BaseModel, Field

from api.l2_client import get_ailee_client

router = APIRouter()


class MetricsResponse(BaseModel):
    """Response model for node metrics"""

    node_metrics: Dict[str, float] = Field(..., description="Node performance metrics")
    system_metrics: Dict[str, float] = Field(..., description="System resource metrics")
    l2_metrics: Dict[str, int] = Field(..., description="Layer-2 specific metrics")
    timestamp: str = Field(..., description="Metrics timestamp (ISO 8601)")


async def get_current_metrics() -> MetricsResponse:
    """
    Get current node metrics from C++ AILEE-Core node
    """
    client = get_ailee_client()

    # Try to get real metrics from C++ node
    cpp_metrics = await client.get_metrics()

    if cpp_metrics:
        # Parse and return C++ node metrics
        # The C++ node returns metrics in a different format, adapt it
        # Convert timestamp from milliseconds (float) to ISO 8601 string if necessary
        raw_timestamp = cpp_metrics.get("timestamp")
        if isinstance(raw_timestamp, (int, float)):
            # Convert milliseconds since epoch to ISO 8601 string
            timestamp_str = datetime.fromtimestamp(raw_timestamp / 1000.0, tz=timezone.utc).isoformat()
        else:
            # If already a string or None, use it or default to now
            timestamp_str = raw_timestamp if raw_timestamp else datetime.now(timezone.utc).isoformat()

        return MetricsResponse(
            node_metrics=cpp_metrics.get("performance", {}),
            system_metrics={},  # C++ node doesn't expose system metrics via this endpoint
            l2_metrics={},
            timestamp=timestamp_str,
        )

    # Fallback: Get system metrics only (C++ node not available)
    import psutil

    try:
        cpu_percent = psutil.cpu_percent(interval=0.1)
        memory = psutil.virtual_memory()
        disk = psutil.disk_usage("/")

        system_metrics = {
            "cpu_usage_percent": round(cpu_percent, 2),
            "memory_used_percent": round(memory.percent, 2),
            "memory_available_mb": round(memory.available / (1024 * 1024), 2),
            "disk_used_percent": round(disk.percent, 2),
            "disk_available_gb": round(disk.free / (1024 * 1024 * 1024), 2),
        }
    except Exception:
        system_metrics = {
            "cpu_usage_percent": 0.0,
            "memory_used_percent": 0.0,
            "memory_available_mb": 0.0,
            "disk_used_percent": 0.0,
            "disk_available_gb": 0.0,
        }

    # Return default metrics when C++ node is unavailable
    # These are zero values to satisfy the Pydantic model, not actual operational metrics
    return MetricsResponse(
        node_metrics={
            "requests_per_second": 0.0,
            "avg_response_time_ms": 0.0,
            "trust_score_computations": 0.0,
            "validations_performed": 0.0,
        },
        system_metrics=system_metrics,
        l2_metrics={
            "current_block_height": 0,
            "pending_transactions": 0,
            "anchors_created": 0,
            "state_updates": 0,
        },
        timestamp=datetime.now(timezone.utc).isoformat(),
    )


async def get_prometheus_metrics() -> str:
    """
    Generate Prometheus-compatible metrics (async for consistency with route handlers)

    Returns mock metrics in Prometheus text format for standalone API testing.
    In production, this would use the C++ PrometheusExporter.
    """

    # Get current metrics
    metrics = await get_current_metrics()

    # Build Prometheus text format
    lines = []

    # System metrics
    lines.append("# HELP ailee_cpu_usage_percent CPU usage percentage")
    lines.append("# TYPE ailee_cpu_usage_percent gauge")
    lines.append(f"ailee_cpu_usage_percent {metrics.system_metrics['cpu_usage_percent']}")
    lines.append("")

    lines.append("# HELP ailee_memory_usage_percent Memory usage percentage")
    lines.append("# TYPE ailee_memory_usage_percent gauge")
    lines.append(f"ailee_memory_usage_percent {metrics.system_metrics['memory_used_percent']}")
    lines.append("")

    lines.append("# HELP ailee_memory_available_bytes Available memory in bytes")
    lines.append("# TYPE ailee_memory_available_bytes gauge")
    lines.append(f"ailee_memory_available_bytes {metrics.system_metrics['memory_available_mb'] * 1024 * 1024}")
    lines.append("")

    # Node metrics
    lines.append("# HELP ailee_requests_per_second Request rate")
    lines.append("# TYPE ailee_requests_per_second gauge")
    lines.append(f"ailee_requests_per_second {metrics.node_metrics.get('requests_per_second', 0.0)}")
    lines.append("")

    lines.append("# HELP ailee_response_time_milliseconds Average response time")
    lines.append("# TYPE ailee_response_time_milliseconds gauge")
    lines.append(f"ailee_response_time_milliseconds {metrics.node_metrics.get('avg_response_time_ms', 0.0)}")
    lines.append("")

    lines.append("# HELP ailee_trust_computations_total Total trust score computations")
    lines.append("# TYPE ailee_trust_computations_total counter")
    lines.append(f"ailee_trust_computations_total {metrics.node_metrics.get('trust_score_computations', 0.0)}")
    lines.append("")

    lines.append("# HELP ailee_validations_total Total validations performed")
    lines.append("# TYPE ailee_validations_total counter")
    lines.append(f"ailee_validations_total {metrics.node_metrics.get('validations_performed', 0.0)}")
    lines.append("")

    # L2 metrics
    lines.append("# HELP ailee_block_height Current block height")
    lines.append("# TYPE ailee_block_height gauge")
    lines.append(f"ailee_block_height {metrics.l2_metrics.get('current_block_height', 0)}")
    lines.append("")

    lines.append("# HELP ailee_pending_transactions Pending transactions")
    lines.append("# TYPE ailee_pending_transactions gauge")
    lines.append(f"ailee_pending_transactions {metrics.l2_metrics.get('pending_transactions', 0)}")
    lines.append("")

    lines.append("# HELP ailee_anchors_created_total Total anchors created")
    lines.append("# TYPE ailee_anchors_created_total counter")
    lines.append(f"ailee_anchors_created_total {metrics.l2_metrics.get('anchors_created', 0)}")
    lines.append("")

    lines.append("# HELP ailee_state_updates_total Total state updates")
    lines.append("# TYPE ailee_state_updates_total counter")
    lines.append(f"ailee_state_updates_total {metrics.l2_metrics.get('state_updates', 0)}")
    lines.append("")

    return "\n".join(lines)


@router.get("/metrics", response_model=MetricsResponse)
async def get_metrics():
    """
    Get Node Metrics (JSON format)

    Returns comprehensive node performance metrics from C++ AILEE-Core node

    Returns:
        Current node metrics with timestamp
    """
    return await get_current_metrics()


@router.get("/metrics/prometheus", response_class=PlainTextResponse)
async def get_prometheus_metrics_endpoint():
    """
    Get Node Metrics (Prometheus format)

    Returns metrics in Prometheus text exposition format for scraping.

    Returns:
        Metrics in Prometheus text format
    """
    return await get_prometheus_metrics()
