#pragma once
#include <cstdint>
#include <cstdio>
#include "pico/stdlib.h"

// A deliberately small temperature-only driver. Raw pressure/humidity bytes
// are read as part of the burst but not converted or advertised as measurements.
template <class Bus> void temperature_demo(Bus& bus) {
    auto fail = [](const char* message) { std::printf("BME280: %s\n", message); };
    sleep_ms(10);
    uint8_t chip_id = 0;
    if (!bus.read(0xd0, &chip_id, 1) || chip_id != 0x60) {
        fail("missing device or wrong chip ID (expected 0x60)");
        return;
    }
    uint8_t calibration_bytes[6];
    if (!bus.read(0x88, calibration_bytes, 6)) {
        fail("calibration read failed");
        return;
    }
    const uint16_t dig_t1 =
        static_cast<uint16_t>(calibration_bytes[0] | (calibration_bytes[1] << 8));
    const auto decode_signed16 = [](unsigned unsigned_value) -> int32_t {
        return unsigned_value < 32768 ? static_cast<int32_t>(unsigned_value)
                                      : static_cast<int32_t>(unsigned_value) - 65536;
    };
    const int32_t dig_t2 = decode_signed16(calibration_bytes[2] |
                                           (calibration_bytes[3] << 8)),
                  dig_t3 = decode_signed16(calibration_bytes[4] |
                                           (calibration_bytes[5] << 8));
    if (dig_t1 == 0 || dig_t1 == 65535) {
        fail("invalid calibration");
        return;
    }
    while (true) {
        // osrs_t=1, pressure skipped, forced mode. No humidity conversion requested.
        if (!bus.write(0xf4, 0x21)) {
            fail("forced-mode write failed");
            return;
        }
        sleep_ms(10); // Datasheet conversion time for temperature x1 is below this.
        bool measurement_ready = false;
        for (unsigned poll_attempt = 0; poll_attempt < 100; ++poll_attempt) {
            uint8_t status;
            if (!bus.read(0xf3, &status, 1)) {
                fail("status read failed");
                return;
            }
            if (!(status & 0x09)) {
                measurement_ready = true;
                break;
            }
            sleep_ms(1);
        }
        if (!measurement_ready) {
            fail("measurement timeout");
            return;
        }
        uint8_t measurement_bytes[8];
        if (!bus.read(0xf7, measurement_bytes, 8)) {
            fail("data read failed");
            return;
        }
        const int32_t raw_temperature =
            (static_cast<int32_t>(measurement_bytes[3]) << 12) |
            (measurement_bytes[4] << 4) | (measurement_bytes[5] >> 4);
        if (raw_temperature == 0x80000) {
            fail("temperature channel skipped");
            return;
        }
        // Bosch datasheet floating-point temperature formula. No signed-shift UB.
        const double linear_correction =
            (raw_temperature / 16384.0 - dig_t1 / 1024.0) * dig_t2;
        const double temperature_delta = raw_temperature / 131072.0 - dig_t1 / 8192.0;
        const double quadratic_correction =
            temperature_delta * temperature_delta * dig_t3;
        std::printf(
            "BME280 temperature %.2f C (chip temperature, not calibrated ambient)\n",
            (linear_correction + quadratic_correction) / 5120.0);
        sleep_ms(1000);
    }
}
