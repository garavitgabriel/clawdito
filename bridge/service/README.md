# Running the bridge as a service

The Clawdito device shows data for exactly as long as the bridge is
answering. Started by hand, the bridge dies with its terminal and does not
come back after a reboot — the device then shows "bridge offline" until you
remember to start it again.

These two units fix that.

| File | Platform | Scope |
|---|---|---|
| [`com.clawdito.bridge.plist`](com.clawdito.bridge.plist) | macOS | LaunchAgent (per-user) |
| [`clawdito-bridge.service`](clawdito-bridge.service) | Linux | systemd `--user` |

Both files have `EDIT ME` markers on the paths. Install steps are in the
comment header of each file.

## Why user-scope, not system-scope

The bridge authenticates to Anthropic's usage endpoint with **your** Claude
Code login, and that credential is stored per-user:

- **macOS** — in the login Keychain, which is unlocked by your GUI login. A
  LaunchDaemon starts before login, runs as root, and will never be able to
  read it. It must be a LaunchAgent.
- **Linux** — in `~/.claude/.credentials.json`, owned by your user.

Running either as root buys nothing and breaks the credential read.

## macOS: the Keychain prompt

The first time the service fetches limits, macOS prompts to allow access to
the `Claude Code-credentials` item. Click **Always Allow**.

If you click plain "Allow", the prompt returns on the next fetch — and under
launchd there's no terminal in front of you to answer it, so the Usage page
silently stays empty while the Cost page keeps working. That split (cost
fine, limits blank) is the signature of a denied or un-persisted Keychain
grant.

## Choosing the host machine

Pick the machine that is actually on when you're working. A laptop that
sleeps takes the device offline every time it closes; an always-on desktop
or mini keeps the display live.

If the bridge machine's LAN IP changes (new network, DHCP lease), the device
can no longer reach it and you'll need to re-provision with the new IP. A
DHCP reservation on your router avoids this permanently.

## Verifying

```bash
curl -s http://127.0.0.1:8787/health        # {"ok": true}
```

Then from any other machine on the LAN, with the token the bridge printed:

```bash
curl -s -H "Authorization: Bearer <token>" http://<bridge-ip>:8787/usage
```

If the first works and the second doesn't, it's a firewall or a
wrong-interface bind, not a Clawdito problem — check that the bridge is
bound to `0.0.0.0` (the default) rather than localhost.
