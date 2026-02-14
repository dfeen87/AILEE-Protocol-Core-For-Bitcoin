"""
AILEE-Core API Configuration
Safe, deterministic configuration loading with validation
"""

import os
from typing import Optional
from pydantic import Field, field_validator
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """
    AILEE-Core API Settings
    All settings can be overridden via environment variables
    """
    
    # Node Identity
    node_id: str = Field(
        default="ailee-node-default",
        description="Unique identifier for this AILEE node"
    )
    
    # Environment
    env: str = Field(
        default="production",
        description="Deployment environment: production, staging, development"
    )
    
    # Logging
    log_level: str = Field(
        default="info",
        description="Logging level: debug, info, warning, error, critical"
    )
    
    # API Server
    host: str = Field(
        default="0.0.0.0",
        description="Host to bind the API server"
    )
    port: int = Field(
        default=8080,
        description="Port to bind the API server"
    )
    
    # CORS
    cors_enabled: bool = Field(
        default=True,
        description="Enable CORS for browser access"
    )
    cors_origins: list = Field(
        default=["*"],
        description="Allowed CORS origins"
    )
    
    # Authentication
    jwt_enabled: bool = Field(
        default=False,
        description="Enable JWT authentication (disabled by default)"
    )
    jwt_secret: Optional[str] = Field(
        default=None,
        description="JWT secret key for token signing"
    )
    jwt_algorithm: str = Field(
        default="HS256",
        description="JWT signing algorithm"
    )
    
    # Rate Limiting
    rate_limit_enabled: bool = Field(
        default=True,
        description="Enable lightweight rate limiting"
    )
    rate_limit_requests: int = Field(
        default=100,
        description="Max requests per window"
    )
    rate_limit_window: int = Field(
        default=60,
        description="Rate limit window in seconds"
    )
    
    # Application Metadata
    app_name: str = Field(
        default="AILEE-Core REST API",
        description="Application name"
    )
    app_version: str = Field(
        default="1.2.1",
        description="Application version"
    )
    app_description: str = Field(
        default="Bitcoin Layer-2 Trust Oracle with deterministic, safe, read-only endpoints",
        description="Application description"
    )
    
    @field_validator("log_level")
    def validate_log_level(cls, v):
        """Validate log level is one of the allowed values"""
        allowed = ["debug", "info", "warning", "error", "critical"]
        if v.lower() not in allowed:
            raise ValueError(f"log_level must be one of {allowed}")
        return v.lower()
    
    @field_validator("env")
    def validate_env(cls, v):
        """Validate environment is one of the allowed values"""
        allowed = ["production", "staging", "development"]
        if v.lower() not in allowed:
            raise ValueError(f"env must be one of {allowed}")
        return v.lower()
    
    @field_validator("jwt_secret")
    def validate_jwt_secret(cls, v, info):
        """Validate JWT secret is set if JWT is enabled"""
        if info.data.get("jwt_enabled") and not v:
            raise ValueError("jwt_secret must be set when jwt_enabled is True")
        return v
    
    class Config:
        env_prefix = "AILEE_"
        case_sensitive = False
        env_file = ".env"
        env_file_encoding = "utf-8"


# Global settings instance
settings = Settings()
