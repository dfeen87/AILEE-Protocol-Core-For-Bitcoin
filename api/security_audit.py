"""
Security Audit Logger for AILEE-Core
Logs all security-sensitive operations for compliance and forensics
"""

import logging
import json
from datetime import datetime, timezone
from typing import Any, Dict, Optional
from enum import Enum
import hashlib
from pathlib import Path


class AuditEventType(Enum):
    """Types of security-sensitive events to audit"""
    # Authentication events
    AUTH_SUCCESS = "auth_success"
    AUTH_FAILURE = "auth_failure"
    AUTH_LOGOUT = "auth_logout"
    
    # API access
    API_KEY_CREATED = "api_key_created"
    API_KEY_REVOKED = "api_key_revoked"
    API_KEY_USED = "api_key_used"
    
    # Configuration changes
    CONFIG_CHANGED = "config_changed"
    CONFIG_LOADED = "config_loaded"
    
    # Key operations
    KEY_GENERATED = "key_generated"
    KEY_LOADED = "key_loaded"
    KEY_BACKUP = "key_backup"
    KEY_ROTATION = "key_rotation"
    KEY_SIGNING = "key_signing"
    
    # Bitcoin RPC operations
    RPC_CONNECTION = "rpc_connection"
    RPC_FAILURE = "rpc_failure"
    RPC_FAILOVER = "rpc_failover"
    
    # Transaction operations
    TX_BROADCAST = "tx_broadcast"
    TX_SIGNED = "tx_signed"
    TX_FAILED = "tx_failed"
    
    # Federation operations
    FEDERATION_QUORUM_REACHED = "federation_quorum_reached"
    FEDERATION_SIGNING = "federation_signing"
    FEDERATION_MEMBER_CHANGE = "federation_member_change"
    
    # L2 state operations
    L2_STATE_SNAPSHOT = "l2_state_snapshot"
    L2_STATE_ANCHORED = "l2_state_anchored"
    L2_STATE_VERIFIED = "l2_state_verified"
    
    # Peg-in/Peg-out operations
    PEG_IN_INITIATED = "peg_in_initiated"
    PEG_IN_CONFIRMED = "peg_in_confirmed"
    PEG_OUT_REQUESTED = "peg_out_requested"
    PEG_OUT_EXECUTED = "peg_out_executed"
    PEG_OUT_REJECTED = "peg_out_rejected"
    
    # Security events
    RATE_LIMIT_EXCEEDED = "rate_limit_exceeded"
    ACCESS_DENIED = "access_denied"
    SUSPICIOUS_ACTIVITY = "suspicious_activity"
    
    # System events
    SYSTEM_STARTUP = "system_startup"
    SYSTEM_SHUTDOWN = "system_shutdown"
    EMERGENCY_HALT = "emergency_halt"


class AuditEventSeverity(Enum):
    """Severity levels for audit events"""
    DEBUG = "debug"
    INFO = "info"
    WARNING = "warning"
    ERROR = "error"
    CRITICAL = "critical"


