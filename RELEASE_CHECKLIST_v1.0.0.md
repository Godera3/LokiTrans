# Svanipp v1.0.0 Release Checklist

## Build & Binary
- [x] Configure project with CMake
- [x] Build `svanipp` target successfully
- [x] Confirm executable starts and shows usage

## Functional Smoke Tests
- [x] Receiver starts and listens on configured port
- [x] Sender transfers single file successfully
- [x] Sender transfers nested folder structure successfully
- [x] Unicode filename transfer verified on disk
- [x] `--no-tui` fallback output verified

## `check` Command Validation
- [x] `svanipp check` prints full report fields
- [x] Includes hostname in output
- [x] READY path returns exit code `0`
- [x] Port-in-use path returns non-zero and `ISSUES FOUND`

## Docs
- [x] README includes quick readiness check section
- [x] How To Use includes quick health check section
- [x] Changelog created for `v1.0.0`
- [x] GitHub release draft text prepared

## Pre-Tag Git Hygiene
- [ ] `git status` is clean
- [ ] Commit message finalized
- [ ] Tag created: `v1.0.0`
- [ ] Tag pushed to remote

## Optional Post-Release
- [ ] Attach binary artifacts to GitHub release
- [ ] Add screenshots/GIF for TUI mode
- [ ] Open v1.1 planning issue
