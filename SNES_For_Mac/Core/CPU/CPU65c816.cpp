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
        case 0xE8: // INX
            op_INX();
            return 2;

        case 0xC8: // INY
            op_INY();
            return 2;

        case 0xCA: // DEX
            op_DEX();
            return 2;

        case 0x88: // DEY
            op_DEY();
            return 2;

        // INC - Increment Memory/Accumulator
        case 0x1A: // INC A
            op_INC_A();
            return 2;

        case 0xE6: // INC Direct Page
            op_INC(getAddress_Direct());
            return isMemory8Bit() ? 5 : 6;

        case 0xF6: // INC Direct Page,X
            op_INC(getAddress_DirectX());
            return isMemory8Bit() ? 6 : 7;

        case 0xEE: // INC Absolute
            op_INC(getAddress_Absolute());
            return isMemory8Bit() ? 6 : 7;

        case 0xFE: // INC Absolute,X
            op_INC(getAddress_AbsoluteX());
            return isMemory8Bit() ? 7 : 8;

        // DEC - Decrement Memory/Accumulator
        case 0x3A: // DEC A
            op_DEC_A();
            return 2;

        case 0xC6: // DEC Direct Page
            op_DEC(getAddress_Direct());
            return isMemory8Bit() ? 5 : 6;

        case 0xD6: // DEC Direct Page,X
            op_DEC(getAddress_DirectX());
            return isMemory8Bit() ? 6 : 7;

        case 0xCE: // DEC Absolute
            op_DEC(getAddress_Absolute());
            return isMemory8Bit() ? 6 : 7;

        case 0xDE: // DEC Absolute,X
            op_DEC(getAddress_AbsoluteX());
            return isMemory8Bit() ? 7 : 8;
        case 0x29: // AND Immediate
            if (isMemory8Bit()) {
                op_AND(getAddress_Immediate());
                return 2;
            } else {
                op_AND(getAddress_Immediate());
                return 3;
            }
        case 0x25: // AND Direct Page
            op_AND(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;

        case 0x35: // AND Direct Page,X
            op_AND(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;

        case 0x2D: // AND Absolute
            op_AND(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;

        case 0x3D: // AND Absolute,X
            op_AND(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;

        case 0x39: // AND Absolute,Y
            op_AND(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;

        case 0x21: // AND (Direct Page,X)
            op_AND(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;

        case 0x31: // AND (Direct Page),Y
            op_AND(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;

        // ORA - Bitwise OR
        case 0x09: // ORA Immediate
            if (isMemory8Bit()) {
                op_ORA(getAddress_Immediate());
                return 2;
            } else {
                op_ORA(getAddress_Immediate());
                return 3;
            }

        case 0x05: // ORA Direct Page
            op_ORA(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;

        case 0x15: // ORA Direct Page,X
            op_ORA(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;

        case 0x0D: // ORA Absolute
            op_ORA(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;

        case 0x1D: // ORA Absolute,X
            op_ORA(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;

        case 0x19: // ORA Absolute,Y
            op_ORA(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;

        case 0x01: // ORA (Direct Page,X)
            op_ORA(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;

        case 0x11: // ORA (Direct Page),Y
            op_ORA(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;

        // EOR - Bitwise Exclusive OR
        case 0x49: // EOR Immediate
            if (isMemory8Bit()) {
                op_EOR(getAddress_Immediate());
                return 2;
            } else {
                op_EOR(getAddress_Immediate());
                return 3;
            }

        case 0x45: // EOR Direct Page
            op_EOR(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;

        case 0x55: // EOR Direct Page,X
            op_EOR(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;

        case 0x4D: // EOR Absolute
            op_EOR(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;

        case 0x5D: // EOR Absolute,X
            op_EOR(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;

        case 0x59: // EOR Absolute,Y
            op_EOR(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;

        case 0x41: // EOR (Direct Page,X)
            op_EOR(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;

        case 0x51: // EOR (Direct Page),Y
            op_EOR(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;
        // CMP - Compare Accumulator
        case 0xC9: // CMP Immediate
            if (isMemory8Bit()) {
                op_CMP(getAddress_Immediate());
                return 2;
            } else {
                op_CMP(getAddress_Immediate());
                return 3;
            }
        case 0xC5: // CMP Direct Page
            op_CMP(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;
        case 0xD5: // CMP Direct Page,X
            op_CMP(getAddress_DirectX());
            return isMemory8Bit() ? 4 : 5;
        case 0xCD: // CMP Absolute
            op_CMP(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;
        case 0xDD: // CMP Absolute,X
            op_CMP(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;
        case 0xD9: // CMP Absolute,Y
            op_CMP(getAddress_AbsoluteY());
            return isMemory8Bit() ? 4 : 5;
        case 0xC1: // CMP (Direct Page,X)
            op_CMP(getAddress_IndirectX());
            return isMemory8Bit() ? 6 : 7;
        case 0xD1: // CMP (Direct Page),Y
            op_CMP(getAddress_IndirectY());
            return isMemory8Bit() ? 5 : 6;
        // CPX - Compare X Register
        case 0xE0: // CPX Immediate
            if (isIndex8Bit()) {
                op_CPX(getAddress_Immediate());
                return 2;
            } else {
                op_CPX(getAddress_Immediate());
                return 3;
            }
        case 0xE4: // CPX Direct Page
            op_CPX(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;
        case 0xEC: // CPX Absolute
            op_CPX(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;

        // CPY - Compare Y Register
        case 0xC0: // CPY Immediate
            if (isIndex8Bit()) {
                op_CPY(getAddress_Immediate());
                return 2;
            } else {
                op_CPY(getAddress_Immediate());
                return 3;
            }
        case 0xC4: // CPY Direct Page
            op_CPY(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;
        case 0xCC: // CPY Absolute
            op_CPY(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;
        case 0xF0: // BEQ - Branch if Equal
            op_BEQ();
            return 2;  // Simplified: 2 cycles (add +1 if taken in real implementation)

        case 0xD0: // BNE - Branch if Not Equal
            op_BNE();
            return 2;

        case 0xB0: // BCS - Branch if Carry Set
            op_BCS();
            return 2;

        case 0x90: // BCC - Branch if Carry Clear
            op_BCC();
            return 2;

        case 0x30: // BMI - Branch if Minus
            op_BMI();
            return 2;

        case 0x10: // BPL - Branch if Plus
            op_BPL();
            return 2;

        case 0x70: // BVS - Branch if Overflow Set
            op_BVS();
            return 2;

        case 0x50: // BVC - Branch if Overflow Clear
            op_BVC();
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

void CPU65c816::op_INX() {
    // Increment X
    if (isIndex8Bit()) {
        registers.X = (registers.X + 1) & 0xFF;
        updateNZ8(registers.X & 0xFF);
    } else {
        registers.X = (registers.X + 1) & 0xFFFF;
        updateNZ16(registers.X);
    }
}

void CPU65c816::op_INY() {
    // Increment Y
    if (isIndex8Bit()) {
        registers.Y = (registers.Y + 1) & 0xFF;
        updateNZ8(registers.Y & 0xFF);
    } else {
        registers.Y = (registers.Y + 1) & 0xFFFF;
        updateNZ16(registers.Y);
    }
}

void CPU65c816::op_DEX() {
    // Decrement X
    if (isIndex8Bit()) {
        registers.X = (registers.X - 1) & 0xFF;
        updateNZ8(registers.X & 0xFF);
    } else {
        registers.X = (registers.X - 1) & 0xFFFF;
        updateNZ16(registers.X);
    }
}

void CPU65c816::op_DEY() {
    // Decrement Y
    if (isIndex8Bit()) {
        registers.Y = (registers.Y - 1) & 0xFF;
        updateNZ8(registers.Y & 0xFF);
    } else {
        registers.Y = (registers.Y - 1) & 0xFFFF;
        updateNZ16(registers.Y);
    }
}

void CPU65c816::op_INC_A() {
    // Increment Accumulator
    if (isMemory8Bit()) {
        uint8 value = (registers.A & 0xFF) + 1;
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        registers.A = (registers.A + 1) & 0xFFFF;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_DEC_A() {
    // Decrement Accumulator
    if (isMemory8Bit()) {
        uint8 value = (registers.A & 0xFF) - 1;
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        registers.A = (registers.A - 1) & 0xFFFF;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_INC(uint32 address) {
    // Increment Memory
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        value = (value + 1) & 0xFF;
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        value = (value + 1) & 0xFFFF;
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_DEC(uint32 address) {
    // Decrement Memory
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        value = (value - 1) & 0xFF;
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        value = (value - 1) & 0xFFFF;
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_AND(uint32 address) {
    // Bitwise AND with Accumulator
    if (isMemory8Bit()) {
        uint8 operand = read8(address);
        uint8 result = (registers.A & 0xFF) & operand;
        registers.A = (registers.A & 0xFF00) | result;
        updateNZ8(result);
    } else {
        uint16 operand = read16(address);
        registers.A = registers.A & operand;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_ORA(uint32 address) {
    // Bitwise OR with Accumulator
    if (isMemory8Bit()) {
        uint8 operand = read8(address);
        uint8 result = (registers.A & 0xFF) | operand;
        registers.A = (registers.A & 0xFF00) | result;
        updateNZ8(result);
    } else {
        uint16 operand = read16(address);
        registers.A = registers.A | operand;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_EOR(uint32 address) {
    // Bitwise Exclusive OR with Accumulator
    if (isMemory8Bit()) {
        uint8 operand = read8(address);
        uint8 result = (registers.A & 0xFF) ^ operand;
        registers.A = (registers.A & 0xFF00) | result;
        updateNZ8(result);
    } else {
        uint16 operand = read16(address);
        registers.A = registers.A ^ operand;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_CMP(uint32 address) {
    // Compare Accumulator
    // Performs: A - operand (but doesn't store result)
    // Updates: N, Z, C flags
    
    if (isMemory8Bit()) {
        uint8 operand = read8(address);
        uint8 a = registers.A & 0xFF;
        
        // Perform subtraction (but don't store)
        uint16 result = a - operand;
        
        // Set flags
        setFlag(FLAG_CARRY, result < 0x100);     // C=1 if no borrow (a >= operand)
        setFlag(FLAG_ZERO, (result & 0xFF) == 0); // Z=1 if equal
        setFlag(FLAG_NEGATIVE, (result & 0x80) != 0); // N=1 if bit 7 set
    } else {
        uint16 operand = read16(address);
        uint16 a = registers.A;
        
        // Perform subtraction (but don't store)
        uint32 result = a - operand;
        
        // Set flags
        setFlag(FLAG_CARRY, result < 0x10000);      // C=1 if no borrow
        setFlag(FLAG_ZERO, (result & 0xFFFF) == 0); // Z=1 if equal
        setFlag(FLAG_NEGATIVE, (result & 0x8000) != 0); // N=1 if bit 15 set
    }
}

void CPU65c816::op_CPX(uint32 address) {
    // Compare X Register
    // Performs: X - operand (but doesn't store result)
    
    if (isIndex8Bit()) {
        uint8 operand = read8(address);
        uint8 x = registers.X & 0xFF;
        
        uint16 result = x - operand;
        
        setFlag(FLAG_CARRY, result < 0x100);
        setFlag(FLAG_ZERO, (result & 0xFF) == 0);
        setFlag(FLAG_NEGATIVE, (result & 0x80) != 0);
    } else {
        uint16 operand = read16(address);
        uint16 x = registers.X;
        
        uint32 result = x - operand;
        
        setFlag(FLAG_CARRY, result < 0x10000);
        setFlag(FLAG_ZERO, (result & 0xFFFF) == 0);
        setFlag(FLAG_NEGATIVE, (result & 0x8000) != 0);
    }
}

void CPU65c816::op_CPY(uint32 address) {
    // Compare Y Register
    // Performs: Y - operand (but doesn't store result)
    
    if (isIndex8Bit()) {
        uint8 operand = read8(address);
        uint8 y = registers.Y & 0xFF;
        
        uint16 result = y - operand;
        
        setFlag(FLAG_CARRY, result < 0x100);
        setFlag(FLAG_ZERO, (result & 0xFF) == 0);
        setFlag(FLAG_NEGATIVE, (result & 0x80) != 0);
    } else {
        uint16 operand = read16(address);
        uint16 y = registers.Y;
        
        uint32 result = y - operand;
        
        setFlag(FLAG_CARRY, result < 0x10000);
        setFlag(FLAG_ZERO, (result & 0xFFFF) == 0);
        setFlag(FLAG_NEGATIVE, (result & 0x8000) != 0);
    }
}

void CPU65c816::branch(bool condition) {
    // Read the signed offset
    int8 offset = static_cast<int8>(fetchByte());
    
    if (condition) {
        // Branch taken - add offset to PC
        // Note: PC already points to next instruction after fetchByte()
        registers.PC = static_cast<uint16>(registers.PC + offset);
        
        // TODO: Add cycle penalties for page crossing if needed
        // For now, we'll just use base cycle counts in decodeAndExecute
    }
    // If condition is false, PC already points to next instruction, so do nothing
}

void CPU65c816::op_BEQ() {
    // Branch if Equal (Z = 1)
    branch(getFlag(FLAG_ZERO));
}

void CPU65c816::op_BNE() {
    // Branch if Not Equal (Z = 0)
    branch(!getFlag(FLAG_ZERO));
}

void CPU65c816::op_BCS() {
    // Branch if Carry Set (C = 1)
    branch(getFlag(FLAG_CARRY));
}

void CPU65c816::op_BCC() {
    // Branch if Carry Clear (C = 0)
    branch(!getFlag(FLAG_CARRY));
}

void CPU65c816::op_BMI() {
    // Branch if Minus (N = 1)
    branch(getFlag(FLAG_NEGATIVE));
}

void CPU65c816::op_BPL() {
    // Branch if Plus (N = 0)
    branch(!getFlag(FLAG_NEGATIVE));
}

void CPU65c816::op_BVS() {
    // Branch if Overflow Set (V = 1)
    branch(getFlag(FLAG_OVERFLOW));
}

void CPU65c816::op_BVC() {
    // Branch if Overflow Clear (V = 0)
    branch(!getFlag(FLAG_OVERFLOW));
}
