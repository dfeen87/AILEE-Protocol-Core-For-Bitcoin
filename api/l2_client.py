"""
AILEE-Core L2 Client
Connects to the C++ AILEE-Core node to fetch real L2 state data
"""

import os
import logging
from typing import Optional, Dict, Any
import httpx

logger = logging.getLogger(__name__)


class AILEECoreClient:
    """Client for C++ AILEE-Core node HTTP API"""
    
    def __init__(self, base_url: Optional[str] = None, timeout: float = 5.0):
        """
        Initialize client
        
        Args:
            base_url: Base URL of C++ node (defaults to env var AILEE_NODE_URL or localhost:9090)
            timeout: Request timeout in seconds
        """
        self.base_url = base_url or os.getenv("AILEE_NODE_URL", "http://localhost:9090")
        self.timeout = timeout
        self.client = httpx.AsyncClient(timeout=self.timeout, follow_redirects=True)
    
    async def get_status(self) -> Optional[Dict[str, Any]]:
        """Get node status from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/api/status")
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            logger.error(f"Failed to get status from C++ node: {e}")
            return None
        except Exception as e:
            logger.error(f"Unexpected error getting status: {e}")
            return None
    
    async def get_metrics(self) -> Optional[Dict[str, Any]]:
        """Get metrics from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/api/metrics")
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            logger.error(f"Failed to get metrics from C++ node: {e}")
            return None
        except Exception as e:
            logger.error(f"Unexpected error getting metrics: {e}")
            return None
    
    async def get_l2_state(self) -> Optional[Dict[str, Any]]:
        """Get L2 state from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/api/l2/state")
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            logger.error(f"Failed to get L2 state from C++ node: {e}")
            return None
        except Exception as e:
            logger.error(f"Unexpected error getting L2 state: {e}")
            return None
    
    async def get_orchestration_tasks(self) -> Optional[Dict[str, Any]]:
        """Get orchestration tasks from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/api/orchestration/tasks")
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            logger.error(f"Failed to get tasks from C++ node: {e}")
            return None
        except Exception as e:
            logger.error(f"Unexpected error getting tasks: {e}")
            return None
    
    async def get_latest_anchor(self) -> Optional[Dict[str, Any]]:
        """Get latest Bitcoin anchor from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/api/anchors/latest")
            response.raise_for_status()
            return response.json()
        except httpx.HTTPError as e:
            logger.error(f"Failed to get latest anchor from C++ node: {e}")
            return None
        except Exception as e:
            logger.error(f"Unexpected error getting latest anchor: {e}")
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
