# Changelog

All notable changes to this project will be documented in this file.

## v1.0.0 - 2026-02-24

### Added
- Interactive device discovery and picker flow for sender mode.
- Multi-file and recursive folder transfer with directory structure preservation.
- Relative-path transfer protocol support for nested files.
- Receiver safety hardening for output paths (prevents path traversal outside output root).
- Overwrite handling with interactive prompt and `--overwrite` option.
- Sender and receiver transfer summaries.
- Unicode-safe path handling in send/receive flows.
- Timeout and retry controls:
  - Sender: `--connect-timeout`, `--io-timeout`, `--idle-timeout`, `--retries`
  - Receiver: `--io-timeout`, `--idle-timeout`
- Terminal dashboard (TUI) with automatic TTY enablement and `--no-tui` fallback.
- Classic console fallback mode with centralized progress/log output.
- New readiness command: `svanipp check`.

### `svanipp check` reports
- Version
- Local IP address
- Hostname
- Transfer port
- Discovery port
- Terminal capabilities (TTY, color support, TUI auto)
- Transfer-port bind test (opens and closes test socket immediately)
- Final verdict (`READY` or `ISSUES FOUND`)
- Exit code policy (`0` ready, non-zero issues)

### Changed
- Usage/help text expanded for timeout, retry, color, TUI, and check options.
- Documentation updated with check workflow and v1.0 operational guidance.

### Notes
- Transfers are integrity-verified with SHA-256.
- Transfers are not encrypted (plaintext LAN transport).
