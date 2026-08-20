# 🦀 Clawdito

A little desk companion that shows your **Claude Code usage** in real time —
your 5-hour rate-limit window, your weekly cap, and what your sessions would
have cost in API dollars — on a 1.47" touchscreen, with a pixel-art Clawd
that blinks at you while you work.

Built for the **Waveshare ESP32-S3-Touch-LCD-1.47** (~$20 board), driven by a
small Python bridge that runs on the computer where Claude Code lives.

## What it shows

Three pages, navigable by **swiping** the touchscreen (or tapping the BOOT
button):

| Page | Content |
|---|---|
| **Usage** | Official rate limits: 5h window % + weekly % with progress bars and exact reset countdowns — the same numbers as `/usage` inside Claude Code |
| **Clawd** | A big pixel-art Clawd. It blinks. That's it. That's the page. |
| **Cost** | Today / this month / yesterday in API-equivalent dollars, plus a 7-day bar chart, parsed from your local Claude Code transcripts |

A rotating status word in Claude Code's spinner style ("Flibbertigibbeting…",
"Noodling…") keeps the bottom edge company.

## How it works

```
┌─────────────┐   WiFi (LAN only)   ┌──────────────────────────┐
│  Clawdito   │ ──── GET /usage ──► │  clawdito_bridge.py      │
│  (ESP32-S3) │ ◄─── JSON ───────── │  on your computer        │
└─────────────┘                     │  • official rate limits   │
                                    │    via your local Claude  │
                                    │    Code login             │
                                    │  • spend stats parsed     │
                                    │    from ~/.claude/        │
                                    └──────────────────────────┘
```

Everything stays on your LAN. The bridge reads your Claude Code OAuth
credential locally (macOS Keychain or `~/.claude/.credentials.json`) to ask
Anthropic's usage endpoint for your official limits — the credential never
leaves the bridge process, and the device only ever receives percentages.
The device endpoint is protected by a bearer token generated on first run.

## Quick start

**1. Run the bridge** (Python 3.10+, standard library only):

```bash
python3 bridge/clawdito_bridge.py
```

Note the endpoint IP and token it prints.

**2. Flash the firmware** (PlatformIO):

```bash
cd firmware
pio run -t upload
```

**3. Provision the device.** On first boot the screen shows a WiFi network
(`Clawdito-XXXX`) and a one-time password. Join it from your phone, the
setup page pops up (or open `http://192.168.4.1`), pick your **2.4 GHz**
WiFi, paste the bridge IP + token, hit **Connect**. Done — numbers appear
within seconds.

To re-provision at any time: hold **BOOT** for 5 seconds.

See [docs/SETUP.md](docs/SETUP.md) for details and troubleshooting, and
[docs/HARDWARE.md](docs/HARDWARE.md) for the board's pin map and its quirks
(there are several, and they were all earned the hard way).

## Notes

- The ESP32 only does **2.4 GHz** WiFi. Your computer can stay on 5 GHz —
  both bands of the same router share a LAN.
- The cost figures are **API-equivalent estimates** priced via
  `bridge/pricing.json` — useful for trends, not an invoice. Rates for
  models without published API pricing are marked as assumptions in the file.
- The 5h/weekly percentages are **not** estimates — they're the official
  numbers from Anthropic's usage endpoint.

## License

MIT — see [LICENSE](LICENSE).
