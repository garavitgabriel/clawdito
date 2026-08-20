#!/usr/bin/env python3
"""Clawdito bridge — serves Claude Code usage data to the Clawdito device.

Runs on the computer where Claude Code is installed. Exposes one JSON
endpoint on the LAN:

    GET /usage   (Bearer auth)   -> rate limits + local spend stats
    GET /health  (anonymous)     -> liveness probe

Two data sources, both local:
  * Official rate limits (5h window + weekly) from Anthropic's OAuth usage
    endpoint, authenticated with the Claude Code login already on this
    machine (macOS Keychain or ~/.claude/.credentials.json). The OAuth
    token never leaves this process.
  * Spend estimates parsed from the transcript files Claude Code writes
    under ~/.claude/projects/ (API-equivalent dollars, priced via
    pricing.json).

Python 3.10+, standard library only.
"""

import argparse
import datetime as dt
import json
import secrets
import socket
import subprocess
import sys
import threading
import time
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

APP_DIR = Path.home() / ".clawdito"
TRANSCRIPTS = Path.home() / ".claude" / "projects"
PRICING_FILE = Path(__file__).parent / "pricing.json"

OAUTH_USAGE_URL = "https://api.anthropic.com/api/oauth/usage"
OAUTH_BETA = "oauth-2025-04-20"


# --------------------------------------------------------------------------
# auth token for the device
# --------------------------------------------------------------------------

def device_token() -> str:
    APP_DIR.mkdir(mode=0o700, exist_ok=True)
    tok_file = APP_DIR / "token"
    if tok_file.exists():
        return tok_file.read_text().strip()
    tok = secrets.token_urlsafe(24)
    tok_file.write_text(tok)
    tok_file.chmod(0o600)
    return tok


# --------------------------------------------------------------------------
# official rate limits via the local Claude Code login
# --------------------------------------------------------------------------

class RateLimits:
    """Cached fetcher for the official 5h/weekly utilization numbers."""

    def __init__(self, ttl: float = 60.0):
        self.ttl = ttl
        self._lock = threading.Lock()
        self._value: dict = {"ok": False}
        self._at = 0.0

    @staticmethod
    def _login_token() -> str | None:
        # macOS: Claude Code stores its OAuth blob in the login keychain
        try:
            proc = subprocess.run(
                ["security", "find-generic-password",
                 "-s", "Claude Code-credentials", "-w"],
                capture_output=True, text=True, timeout=5)
            if proc.returncode == 0 and proc.stdout.strip():
                blob = json.loads(proc.stdout)
                return (blob.get("claudeAiOauth") or blob).get("accessToken")
        except Exception:
            pass
        # Linux / older installs: credentials file
        try:
            f = Path.home() / ".claude" / ".credentials.json"
            if f.exists():
                blob = json.loads(f.read_text())
                return (blob.get("claudeAiOauth") or blob).get("accessToken")
        except Exception:
            pass
        return None

    @staticmethod
    def _window(raw: dict | None) -> dict | None:
        if not isinstance(raw, dict) or raw.get("utilization") is None:
            return None
        pct = float(raw["utilization"])
        if pct <= 1.0:
            pct *= 100.0
        out = {"pct": round(pct)}
        if raw.get("resets_at"):
            try:
                when = dt.datetime.fromisoformat(
                    str(raw["resets_at"]).replace("Z", "+00:00"))
                now = dt.datetime.now(dt.timezone.utc)
                out["resets_in_s"] = max(0, int((when - now).total_seconds()))
            except Exception:
                pass
        return out

    def get(self) -> dict:
        with self._lock:
            if time.monotonic() - self._at < self.ttl:
                return self._value

        value: dict = {"ok": False}
        token = self._login_token()
        if not token:
            sys.stderr.write("[limits] no Claude Code login found\n")
        else:
            try:
                req = urllib.request.Request(OAUTH_USAGE_URL, headers={
                    "Authorization": f"Bearer {token}",
                    "anthropic-beta": OAUTH_BETA,
                    "Content-Type": "application/json",
                })
                with urllib.request.urlopen(req, timeout=8) as resp:
                    raw = json.loads(resp.read())
                fh = self._window(raw.get("five_hour") or raw.get("session"))
                wk = self._window(raw.get("seven_day") or raw.get("weekly"))
                if fh or wk:
                    value = {"ok": True}
                    if fh:
                        value["five_hour"] = fh
                    if wk:
                        value["seven_day"] = wk
            except Exception as exc:
                sys.stderr.write(f"[limits] fetch failed: {exc!r}\n")

        with self._lock:
            self._value = value
            self._at = time.monotonic()
        return value


# --------------------------------------------------------------------------
# spend stats from local transcripts
# --------------------------------------------------------------------------

def load_pricing() -> dict:
    try:
        return json.loads(PRICING_FILE.read_text())
    except Exception:
        return {"_default": {"in": 3.0, "out": 15.0,
                             "cache_read": 0.3, "cache_write": 3.75}}


