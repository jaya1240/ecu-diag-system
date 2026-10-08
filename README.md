# Automotive ECU Communication & Diagnostics System

Embedded C firmware for STM32 microcontrollers implementing a lightweight
automotive-style diagnostics stack over **CAN** (vehicle bus, UDS-style
requests) and **UART** (bench/debug console), inspired by ISO 14229 (UDS)
and ISO 15765 (CAN transport).

This project simulates the core responsibilities of an ECU's diagnostic
layer: session control, reading live data (PIDs), reading/clearing
Diagnostic Trouble Codes (DTCs), ECU reset, and a simple security-access
gate — all built on top of hand-written CAN and UART drivers on the
STM32 HAL.

---

## Features

- **CAN driver** (`can_driver.c/h`)
  - HAL-based CAN peripheral init (500 kbps typical for HS-CAN)
  - Standard 11-bit identifiers, configurable filter bank
  - Non-blocking TX queue + RX interrupt callback
  - CAN frame ↔ diagnostic message adapter (single-frame, ISO-TP style length byte)

- **UART driver** (`uart_driver.c/h`)
  - Interrupt-driven RX with a ring buffer
  - Line-based command parsing for a bench diagnostic console
  - Used for local debugging without a CAN tool (PCAN/Vector) attached

- **Diagnostic services** (`diagnostics.c/h`), modeled loosely on UDS (ISO 14229):
  | SID  | Service                          | Description                          |
  |------|-----------------------------------|---------------------------------------|
  | 0x10 | Diagnostic Session Control        | Default / Programming / Extended     |
  | 0x11 | ECU Reset                         | Soft reset via NVIC_SystemReset      |
  | 0x14 | Clear Diagnostic Information      | Clears stored DTCs                    |
  | 0x19 | Read DTC Information              | Returns stored DTC list + status     |
  | 0x22 | Read Data By Identifier (PID)     | Reads live signals (RPM, coolant, battery voltage, vehicle speed) |
  | 0x27 | Security Access                   | Seed/key gate before privileged services |
  | 0x3E | Tester Present                    | Keeps an extended session alive       |

- **Ring buffer** (`ring_buffer.c/h`) — generic byte FIFO shared by UART RX and CAN RX queues.

- **Fault simulation** — a background task injects simulated faults
  (coolant over-temp, low battery voltage) so DTC set/clear logic can be
  exercised without real sensors.

- **Dual transport, one service layer** — the same `Diag_ProcessRequest()`
  handles requests whether they arrive framed as a CAN diagnostic frame or
  as a UART console command, so the diagnostic logic is transport-agnostic.

---

## Architecture

```
                +--------------------+
                |   Application      |
                |  (main.c - 1ms     |
                |   scheduler loop)  |
                +---------+----------+
                          |
              +-----------+-----------+
              |                       |
     +--------v-------+       +-------v--------+
     | diagnostics.c   |       | fault_sim (in   |
     | (UDS-style       |       |  main.c)        |
     |  service layer)  |       +----------------+
     +--------+-------+
              |
     +--------+---------+
     |                  |
+----v-----+      +-----v------+
| can_driver|      | uart_driver|
| (HS-CAN)  |      | (console)  |
+----+-----+      +-----+------+
     |                  |
  CAN bus            USB/UART
 (bench tool /        terminal
  vehicle network)    (PuTTY, minicom)
```

---

## Hardware target

- **MCU:** STM32F103C8T6 ("Blue Pill") or STM32F407 Discovery (any STM32
  with a bxCAN peripheral works with minor pin remapping)
- **CAN transceiver:** MCP2551 / TJA1050 (3.3V/5V level shifted as needed)
- **UART:** onboard ST-Link VCP or USB-TTL adapter (115200 8N1)
- **Bus tooling (optional):** PCAN-USB, Vector CANoe/CANalyzer, or a second
  STM32 running a simple CAN echo for loopback testing

See [`Docs/WIRING.md`](Docs/WIRING.md) for pinout.

---

## Repository layout

