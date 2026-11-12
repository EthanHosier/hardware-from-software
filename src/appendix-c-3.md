# C.3 — Convert raw sensor data

A successful bus read gives bytes, not degrees Celsius. This lesson follows those bytes through the **temperature-only** program already installed in [C.2](appendix-c-2.md): decode factory calibration, assemble a sample and apply compensation. You can work through the arithmetic on the Mac without a sensor. The worked values below are synthetic teaching inputs, not measurements or calibration recorded from a real board.

Keep C.2's bench unchanged if you observe the hardware: Pico 2 physical 36 supplies Adafruit 2652 VIN, physical 8 connects GND, GP4 physical 6 connects SDI/SDA, and GP5 physical 7 connects SCK/SCL. CS, SDO and 3Vo remain externally unconnected, ADDR open. The Pi stays **off and physically separate**, with no Pi–Pico wires; the probe is detached. The local GP16 LED and GP17 button circuit may remain, but this firmware does not use them. [C.2's electrical map](assets/C.2-bme280-i2c.svg) shows the complete connection state. No rewiring or new firmware is needed here.

## 1. Read the conversion code

Both transport programs call the same `temperature_demo`. Here is the entire actual `examples/pico/bme280_common.hpp`, also supplied in the [examples archive](downloads/hardware-examples.zip):

```cpp
{{#include ../examples/pico/bme280_common.hpp}}
```

The complete I²C adapter and build/flash procedure are in [C.2](appendix-c-2.md#2-read-and-build-the-complete-program). The complete SPI adapter is in [C.4](appendix-c-4.md). Neither adapter changes the conversion formula: their `read` and `write` methods move register bytes over a different bus.

**Compensation** corrects the raw ADC count using factory coefficients for that particular sensing element. A raw count is an internal converter output, with no direct Celsius scale. The coefficients describe how to turn that chip's response into a physical value; they are not portable constants to copy from another sensor. The program reads them from the connected device once before its measurement loop. [Bosch's output-compensation description, §4.2](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf), defines the calibration layout and algorithms.

## 2. Decode calibration before doing arithmetic

The six-byte read beginning at `0x88` supplies three **little-endian** words: low byte first, then high byte. The bus's byte order is a file-format-like contract; do not infer it from the Pico CPU's byte order or cast the buffer to a struct.

| Registers | Array positions | Bosch name | Interpretation |
| --- | --- | --- | --- |
| `0x88`, `0x89` | `cal[0]`, `cal[1]` | `dig_T1` (`t1`) | Unsigned 16-bit |
| `0x8a`, `0x8b` | `cal[2]`, `cal[3]` | `dig_T2` (`t2`) | Signed 16-bit |
| `0x8c`, `0x8d` | `cal[4]`, `cal[5]` | `dig_T3` (`t3`) | Signed 16-bit |

