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
        case 0x48:              // PHA
            op_PHA();
            return isMemory8Bit() ? 3 : 4;
        case 0xDA:              // PHX
            op_PHX();
            return isIndex8Bit() ? 3 : 4;
        case 0x5A:              // PHY
            op_PHY();
            return isIndex8Bit() ? 3 : 4;
        case 0x08:              // PHP
            op_PHP();
            return 3;
        case 0x0B:
            op_PHD();
            return 4;
        case 0x8B:
            op_PHB();
            return 3;
        case 0x4B:
            op_PHK();
            return 3;
        case 0x68:
            op_PLA();
            return isMemory8Bit() ? 4 : 5;
        case 0xFA:
            op_PLX();
            return isIndex8Bit() ? 4 : 5;
        case 0x7A:
            op_PLY();
            return isIndex8Bit() ? 4 : 5;
        case 0x28:
            op_PLP();
            return 4;
        case 0x2B:
            op_PLD();
            return 5;
        case 0xAB:
            op_PLB();
            return 4;
        case 0x69:
            if (isMemory8Bit()) {
                op_ADC(getAddress_Immediate());
                return 2;
            } else {
                op_ADC(getAddress_Immediate());
                return 3;
            }
        case 0x65:
            op_ADC(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;
        case 0x75:
            op_ADC(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;
        case 0x6D:
            op_ADC(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;
        case 0x7D:
            op_ADC(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;
        case 0x79:
            op_ADC(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;
        case 0x61:
            op_ADC(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;
        case 0x71:
            op_ADC(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;
        // SBC - subtract with Carry
        case 0xE9:
            if (isMemory8Bit()) {
                op_SBC(getAddress_Immediate());
                return 2;
            } else {
                op_SBC(getAddress_Immediate());
                return 3;
            }
        case 0xE5:
            op_SBC(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;
        case 0xF5:
            op_SBC(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;
        case 0xED:
            op_SBC(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;
        case 0xFD:
            op_SBC(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;
        case 0xF9:
            op_SBC(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;
        case 0xE1:
            op_SBC(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;
        case 0xF1:
            op_SBC(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;
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

void CPU65c816::op_PHA() {
    // Push Accumulator
    if (isMemory8Bit()) {
        push8(registers.A & 0xFF);
    } else {
        push16(registers.A);
    }
    // Note: PHA DOES NOT affect flags!
}

void CPU65c816::op_PHX() {
    // Push X
    if (isIndex8Bit()) {
        push8(registers.X & 0xFF);
    } else {
        push16(registers.X);
    }
    // Note: PHX DOES NOT affect flags!
}

void CPU65c816::op_PHY() {
    // Push Y
    if (isIndex8Bit()) {
        push8(registers.Y & 0xFF);
    } else {
        push16(registers.Y);
    }
    // Note: PHY DOES NOT affect flags!
}

void CPU65c816::op_PHP() {
    push8(registers.P);
    // Note: PHP DOES NOT affect flags!
}

void CPU65c816::op_PHD() {
    // Push Direct Page (always 16-bit)
    push16(registers.D);
    // Note: PHD DOES NOT affect flags!
}

void CPU65c816::op_PHB() {
    // Push Data Bank
    push8(registers.DBR);
    // Note: PHB DOES NOT affect flags!
}

void CPU65c816::op_PHK() {
    // Push Program Bank
    push8(registers.PBR);
    // Note: PHK DOES NOT affect flags!
}

void CPU65c816::op_PLA() {
    // Pull Accumulator
    if (isMemory8Bit()) {
        uint8 value = pull8();
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        registers.A = pull16();
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_PLX() {
    // Pull X
    if (isIndex8Bit()) {
        registers.X = pull8();
        updateNZ8(registers.X & 0xFF);
    } else {
        registers.X = pull16();
        updateNZ16(registers.X);
    }
}

void CPU65c816::op_PLY() {
    // Pull Y
    if (isIndex8Bit()) {
        registers.Y = pull8();
        updateNZ8(registers.Y & 0xFF);
    } else {
        registers.Y = pull16();
        updateNZ16(registers.Y);
    }
}

void CPU65c816::op_PLP() {
    // Pull Processor Status
    registers.P = pull8();
    // Note: PLP affects ALL flags (it restores the entire P register!)
    // in emulation mode, bit 5(M flag) and bit 4(X flag) are forced to 1
    if (registers.E) {
        registers.P |= 0x30;  // Force M and X to 1 in emulation mode
    }
}

void CPU65c816::op_PLD() {
    // Pull Direct Page (always 16-bit)
    registers.D = pull16();
    updateNZ16(registers.D);
}

void CPU65c816::op_PLB() {
    // Pull Data Bank
    registers.DBR = pull8();
    updateNZ8(registers.DBR);
}

bool CPU65c816::checkOverflow8(uint8 a, uint b, uint8 result) {
    // Overflow occurs when:
    // - Adding 2 positive numbers gives negative result
    // - Adding 2 negative numbers gives positive result
    // Check if sign bits of operands are same, but different from result
    return ((a ^ result) & (b ^ result) & 0x80) != 0;
}

bool CPU65c816::checkOverflow16(uint16 a, uint16 b, uint result) {
    return ((a ^ result) & (b ^ result) & 0x8000) != 0;
}

void CPU65c816::op_ADC(uint32 address) {
    if (isMemory8Bit()) {
        // 8-bit operation
        uint8 operand = read8(address);
        uint8 a = registers.A & 0xFF;
        uint8 carry = getFlag(FLAG_CARRY) ? 1 : 0;
        
        if (getFlag(FLAG_DECIMAL)) {
            // BCD mode (optional - can implement later)
            // For now just do binary
            uint16 result = a + operand + carry;
            // Set carry if result > 0xFF
            setFlag(FLAG_CARRY, result > 0xFF);
            uint8 result8 = result & 0xFF;
            // Set overflow
            setFlag(FLAG_OVERFLOW, checkOverflow8(a, operand, result8));
            // Update A (preserve high byte)
            registers.A = (registers.A & 0xFF00) | result8;
            // Update N and Z
            updateNZ8(result8);
        } else {
            // Binary mode
            uint16 result = a + operand + carry;
            // Set carry if result > 0xFF
            setFlag(FLAG_CARRY, result > 0xFF);
            uint8 result8 = result & 0xFF;
            // Set overflow
            setFlag(FLAG_OVERFLOW, checkOverflow8(a, operand, result8));
            // Update A (preserve high byte)
            registers.A = (registers.A & 0xFF00) | result8;
            // Update N and Z
            updateNZ8(result8);
        }
    } else {
        // 16-bit operation
        uint16 operand = read16(address);
        uint16 a = registers.A;
        uint16 carry = getFlag(FLAG_CARRY) ? 1 : 0;
        
        if (getFlag(FLAG_DECIMAL)) {
            // BCD mode (optional - cam implement later)
            uint32 result = a + operand + carry;
            setFlag(FLAG_CARRY, result > 0xFFFF);
            uint16 result16 = result & 0xFFFF;
            setFlag(FLAG_OVERFLOW, checkOverflow16(a, operand, result16));
            registers.A = result16;
            updateNZ16(result16);
        } else {
            // Binary mode
            uint32 result = a + operand + carry;
            setFlag(FLAG_CARRY, result > 0xFFFF);
            uint16 result16 = result & 0xFFFF;
            setFlag(FLAG_OVERFLOW, checkOverflow16(a, operand, result16));
            registers.A = result16;
            updateNZ16(result16);
        }
    }
}

void CPU65c816::op_SBC(uint32 address) {
    if (isMemory8Bit()) {
        // 8-bit operation
        uint8 operand = read8(address);
        uint8 a = registers.A & 0xFF;
        uint8 carry = getFlag(FLAG_CARRY) ? 1 : 0;
        
        if (getFlag(FLAG_DECIMAL)) {
            // BCD mode (optional - can implement later)
            // For SBC: A = A - operand - (1 - carry)
            // Which is: A = A - operand - !carry
            uint16 result = a - operand - (1 - carry);
            // Carry is SET if NO borrow occurred (result >= 0)
            setFlag(FLAG_CARRY, (result & 0x100) == 0);
            
            uint8 result8 = result & 0xFF;
            
            // Overflow for subtraction
            // V = (A ^ operand) & (A ^ result) & 0x80
            setFlag(FLAG_OVERFLOW, ((a ^ operand) & (a ^ result) & 0x80) != 0);
            
            registers.A = (registers.A & 0xFF00) | result8;
            updateNZ8(result8);
        } else {
            // Binary mode
            // SBC: A = A - operand - (1 - carry)
            uint16 result = a - operand - (1 - carry);
            setFlag(FLAG_CARRY, (result & 0x100) == 0);
            
            uint8 result8 = result & 0xFF;
            
            // Overflow for subtraction
            setFlag(FLAG_OVERFLOW, ((a ^ operand) & (a ^ result8) & 0x80) != 0);
            registers.A = (registers.A & 0xFF00) | result8;
            updateNZ8(result8);
        }
    } else {
        // 16-bit operation
        uint16 operand = read16(address);
        uint16 a = registers.A;
        uint16 carry = getFlag(FLAG_CARRY) ? 1 : 0;
        
        if (getFlag(FLAG_DECIMAL)) {
            // BCD mode (optional)
            uint32 result = a - operand - (1 - carry);
            setFlag(FLAG_CARRY, (result & 0x10000) == 0);
            uint16 result16 = result & 0xFFFF;
            setFlag(FLAG_OVERFLOW, ((a ^ operand) & (a ^ result16) & 0x8000) != 0);
            registers.A = result16;
            updateNZ16(result16);
        } else {
            // Binary mode
            uint32 result = a - operand - (1 - carry);
            setFlag(FLAG_CARRY, (result & 0x10000) == 0);
            uint16 result16 = result & 0xFFFF;
            setFlag(FLAG_OVERFLOW, ((a ^ operand) & (a ^ result16) & 0x8000) != 0);
            registers.A = result16;
            updateNZ16(result16);
        }
    }
}
