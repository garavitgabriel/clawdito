# Contributing to Clawdito

This is a small hardware project maintained in spare time. Issues and PRs
are welcome; the notes below are here so a contribution doesn't stall on
something avoidable.

## The most useful contribution: another board

The architecture is deliberately split so that porting is tractable:

- The **bridge** (`bridge/clawdito_bridge.py`) knows nothing about the
  device. It serves JSON over HTTP. Any board that can do an authenticated
  `GET` can use it unchanged.
- The **firmware** is where the board-specific parts live: a display driver
  (`DisplayJD9853`), a touch driver (`TouchAXS5106`), and a pin map.

So a port to another ESP32-S3 board is mostly: swap those two drivers, set
the pins, adjust the LVGL layout for the new resolution. The UI code
(`Ui.cpp`), config, provisioning portal and polling all carry over.

If you port one, please add its pin map and quirks to
[`docs/HARDWARE.md`](docs/HARDWARE.md). That file exists because this board's
traps cost a full evening each; saving the next person that is the point.

## Before you open a PR

**Firmware** — it must compile:

```bash
cd firmware
pio run
```

CI runs exactly this on every PR. There's no test suite; the display is the
test. Say in the PR description whether you ran it on real hardware or only
compiled it, and which board. "Compile-only" is an acceptable answer — an
untested-on-hardware PR that says so is far more useful than one that
implies a test that didn't happen.

**Bridge** — Python 3.10+, **standard library only**. This is a hard
constraint, not a preference: the bridge has to be runnable with
`python3 clawdito_bridge.py` on a machine with no virtualenv and no
`pip install` step. A PR that adds a dependency needs to argue for it.

```bash
python3 -m compileall bridge/
python3 bridge/clawdito_bridge.py --port 8788   # smoke test on a spare port
```

## Reporting a bug

Please include:

- Your board (exact Waveshare model — the Touch and non-Touch 1.47" boards
  look identical and are not compatible).
- Which page misbehaves, and whether the **other** pages work. Cost and
  Usage come from different sources, so "Cost works, Usage is blank" is a
  much sharper signal than "it's broken" — it points at the credential read.
- What `curl -s -H "Authorization: Bearer <token>" http://<ip>:8787/usage`
  returns from another machine. **Redact the token** before pasting.

## Please don't paste

- Your bridge **token** (`~/.clawdito/token`).
- Anything out of `~/.claude/.credentials.json` or the `Claude Code-credentials`
  Keychain item — that's a live OAuth credential for your Anthropic account.

Bridge logs are safe; the bridge never logs the token or the OAuth blob.

## Scope

Things that fit this project: more boards, better provisioning, additional
pages, packaging, a case, docs.

Things that don't: anything that needs the bridge to reach the internet
beyond the single usage read, anything that writes to your Claude Code
state, or anything that ships a credential to the device. The device is
deliberately dumb — it receives numbers and renders them.
