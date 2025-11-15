//
//  CPU65c816.cpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

#include "CPU65c816.hpp"
#include "../Memory/Memory.hpp"

CPU65c816::CPU65c816(): memory(nullptr), totalCycles(0) {
    reset();
}

void CPU65c816::reset() {
    // Initialize registers to reset state
    registers.A = 0;
    registers.X = 0;
    registers.Y = 0;
    registers.SP = 0x01FF;      // Stack starts at 0x01FF in emulation mode
    registers.P = 0x34;         // Set M and X flags (8-bit mode), I flag
    registers.DBR = 0;
    registers.PBR = 0;
    registers.D = 0;
    registers.E = true;         // Start in emulation mode
    // PC should be loaded from reset vector at 0xFFFC
    // For now, we'll set it to 0x8000 at a placeholder
    registers.PC = 0x8000;
    totalCycles = 0;
}

void CPU65c816::setMemory(Memory* mem) {
    memory = mem;
}

bool CPU65c816::getFlag(StatusFlag flag) const {
    return (registers.P & flag) != 0;
}

void CPU65c816::setFlag(StatusFlag flag, bool value) {
    if (value) {
        registers.P |= flag;
    } else {
        registers.P &= ~flag;
    }
}

// Memory access functions
uint8 CPU65c816::read8(uint32 address) {
    // TODO: Implement with actual memory interface
    if (memory) {
        return memory->read(address);
    }
    return 0;
}

uint16 CPU65c816::read16(uint32 address) {
    // Little-endian read
    uint8 lo = read8(address);
    uint8 hi = read8(address + 1);
    return (static_cast<uint16>(hi) << 8) | lo;
}

void CPU65c816::write8(uint32 address, uint8 value) {
    if (memory) {
        memory->write(address, value);
    }
}

void CPU65c816::write16(uint32 address, uint16 value) {
    // Little-endian write
    write8(address, value & 0xFF);
    write8(address + 1, (value >> 8) & 0xFF);
}

// Fetch operations
uint8 CPU65c816::fetchByte() {
    uint32 address = (static_cast<uint32>(registers.PBR) << 16) | registers.PC;
    uint8 value = read8(address);
    registers.PC++;
    return value;
}

uint16 CPU65c816::fetchWord() {
    uint8 lo = fetchByte();
    uint8 hi = fetchByte();
    return (static_cast<uint16>(hi) << 8) | lo;
}

// Stack operations
void CPU65c816::push8(uint8 value) {
    write8(registers.SP, value);
    registers.SP--;
    if (registers.E) {
        // In emulation mode, stack wraps within page 1
        registers.SP = 0x0100 | (registers.SP & 0xFF);
    }
}

void CPU65c816::push16(uint16 value) {
    push8((value >> 8) & 0xFF);     // High byte first
    push8(value & 0xFF);            // Low byte second
}

uint8 CPU65c816::pull8() {
    registers.SP++;
    if (registers.E) {
        registers.SP = 0x0100 | (registers.SP & 0xFF);
    }
    return read8(registers.SP);
}

uint16 CPU65c816::pull16() {
    uint8 lo = pull8();
    uint8 hi = pull8();
    return (static_cast<uint16>(hi) << 8) | lo;
}

// Flag update helpers
void CPU65c816::updateNZ8(uint8 value) {
    setFlag(FLAG_ZERO, value == 0);
    setFlag(FLAG_NEGATIVE, (value & 0x80) != 0);
}

void CPU65c816::updateNZ16(uint16 value) {
    setFlag(FLAG_ZERO, value == 0);
    setFlag(FLAG_NEGATIVE, (value & 0x8000) != 0);
}

int CPU65c816::executeInstruction() {
    uint8 opcode = fetchByte();
    int cycles = decodeAndExecute(opcode);
    totalCycles += cycles;
    return cycles;
}

int CPU65c816::decodeAndExecute(uint8 opcode) {
    // This is where we'll decode opcode and execute instructions
    // For now, just a skeleton with a few basic instructions
    switch (opcode) {
        case 0xEA:          // NOP
            op_NOP();
            return 2;
        case 0xA9:          // LDA Immediate
            if (isMemory8Bit()) {
                uint8 value = fetchByte();
                registers.A = (registers.A & 0xFF00) | value;
                updateNZ8(value);
                return 2;
            } else {
                uint16 value = fetchWord();
                registers.A = value;
                updateNZ16(value);
                return 3;
            }
        case 0xAA:          // TAX
            op_TAX();
            return 2;
        case 0xA8:
            op_TAY();
            return 2;
        case 0x8A:
            op_TXA();
            return 2;
        case 0x98:
            op_TYA();
            return 2;
        case 0xBA:
            op_TSX();
            return 2;
        case 0x9A:
            op_TXS();
            return 2;
        case 0x5B:
            op_TCD();
            return 2;
        case 0x7B:
            op_TDC();
            return 2;
        case 0x1B:
            op_TCS();
            return 2;
        case 0x3B:
            op_TSC();
            return 2;
        default:
            // Unknown opcode - for now just NOP
            return 2;
    }
}

// Addressing mode implementation (stubs for now)
uint32 CPU65c816::getAddress_Immediate() {
    uint32 address = (static_cast<uint32>(registers.PBR) << 16) | registers.PC;
    registers.PC += isMemory8Bit() ? 1 : 2;
    return address;
}

