//
//  Types.hpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
// Common type definition for SNES emulator
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

struct Address24 {
    uint8 bank;
    uint16 offset;
    
    Address24(): bank(0), offset(0) {}
    Address24(uint8 b, uint16 o): bank(b), offset(o) {}
    Address24(uint32 addr): bank((addr >> 16) & 0xFF), offset(addr & 0xFFFF) {}
    
    uint32 toLinear() const {
        return (static_cast<uint32>(bank) << 16) | offset;
    }
};
#endif
