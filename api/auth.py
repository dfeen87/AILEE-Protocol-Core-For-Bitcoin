"""
API Key Authentication for AILEE-Core
Protects write endpoints with Bearer token authentication
"""

import logging
import os
import secrets
from typing import Optional

from fastapi import Security, HTTPException, status
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials

logger = logging.getLogger(__name__)

# Security scheme
security = HTTPBearer()

# API key storage
_api_key: Optional[str] = None


def init_api_key() -> str:
    """
    Initialize API key from environment or generate a secure random key
    
    Returns:
        The API key being used
    """
    global _api_key
    
    # Check environment variable
    env_key = os.getenv("AILEE_API_KEY")
    
    if env_key:
        _api_key = env_key
        logger.info("API key loaded from AILEE_API_KEY environment variable")
    else:
        # Generate secure random key (32 bytes = 64 hex chars)
        _api_key = secrets.token_hex(32)
        logger.warning("=" * 80)
        logger.warning("AILEE_API_KEY environment variable not set")
        logger.warning("Generated random API key for this session (in-memory only):")
        logger.warning("")
        logger.warning(f"  {_api_key}")
        logger.warning("")
        logger.warning("Set AILEE_API_KEY environment variable to use a persistent key.")
        logger.warning("On Fly.io: fly secrets set AILEE_API_KEY=<your-key>")
        logger.warning("=" * 80)
    
    return _api_key


def get_api_key() -> str:
    """
    Get the current API key
    
    Returns:
        The API key
        
    Raises:
        RuntimeError: If API key is not initialized
    """
    if _api_key is None:
        raise RuntimeError("API key not initialized. Call init_api_key() first.")
    return _api_key


async def verify_api_key(
    credentials: HTTPAuthorizationCredentials = Security(security)
) -> str:
    """
    Verify API key from Authorization header
    
    Args:
        credentials: HTTP authorization credentials
        
    Returns:
        The validated API key
        
    Raises:
        HTTPException: If API key is invalid or missing
    """
    api_key = get_api_key()
    
    if credentials.credentials != api_key:
        logger.warning(f"Invalid API key attempt from credentials")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid or missing API key",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    return credentials.credentials
