# Svanipp v1.0.0

A lightweight LAN file transfer CLI with integrity verification, discovery, robust transfer controls, and an optional terminal dashboard.

## Highlights
- Interactive device discovery and pick mode
- Multi-file and recursive folder transfer (structure preserved)
- SHA-256 integrity verification per file
- Overwrite safeguards (`--overwrite` or prompt)
- Timeout and retry controls for unstable networks
- TUI dashboard (auto in TTY) with `--no-tui` fallback
- New `svanipp check` readiness command

## New: `svanipp check`
Run this before demos or transfers:

```powershell
build\svanipp.exe check
```

It reports:
- Version
- Local IP
- Hostname
- Transfer/discovery ports
- TTY / Colors / TUI auto status
- Transfer-port bind test
- Final status (`READY` or `ISSUES FOUND`)

Exit code:
- `0` = ready
- non-zero = issues found

## Quick Start
1. Build:
   ```powershell
   cmake -S . -B build
   cmake --build build
   ```
2. Sanity check:
   ```powershell
   build\svanipp.exe check
   ```
3. Start receiver:
   ```powershell
   build\svanipp.exe receive --port 39000 --out Downloads
   ```
4. Send files/folders:
   ```powershell
   build\svanipp.exe send "C:\some\folder"
   ```

## Notes
- Transfers are verified with SHA-256.
- Traffic is plaintext on LAN (no TLS in v1.0.0).

## Thanks
Thanks to everyone who tested discovery, transfer reliability, and terminal UX on the path to v1.0.0.