class SecurityAuditLogger:
    """
    Secure audit logger for security-sensitive operations
    
    Features:
    - Structured JSON logging
    - Tamper-evident logging (hashed chain)
    - Separate file for security audits
    - Required for compliance (SOC 2, PCI DSS, etc.)
    """
    
    def __init__(
        self,
        log_file: str = "/var/log/ailee/security-audit.log",
        enable_console: bool = False,
        enable_hash_chain: bool = True
    ):
        """
        Initialize security audit logger
        
        Args:
            log_file: Path to audit log file
            enable_console: Also log to console
            enable_hash_chain: Enable tamper-evident hash chain
        """
        self.log_file = Path(log_file)
        self.enable_console = enable_console
        self.enable_hash_chain = enable_hash_chain
        
        # Create log directory if it doesn't exist
        self.log_file.parent.mkdir(parents=True, exist_ok=True)
        
        # Set up file logger
        self.logger = logging.getLogger("ailee.security.audit")
        self.logger.setLevel(logging.DEBUG)
        self.logger.propagate = False  # Don't propagate to root logger
        
        # File handler (append mode)
        file_handler = logging.FileHandler(self.log_file, mode='a')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(logging.Formatter('%(message)s'))
        self.logger.addHandler(file_handler)
        
        # Console handler (optional)
        if enable_console:
            console_handler = logging.StreamHandler()
            console_handler.setLevel(logging.INFO)
            console_handler.setFormatter(
                logging.Formatter(
                    '[AUDIT] %(levelname)s - %(message)s'
                )
            )
            self.logger.addHandler(console_handler)
        
        # Hash chain for tamper evidence
        self._last_hash = "0" * 64  # Initial hash
        
        # Log that audit logging started
        self.log_event(
            event_type=AuditEventType.SYSTEM_STARTUP,
            severity=AuditEventSeverity.INFO,
            message="Security audit logging initialized",
            metadata={"log_file": str(self.log_file)}
        )
    
    def log_event(
        self,
        event_type: AuditEventType,
        severity: AuditEventSeverity,
        message: str,
        user: Optional[str] = None,
        ip_address: Optional[str] = None,
        metadata: Optional[Dict[str, Any]] = None
    ):
        """
        Log a security-sensitive event
        
        Args:
            event_type: Type of event
            severity: Severity level
            message: Human-readable message
            user: User who triggered the event
            ip_address: IP address of the request
            metadata: Additional context
        """
        # Build audit entry
        audit_entry = {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "event_type": event_type.value,
            "severity": severity.value,
            "message": message,
            "user": user,
            "ip_address": ip_address,
            "metadata": metadata or {},
        }
        
        # Add hash chain for tamper evidence
        if self.enable_hash_chain:
            audit_entry["previous_hash"] = self._last_hash
            current_hash = self._compute_hash(audit_entry)
            audit_entry["hash"] = current_hash
            self._last_hash = current_hash
        
        # Log as JSON
        log_line = json.dumps(audit_entry, default=str)
        
        # Map severity to logging level
        level_map = {
            AuditEventSeverity.DEBUG: logging.DEBUG,
            AuditEventSeverity.INFO: logging.INFO,
            AuditEventSeverity.WARNING: logging.WARNING,
            AuditEventSeverity.ERROR: logging.ERROR,
            AuditEventSeverity.CRITICAL: logging.CRITICAL,
        }
        
        self.logger.log(level_map[severity], log_line)
    
    def _compute_hash(self, entry: Dict[str, Any]) -> str:
        """Compute SHA-256 hash of audit entry for tamper evidence"""
        # Remove hash field if present
        entry_copy = entry.copy()
        entry_copy.pop("hash", None)
        
        # Convert to deterministic JSON string
        json_str = json.dumps(entry_copy, sort_keys=True, default=str)
        
        # Compute SHA-256
        return hashlib.sha256(json_str.encode()).hexdigest()
    
    def verify_integrity(self, log_file: Optional[str] = None) -> bool:
        """
        Verify integrity of audit log using hash chain
        
        Args:
            log_file: Optional path to log file (defaults to current)
        
        Returns:
            True if log is intact, False if tampered
        """
        if not self.enable_hash_chain:
            raise ValueError("Hash chain not enabled")
        
        log_path = Path(log_file) if log_file else self.log_file
        
        if not log_path.exists():
            return False
        
        previous_hash = "0" * 64
        
        with open(log_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                try:
                    entry = json.loads(line.strip())
                    
                    # Check previous hash matches
                    if entry.get("previous_hash") != previous_hash:
                        self.logger.error(
                            f"Hash chain broken at line {line_num}: "
                            f"expected {previous_hash}, got {entry.get('previous_hash')}"
                        )
                        return False
                    
                    # Verify current hash
                    stored_hash = entry.pop("hash", None)
                    computed_hash = self._compute_hash(entry)
                    
                    if stored_hash != computed_hash:
                        self.logger.error(
                            f"Entry hash mismatch at line {line_num}: "
                            f"expected {computed_hash}, got {stored_hash}"
                        )
                        return False
                    
                    previous_hash = stored_hash
                
                except json.JSONDecodeError as e:
                    self.logger.error(
                        f"Invalid JSON at line {line_num}: {e}"
                    )
                    return False
        
        return True


# Global audit logger instance
_audit_logger: Optional[SecurityAuditLogger] = None


def get_audit_logger() -> SecurityAuditLogger:
    """Get global audit logger instance"""
    global _audit_logger
    if _audit_logger is None:
        _audit_logger = SecurityAuditLogger()
    return _audit_logger


def audit_log(
    event_type: AuditEventType,
    message: str,
    severity: AuditEventSeverity = AuditEventSeverity.INFO,
    **kwargs
):
    """
    Convenience function to log audit event
    
    Args:
        event_type: Type of event
        message: Human-readable message
        severity: Severity level
        **kwargs: Additional arguments (user, ip_address, metadata)
    """
    logger = get_audit_logger()
    logger.log_event(
        event_type=event_type,
        severity=severity,
        message=message,
        **kwargs
    )


# Example usage
if __name__ == "__main__":
    # Initialize logger
    logger = SecurityAuditLogger(
        log_file="/tmp/ailee-audit-test.log",
        enable_console=True
    )
    
    # Log various events
    logger.log_event(
        event_type=AuditEventType.AUTH_SUCCESS,
        severity=AuditEventSeverity.INFO,
        message="User logged in successfully",
        user="admin",
        ip_address="192.168.1.100"
    )
    
    logger.log_event(
        event_type=AuditEventType.TX_BROADCAST,
        severity=AuditEventSeverity.INFO,
        message="Bitcoin transaction broadcasted",
        user="system",
        metadata={
            "txid": "abc123...",
            "amount_btc": 0.5,
            "destination": "bc1q..."
        }
    )
    
    logger.log_event(
        event_type=AuditEventType.RATE_LIMIT_EXCEEDED,
        severity=AuditEventSeverity.WARNING,
        message="Rate limit exceeded",
        ip_address="10.0.0.5",
        metadata={"endpoint": "/api/peg-out", "attempts": 150}
    )
    
    # Verify integrity
    print(f"\nLog integrity check: {logger.verify_integrity()}")
