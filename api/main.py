"""
AILEE-Core REST API - Main Application
Bitcoin Layer-2 Trust Oracle
Production-ready FastAPI implementation with deterministic, safe, read-only endpoints
"""

import logging
import sys
import time
from contextlib import asynccontextmanager
from datetime import datetime, timezone

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

from api.config import settings
from api.routers import health, status, trust, l2, metrics
from api.security_audit import get_audit_logger, audit_log, AuditEventType, AuditEventSeverity


# Configure logging
logging.basicConfig(
    level=getattr(logging, settings.log_level.upper()),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    stream=sys.stdout
)
logger = logging.getLogger(__name__)

# Initialize security audit logger
audit_logger = get_audit_logger(log_file=settings.audit_log_path)

# Global state
startup_time = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    Application lifespan manager
    Handles startup and shutdown events with deterministic initialization
    """
    global startup_time
    
    # Startup
    logger.info("=" * 80)
    logger.info("AILEE-Core REST API")
    logger.info("Bitcoin Layer-2 Trust Oracle")
    logger.info(f"Version: {settings.app_version}")
    logger.info(f"Environment: {settings.env}")
    logger.info(f"Node ID: {settings.node_id}")
    logger.info("=" * 80)
    logger.info("")
    logger.info("🚀 Starting AILEE-Core API Server...")
    logger.info(f"📍 Binding to {settings.host}:{settings.port}")
    logger.info(f"🔒 JWT Auth: {'Enabled' if settings.jwt_enabled else 'Disabled'}")
    logger.info(f"🌐 CORS: {'Enabled' if settings.cors_enabled else 'Disabled'}")
    logger.info(f"⏱️  Rate Limiting: {'Enabled' if settings.rate_limit_enabled else 'Disabled'}")
    
    if settings.rate_limit_enabled:
        logger.info(f"   └─ Limit: {settings.rate_limit_requests} requests per {settings.rate_limit_window}s")
    
    logger.info("")
    logger.info("✅ Deterministic initialization complete")
    logger.info("✅ Configuration loaded and validated")
    logger.info("✅ All endpoints registered")
    logger.info("")
    logger.info("🎯 API is ready to serve requests")
    logger.info("📚 OpenAPI documentation available at: /docs")
    logger.info("🔍 Alternative docs available at: /redoc")
    logger.info("")
    
    startup_time = time.time()
    
    yield
    
    # Shutdown
    logger.info("")
    logger.info("🛑 Shutting down AILEE-Core API Server...")
    uptime = time.time() - startup_time
    logger.info(f"⏱️  Total uptime: {uptime:.2f} seconds")
    logger.info("✅ Graceful shutdown complete")
    logger.info("=" * 80)


# Initialize rate limiter (if enabled)
limiter = None
if settings.rate_limit_enabled:
    limiter = Limiter(key_func=get_remote_address)


# Create FastAPI application
app = FastAPI(
    title=settings.app_name,
    description=settings.app_description,
    version=settings.app_version,
    lifespan=lifespan,
    docs_url="/docs",
    redoc_url="/redoc",
    openapi_url="/openapi.json"
)


# Add rate limiting middleware
if settings.rate_limit_enabled and limiter:
    app.state.limiter = limiter
    app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)


# Add CORS middleware
if settings.cors_enabled:
    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.cors_origins,
        allow_credentials=True,
        allow_methods=["GET", "POST", "OPTIONS"],
        allow_headers=["*"],
    )


# Global exception handler
@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """
    Global exception handler for unhandled errors
    Returns safe, deterministic error responses
    """
    logger.error(f"Unhandled exception: {exc}", exc_info=True)
    
    return JSONResponse(
        status_code=500,
        content={
            "error": "internal_server_error",
            "message": "An internal error occurred",
            "timestamp": datetime.now(timezone.utc).isoformat()
        }
    )


# Request logging middleware
@app.middleware("http")
async def log_requests(request: Request, call_next):
    """
    Log all incoming requests with timing information
    """
    start_time = time.time()
    
    # Log request
    logger.debug(f"→ {request.method} {request.url.path}")
    
    # Process request
    response = await call_next(request)
    
    # Log response
    duration = (time.time() - start_time) * 1000  # Convert to ms
    logger.debug(f"← {request.method} {request.url.path} - {response.status_code} ({duration:.2f}ms)")
    
    return response


# Include routers
app.include_router(health.router, tags=["Health"])
app.include_router(status.router, tags=["Status"])
app.include_router(trust.router, prefix="/trust", tags=["Trust"])
app.include_router(l2.router, prefix="/l2", tags=["Layer-2"])
app.include_router(metrics.router, tags=["Metrics"])


# Root endpoint
@app.get("/", include_in_schema=True)
async def root():
    """
    Root endpoint - API information
    
    Returns:
        Basic information about the AILEE-Core REST API
    """
    return {
        "name": settings.app_name,
        "version": settings.app_version,
        "description": settings.app_description,
        "node_id": settings.node_id,
        "environment": settings.env,
        "documentation": {
            "openapi": "/openapi.json",
            "swagger": "/docs",
            "redoc": "/redoc"
        },
        "endpoints": {
            "health": "/health",
            "status": "/status",
            "trust_score": "/trust/score",
            "trust_validate": "/trust/validate",
            "l2_state": "/l2/state",
            "l2_anchors": "/l2/anchors",
            "metrics": "/metrics"
        },
        "timestamp": datetime.now(timezone.utc).isoformat()
    }


# Get startup time for status endpoint
def get_startup_time():
    """Get the startup time of the application"""
    return startup_time
