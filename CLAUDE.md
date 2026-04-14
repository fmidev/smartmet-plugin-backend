# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The **backend plugin** (`smartmet-plugin-backend`) is a SmartMet Server plugin that handles requests routed from frontend servers in a cluster deployment. It works with `smartmet-engine-sputnik` (service discovery) and `smartmet-plugin-frontend` (load balancer) to enable load-balanced multi-server setups.

The plugin is minimal: two source files (`backend/Plugin.h`, `backend/Plugin.cpp`) producing `backend.so`.

## Build commands

```bash
make            # Build backend.so
make clean      # Clean build artifacts
make format     # Run clang-format on source files
make rpm        # Build RPM package
make install    # Install plugin, systemd service, and env files
```

There is no test suite — `make test` is a CI-only no-op (succeeds only when `$CI=true`).

## Dependencies

- **Build-time**: `smartmet-library-spine`, `smartmet-engine-sputnik`, libconfig, protobuf, Boost
- **Runtime**: Requires a running SmartMet Server that loads this plugin via its config. Requires the Sputnik engine for cluster discovery.
- **pkg-config**: `REQUIRES = configpp` in the Makefile

## How the plugin works

1. **Initialization**: Reads an optional libconfig configuration file (only setting: `favicon` path). Gets the Sputnik engine from the Reactor and launches it in **Backend mode**.

2. **HTTP handlers registered**:
   - `GET /` — Returns server status text ("SmartMet Server", "...initializing", "...paused", "...shutting down")
   - `GET /favicon.ico` — Serves configured favicon or returns 204

3. **Admin handlers** (registered via `addAdmin*RequestHandler`):
   - `clusterinfo` (public) — HTML view of cluster status from Sputnik
   - `backends` (public) — Table of backend servers, filterable by `?service=`
   - `pause` (authenticated) — Pause Sputnik broadcasting; optional `?time=` or `?duration=`
   - `continue` (authenticated) — Resume Sputnik; also accepts `?time=`/`?duration=` for timed pause

4. **Plugin loading**: Exported via `extern "C" create/destroy` functions — the server's dynamic loader instantiates the plugin at startup.

## Configuration

- `cnf/backend.conf.sample` — Sample libconfig file (only `favicon` setting)
- `etc/smartmet-backend.env` — Systemd environment overrides (`CONFIGFILE`, `COREDUMP_MASK`, `PRELOAD`)
- `systemd/smartmet-backend.service` — Runs `smartmetd` as `smartmet-server` user on configurable port, with jemalloc preloaded

## CI

CircleCI builds RPMs on RHEL 8 and RHEL 10 using `fmidev/smartmet-cibase-{8,10}` Docker images with `ci-build deps && ci-build rpm`. No test phase in CI for this plugin.