def price_row(model: str, pricing: dict) -> dict:
    best, best_len = pricing.get("_default", {}), 0
    for prefix, row in pricing.items():
        if prefix != "_default" and model.startswith(prefix) and len(prefix) > best_len:
            best, best_len = row, len(prefix)
    return best


def usage_cost(usage: dict, model: str, pricing: dict) -> float:
    row = price_row(model, pricing)
    per_m = 1_000_000
    return (usage.get("input_tokens", 0) * row.get("in", 0) / per_m
            + usage.get("output_tokens", 0) * row.get("out", 0) / per_m
            + usage.get("cache_read_input_tokens", 0) * row.get("cache_read", 0) / per_m
            + usage.get("cache_creation_input_tokens", 0) * row.get("cache_write", 0) / per_m)


class SpendScanner:
    """Walks the transcript JSONLs and aggregates daily/monthly spend.

    Claude Code rewrites an assistant message several times while it runs
    tool calls, so rows are deduplicated by message id (keeping the last
    occurrence's usage, which is cumulative).
    """

    def __init__(self, ttl: float = 5.0):
        self.pricing = load_pricing()
        self.ttl = ttl
        self._lock = threading.Lock()
        self._value: dict | None = None
        self._at = 0.0

    def _scan(self) -> dict:
        per_message: dict[str, tuple[str, float]] = {}   # id -> (date, cost)
        if TRANSCRIPTS.exists():
            for path in TRANSCRIPTS.rglob("*.jsonl"):
                try:
                    with path.open() as fh:
                        for line in fh:
                            try:
                                row = json.loads(line)
                            except json.JSONDecodeError:
                                continue
                            if row.get("type") != "assistant":
                                continue
                            msg = row.get("message") or {}
                            usage = msg.get("usage")
                            mid = msg.get("id")
                            ts = row.get("timestamp")
                            if not (usage and mid and ts):
                                continue
                            try:
                                when = dt.datetime.fromisoformat(
                                    ts.replace("Z", "+00:00")).astimezone()
                            except ValueError:
                                continue
                            cost = usage_cost(usage, msg.get("model", ""), self.pricing)
                            per_message[mid] = (when.date().isoformat(), cost)
                except OSError:
                    continue

        by_day: dict[str, float] = {}
        for day, cost in per_message.values():
            by_day[day] = by_day.get(day, 0.0) + cost

        today = dt.date.today()
        last7 = []
        for offset in range(6, -1, -1):
            day = today - dt.timedelta(days=offset)
            last7.append({"date": day.isoformat(),
                          "cost_usd": round(by_day.get(day.isoformat(), 0.0), 4)})
        month_prefix = today.strftime("%Y-%m")
        month = sum(v for k, v in by_day.items() if k.startswith(month_prefix))

        return {
            "today": {"cost_usd": round(by_day.get(today.isoformat(), 0.0), 4)},
            "month": {"cost_usd": round(month, 4)},
            "last7": last7,
        }

    def get(self) -> dict:
        with self._lock:
            if self._value is not None and time.monotonic() - self._at < self.ttl:
                return self._value
        value = self._scan()
        with self._lock:
            self._value = value
            self._at = time.monotonic()
        return value


# --------------------------------------------------------------------------
# HTTP server
# --------------------------------------------------------------------------

def lan_ip() -> str:
    try:
        probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        probe.connect(("192.0.2.1", 80))    # no packets sent for UDP connect
        ip = probe.getsockname()[0]
        probe.close()
        return ip
    except OSError:
        return "127.0.0.1"


def serve(host: str, port: int) -> None:
    token = device_token()
    limits = RateLimits()
    spend = SpendScanner()

    class Handler(BaseHTTPRequestHandler):
        def _json(self, code: int, payload: dict) -> None:
            body = json.dumps(payload).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)

        def _authed(self) -> bool:
            header = self.headers.get("Authorization", "")
            return secrets.compare_digest(header, f"Bearer {token}")

        def do_GET(self):  # noqa: N802
            route = self.path.split("?", 1)[0]
            if route == "/health":
                self._json(200, {"ok": True})
                return
            if route != "/usage":
                self._json(404, {"error": "not found"})
                return
            if not self._authed():
                self.send_response(401)
                self.send_header("WWW-Authenticate", "Bearer")
                self.end_headers()
                return
            try:
                payload = dict(spend.get())
                payload["limits"] = limits.get()
                payload["ts"] = dt.datetime.now().astimezone().isoformat(
                    timespec="seconds")
                self._json(200, payload)
            except Exception as exc:
                sys.stderr.write(f"[usage] {exc!r}\n")
                self._json(500, {"error": "internal"})

        def log_message(self, fmt, *args):
            pass

    server = ThreadingHTTPServer((host, port), Handler)
    print("Clawdito bridge running")
    print(f"  device endpoint:  http://{lan_ip()}:{port}/usage")
    print(f"  device token:     {token}")
    print("  (enter both in the Clawdito setup portal)")
    server.serve_forever()


def main() -> None:
    ap = argparse.ArgumentParser(description="Clawdito bridge")
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=8787)
    args = ap.parse_args()
    serve(args.host, args.port)


if __name__ == "__main__":
    main()
