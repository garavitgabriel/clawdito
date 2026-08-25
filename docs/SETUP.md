# Clawdito — Setup Guide

## Prerequisites

- **Hardware**: Waveshare ESP32-S3-Touch-LCD-1.47 + a USB-C *data* cable
  (charge-only cables won't flash).
- **Bridge machine**: the computer where Claude Code is installed and
  logged in. Python 3.10+. macOS or Linux.
- **Firmware tooling**: [PlatformIO Core](https://platformio.org/install/cli)
  (`brew install platformio` on macOS). First build downloads the ESP32
  toolchain (~1.5 GB, one time).
- **Network**: a **2.4 GHz** WiFi the device can join. The bridge machine
  can be on the same router's 5 GHz band — same LAN either way.

## 1. Bridge

```bash
python3 bridge/clawdito_bridge.py          # default port 8787
```

First run generates a bearer token at `~/.clawdito/token` and prints:

```
Clawdito bridge running
  device endpoint:  http://192.168.x.x:8787/usage
  device token:     <token>
```

Keep this terminal running — or better, install it as a login service so it
survives reboots. Ready-made launchd and systemd units are in
[`../bridge/service/`](../bridge/service/README.md).

Flags: `--port` (default 8787) and `--host` (default `0.0.0.0`, i.e. all
interfaces — the device needs to reach it over the LAN).

**What it serves** (only on your LAN, only with the token):

- `limits` — official 5h-window and weekly utilization + reset times,
  fetched with your local Claude Code login and cached for 60s.
- `today` / `month` / `last7` — API-equivalent spend parsed from
  `~/.claude/projects/**/*.jsonl` (deduplicated by message id, priced via
  `pricing.json`), cached for 5s.

macOS may show a Keychain prompt on the first limits fetch (the bridge
reading your own Claude Code credential) — click Allow.

## 2. Firmware

```bash
cd firmware
pio run -t upload
```

PlatformIO auto-detects the serial port. If the upload doesn't start: hold
**BOOT**, tap **RESET**, release both, retry (that's the ESP32 download-mode
dance).

## 3. Provisioning

On first boot (or after a config wipe) the display shows:

```
Setup Mode
1. Join WiFi:  Clawdito-XXXX
2. Password:   <one-time password>
3. Open:       http://192.168.4.1
```

From your phone: join that network, wait for the captive page (or open the
URL manually), then fill in:

| Field | Value |
|---|---|
| WiFi network | your 2.4 GHz SSID (dropdown scans automatically) |
| WiFi password | — |
| **Profile A** — Name | what to call this account, e.g. `personal` |
| **Profile A** — Bridge host | the IP the bridge printed |
| **Profile A** — Port | 8787 |
| **Profile A** — Bridge token | the token the bridge printed |
| **Profile B** | same four fields for a second machine — leave blank for one bridge |

Hit **Connect**. The device reboots, joins your WiFi, and data appears
within ~10 seconds (green dot = bridge reachable).

Updating from a single-bridge v1.0 device? Its existing bridge target is
migrated to Profile A (named `default`) on the first boot — no need to
re-provision.

## Two profiles

Two computers, two Anthropic accounts, one LAN: run the same unmodified
`clawdito_bridge.py` on each and enter both in the setup page. The device
polls both bridges from one loop.

- Usage and Cost show the **active** profile; the page title is its name.
- **Hold BOOT 2s** to switch which one that is.
- A fourth page, **Both**, appears and shows each account's 5h bar, weekly
  bar and today's spend, each with its own online dot — so one bridge going
  down only reddens its own row.

## Controls

| Action | Effect |
|---|---|
| Swipe left/right | Next / previous page |
| Tap BOOT | Next page |
| Hold BOOT 2s | Switch active profile (two profiles only) |
| Hold BOOT 5s | Wipe config → back to Setup Mode |

## Troubleshooting

| Symptom | Fix |
|---|---|
| "bridge offline" on the cards | Bridge not running, wrong IP, or the bridge machine changed networks (its IP changed — re-provision). If this happens after every reboot, install the bridge as a service: [`../bridge/service/`](../bridge/service/README.md) |
| Cost page works, Usage page blank | The spend scan is local (always works) but the limits fetch needs your Claude Code login. On macOS that's a Keychain grant — re-run the bridge in a terminal and click **Always Allow**. Also check you're still logged in (`claude` → `/status`) |
| Red dot, no data | Device lost WiFi; it retries automatically and falls back to Setup Mode after repeated failures |
| Your WiFi missing from the scan | It's 5 GHz-only. Enable the router's 2.4 GHz band or use a second SSID for IoT devices |
| Setup page won't pop up | Open `http://192.168.4.1` manually while joined to the Clawdito AP |
| Percentages ≠ what you expected | Cost figures are estimates (see `pricing.json`); the limit percentages are official and update within ~60s |
| Upload fails | BOOT+RESET download-mode dance, then retry |