```
ecu-diag-system/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── can_driver.h
│   │   ├── uart_driver.h
│   │   ├── diagnostics.h
│   │   ├── ring_buffer.h
│   │   ├── stm32f1xx_it.h
│   │   └── stm32f1xx_hal_conf.h
│   └── Src/
│       ├── main.c              (clock tree, GPIO, CAN/UART peripheral init)
│       ├── can_driver.c
│       ├── uart_driver.c
│       ├── diagnostics.c
│       ├── ring_buffer.c
│       ├── stm32f1xx_it.c      (IRQ handlers)
│       └── system_stm32f1xx.c  (CMSIS SystemInit / SystemCoreClockUpdate)
├── Startup/
│   └── startup_stm32f103xb.s   (vector table + Reset_Handler, GNU assembler)
├── Docs/
│   ├── PROTOCOL.md
│   └── WIRING.md
├── STM32F103C8Tx_FLASH.ld      (linker script: 64KB flash / 20KB RAM)
├── Makefile
├── .gitignore
└── LICENSE
```

Everything above — clock tree, GPIO/CAN/UART peripheral init, IRQ handlers,
startup file, and linker script — is filled in and specific to an
STM32F103C8T6 ("Blue Pill") at 72MHz, not left as a stub. The one thing
this repo intentionally does **not** vendor is ST's own HAL/CMSIS driver
*sources* (`stm32f1xx_hal_*.c`, ARM CMSIS core headers) — that's ST's
code under its own license, so it's pulled in from a local
`STM32CubeF1` package at build time (`HAL_DIR`, see Build below) rather
than committed to this repo.

---

## Building

### Option A — Makefile + arm-none-eabi-gcc (this repo builds as-is)
```bash
# Requires: arm-none-eabi-gcc/newlib, and a local STM32CubeF1 checkout
# (for ST's HAL driver .c sources + CMSIS device/core headers — see note above)
git clone https://github.com/STMicroelectronics/STM32CubeF1.git /opt/STM32CubeF1

make HAL_DIR=/opt/STM32CubeF1        # -> build/ecu-diag-system.bin
make HAL_DIR=/opt/STM32CubeF1 flash  # uses st-flash (stlink-tools); edit
                                      # the Makefile's `flash` target if you
                                      # use OpenOCD/J-Link instead
```

### Option B — STM32CubeIDE
1. Create a new STM32F103C8 project in CubeIDE (this generates its own
   HAL sources, startup file and linker script for you).
2. Delete/skip the generated `Core/Src/main.c` and copy this repo's
   `Core/Inc` and `Core/Src` over the generated ones (drop this repo's
   `startup_*.s` and `.ld` — CubeIDE's generated ones work fine too).
3. Build and flash via ST-Link, as usual.

---

## Talking to it

### Over UART (bench console, 115200 8N1)
```
> session extended
OK: session=EXTENDED

> readpid 0x0C
OK: RPM = 2350 rpm

> readdtc
OK: 1 DTC(s)
  P0128 - Coolant Thermostat (status: 0x09 confirmed+active)

> cleardtc
OK: DTCs cleared

> reset
OK: resetting...
```

### Over CAN (diagnostic request/response, ID 0x7E0 / 0x7E8 by default)
```
TX 0x7E0  [02 10 03]        -> Diagnostic Session Control (Extended)
RX 0x7E8  [02 50 03]        -> Positive response

TX 0x7E0  [03 22 0C 00]     -> Read Data By Identifier (RPM)
RX 0x7E8  [04 62 0C 00 5C]  -> Positive response, RPM raw = 0x5C
```

Full service/PID table and byte layouts are in
[`Docs/PROTOCOL.md`](Docs/PROTOCOL.md).

---

## Roadmap / ideas for extension
- Real ISO-TP multi-frame support (currently single-frame only)
- Persist DTCs to on-chip flash so they survive a reset
- CRC/checksum on UART frames for noisy bench setups
- Python host-side test script using `python-can` to drive the CAN service layer in CI

## License
MIT — see [LICENSE](LICENSE).
