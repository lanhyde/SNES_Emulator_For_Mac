//
//  Memory.cpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//
#include "Memory.hpp"
#include <cstring>

Memory::Memory() {
    wram.resize(128 * 1024);        // 128KB Work RAM
    sram.resize(32 * 1024);         // 32KB Save RAM (can vary)
    vram.resize(64 * 1024);         // 64KB Video RAM
    cgram.resize(512);              // 512 bytes Color RAM
    oam.resize(544);                // 544 bytes OAM
}

Memory::~Memory() { }

void Memory::reset() {
    // Clear all RAM
    std::fill(wram.begin(), wram.end(), 0);
    std::fill(sram.begin(), sram.end(), 0);
    std::fill(vram.begin(), vram.end(), 0);
    std::fill(cgram.begin(), cgram.end(), 0);
    std::fill(oam.begin(), oam.end(), 0);
}

bool Memory::loadROM(const std::vector<uint8> &romData) {
    if (romData.empty()) {
        return false;
    }
    
    rom = romData;
    return true;
}

uint8 Memory::read(uint32 address) {
    return readMapped(address & 0xFFFFFF);
}

void Memory::write(uint32 address, uint8 value) {
    writeMapped(address & 0xFFFFFF, value);
}

Memory::MemoryRegion Memory::getRegion(uint32 address) {
    uint8 bank = (address >> 16) & 0xFF;
    uint16 offset = address & 0xFFFF;
    
    // Simplified SNES memory map
    // This is a basic implementation
    
    // Banks 0x00-0x3F and 0x80-0xBF (mirror)
    if ((bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) {
        if (offset < 0x2000) {
            return REGION_WRAM;         // Low RAM
        }
        if (offset >= 0x2000 && offset < 0x6000) {
            return REGION_HARDWARE;     // Hardware registers
        }
        if (offset >= 0x6000 && offset < 0x8000) {
            return REGION_SRAM;         // Save RAM (if present)
        }
        return REGION_ROM;              // ROM
    }
    
    // Banks 0x7E-0x7F: Extended WORK RAM
    if (bank == 0x7E || bank == 0x7F) {
        return REGION_WRAM;
    }
    
    // Banks 0x40-0x7D and 0xC0-0xFF: ROM
    return REGION_ROM;
}

uint8 Memory::readMapped(uint32 address) {
    MemoryRegion region = getRegion(address);
    uint8 bank = (address >> 16) & 0xFF;
    uint16 offset = address & 0xFFFF;
    
    switch (region) {
        case REGION_WRAM:
            if (bank == 0x7E || bank == 0x7F) {
                // Extended WRAM (banks 0x7E-0x7F map to full 128KB)
                uint32 wramAddr = ((bank & 0x01) << 16) | offset;
                if (wramAddr < wram.size()) {
                    return wram[wramAddr];
                }
            } else {
                // Low WRAM (0x0000-0x1FFF mirros first 8KB)
                if (offset < 0x2000) {
                    return wram[offset];
                }
            }
            break;
        case REGION_ROM:
        {
            // Simplified ROM mapping
            // Real SNES has LoRAM, HiROM, ExHiROM etc.
            uint32 romAddr = address & (rom.size() - 1);
            if (romAddr < rom.size()) {
                return rom[romAddr];
            }
        }
            break;
        case REGION_SRAM:
        {
            uint32 sramAddr = (offset - 0x6000) & (sram.size() - 1);
            if (sramAddr < sram.size()) {
                return sram[sramAddr];
            }
        }
            break;
        case REGION_HARDWARE:
            // TODO: Implement hardware register reads
            // For now, return open bus value
            return 0xFF;
        case REGION_UNMAPPED:
        default:
            return 0xFF;        // Open bus
    }
    return 0xFF;
}

void Memory::writeMapped(uint32 address, uint8 value) {
    MemoryRegion region = getRegion(address);
    uint8 bank = (address >> 16) & 0xFF;
    uint16 offset = address & 0xFFFF;
    
    switch (region) {
        case REGION_WRAM:
            if (bank == 0x7E || bank == 0x7F) {
                // Extended WRAM
                uint32 wramAddr = ((bank & 0x01) << 16) | offset;
                if (wramAddr < wram.size()) {
                    wram[wramAddr] = value;
                }
            } else {
                // Low WRAM
                if (offset < 0x2000) {
                    wram[offset] = value;
                }
            }
            break;
        case REGION_SRAM:
        {
            uint32 sramAddr = (offset - 0x6000) & (sram.size() - 1);
            if (sramAddr < sram.size()) {
                sram[sramAddr] = value;
            }
        }
            break;
        case REGION_HARDWARE:
            // TODO: Implement hardware register writes
            break;
        case REGION_ROM:
        case REGION_UNMAPPED:
        default:
            // Can't write to ROM or unmapped regions
            break;
    }
}

uint16 Memory::read16(uint32 address) {
    // Little-endian read: low byte first, then high byte
    uint8 lo = read(address);
    uint8 hi = read(address + 1);
    return (static_cast<uint16>(hi) << 8) | lo;
}

void Memory::write16(uint32 address, uint16 value) {
    // Little-endian write: low byte first, then high byte
    write(address, value & 0xFF);           // Low byte
    write(address + 1, (value >> 8) & 0xFF); // High byte
}
