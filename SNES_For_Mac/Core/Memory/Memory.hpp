//
//  Memory.hpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

#ifndef MEMORY_HPP
#define MEMORY_HPP


#include "../Types/Types.hpp"
#include <vector>

class Memory {
public:
    Memory();
    ~Memory();
    
    // Read/Write operations
    uint8 read(uint32 address);
    void write(uint32 address, uint8 value);
    
    // Load ROM data
    bool loadROM(const std::vector<uint8>& romData);
    
    // Reset memory to initial state
    void reset();
    
private:
    // SNES Memory Map (simplified for now)
    // Total addressable space: 16MB (24-bit addressing)
    static const uint32 MEMORY_SIZE = 0x1000000;             // 16MB
    
    // RAM regions
    std::vector<uint8> wram;                    // Work RAM (128KB)
    std::vector<uint8> sram;                    // Save RAM (varies by cart)
    std::vector<uint8> vram;                    // Video RAM (64KB)
    std::vector<uint8> cgram;                   // Color RAM (512 bytes)
    std::vector<uint8> oam;                     // Object Attribute Memory (544 bytes)
    
    // ROM data
    std::vector<uint8> rom;
    
    // Memory mapping helper
    uint8 readMapped(uint32 address);
    void writeMapped(uint32 address, uint8 value);
    
    // Map SNES address to physical memory
    // SNES has a complex memory map that varies by region
    
    enum MemoryRegion {
        REGION_ROM,
        REGION_WRAM,
        REGION_SRAM,
        REGION_HARDWARE,
        REGION_UNMAPPED
    };
    
    MemoryRegion getRegion(uint32 address);
};
#endif
