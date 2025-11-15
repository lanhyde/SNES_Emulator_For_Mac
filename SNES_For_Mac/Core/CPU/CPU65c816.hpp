//
//  CPU85c816.hpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

#ifndef CPU65C816_H
#define CPU65C816_H

#include "../Types/Types.hpp"
#include <functional>

class Memory;

// Processor Status flags (P register)
enum StatusFlag: uint8 {
    FLAG_CARRY        = 0x01,           // C
    FLAG_ZERO         = 0x02,           // Z
    FLAG_IRQ_DISABLE  = 0x04,           // I
    FLAG_DECIMAL      = 0x08,           // D
    FLAG_INDEX_WIDTH  = 0x10,           // X (0 = 16-bit, 1 = 8-bit)
    FLAG_MEMORY_WIDTH = 0x20,           // M (0 = 16-bit, 1 = 8-bit)
    FLAG_OVERFLOW     = 0x40,           // V
    FLAG_NEGATIVE     = 0x80            // N
};

// Enumation mode flag (not part of P register)
enum EmulationFlag {
    FLAG_EMULATION    = 0x01            // E (0 = native mode, 1 = emulation mode)
};

class CPU65c816 {
public:
    struct Registers {
        uint16 A;                       // Accumulator
        uint16 X;                       // X index register
        uint16 Y;                       // Y index register
        uint16 SP;                      // Stack pointer
        uint16 PC;                      // Program Counter
        uint8  P;                       // Processor Status
        uint8  DBR;                     // Data Bank Register
        uint8  PBR;                     // Program Bank Register
        uint16 D;                       // Direct Page Register
        bool   E;                       // Emulation mode flag
    } registers;
    
    CPU65c816();
    
    // Reset the CPU to initial state
    void reset();
    
    // Execute one instruction
    // Returns number of cycles taken
    int executeInstruction();
    
    // Set memory interface
    void setMemory(Memory* mem);
    
    bool getFlag(StatusFlag flag) const;
    void setFlag(StatusFlag flag, bool value);
    
    // Check if in emulation mode
    bool isEmulationMode() const { return registers.E; }
    
    // Check register widths
    bool isMemory8Bit() const { return registers.P & FLAG_MEMORY_WIDTH; }
    bool isIndex8Bit() const { return registers.P & FLAG_INDEX_WIDTH; }
    
    // Cycle counter
    uint64 totalCycles;
    
private:
    Memory* memory;
    
    // Memory access functions
    uint8 read8(uint32 address);
    uint16 read16(uint32 address);
    void write8(uint32 address, uint8 value);
    void write16(uint32 address, uint16 value);
    
    // Fetch next byte/word from PC
    uint8 fetchByte();
    uint16 fetchWord();
    
    // Stack operations
    void push8(uint8 value);
    void push16(uint16 value);
    uint8 pull8();
    uint16 pull16();
    
    // Instruction decode and execute
    int decodeAndExecute(uint8 opcode);
    
    // Flag update helpers
    void updateNZ8(uint8 value);
    void updateNZ16(uint16 value);
    
    // Addressing mode helpers (to be implemented)
    uint32 getAddress_Immediate();
    uint32 getAddress_Absolute();
    uint32 getAddress_AbsoluteX();
    uint32 getAddress_AbsoluteY();
    uint32 getAddress_Direct();
    uint32 getAddress_DirectX();
    uint32 getAddress_DirectY();
    uint32 getAddress_Indirect();
    uint32 getAddress_IndirectX();
    uint32 getAddress_IndirectY();
    
    // Instruction implementations (to be implemented)
    void op_LDA(uint32 address);
    void op_LDX(uint32 address);
    void op_LDY(uint32 address);
    void op_STA(uint32 address);
    void op_STX(uint32 address);
    void op_STY(uint32 address);
    void op_NOP();
    void op_TAX();
    void op_TAY();
    void op_TXA();
    void op_TYA();
    void op_TSX();
    void op_TXS();
    void op_TCD();
    void op_TDC();
    void op_TCS();
    void op_TSC();
};
#endif
