#pragma once
#include <cstdint>
#include <cstdio>
#include "pico/stdlib.h"
// A deliberately small temperature-only driver. Raw pressure/humidity bytes
// are read as part of the burst but not converted or advertised as measurements.
template<class Bus> void temperature_demo(Bus& bus) {
    auto fail = [](const char* why) { std::printf("BME280: %s\n",why); };
    sleep_ms(10);
    uint8_t id = 0;
    if (!bus.read(0xd0,&id,1) || id != 0x60) {
        fail("missing device or wrong chip ID (expected 0x60)"); return;
    }
    uint8_t cal[6];
    if (!bus.read(0x88,cal,6)) { fail("calibration read failed"); return; }
    const uint16_t t1 = static_cast<uint16_t>(cal[0] | (cal[1]<<8));
    const auto signed16 = [](unsigned x) -> int32_t { return x < 32768 ? static_cast<int32_t>(x) : static_cast<int32_t>(x)-65536; };
    const int32_t t2=signed16(cal[2] | (cal[3]<<8)), t3=signed16(cal[4] | (cal[5]<<8));
    if (t1 == 0 || t1 == 65535) { fail("invalid calibration"); return; }
    while (true) {
        // osrs_t=1, pressure skipped, forced mode. No humidity conversion requested.
        if (!bus.write(0xf4,0x21)) { fail("forced-mode write failed"); return; }
        sleep_ms(10); // Datasheet conversion time for temperature x1 is below this.
        bool ready=false;
        for (unsigned n=0;n<100;++n) {
            uint8_t status;
            if (!bus.read(0xf3,&status,1)) { fail("status read failed"); return; }
            if (!(status & 0x09)) { ready=true; break; }
            sleep_ms(1);
        }
        if (!ready) { fail("measurement timeout"); return; }
        uint8_t raw[8];
        if (!bus.read(0xf7,raw,8)) { fail("data read failed"); return; }
        const int32_t adc=(static_cast<int32_t>(raw[3])<<12) | (raw[4]<<4) | (raw[5]>>4);
        if (adc == 0x80000) { fail("temperature channel skipped"); return; }
        // Bosch datasheet floating-point temperature formula. No signed-shift UB.
        const double v1=(adc/16384.0-t1/1024.0)*t2;
        const double d=adc/131072.0-t1/8192.0;
        const double v2=d*d*t3;
        std::printf("BME280 temperature %.2f C (chip temperature, not calibrated ambient)\n",(v1+v2)/5120.0);
        sleep_ms(1000);
    }
}
