# C.1 — Meet an optional BME280

The main book needs no purchased sensor. Its Pico register interface and internal temperature reading already let you explore buses with the existing boards. This optional sequence replaces that simulated peripheral with an **Adafruit BME280 breakout, product 2652**. You will identify the extra board here, read it over I²C in [C.2](appendix-c-2.md), understand temperature conversion in [C.3](appendix-c-3.md), and reconnect it for SPI in [C.4](appendix-c-4.md).

No sensor is assumed to be in your kit. You can read this page and leave the bench unchanged if you do not have the part. If you do, the result today is an identified, unpowered breakout and an isolated Pico ready for C.2—not a temperature measurement. Revisit [breadboard contacts](0.5-recognise-the-breadboard-and-kit-components.md), [electrical rules](0.6-assemble-the-bench-and-understand-the-electrical-rules.md), [I²C](6.1-i-c-on-the-wires.md) and [SPI](6.4-compare-spi-with-a-loopback-experiment.md) as needed.

## Identify the chip and the board separately

The **BME280** is the tiny metal-topped sensor package. It can measure barometric pressure, relative humidity and temperature. The larger **breakout** is a printed circuit board that brings the chip's small connections to usable contacts and adds supporting electronics. The current product has two white STEMMA QT connectors; earlier product-2652 boards lack them. Adafruit documents changes to board shape and silkscreen while retaining the header pinout. Compare the exact board and its markings with the [official product photographs and revision history](https://www.adafruit.com/product/2652).

![Original top-view orientation drawing of the QT breakout: mounting holes above, seven header holes below ordered VIN, 3Vo, GND, SCK, SDO, SDI, CS. The sensor is central, regulator left of it, interface circuitry right, and QT sockets at the sides. A separate lower panel explains their electrical roles.](assets/C.1-bme280-orientation.svg)

[Open the larger orientation drawing](assets/C.1-bme280-orientation.svg). This is an original, simplified drawing checked against Adafruit's [QT board CAD and schematic](https://github.com/adafruit/Adafruit-BMP280-Breakout-PCB), linked from the [BME280 downloads page](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/downloads). Adafruit uses the same QT PCB design for its BMP280 and BME280 products; the populated sensor determines which product it is. The drawing shows the component side with the seven-hole header below the sensor. It is not an underside view or a photograph of your board. Tiny support components are grouped; silk fonts and exact appearance can differ. Match **contact names**, rather than transferring this orientation to an unidentified board.

| Label | What and where | Purpose; when we use it |
| --- | --- | --- |
| 1 | Metal-topped BME280, near the centre | Performs sensing and contains the register interface. Both optional firmware targets talk to this chip. Keep its sensing opening unobstructed. |
| 2 | Regulator and capacitors, left of the sensor | Supply conditioning between VIN and the chip supply. Capacitors stabilise local power; they are not additional sensors. |
| 3 | Interface transistors, resistor network and CS support components, right/above the sensor | Adapt breakout signals and provide pull-ups. These are physical board electronics, not firmware features. |
| 4 | Seven plated header holes along the lower edge | Our jumper-wire connection points for either bus. A hole is not a reliably connected plug until a suitable header is soldered. |
| 5 | White QT sockets on the two side edges | Alternative I²C connections. We leave them empty; they do not expose the full SPI interface. |
| 6 | ADDR solder jumper on the reverse | Selects the alternative I²C address when closed. Inspect it unpowered; do not close it for the supplied example. |

A similar-looking **BMP280** is not the specified part: it has no humidity channel. Appearance, a seller's label or an I²C acknowledgement is insufficient identity evidence. The later source reads chip-ID register `0xD0` and requires **`0x60`**; Bosch lists `0x58` for production BMP280 and `0x56`/`0x57` for samples. A different ID must be investigated, not accepted by weakening the check. A matching byte supports device identification, but cannot authenticate a manufacturer or prove measurement accuracy. Do not assume an unrelated clone has the same pin order or protection circuitry. [Bosch datasheet, §5.2–5.4](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)

