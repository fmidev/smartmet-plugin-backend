# Backend plugin developer guide

`smartmet-plugin-backend` is the small plugin that makes a SmartMet Server a member of a
cluster. It starts the sputnik engine in **backend mode**, which advertises this server's
services to the frontends, and it provides the health check and the admin requests that
operators use to take the server out of the cluster.

The rest of the cluster is described in the frontend plugin's
[developer guide](https://github.com/fmidev/smartmet-plugin-frontend/blob/master/docs/developer-guide.md),
the server's [developer guide](https://github.com/fmidev/smartmet-server/blob/master/docs/developer-guide.md)
("When this server is a backend") and the sputnik engine.

## What it does

* **`init()`** reads the optional configuration (only `favicon`), gets the sputnik
  engine, and launches it in backend mode. From then on sputnik broadcasts this server's
  public URIs, load and status by UDP, and the frontends route requests here.
* **`GET /`** is the health check. It always answers **200**; the body tells the state:

  | State | Body |
  |-------|------|
  | normal | `SmartMet Server` |
  | still initialising (`Reactor::isInitializing()`) | `SmartMet Server initializing` |
  | paused | `SmartMet Server paused` |
  | shutting down | `SmartMet Server shutting down` |

* **`GET /favicon.ico`** serves the configured icon (7-day `Expires`), or 204.
* **Admin requests:**

  | Request | Access | Effect |
  |---------|--------|--------|
  | `clusterinfo` | public | HTML view of the cluster from sputnik. |
  | `backends` (`?service=`) | public | Table of the backends and their services. |
  | `pause` (`?time=` / `?duration=`) | requires authentication | Stop broadcasting, forever or until the given time, so the frontends stop routing here. Requests already sent still finish. |
  | `continue` (`?time=` / `?duration=`) | requires authentication | Resume broadcasting now. With `time` or `duration`, it schedules the resume, which means pausing until then. |

`test/RunClusterTests.cpp` in the frontend repository checks that a paused backend is
drained and restored without failing client requests.

## Files

| File | Contents |
|------|----------|
| `backend/Plugin.{h,cpp}` | The whole plugin. |
| `cnf/backend.conf.sample` | Sample configuration (`favicon`). |
| `systemd/smartmet-backend.service` | Runs `/usr/sbin/smartmetd --port=${PORT} --configfile ${CONFIGFILE}` as `smartmet-server`, with `Restart=always`, unlimited core size and descriptors, and `TimeoutStopSec=35s`. |
| `etc/smartmet-backend.defaults.env.in` | Installed as `/etc/smartmet/smartmet-backend.defaults.env` (overwritten on upgrade): `LD_PRELOAD` of the detected jemalloc, and `COREDUMP_FILTER=0x33`. |
| `etc/smartmet-backend.env` | Installed as `/etc/smartmet/smartmet-backend.env`, the local overrides (`CONFIGFILE`, …). |

There are no tests in this repository: `make test` succeeds only when `CI=true` and fails
elsewhere.

## Known pitfalls

* **`pause` and `continue` need admin credentials.** Configure `admin.user` and
  `admin.password` on every backend.
* **The health check is always 200.** Anything that monitors a backend directly must match
  the body, and a paused backend's body (`SmartMet Server paused`) still contains the word
  `SmartMet` that the F5 checks match for frontends. Backends are taken out of rotation by
  sputnik, not by the health check.
