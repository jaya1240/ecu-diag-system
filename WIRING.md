# Wiring Reference (STM32F103C8T6 "Blue Pill")

## CAN (bxCAN via an external transceiver)

| STM32 pin | Function | Connects to           |
|-----------|----------|------------------------|
| PA11      | CAN_RX   | Transceiver RXD (e.g. TJA1050 pin 4) |
| PA12      | CAN_TX   | Transceiver TXD (e.g. TJA1050 pin 1) |
| 3V3/5V    | Power    | Transceiver VCC (check your transceiver's voltage!) |
| GND       | Ground   | Transceiver GND, and common ground with the bus/tool |

Transceiver CANH/CANL go to the bus (PCAN-USB, Vector box, or a second
node). Terminate both ends of the bus with 120Ω if you're building a
point-to-point bench setup.

> ⚠️ The Blue Pill's PA11/PA12 are shared with USB D-/D+ on some
> variants — check your specific board's silkscreen/schematic before
> wiring, and remap CAN pins (PB8/PB9) via `HAL_AFIO_REMAP` if needed.

## UART (console)

| STM32 pin | Function | Connects to                        |
|-----------|----------|--------------------------------------|
| PA9       | USART1_TX | RX of USB-TTL adapter / ST-Link VCP |
| PA10      | USART1_RX | TX of USB-TTL adapter / ST-Link VCP |
| GND       | Ground    | Common ground with the adapter      |

Terminal settings: **115200 baud, 8 data bits, no parity, 1 stop bit**
(8N1), no flow control. Any serial terminal works (PuTTY, minicom,
`screen /dev/ttyUSB0 115200`, the Arduino IDE Serial Monitor, etc.).

## Status LED

| STM32 pin | Function        |
|-----------|-------------------|
| PC13      | Onboard LED, used as an error/fault indicator (fast blink = `Error_Handler`) |

## Bench test without a real vehicle bus

You don't need a car to exercise this: two Blue Pills wired
CANH-to-CANH / CANL-to-CANL (with 120Ω termination at each end) form a
minimal two-node bus. Flash this firmware to one board as the "ECU" and
use a PC CAN tool (or a second, simpler "tester" sketch) on the other
node to send the request frames from `Docs/PROTOCOL.md`.
