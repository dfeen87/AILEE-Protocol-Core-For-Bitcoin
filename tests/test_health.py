"""
Tests for the /health endpoint.

Verifies that the health check performs a real database readiness probe
so that Railway's postDeployCommand only passes when the full stack is up.
"""

import pytest
from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)


def test_health_returns_200_when_db_ready():
    """Health endpoint should return 200 when the database is initialised."""
    response = client.get("/health")
    assert response.status_code == 200
    data = response.json()
    assert data["status"] == "ok"
    assert data["database"] == "ok"


def test_health_returns_503_when_db_unavailable(monkeypatch):
    """Health endpoint should return 503 when the database probe fails."""
    import api.routers.health as health_module

    def _bad_get_db():
        raise RuntimeError("Database not initialized")

    monkeypatch.setattr(health_module, "get_db", _bad_get_db, raising=False)
    # Patch inside the health module's local import scope
    import api.database as db_module
    monkeypatch.setattr(db_module, "get_db", _bad_get_db)

    response = client.get("/health")
    assert response.status_code == 503
    data = response.json()
    assert data["status"] == "degraded"
    assert data["database"] == "error"


def test_health_response_shape():
    """Health response must always include 'status' and 'database' keys."""
    response = client.get("/health")
    data = response.json()
    assert "status" in data
    assert "database" in data