## Resolve the physical prerequisites

For the header route you need the identified breakout with **properly soldered seven-pin 2.54 mm headers**, free breadboard contact groups and suitable additional jumpers. Reserve at least four jumpers for I²C and six for SPI; the later tables specify each connection. The Pico, its USB data cable and the Mac toolchain are reused. A cramped breadboard may need additional space or another breadboard; the existing kit's available space and spare wires are unverified.

A loose header supplied in the bag does not mean the board is ready. If headers are absent, fitting them requires soldering equipment, solder, a suitable work area and the ability to make and inspect sound joints. Those tools and skills are not assumed. Use an already assembled board or complete that preparation separately before continuing. Do not push unsoldered pins through holes and rely on friction. Adafruit's [assembly guide](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/assembly) shows the intended header assembly. QT cables are an alternative connection system, but this course's exact wiring uses the header, including SPI contacts.

## Read the contact names as functions

The map below governs this selected breakout. In the drawing's top view the header reads **VIN, 3Vo, GND, SCK, SDO, SDI, CS**, left to right. I²C aliases explain why two labels can describe one contact; they do not introduce extra pins. See [Adafruit's pinout](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/pinouts).

| Breakout contact | Meaning and planned use |
| --- | --- |
| VIN | Power input. Our route supplies it from **Pico 3V3(OUT), physical 36**. |
| 3Vo | Regulator output. Leave unconnected; it is not a second supply input. |
| GND | Power return and signal reference; later connects to Pico GND physical 8. |
| SCK / SCL | Clock input: I²C SCL from Pico GP5 physical 7, or SPI SCK from GP18 physical 24. |
| SDI / SDA | Serial data input for SPI, also bidirectional I²C SDA: GP19 physical 25 for SPI, or GP4 physical 6 for I²C. |
| SDO / ADR | SPI data output to Pico GP20 physical 26; in I²C it selects the address instead. |
| CS | Active-low SPI select from Pico GP21 physical 27. In I²C the sensor-side CS must stay high; the board provides that pull-up, so leave the header contact unconnected. |

These are **alternative bus states**, not a combined wiring recipe. C.2 and C.4 provide their complete tables and source. In particular, remove any GP19–GP20 loopback jumper before attaching a real SPI device.

The bare chip's operating supplies are **VDD 1.71–3.6 V and VDDIO 1.2–3.6 V**; it is not a bare 5 V sensor. [Bosch electrical specifications](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) apply inside the breakout. Adafruit's VIN regulator and input-interface circuitry explain its wider board-level supply compatibility. We nevertheless use **3.3 V at VIN and 3.3 V GPIO throughout**, so the breakout's controller-side I²C pull-ups also use the intended level. Leave Pico VBUS/VSYS and every Pi supply pin out of this circuit. The breakout is powered by the Pico, not independently from the Pi.

The checked `examples/pico/bme280_i2c.cpp` fixes the seven-bit address at **`0x77`**. The unmodified Adafruit board pulls SDO high for that address; grounding SDO or closing ADDR selects `0x76`. For the supplied source, leave SDO externally unconnected and ADDR open. This differs from Chapter 6's Pico target address `0x42`. Do not shift the address or guess a generic module default. [Adafruit address selection](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/pinouts)

## Find the useful registers

A bus address selects a device; a register address selects something **inside** that device. Neither is an RP2350 memory address. These landmarks come from Bosch's [memory map and register descriptions, §5.3–5.4](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf), compared with the actual course source:

| Register | Landmark | Course use |
| --- | --- | --- |
| `0xD0` | Chip ID | Requires `0x60` before continuing. |
| `0xE0` | Reset; writing `0xB6` requests soft reset | Not written by these examples. |
| `0x88`–`0xA1`, `0xE1`–`0xF0` | Factory calibration regions | Only six temperature bytes, `0x88`–`0x8D`, are read. |
| `0xF2` | Humidity control | Not written; no humidity conversion requested. |
| `0xF3` | Status | Polls bits 3 and 0 with mask `0x09`. |
| `0xF4` | Measurement control | Writes `0x21`: temperature ×1, pressure skipped, forced mode. |
| `0xF5` | Filter/standby/interface configuration | Not written. |
| `0xF7`–`0xF9` | Raw pressure | Included in burst, not converted. |
| `0xFA`–`0xFC` | Raw temperature | Assembled and compensated. |
| `0xFD`–`0xFE` | Raw humidity | Included in burst, not converted. |

Both transports call `temperature_demo` in `examples/pico/bme280_common.hpp`. After startup waits it checks identity, reads temperature calibration, then repeatedly requests one forced measurement, waits, polls readiness with a bound, reads eight bytes from `0xF7`, and prints temperature. Forced mode returns to sleep after a measurement. Start the optional lessons from a fresh power-up; the code does not reset every configuration register or sanitise settings left by an arbitrary earlier driver.

Reading pressure/humidity byte positions does not implement those measurements. The shared code implements **temperature compensation only**, using the temperature portion of Bosch's formula. It does not implement the complete Bosch pressure/humidity algorithms. Nor does two-decimal-place output prove ambient accuracy: board heat, placement and thermal settling affect what the sensor experiences. C.3 separates those limits from the arithmetic.

## Leave the capstone safely before the sensor lesson

1. Stop the capstone, serial/I²C/GPIO clients, USB terminals and any OpenOCD/GDB sessions. If the Pi is running, use **Pi — SSH shell, any directory:** `sudo poweroff`. Wait for shutdown, then unplug its USB-C supply. Unplug Pico USB and powered debug equipment too.
2. With both boards unpowered, remove **all three UART wires**, including their shared ground, plus any remaining Pi–Pico I²C or bus wires and all probe/debug connections. Remove a retained SPI loopback jumper. Keep the Pi off and physically separate throughout the optional sensor work.
3. Leave the Pico's local GP16 LED and GP17 button/pull-up circuit if desired. These BME280 programs do not configure or drive them. Check that the sensor will have distinct, accessible breadboard nodes without disturbing those circuits.
4. Inspect the breakout's identity, revision, header soldering, labels and open ADDR jumper. Set it beside the unpowered Pico. **Do not attach or change sensor wiring while powered.** Leave all sensor wires unattached until C.2's complete wiring step.

| Inspection or later symptom | Next action |
| --- | --- |
| Missing headers, loose pins or solder bridges | Resolve assembly before applying power; software cannot repair a connection. |
| Labels/order differ from this map | Identify the exact board and obtain its schematic. Do not rotate or rename wires by guesswork. |
| Later I²C NACK | Power down before checking VIN/GND, SDA/SCL, address strap and installed firmware. A `0x76` board will not answer this code's `0x77`. |
| Later wrong-chip-ID error | Separate failed transport from an unexpected returned ID. Check the exact part; do not bypass the `0x60` requirement. |
| LED/button stops responding after flashing | Expected for these temperature-only images; use USB diagnostics for their observations. |

No firmware is built or run on this orientation page. `verification.json` records successful Pico 2 target compilation for both BME280 images, including a later I²C pull-configuration rebuild. Their source has been inspected; **sensor identity, header joints, physical wiring, bus operation and readings have not been verified on hardware**. Continue to [C.2](appendix-c-2.md) with Pico unplugged, sensor unattached and Pi off/separate.

## References

- [Adafruit product 2652](https://www.adafruit.com/product/2652) — identity, photographs and revisions.
- [Adafruit BME280 pinouts](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/pinouts) — header functions and address selection.
- [Adafruit downloads](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/downloads) — original and QT schematics, board layouts and CAD links.
- [Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) — electrical limits, identity, registers and compensation.
