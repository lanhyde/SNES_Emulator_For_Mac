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
    // Stack Operations
    void op_PHA();
    void op_PHX();
    void op_PHY();
    void op_PHP();
    void op_PHD();
    void op_PHB();
    void op_PHK();
    void op_PLA();
    void op_PLX();
    void op_PLY();
    void op_PLP();
    void op_PLD();
    void op_PLB();
    // arithmetic operations
    void op_ADC(uint32 address);
    void op_SBC(uint32 address);
    // Helper for overflow calculation
    bool checkOverflow8(uint8 a, uint b, uint8 result);
    bool checkOverflow16(uint16 a, uint16 b, uint result);
    // Register increment/decrement
    void op_INX();
    void op_INY();
    void op_DEX();
    void op_DEY();

    // Memory increment/decrement (takes address)
    void op_INC(uint32 address);
    void op_DEC(uint32 address);

    // Accumulator versions (no address needed)
    void op_INC_A();
    void op_DEC_A();
    // Logic operations
    void op_AND(uint32 address);
    void op_ORA(uint32 address);
    void op_EOR(uint32 address);
    // Compare operations
    void op_CMP(uint32 address);
    void op_CPX(uint32 address);
    void op_CPY(uint32 address);
    // Helper for all branches
    void branch(bool condition);
    // Individual branch instructions
    void op_BEQ();  // Branch if Equal (Z = 1)
    void op_BNE();  // Branch if Not Equal (Z = 0)
    void op_BCS();  // Branch if Carry Set (C = 1)
    void op_BCC();  // Branch if Carry Clear (C = 0)
    void op_BMI();  // Branch if Minus (N = 1)
    void op_BPL();  // Branch if Plus (N = 0)
    void op_BVS();  // Branch if Overflow Set (V = 1)
    void op_BVC();  // Branch if Overflow Clear (V = 0)
    // Jump instructions
    void op_JMP(uint32 address);
    void op_JSR(uint32 address);
    void op_RTS();
    void op_RTI();
    // Bit manipulation and shift/rotate operations
    void op_BIT(uint32 address);        // Bit test
    void op_ASL(uint32 address);        // Arithmetic Shift Left (memory)
    void op_ASL_A();                    // Arithmetic Shift Left (accumulator)
    void op_LSR(uint32 address);        // Logical Shift Right (memory)
    void op_LSR_A();                    // Logical Shift Right (accumulator)
    void op_ROL(uint32 address);        // Rotate Left (memory)
    void op_ROL_A();                    // Rotate Left (accumulator)
    void op_ROR(uint32 address);        // Rotate Right (memorhy)
    void op_ROR_A();                    // Rotate Right (accumulator)
    // Flag manipulation instructions
    void op_CLC();                      // Clear Carry
    void op_SEC();                      // Set Carry
    void op_CLI();                      // Clear Interrupt Disable
    void op_SEI();                      // Set Interrupt Disable
    void op_CLV();                      // Clear Overflow
    void op_CLD();                      // Clear Decimal
    void op_SED();                      // Set Decimal
    void op_REP();                      // Reset Processor Status Bits
    void op_SEP();                      // Set Processor Status Bits
    void op_XCE();                      // Exchange Carry and Emulation
    // Test and Set/Reset Bit operations
    void op_TSB(uint32 address);  // Test and Set Bits
    void op_TRB(uint32 address);  // Test and Reset Bits
    
    // Block move instructions
    void op_MVP();  // Block Move Previous (decrement)
    void op_MVN();  // Block Move Next (increment)
    
    // Interrupt and system instructions
    void op_BRK();  // Break (software interrupt)
    void op_COP();  // Coprocessor instruction
    void op_WDM();  // Reserved for future expansion
    void op_STP();  // Stop processor
    void op_WAI();  // Wait for interrupt
};
#endif