These types and pairings match [Bosch's `parse_temp_press_calib_data` function](https://github.com/boschsensortec/BME280_SensorAPI/blob/master/bme280.c). In our C++17 source, `uint8_t` holds a byte without treating values above 127 as negative. On this target its operands promote to 32-bit `int` before shifts: `cal[1] << 8` can safely represent up to 65280. OR combines the disjoint low and high parts. `uint16_t` then stores `t1` in the range 0–65535.

For the signed coefficients, first assemble the same unsigned numerical word `x`. The `signed16` lambda returns `x` for 0–32767, or **`x - 65536`** for 32768–65535. For example, bytes `18 fc` assemble to `0xfc18 = 64536`, which decodes to `-1000`. The conversion to `int32_t` occurs while the value still fits; subtraction produces the negative result. This avoids depending on an out-of-range unsigned-to-signed 16-bit cast. Keeping the decoded values in 32-bit signed variables also gives the arithmetic more room than their storage format.

Rejecting `t1 == 0` or `65535` catches conspicuously bad calibration. It does **not** validate every coefficient: the program checks no `t2`/`t3` limits and performs no calibration CRC. A plausible-looking value is not an integrity proof.

## 3. Assemble the raw temperature

The data read starts at `0xf7` and returns eight bytes. Temperature therefore occupies **indexes 3, 4 and 5**, not the start of the buffer:

| Position | Register | Contribution to the 20-bit temperature count |
| --- | --- | --- |
| `raw[3]` | `0xfa`, temperature MSB | Bits 19–12, shifted left 12 |
| `raw[4]` | `0xfb`, temperature LSB | Bits 11–4, shifted left 4 |
| `raw[5]` | `0xfc`, temperature XLSB | Upper nibble supplies bits 3–0, shifted right 4 |

The low nibble of `0xfc` is reserved and discarded. The sample is **unsigned 20-bit**, although the program stores it in `int32_t`; its maximum numerical value, 1048575, fits comfortably. These shifts operate on nonnegative values that fit the promoted types. A sample of `0x80000` is the skipped-temperature sentinel and must not be compensated into an apparent reading. Register layout and the skipped value are specified in [Bosch §5.4](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf).

The other burst positions contain pressure and humidity data. Reading their positions does not enable or convert those channels. One continuous burst also preserves the sensor's data-read shadowing guarantee: updates are held until the burst ends, avoiding a sample assembled from separately updated registers. See [Bosch §4.1](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf).

## 4. Work through one synthetic conversion

Use these illustrative inputs solely to check decoding and arithmetic:

```text
calibration bytes at 0x88..0x8d: 70 6b 43 67 18 fc
T1 = 27504; T2 = 26435; T3 = -1000
temperature bytes at 0xfa..0xfc: 7e ed 00
adc = 0x7eed0 = 519888
```

They are not claimed to be a Bosch-provided test vector, and must not replace the real program's calibration read. Its calculation is:

```text
v1 = (adc / 16384.0 - T1 / 1024.0) * T2
 d =  adc / 131072.0 - T1 / 8192.0
v2 = d * d * T3
 T = (v1 + v2) / 5120.0
```

For these inputs, `v1 = 128793.1787109375`, `d = 0.6090087890625`, and `v2 ≈ -370.891705155373`. Their sum is approximately `128422.287005782`; dividing by 5120 gives **25.08247793081682 °C**, printed as `25.08`.

The `.0` divisors make these floating-point divisions; integer division would discard fractions before the correction is finished. The two terms provide the linear and quadratic parts of the compensation. They match the temperature equation in [Bosch's floating-point appendix, §8.1](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) and the [SensorAPI double `compensate_temperature` function](https://github.com/boschsensortec/BME280_SensorAPI/blob/master/bme280.c). Bosch also retains an integer `t_fine` derived from this sum for the other channels; its floating temperature return uses the untruncated sum divided by 5120. Our temperature-only helper needs no stored `t_fine`. Unlike the API's full routine, it does not clamp the temperature to the sensor operating range.

Reproduce this calculation with decimal arithmetic independently of the firmware. This is a **Mac calculator exercise**, not an alternative firmware implementation.

**Mac — Terminal, project root (substitute your checkout location):**

```sh
cd ~/hardware-from-software
python3 - <<'PY'
from decimal import Decimal as D
adc, t1, t2, t3 = map(D, (519888, 27504, 26435, -1000))
v1 = (adc / 16384 - t1 / 1024) * t2
d = adc / 131072 - t1 / 8192
v2 = d * d * t3
print((v1 + v2) / 5120)
PY
```

The calculated output is `25.08247793081682175397872926`. Agreement checks this example's arithmetic, not the sensor or every possible input. Floating point keeps this small example readable and avoids signed-shift hazards in naive C++ ports of older integer formulas. It is not necessarily faster or exact. Bosch's maintained fixed-point implementations are valid alternatives when their types, ranges and rounding are respected.

## What happens before each printed value

Follow the actual control flow rather than assuming the calculation runs on every received byte:

1. After `main`'s two-second startup wait, the helper waits another 10 ms, reads ID `0xd0` and requires `0x60`. It then reads and decodes calibration once, applying the limited `t1` sanity check.
2. Each iteration writes `ctrl_meas` (`0xf4`) with `0x21`: temperature oversampling ×1, pressure skipped, forced mode. Forced mode returns to sleep after conversion. The code makes no `ctrl_hum` write; humidity remains skipped at its reset value zero. It also makes no reset or `config` write. Use C.2's **fresh sensor power-up** baseline; restarting only the Pico CPU need not clear settings left in a still-powered sensor. [Bosch §3.3 and §5.4](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)
3. It sleeps 10 ms, then makes up to 100 status reads, requiring both `measuring` bit 3 and `im_update` bit 0 clear (`status & 0x09` is zero). Busy results are followed by 1 ms sleeps. This bounds **attempts**, not elapsed time to 100 ms: C.2's I²C adapter gives each underlying SDK call up to 100 ms, and bus time adds to sleeps.
4. It reads the eight-byte burst, rejects the temperature sentinel, compensates and prints. A one-second sleep follows; that is not an exact one-second sampling period. On an error it prints one diagnostic and returns; `main` keeps USB alive without retrying.

## Observe and interpret the result

Optionally keep C.2's `bme280_i2c` running and its USB terminal open; no rebuild is required. If firmware is absent, use C.2's complete setup/build/flash instructions. For a fresh run, quit the terminal, unplug Pico USB so **both Pico and sensor lose power**, then reconnect normally and reopen the discovered USB port as described there. Keep all wires unchanged and the Pi separate.

A successful line has this exact shape; the number here is **illustrative, not a recorded sensor reading**:

```text
BME280 temperature 25.08 C (chip temperature, not calibrated ambient)
```

The line contains compensated chip temperature, not raw bytes or calibration values. Two decimal places describe formatting, not measurement accuracy. Heat from the board, nearby electronics or your hands and the time needed to reach thermal equilibrium affect the result. Let placement and temperature settle before comparing readings; that still does not establish a calibrated ambient thermometer.

Pressure and humidity require their own factory coefficients and dedicated compensation functions, which depend on temperature-derived `t_fine`. They also require enabling the desired channels: pressure oversampling in `ctrl_meas`, humidity oversampling in `ctrl_hum`, with the humidity control written **before** the `ctrl_meas` write that takes it into effect. The complete [Bosch SensorAPI](https://github.com/boschsensortec/BME280_SensorAPI) provides those paths. Neither supplied target implements them.

| Symptom | Interpretation and next action |
| --- | --- |
| Wrong-chip-ID diagnostic | Check C.2's identity, address and wiring procedure; this message also covers a failed ID transfer. Do not bypass the `0x60` check. |
| Implausible temperature after modifying code | Compare byte indexes, little-endian coefficient assembly and signed `T2/T3` decoding with the included source. Check that fractions survived division. Restore the canonical example. |
| `invalid calibration` or calibration-read failure | Follow C.2's connection checks with power removed. A successful `t1` check alone does not prove all six bytes correct. |
| `temperature channel skipped` | The sentinel is a state/configuration clue, not a low temperature. Restore the exact firmware and fresh power-on baseline. |
| Status/read/write failure or `measurement timeout` | Check C.2's bus diagnostics and power-on state; retain readiness checks. Reopening a terminal does not retry the helper. |
| Plausible but warm or drifting result | Consider board heat, placement and thermal settling before changing the compensation constants. |

The recorded build compiled both Pico 2 BME280 targets, including this header. The synthetic calculation above was checked on the host. **Physical sensor operation and measurement accuracy have not been exercised or established.**

Quit `screen` with **Ctrl+A**, then **Ctrl+\\**, confirming **Y** if asked. Unplug Pico USB to leave both it and its sensor unpowered; keep the Pi off/separate. [C.4](appendix-c-4.md) changes to SPI: power must be removed before replacing bus wires. Retain the four-wire I²C state until that lesson explicitly changes it.

## References

- [Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) — §4.1–4.2 data consistency/calibration, §5.4 registers and §8.1 floating compensation.
- [Bosch BME280 SensorAPI source](https://github.com/boschsensortec/BME280_SensorAPI/blob/master/bme280.c) — calibration parsing, floating/fixed-point compensation and channel configuration.
