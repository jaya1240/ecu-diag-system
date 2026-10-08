# Diagnostic Protocol Reference

This firmware implements a small subset of **ISO 14229 (UDS)** service IDs,
carried either as single-frame CAN messages (ID `0x7E0` request /
`0x7E8` response) or as human-typed commands over the UART console.
It is a learning/demo protocol, not a certified UDS stack — see
"Known limitations" at the bottom.

## CAN framing

Single-frame only (payload ≤ 7 bytes):

```
byte 0       : payload length N (of SID + params)
byte 1..N    : SID + params
byte N+1..7  : unused / zero-padded
```

Example — Tester Present:
```
TX 0x7E0  [01 3E]           -> length=1, SID=0x3E
RX 0x7E8  [02 7E 00]        -> length=2, SID=0x7E (0x3E+0x40), sub=0x00
```

## Service table

| SID  | Name                          | Request params                | Positive response                        |
|------|-------------------------------|--------------------------------|-------------------------------------------|
| 0x10 | Diagnostic Session Control    | `sub` (0x01/0x02/0x03)         | `0x50 sub`                                |
| 0x11 | ECU Reset                     | —                               | `0x51 0x01`                               |
| 0x14 | Clear Diagnostic Information  | —                               | `0x54`                                    |
| 0x19 | Read DTC Information          | —                               | `0x59 0x02 [DTC(3B)+status(1B)]...`       |
| 0x22 | Read Data By Identifier       | `pidHi pidLo`                  | `0x62 pidHi pidLo data...`                |
| 0x27 | Security Access (requestSeed) | `0x01`                          | `0x67 0x01 seedHi seedLo`                 |
| 0x27 | Security Access (sendKey)     | `0x02 keyHi keyLo`             | `0x67 0x02`                               |
| 0x3E | Tester Present                | —                               | `0x7E 0x00`                               |
| —    | Negative response             | —                               | `0x7F sid nrc`                            |

## Session types (sub-function of 0x10)

| Value | Session      |
|-------|--------------|
| 0x01  | Default      |
| 0x02  | Programming  |
| 0x03  | Extended     |

Extended session auto-drops back to Default after
`TESTER_PRESENT_TIMEOUT_MS` (5s) with no request seen — send `0x3E`
periodically (or type `session extended` again) to hold it open.

## Live data identifiers (Read Data By Identifier, SID 0x22)

| PID (hex) | Signal            | Encoding                          |
|-----------|--------------------|-------------------------------------|
| 0x0C00    | Engine RPM         | 2 bytes, big-endian, raw RPM       |
| 0x0500    | Coolant Temp       | 1 byte, value = °C + 40 (OBD-style)|
| 0x0D00    | Vehicle Speed      | 1 byte, km/h                        |
| 0x0F00    | Battery Voltage    | 2 bytes, big-endian, millivolts    |

## Negative response codes (NRC)

| Code | Meaning                        |
|------|----------------------------------|
| 0x11 | Service not supported            |
| 0x12 | Sub-function not supported       |
| 0x22 | Conditions not correct           |
| 0x31 | Request out of range             |
| 0x33 | Security access denied           |

## DTC status byte

| Bit | Meaning                                  |
|-----|-------------------------------------------|
| 0   | testFailed                                |
| 1   | confirmedDTC                              |
| 2   | pendingDTC                                |
| 3   | testNotCompletedSinceLastClear            |

The demo firmware only ever sets bits 0+1 (`0x03`) when a simulated fault
is active, and clears the whole byte to `0x00` on `cleardtc` / SID 0x14.

## Security access (demo only — not real crypto)

`requestSeed` returns `seed = 0xA5A5 ^ (tick & 0xFFFF)`. The expected key
is `seed ^ 0x1234`. This exists purely to demonstrate a seed/key gate in
the service dispatcher — do not reuse this scheme for anything that needs
actual security.

## Known limitations / roadmap

- **No multi-frame ISO-TP.** Requests/responses over CAN are capped at a
  7-byte payload. Extend `Diag_HandleCanFrame` with First/Consecutive
  Frame handling (ISO 15765-2) to lift this.
- **DTCs are volatile.** They reset on power-cycle; nothing is written to
  flash.
- **Security access gates nothing yet.** `s_security_unlocked` is tracked
  but no service currently checks it — wire it into whichever services you
  want gated (e.g. require unlock before `ECU Reset` in Programming
  session).
