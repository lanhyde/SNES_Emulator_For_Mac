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
        case 0xA2: // LDX Immediate
            if (isIndex8Bit()) {
                op_LDX(getAddress_Immediate());
                return 2;
            } else {
                op_LDX(getAddress_Immediate());
                return 3;
            }
        case 0xAD: // LDA Absolute
            op_LDA(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;
        case 0xBD: // LDA Absolute,X
            op_LDA(getAddress_AbsoluteX());
            return isMemory8Bit() ? 4 : 5;

        case 0x9D: // STA Absolute,X
            op_STA(getAddress_AbsoluteX());
            return isMemory8Bit() ? 5 : 6;
        case 0xA6: // LDX Direct Page
            op_LDX(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;

        case 0xB6: // LDX Direct Page,Y
            op_LDX(getAddress_DirectY());
            return isIndex8Bit() ? 4 : 5;

        case 0xAE: // LDX Absolute
            op_LDX(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;

        case 0xBE: // LDX Absolute,Y
            op_LDX(getAddress_AbsoluteY());
            return isIndex8Bit() ? 4 : 5;

        // LDY - Load Y Register
        case 0xA0: // LDY Immediate
            if (isIndex8Bit()) {
                op_LDY(getAddress_Immediate());
                return 2;
            } else {
                op_LDY(getAddress_Immediate());
                return 3;
            }

        case 0xA4: // LDY Direct Page
            op_LDY(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;

        case 0xB4: // LDY Direct Page,X
            op_LDY(getAddress_DirectX());
            return isIndex8Bit() ? 4 : 5;

        case 0xAC: // LDY Absolute
            op_LDY(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;

        case 0xBC: // LDY Absolute,X
            op_LDY(getAddress_AbsoluteX());
            return isIndex8Bit() ? 4 : 5;

        // STX - Store X Register
        case 0x86: // STX Direct Page
            op_STX(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;

        case 0x96: // STX Direct Page,Y
            op_STX(getAddress_DirectY());
            return isIndex8Bit() ? 4 : 5;

        case 0x8E: // STX Absolute
            op_STX(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;

        // STY - Store Y Register
        case 0x84: // STY Direct Page
            op_STY(getAddress_Direct());
            return isIndex8Bit() ? 3 : 4;

        case 0x94: // STY Direct Page,X
            op_STY(getAddress_DirectX());
            return isIndex8Bit() ? 4 : 5;

        case 0x8C: // STY Absolute
            op_STY(getAddress_Absolute());
            return isIndex8Bit() ? 4 : 5;
        case 0x8D: // STA Absolute ← CRITICAL FOR YOUR TESTS!
            op_STA(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;

        case 0x85: // STA Direct Page
            op_STA(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;

        case 0x99: // STA Absolute,Y
            op_STA(getAddress_AbsoluteY());
            return isMemory8Bit() ? 5 : 6;
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
        case 0x4C: // JMP Absolute
            {
                uint16 address = fetchWord();
                uint32 fullAddress = (static_cast<uint32>(registers.PBR) << 16) | address;
                op_JMP(fullAddress);
                return 3;
            }

        case 0x6C: // JMP (Absolute Indirect)
            {
                uint16 pointer = fetchWord();
                uint32 pointerAddress = (static_cast<uint32>(registers.DBR) << 16) | pointer;
                uint16 target = read16(pointerAddress);
                uint32 fullAddress = (static_cast<uint32>(registers.PBR) << 16) | target;
                op_JMP(fullAddress);
                return 5;
            }

        case 0x7C: // JMP (Absolute Indexed Indirect,X)
            {
                uint16 pointer = fetchWord();
                uint16 indexedPointer = pointer + registers.X;
                uint32 pointerAddress = (static_cast<uint32>(registers.PBR) << 16) | indexedPointer;
                uint16 target = read16(pointerAddress);
                uint32 fullAddress = (static_cast<uint32>(registers.PBR) << 16) | target;
                op_JMP(fullAddress);
                return 6;
            }

        // JSR - Jump to Subroutine
        case 0x20: // JSR Absolute
            {
                uint16 address = fetchWord();
                uint32 fullAddress = (static_cast<uint32>(registers.PBR) << 16) | address;
                op_JSR(fullAddress);
                return 6;
            }

        // RTS - Return from Subroutine
        case 0x60: // RTS
            op_RTS();
            return 6;

        // RTI - Return from Interrupt
        case 0x40: // RTI
            op_RTI();
            return 6;
        case 0x24:                  // BIT Direct Page
            op_BIT(getAddress_Direct());
            return isMemory8Bit() ? 3 : 4;
        case 0x2C:                  // BIT Absolute
            op_BIT(getAddress_Absolute());
            return isMemory8Bit() ? 4 : 5;
        case 0x34: // BIT Direct Page,X
             op_BIT(getAddress_DirectX());
             return isMemory8Bit() ? 4 : 5;
             
         case 0x3C: // BIT Absolute,X
             op_BIT(getAddress_AbsoluteX());
             return isMemory8Bit() ? 4 : 5;
             
         case 0x89: // BIT Immediate
             if (isMemory8Bit()) {
                 op_BIT(getAddress_Immediate());
                 return 2;
             } else {
                 op_BIT(getAddress_Immediate());
                 return 3;
             }
         
         // ASL - Arithmetic Shift Left
         case 0x0A: // ASL A
             op_ASL_A();
             return 2;
             
         case 0x06: // ASL Direct Page
             op_ASL(getAddress_Direct());
             return isMemory8Bit() ? 5 : 6;
             
         case 0x16: // ASL Direct Page,X
             op_ASL(getAddress_DirectX());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x0E: // ASL Absolute
             op_ASL(getAddress_Absolute());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x1E: // ASL Absolute,X
             op_ASL(getAddress_AbsoluteX());
             return isMemory8Bit() ? 7 : 8;
         
         // LSR - Logical Shift Right
         case 0x4A: // LSR A
             op_LSR_A();
             return 2;
             
         case 0x46: // LSR Direct Page
             op_LSR(getAddress_Direct());
             return isMemory8Bit() ? 5 : 6;
             
         case 0x56: // LSR Direct Page,X
             op_LSR(getAddress_DirectX());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x4E: // LSR Absolute
             op_LSR(getAddress_Absolute());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x5E: // LSR Absolute,X
             op_LSR(getAddress_AbsoluteX());
             return isMemory8Bit() ? 7 : 8;
         
         // ROL - Rotate Left
         case 0x2A: // ROL A
             op_ROL_A();
             return 2;
             
         case 0x26: // ROL Direct Page
             op_ROL(getAddress_Direct());
             return isMemory8Bit() ? 5 : 6;
             
         case 0x36: // ROL Direct Page,X
             op_ROL(getAddress_DirectX());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x2E: // ROL Absolute
             op_ROL(getAddress_Absolute());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x3E: // ROL Absolute,X
             op_ROL(getAddress_AbsoluteX());
             return isMemory8Bit() ? 7 : 8;
         
         // ROR - Rotate Right
         case 0x6A: // ROR A
             op_ROR_A();
             return 2;
             
         case 0x66: // ROR Direct Page
             op_ROR(getAddress_Direct());
             return isMemory8Bit() ? 5 : 6;
             
         case 0x76: // ROR Direct Page,X
             op_ROR(getAddress_DirectX());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x6E: // ROR Absolute
             op_ROR(getAddress_Absolute());
             return isMemory8Bit() ? 6 : 7;
             
         case 0x7E: // ROR Absolute,X
             op_ROR(getAddress_AbsoluteX());
             return isMemory8Bit() ? 7 : 8;
         // Flag Manipulation
         case 0x18: // CLC - Clear Carry
             op_CLC();
             return 2;
             
         case 0x38: // SEC - Set Carry
             op_SEC();
             return 2;
             
         case 0x58: // CLI - Clear Interrupt Disable
             op_CLI();
             return 2;
             
         case 0x78: // SEI - Set Interrupt Disable
             op_SEI();
             return 2;
             
         case 0xB8: // CLV - Clear Overflow
             op_CLV();
             return 2;
             
         case 0xD8: // CLD - Clear Decimal
             op_CLD();
             return 2;
             
         case 0xF8: // SED - Set Decimal
             op_SED();
             return 2;
             
         case 0xC2: // REP - Reset Processor Status Bits
             op_REP();
             return 3;
             
         case 0xE2: // SEP - Set Processor Status Bits
             op_SEP();
             return 3;
             
         case 0xFB: // XCE - Exchange Carry and Emulation
             op_XCE();
             return 2;
            // Test and Set/Reset Bit instructions
        case 0x04: // TSB Direct Page
            op_TSB(getAddress_Direct());
            return isMemory8Bit() ? 5 : 6;
            
        case 0x0C: // TSB Absolute
            op_TSB(getAddress_Absolute());
            return isMemory8Bit() ? 6 : 7;
            
        case 0x14: // TRB Direct Page
            op_TRB(getAddress_Direct());
            return isMemory8Bit() ? 5 : 6;
            
        case 0x1C: // TRB Absolute
            op_TRB(getAddress_Absolute());
            return isMemory8Bit() ? 6 : 7;
            // Block move instructions
        case 0x44: // MVP - Block Move Previous
            op_MVP();
            return 7;  // 7 cycles per byte
            
        case 0x54: // MVN - Block Move Next
            op_MVN();
            return 7;  // 7 cycles per byte
            // Interrupt instructions
        case 0x00: // BRK - Software Break
            op_BRK();
            return registers.E ? 7 : 8;
                    
        case 0x02: // COP - Coprocessor
            op_COP();
            return registers.E ? 7 : 8;
                    
        case 0x42: // WDM - Reserved
            op_WDM();
            return 2;
                    
        case 0xDB: // STP - Stop Processor
            op_STP();
            return 3;
                    
        case 0xCB: // WAI - Wait for Interrupt
            op_WAI();
            return 3;
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

void CPU65c816::op_JMP(uint32 address) {
    // Simple unconditional jump
    // For absolute: address is already the full 24-bit address
    // Just set PC to the low 16 bits, PBR to high 8 bits
    
    registers.PC = address & 0xFFFF;
    registers.PBR = (address >> 16) & 0xFF;
}

void CPU65c816::op_JSR(uint32 address) {
    // Jump to Subroutine
    // 1. Push return address (PC - 1) to stack
    // 2. Set PC to target address
    
    // PC currently points to the next instruction
    // JSR pushes (PC - 1), which points to the last byte of the JSR instruction
    uint16 returnAddress = registers.PC - 1;
    
    // Push high byte first, then low byte (stack grows down)
    push16(returnAddress);
    
    // Jump to subroutine
    registers.PC = address & 0xFFFF;
    // Note: JSR doesn't change PBR, stays in same bank
}

void CPU65c816::op_RTS() {
    // Return from Subroutine
    // 1. Pull return address from stack
    // 2. Add 1 to it (JSR pushed PC-1)
    // 3. Set PC to that address
    
    uint16 returnAddress = pull16();
    registers.PC = returnAddress + 1;
    
    // Note: RTS doesn't affect PBR
}

void CPU65c816::op_RTI() {
    // Return from Interrupt
    // 1. Pull processor status (P) from stack
    // 2. Pull PC from stack
    // 3. In native mode, also pull PBR
    
    // Pull P register
    registers.P = pull8();
    
    if (registers.E) {
        // Emulation mode: force M and X flags
        registers.P |= 0x30;
    }
    
    // Pull PC (16-bit)
    registers.PC = pull16();
    
    // In native mode (E=0), also pull PBR
    if (!registers.E) {
        registers.PBR = pull8();
    }
}

void CPU65c816::op_BIT(uint32 address) {
    // BIT - Test Bits in Memory with Accumulator
    // Performs AND operation but doesn't store result
    // Sets Z flag based on result
    // Copies bit 7 to N flag and bit 6 to V flag (from memory operand)
    
    if (isMemory8Bit()) {
        uint8 operand = read8(address);
        uint8 result = (registers.A & 0xFF) & operand;
        
        setFlag(FLAG_ZERO, result == 0);
        setFlag(FLAG_NEGATIVE, (operand & 0x80) != 0);  // Copy bit 7 from memory
        setFlag(FLAG_OVERFLOW, (operand & 0x40) != 0);  // Copy bit 6 from memory
    } else {
        uint16 operand = read16(address);
        uint16 result = registers.A & operand;
        
        setFlag(FLAG_ZERO, result == 0);
        setFlag(FLAG_NEGATIVE, (operand & 0x8000) != 0);  // Copy bit 15
        setFlag(FLAG_OVERFLOW, (operand & 0x4000) != 0);  // Copy bit 14
    }
}

void CPU65c816::op_ASL(uint32 address) {
    // ASL - Arithmetic Shift Left (memory)
    // Shifts all bits left one position. 0 is shifted into bit 0 and bit 7/15 goes to carry
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        
        // Bit 7 goes to carry
        setFlag(FLAG_CARRY, (value & 0x80) != 0);
        
        // Shift left
        value = (value << 1) & 0xFF;
        
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        
        // Bit 15 goes to carry
        setFlag(FLAG_CARRY, (value & 0x8000) != 0);
        
        // Shift left
        value = (value << 1) & 0xFFFF;
        
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_ASL_A() {
    // ASL - Arithmetic Shift Left (accumulator)
    
    if (isMemory8Bit()) {
        uint8 value = registers.A & 0xFF;
        
        setFlag(FLAG_CARRY, (value & 0x80) != 0);
        value = (value << 1) & 0xFF;
        
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        setFlag(FLAG_CARRY, (registers.A & 0x8000) != 0);
        registers.A = (registers.A << 1) & 0xFFFF;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_LSR(uint32 address) {
    // LSR - Logical Shift Right (memory)
    // Shifts all bits right one position. 0 is shifted into bit 7/15 and bit 0 goes to carry
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        
        // Bit 0 goes to carry
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        
        // Shift right
        value = value >> 1;
        
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        
        // Bit 0 goes to carry
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        
        // Shift right
        value = value >> 1;
        
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_LSR_A() {
    // LSR - Logical Shift Right (accumulator)
    
    if (isMemory8Bit()) {
        uint8 value = registers.A & 0xFF;
        
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        value = value >> 1;
        
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        setFlag(FLAG_CARRY, (registers.A & 0x01) != 0);
        registers.A = registers.A >> 1;
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_ROL(uint32 address) {
    // ROL - Rotate Left (memory)
    // Shifts all bits left one position. Carry goes into bit 0 and bit 7/15 goes to carry
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        bool oldCarry = getFlag(FLAG_CARRY);
        
        // Bit 7 goes to carry
        setFlag(FLAG_CARRY, (value & 0x80) != 0);
        
        // Shift left and insert old carry at bit 0
        value = ((value << 1) & 0xFF) | (oldCarry ? 1 : 0);
        
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        bool oldCarry = getFlag(FLAG_CARRY);
        
        // Bit 15 goes to carry
        setFlag(FLAG_CARRY, (value & 0x8000) != 0);
        
        // Shift left and insert old carry at bit 0
        value = ((value << 1) & 0xFFFF) | (oldCarry ? 1 : 0);
        
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_ROL_A() {
    // ROL - Rotate Left (accumulator)
    
    if (isMemory8Bit()) {
        uint8 value = registers.A & 0xFF;
        bool oldCarry = getFlag(FLAG_CARRY);
        
        setFlag(FLAG_CARRY, (value & 0x80) != 0);
        value = ((value << 1) & 0xFF) | (oldCarry ? 1 : 0);
        
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        bool oldCarry = getFlag(FLAG_CARRY);
        
        setFlag(FLAG_CARRY, (registers.A & 0x8000) != 0);
        registers.A = ((registers.A << 1) & 0xFFFF) | (oldCarry ? 1 : 0);
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_ROR(uint32 address) {
    // ROR - Rotate Right (memory)
    // Shifts all bits right one position. Carry goes into bit 7/15 and bit 0 goes to carry
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        bool oldCarry = getFlag(FLAG_CARRY);
        
        // Bit 0 goes to carry
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        
        // Shift right and insert old carry at bit 7
        value = (value >> 1) | (oldCarry ? 0x80 : 0);
        
        write8(address, value);
        updateNZ8(value);
    } else {
        uint16 value = read16(address);
        bool oldCarry = getFlag(FLAG_CARRY);
        
        // Bit 0 goes to carry
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        
        // Shift right and insert old carry at bit 15
        value = (value >> 1) | (oldCarry ? 0x8000 : 0);
        
        write16(address, value);
        updateNZ16(value);
    }
}

void CPU65c816::op_ROR_A() {
    // ROR - Rotate Right (accumulator)
    
    if (isMemory8Bit()) {
        uint8 value = registers.A & 0xFF;
        bool oldCarry = getFlag(FLAG_CARRY);
        
        setFlag(FLAG_CARRY, (value & 0x01) != 0);
        value = (value >> 1) | (oldCarry ? 0x80 : 0);
        
        registers.A = (registers.A & 0xFF00) | value;
        updateNZ8(value);
    } else {
        bool oldCarry = getFlag(FLAG_CARRY);
        
        setFlag(FLAG_CARRY, (registers.A & 0x01) != 0);
        registers.A = (registers.A >> 1) | (oldCarry ? 0x8000 : 0);
        updateNZ16(registers.A);
    }
}

void CPU65c816::op_CLC() {
    // CLC - Clear Carry Flag
    setFlag(FLAG_CARRY, false);
}

void CPU65c816::op_SEC() {
    // SEC - Set Carry Flag
    setFlag(FLAG_CARRY, true);
}

void CPU65c816::op_CLI() {
    // CLI - Clear Interrupt Disable Flag
    setFlag(FLAG_IRQ_DISABLE, false);
}

void CPU65c816::op_SEI() {
    // SEI - Set Interrupt Disable Flag
    setFlag(FLAG_IRQ_DISABLE, true);
}

void CPU65c816::op_CLV() {
    // CLV - Clear Overflow Flag
    setFlag(FLAG_OVERFLOW, false);
}

void CPU65c816::op_CLD() {
    // CLD - Clear Decimal Flag
    setFlag(FLAG_DECIMAL, false);
}

void CPU65c816::op_SED() {
    // SED - Set Decimal Flag
    setFlag(FLAG_DECIMAL, true);
}

void CPU65c816::op_REP() {
    // REP - Reset Processor Status Bits
    // Clears bits in P register based on immediate operand
    // In emulation mode, M and X flags (bits 4 and 5) cannot be cleared
    
    uint8 mask = fetchByte();
    
    if (registers.E) {
        // In emulation mode, preserve M and X flags
        mask &= ~0x30;
    }
    
    registers.P &= ~mask;  // Clear the bits specified by mask
}

void CPU65c816::op_SEP() {
    // SEP - Set Processor Status Bits
    // Sets bits in P register based on immediate operand
    
    uint8 mask = fetchByte();
    
    registers.P |= mask;  // Set the bits specified by mask
    
    // In emulation mode, M and X are always set
    if (registers.E) {
        registers.P |= 0x30;
    }
}

void CPU65c816::op_XCE() {
    // XCE - Exchange Carry and Emulation flags
    // Swaps the Carry flag with the Emulation mode flag
    // This is how you switch between emulation and native mode
    
    bool oldCarry = getFlag(FLAG_CARRY);
    bool oldEmulation = registers.E;
    
    setFlag(FLAG_CARRY, oldEmulation);
    registers.E = oldCarry;
    
    // When entering emulation mode, set M and X flags
    if (registers.E) {
        registers.P |= 0x30;  // Force 8-bit mode
        // Also reset high bytes of index registers and stack pointer
        registers.X &= 0xFF;
        registers.Y &= 0xFF;
        registers.SP = 0x0100 | (registers.SP & 0xFF);
    }
}

void CPU65c816::op_TSB(uint32 address) {
    // TSB - Test and Set Bits
    // Tests bits (like BIT) and then sets those bits in memory
    // Z flag reflects the test result (A & memory)
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        uint8 a = registers.A & 0xFF;
        
        // Test: set Z if (A & memory) == 0
        setFlag(FLAG_ZERO, (a & value) == 0);
        
        // Set: memory |= A
        value |= a;
        write8(address, value);
    } else {
        uint16 value = read16(address);
        uint16 a = registers.A;
        
        // Test: set Z if (A & memory) == 0
        setFlag(FLAG_ZERO, (a & value) == 0);
        
        // Set: memory |= A
        value |= a;
        write16(address, value);
    }
}

void CPU65c816::op_TRB(uint32 address) {
    // TRB - Test and Reset Bits
    // Tests bits (like BIT) and then clears those bits in memory
    // Z flag reflects the test result (A & memory)
    
    if (isMemory8Bit()) {
        uint8 value = read8(address);
        uint8 a = registers.A & 0xFF;
        
        // Test: set Z if (A & memory) == 0
        setFlag(FLAG_ZERO, (a & value) == 0);
        
        // Reset: memory &= ~A
        value &= ~a;
        write8(address, value);
    } else {
        uint16 value = read16(address);
        uint16 a = registers.A;
        
        // Test: set Z if (A & memory) == 0
        setFlag(FLAG_ZERO, (a & value) == 0);
        
        // Reset: memory &= ~A
        value &= ~a;
        write16(address, value);
    }
}

void CPU65c816::op_MVP() {
    // MVP - Block Move Previous (decrement addresses)
    // Moves (A + 1) bytes from source to destination, decrementing addresses
    // Source bank in X, destination bank in Y (first operand byte)
    // Source address in X, destination address in Y
    
    uint8 destBank = fetchByte();  // First operand
    uint8 srcBank = fetchByte();   // Second operand
    
    // Copy one byte from source to destination
    uint32 srcAddr = (static_cast<uint32>(srcBank) << 16) | registers.X;
    uint32 destAddr = (static_cast<uint32>(destBank) << 16) | registers.Y;
    
    uint8 data = read8(srcAddr);
    write8(destAddr, data);
    
    // Decrement addresses
    registers.X = (registers.X - 1) & 0xFFFF;
    registers.Y = (registers.Y - 1) & 0xFFFF;
    
    // Decrement counter
    if (registers.A == 0) {
        registers.A = 0xFFFF;  // Will underflow
    } else {
        registers.A = (registers.A - 1) & 0xFFFF;
        // If not done, repeat this instruction (don't advance PC)
        registers.PC -= 3;  // Back to start of MVP instruction
    }
    
    // Update data bank register
    registers.DBR = destBank;
}

void CPU65c816::op_MVN() {
    // MVN - Block Move Next (increment addresses)
    // Moves (A + 1) bytes from source to destination, incrementing addresses
    // Source bank in X, destination bank in Y (first operand byte)
    // Source address in X, destination address in Y
    
    uint8 destBank = fetchByte();  // First operand
    uint8 srcBank = fetchByte();   // Second operand
    
    // Copy one byte from source to destination
    uint32 srcAddr = (static_cast<uint32>(srcBank) << 16) | registers.X;
    uint32 destAddr = (static_cast<uint32>(destBank) << 16) | registers.Y;
    
    uint8 data = read8(srcAddr);
    write8(destAddr, data);
    
    // Increment addresses
    registers.X = (registers.X + 1) & 0xFFFF;
    registers.Y = (registers.Y + 1) & 0xFFFF;
    
    // Decrement counter
    if (registers.A == 0) {
        registers.A = 0xFFFF;  // Will underflow
    } else {
        registers.A = (registers.A - 1) & 0xFFFF;
        // If not done, repeat this instruction (don't advance PC)
        registers.PC -= 3;  // Back to start of MVN instruction
    }
    
    // Update data bank register
    registers.DBR = destBank;
}

void CPU65c816::op_BRK() {
    // BRK - Break (Software Interrupt)
    // Triggers a software interrupt
    // In emulation mode: jumps to vector at 0xFFFE-0xFFFF
    // In native mode: jumps to vector at 0xFFE6-0xFFE7
    
    // Skip the signature byte
    fetchByte();
    
    if (registers.E) {
        // Emulation mode behavior (like 6502)
        // Push PC+1 (already incremented past signature byte)
        push16(registers.PC);
        
        // Push P with B flag set
        push8(registers.P | 0x10);  // Set B flag
        
        // Set I flag (disable interrupts)
        setFlag(FLAG_IRQ_DISABLE, true);
        
        // Clear D flag (on real 6502, not enforced but conventional)
        setFlag(FLAG_DECIMAL, false);
        
        // Jump to BRK vector
        registers.PC = read16(0xFFFE);
        registers.PBR = 0;
    } else {
        // Native mode behavior
        // Push PBR
        push8(registers.PBR);
        
        // Push PC
        push16(registers.PC);
        
        // Push P
        push8(registers.P);
        
        // Set I flag
        setFlag(FLAG_IRQ_DISABLE, true);
        
        // Clear D flag
        setFlag(FLAG_DECIMAL, false);
        
        // Jump to BRK vector
        registers.PC = read16(0xFFE6);
        registers.PBR = 0;
    }
}

void CPU65c816::op_COP() {
    // COP - Coprocessor instruction
    // Similar to BRK but uses different vector
    // Used for coprocessor interface or debugging
    
    // Skip the signature byte
    fetchByte();
    
    if (registers.E) {
        // Emulation mode: use 0xFFF4-0xFFF5
        push16(registers.PC);
        push8(registers.P);
        
        setFlag(FLAG_IRQ_DISABLE, true);
        setFlag(FLAG_DECIMAL, false);
        
        registers.PC = read16(0xFFF4);
        registers.PBR = 0;
    } else {
        // Native mode: use 0xFFE4-0xFFE5
        push8(registers.PBR);
        push16(registers.PC);
        push8(registers.P);
        
        setFlag(FLAG_IRQ_DISABLE, true);
        setFlag(FLAG_DECIMAL, false);
        
        registers.PC = read16(0xFFE4);
        registers.PBR = 0;
    }
}

void CPU65c816::op_WDM() {
    // WDM - Reserved for future expansion
    // On actual hardware, this is a 2-byte NOP
    // The second byte is reserved for future use
    fetchByte();  // Skip reserved byte
    // Do nothing else
}

void CPU65c816::op_STP() {
    // STP - Stop the processor
    // Halts execution until reset
    // For emulator purposes, we can set a flag or just halt
    // In a real implementation, this would stop the CPU clock
    
    // For now, just set PC to itself to create an infinite loop
    // The emulator framework can detect this if needed
    registers.PC -= 1;  // Stay at STP instruction
    
    // In a more sophisticated implementation, you'd set a "stopped" flag
    // that the emulator checks before each instruction
}

void CPU65c816::op_WAI() {
    // WAI - Wait for Interrupt
    // Stops execution until an interrupt occurs (IRQ or NMI)
    // For emulator purposes, similar to STP but waits for interrupt
    
    // For now, implement as a simple wait
    // In real implementation, this would wait for interrupt signal
    registers.PC -= 1;  // Stay at WAI instruction
    
    // A proper implementation would check for pending interrupts
    // before each instruction and resume when one occurs
}
