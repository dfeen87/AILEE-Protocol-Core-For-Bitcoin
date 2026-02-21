"""
Pytest configuration for AILEE-Core tests.

Environment variables are set at module level so they are in place before
any api.* module is imported (pydantic-settings reads them at Settings()
instantiation time, which happens on first import of api.config).
"""
import os
import tempfile

_tmp = tempfile.gettempdir()

os.environ.setdefault("AILEE_DB_PATH", os.path.join(_tmp, "ailee-test.db"))
os.environ.setdefault("AILEE_AUDIT_LOG_PATH", os.path.join(_tmp, "ailee-test-audit.log"))
# Fixed key used only in the test suite — NEVER use this value in production.
# Override via AILEE_API_KEY env var or fly secrets set AILEE_API_KEY=<your-key>.
os.environ.setdefault("AILEE_API_KEY", "testnet-test-api-key-minimum-32-chars-ok")
# Point at a port that will never be open so the health-check against the
# C++ node returns False immediately (standalone / mock-data mode).
os.environ.setdefault("AILEE_NODE_URL", "http://localhost:19999")
os.environ.setdefault("AILEE_ENV", "testnet")

import pytest

# Expose the fixed key as a module-level constant for tests that need it.
TEST_API_KEY: str = os.environ["AILEE_API_KEY"]


@pytest.fixture(scope="session", autouse=True)
def _app_lifespan():
    """Hold the FastAPI app lifespan open for the entire test session.

    ``TestClient`` only triggers the ASGI lifespan (startup/shutdown events)
    when used as a context manager.  Module-level ``TestClient(app)`` instances
    in test files skip the lifespan, so ``init_api_key()`` and
    ``init_database()`` are never called.  This session-scoped autouse fixture
    runs the lifespan once and keeps global state (``_api_key``, ``_db_pool``)
    populated for every subsequent test.
    """
    from api.main import app
    from fastapi.testclient import TestClient

    with TestClient(app):
        yield


@pytest.fixture
def auth_headers() -> dict:
    """Authorization header required by write endpoints."""
    return {"Authorization": f"Bearer {TEST_API_KEY}"}
