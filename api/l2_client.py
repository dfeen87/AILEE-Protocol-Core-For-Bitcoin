"""
AILEE-Core L2 Client
Connects to the C++ AILEE-Core node to fetch real L2 state data
"""

import os
import logging
import time
from typing import Optional, Dict, Any
import httpx

logger = logging.getLogger(__name__)


class AILEECoreClient:
    """Client for C++ AILEE-Core node HTTP API"""
    
    # Configuration constants
    DEFAULT_CHECK_INTERVAL = 60  # Re-check availability every 60 seconds
    DEFAULT_MAX_LOG_FAILURES = 3  # Only log first 3 failures to reduce noise
    DEFAULT_HEALTH_CHECK_TIMEOUT = 2.0  # Timeout for health checks in seconds
    
    def __init__(self, base_url: Optional[str] = None, timeout: float = 5.0):
        """
        Initialize client
        
        Args:
            base_url: Base URL of C++ node (defaults to env var AILEE_NODE_URL or localhost:8080)
            timeout: Request timeout in seconds
        """
        self.base_url = base_url or os.getenv("AILEE_NODE_URL", "http://localhost:8080")
        self.timeout = timeout
        # Disable SSL verification for localhost/127.0.0.1 (same container in Fly.io)
        # Enable SSL verification for remote HTTPS endpoints
        is_localhost = any(x in self.base_url for x in ["localhost", "127.0.0.1"])
        verify_ssl = not is_localhost
        self.client = httpx.AsyncClient(
            timeout=self.timeout, 
            follow_redirects=True,
            verify=verify_ssl
        )
        
        # Node availability tracking for smarter error handling
        self._node_available = None  # None = unknown, True = available, False = unavailable
        self._last_check_time = 0
        self._check_interval = self.DEFAULT_CHECK_INTERVAL
        self._consecutive_failures = 0
        self._max_log_failures = self.DEFAULT_MAX_LOG_FAILURES
        self._health_check_timeout = self.DEFAULT_HEALTH_CHECK_TIMEOUT
    
    async def get_status(self) -> Optional[Dict[str, Any]]:
        """Get node status from C++ node"""
        # Check node availability first
        if not await self._check_and_update_availability():
            return None
        
        try:
            response = await self.client.get(f"{self.base_url}/api/status")
            response.raise_for_status()
            self._consecutive_failures = 0
            return response.json()
        except httpx.HTTPError as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Failed to get status from C++ node: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
        except Exception as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Unexpected error getting status: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
    
    async def get_metrics(self) -> Optional[Dict[str, Any]]:
        """Get metrics from C++ node"""
        # Check node availability first
        if not await self._check_and_update_availability():
            return None
        
        try:
            response = await self.client.get(f"{self.base_url}/api/metrics")
            response.raise_for_status()
            self._consecutive_failures = 0
            return response.json()
        except httpx.HTTPError as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Failed to get metrics from C++ node: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
        except Exception as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Unexpected error getting metrics: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
    
    async def _check_and_update_availability(self) -> bool:
        """
        Check if C++ node is available and update internal state
        
        Returns:
            True if node is available, False otherwise
        """
        current_time = time.time()
        
        # If we recently checked, return cached result
        if self._node_available is not None and (current_time - self._last_check_time) < self._check_interval:
            return self._node_available
        
        # Perform health check
        try:
            response = await self.client.get(f"{self.base_url}/api/health", timeout=self._health_check_timeout)
            is_available = response.status_code == 200
            
            if is_available and self._node_available is False:
                # Node came back online (transition from False to True)
                logger.info(f"C++ AILEE-Core node is now available at {self.base_url}")
                self._consecutive_failures = 0
            
            self._node_available = is_available
            self._last_check_time = current_time
            return is_available
        except Exception:
            if self._node_available is not False:
                # First time we detect node is unavailable (transition from None/True to False)
                logger.warning(f"C++ AILEE-Core node unavailable at {self.base_url}. API will run in standalone mode.")
            
            self._node_available = False
            self._last_check_time = current_time
            return False
    
    async def get_l2_state(self) -> Optional[Dict[str, Any]]:
        """Get L2 state from C++ node"""
        # Check node availability first
        if not await self._check_and_update_availability():
            # Node is known to be unavailable, skip connection attempt
            return None
        
        try:
            response = await self.client.get(f"{self.base_url}/api/l2/state")
            response.raise_for_status()
            self._consecutive_failures = 0  # Reset failure counter on success
            return response.json()
        except httpx.HTTPError as e:
            self._consecutive_failures += 1
            
            # Only log first few failures to avoid log spam
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Failed to get L2 state from C++ node: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            
            # Mark node as unavailable after failures
            self._node_available = False
            self._last_check_time = time.time()
            return None
        except Exception as e:
            self._consecutive_failures += 1
            
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Unexpected error getting L2 state: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            
            self._node_available = False
            self._last_check_time = time.time()
            return None
    
    async def get_orchestration_tasks(self) -> Optional[Dict[str, Any]]:
        """Get orchestration tasks from C++ node"""
        # Check node availability first
        if not await self._check_and_update_availability():
            return None
        
        try:
            response = await self.client.get(f"{self.base_url}/api/orchestration/tasks")
            response.raise_for_status()
            self._consecutive_failures = 0
            return response.json()
        except httpx.HTTPError as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Failed to get tasks from C++ node: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
        except Exception as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Unexpected error getting tasks: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
    
    async def get_latest_anchor(self) -> Optional[Dict[str, Any]]:
        """Get latest Bitcoin anchor from C++ node"""
        # Check node availability first
        if not await self._check_and_update_availability():
            return None
        
        try:
            response = await self.client.get(f"{self.base_url}/api/anchors/latest")
            response.raise_for_status()
            self._consecutive_failures = 0
            return response.json()
        except httpx.HTTPError as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Failed to get latest anchor from C++ node: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
        except Exception as e:
            self._consecutive_failures += 1
            if self._consecutive_failures <= self._max_log_failures:
                logger.error(f"Unexpected error getting latest anchor: {e}")
            elif self._consecutive_failures == self._max_log_failures + 1:
                logger.warning(f"Suppressing further C++ node connection errors (node appears unavailable)")
            self._node_available = False
            self._last_check_time = time.time()
            return None
    
    async def health_check(self) -> bool:
        """Check if C++ node is healthy"""
        try:
            response = await self.client.get(f"{self.base_url}/api/health", timeout=2.0)
            return response.status_code == 200
        except:
            return False
    
    async def close(self):
        """Close the HTTP client"""
        await self.client.aclose()


# Global client instance
_client: Optional[AILEECoreClient] = None


def get_ailee_client() -> AILEECoreClient:
    """Get or create the global AILEE Core client instance"""
    global _client
    if _client is None:
        _client = AILEECoreClient()
    return _client


async def close_ailee_client():
    """Close the global client"""
    global _client
    if _client:
        await _client.close()
        _client = None