uint32 CPU65c816::getAddress_Absolute() {
    uint16 offset = fetchWord();
    return (static_cast<uint32>(registers.DBR) << 16) | offset;
}

uint32 CPU65c816::getAddress_AbsoluteX() {
    uint16 offset = fetchWord();
    int16 address = offset + registers.X;
    return (static_cast<uint32>(registers.DBR) << 16) | address;
}

uint32 CPU65c816::getAddress_AbsoluteY() {
    uint16 offset = fetchWord();
    uint16 address = offset + registers.Y;
    return (static_cast<uint32>(registers.DBR) << 16) | address;
}

uint32 CPU65c816::getAddress_Direct() {
    uint8 offset = fetchByte();
    return registers.D + offset;
}

uint32 CPU65c816::getAddress_DirectX() {
    uint8 offset = fetchByte();
    return registers.D + offset + (registers.X & 0xFF);
}

uint32 CPU65c816::getAddress_DirectY() {
    uint8 offset = fetchByte();
    return registers.D + offset + (registers.Y & 0xFF);
}

uint32 CPU65c816::getAddress_Indirect() {
    uint16 pointer = fetchWord();
    return read16(pointer);
}

uint32 CPU65c816::getAddress_IndirectX() {
    uint8 pointer = fetchByte();
    uint16 address = registers.D + pointer + (registers.X & 0xFF);
    return read16(address);
}

uint32 CPU65c816::getAddress_IndirectY() {
    uint8 pointer = fetchByte();
    uint16 baseAddress = read16(registers.D + pointer);
    return baseAddress + registers.Y;
}

void CPU65c816::op_LDA(uint32 address) {
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        registers.A = value;
        updateNZ16(value);
    }
}

void CPU65c816::op_LDX(uint32 address) {
    if (isIndex8Bit()) {
        uint8 value = read8(address);
        registers.X = value;
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        registers.X = value;
        updateNZ16(value);
    }
}

void CPU65c816::op_LDY(uint32 address) {
    if (isIndex8Bit()) {
        uint8 value = read8(address);
        registers.Y = value;
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        registers.Y = value;
        updateNZ16(value);
    }
}

void CPU65c816::op_STA(uint32 address) {
    if (isMemory8Bit()) {
        write8(address, registers.A & 0xFF);
    } else {
        write16(address, registers.A);
    }
}

void CPU65c816::op_STX(uint32 address) {
    if (isIndex8Bit()) {
        write8(address, registers.X & 0xFF);
    } else {
        write16(address, registers.X);
    }
}

void CPU65c816::op_STY(uint32 address) {
    if (isIndex8Bit()) {
        write8(address, registers.Y & 0xFF);
    } else {
        write16(address, registers.Y);
    }
}

void CPU65c816::op_NOP() {
    // DO nothing
}

void CPU65c816::op_TAX() {
    // Transfer A to X
    // The size depends on the X flag (FLAG_INDEX_WIDTH)
    if (isIndex8Bit()) {
        registers.X = registers.A & 0xFF;
        updateNZ8(registers.X & 0xFF);
    } else {
        // 16-bit mode
        registers.X = registers.A;
        updateNZ16(registers.X);
    }
}

void CPU65c816::op_TAY() {
    // Transfer A to Y
    if (isIndex8Bit()) {
        registers.Y = registers.A & 0xFF;
        updateNZ8(registers.Y & 0xFF);
    } else {
        registers.Y = registers.A;
        updateNZ16(registers.Y);
    }
}

void CPU65c816::op_TXA() {
    // Transfer X to A
    if (isMemory8Bit()) {
        registers.A = (registers.A & 0xFF00) | (registers.X & 0xFF);
        updateNZ8(registers.A & 0xFF);
    } else {
        registers.A = registers.X;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_TYA() {
    // Transfer Y to A
    if (isMemory8Bit()) {
        registers.A = (registers.A & 0xFF00) | (registers.Y & 0xFF);
        updateNZ8(registers.A & 0xFF);
    } else {
        registers.A = registers.Y;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_TSX() {
    // Transfer SP to X
    // Always transfers full 16-bit value, but flags depends on X width
    registers.X = registers.SP;
    if (isIndex8Bit()) {
        updateNZ8(registers.X & 0xFF);
    } else {
        updateNZ16(registers.X);
    }
}

void CPU65c816::op_TXS() {
    // Transfer X to SP
    // ALways sets full 16-bit SP
    registers.SP = registers.X;
    // Note: TXS DOES NOT affect flags!
}

void CPU65c816::op_TCD() {
    // Transfer C (16-bit A) to D (Direct page register)
    // Always 16-bit transfer
    registers.D = registers.A;
    updateNZ16(registers.D);
}

void CPU65c816::op_TDC() {
    // Transfer D to C (16-bit A)
    // ALways 16-bit transfer
    registers.A = registers.D;
    updateNZ16(registers.A);
}

void CPU65c816::op_TCS() {
    // Transfer C (16-bit A) to SP
    // Alaways 16-bit transfer
    registers.SP = registers.A;
    // Note: TCS DOES NOT affect flags!
}

void CPU65c816::op_TSC() {
    // Transfer SP to C (16-bit A)
    // Always 16-bit transfer
    registers.A = registers.SP;
    updateNZ16(registers.A);
}
