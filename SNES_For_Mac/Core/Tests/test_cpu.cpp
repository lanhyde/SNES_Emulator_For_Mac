//
//  CPUTester.cpp
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/14.
//

#include "../CPU/CPU65c816.hpp"
#include "../Memory/Memory.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>

#define COLOR_RESET     "\033[0m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_RED       "\033[31m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_CYAN      "\033[36m"

using namespace std;

class CPUTester{
private:
    CPU65c816 cpu;
    Memory memory;
    int testsPassed;
    int testsFailed;
    
public:
    CPUTester() {
        cpu.setMemory(&memory);
        testsPassed = 0;
        testsFailed = 0;
    }
    
    void runAllTests() {
        cout << "COLOR_CYAN" << "=== SNES Emulator CPU Tests ===" << COLOR_RESET << endl;
        cout << endl;
        
        testReset();
        testLDAImmediate8Bit();
        testLDAImmediate16Bit();
        testNOP();
        testFlags();
        testMemoryReadWrite();
        
        
        // NEW: Add these lines
        testTransferInstructions8Bit();
        testTransferInstructions16Bit();
        testStackPointerTransfers();
        testDirectPageTransfers();
        // Inc/Dec tests
        testRegisterIncrement8Bit();
        testRegisterIncrement16Bit();
        testRegisterDecrement8Bit();
        testRegisterDecrement16Bit();
        testAccumulatorIncDec();
        testMemoryIncDec();
        // Logic operations
        testAND_Operation();
        testORA_Operation();
        testEOR_Operation();
        testLogicOperations16Bit();
        
        testCMP_Equal();
        testCMP_GreaterThan();
        testCMP_LessThan();
        testCPX_CPY_Operations();
        testComparisons16Bit();
        testComparisonAddressingModes();
        
        testBranchTaken();
        testBranchNotTaken();
        testBackwardBranch();
        testAllBranchInstructions();
        testLoopWithBranch();
        testSignedOffsetConversion();
        
        cout << endl;
        cout << COLOR_CYAN << "=== Test Summary ===" << COLOR_RESET << endl;
        cout << COLOR_GREEN << "Passed: " << testsPassed << COLOR_RESET << endl;
        if (testsFailed > 0) {
            cout << COLOR_RED << "Failed: " << testsFailed << COLOR_RESET << endl;
        } else {
            cout << COLOR_GREEN << "All tests Passed! ✓" << COLOR_RESET << endl;
        }
    }
    
private:
    void assert_equal(const string& testName, uint32 expected, uint32 actual) {
        if (expected == actual) {
            cout << COLOR_GREEN << "✓ " << testName << COLOR_RESET << endl;
            testsPassed++;
        } else {
            cout << COLOR_RED << "✗ " << testName << COLOR_RESET << endl;
            cout << " Expected: 0x" << hex << expected << ", Got: 0x" << actual << dec << endl;
            testsFailed++;
        }
    }
    
    void assert_true(const string& testName, bool condition) {
        if (condition) {
            std::cout << COLOR_GREEN << "✓ " << testName << COLOR_RESET << std::endl;
            testsPassed++;
        } else {
            std::cout << COLOR_RED << "✗ " << testName << COLOR_RESET << std::endl;
            testsFailed++;
        }
    }
    
    void printTestHeader(const string& testName) {
        cout << endl << COLOR_YELLOW << "--- " << testName << " ---" << COLOR_RESET << endl;
    }
    
    void testReset() {
        printTestHeader("Test CPU Reset");
        cpu.reset();
        
        assert_equal("A register after reset", 0x0000, cpu.registers.A);
        assert_equal("X register after reset", 0x0000, cpu.registers.X);
        assert_equal("Y register after reset", 0x0000, cpu.registers.Y);
        assert_equal("SP after reset", 0x01FF, cpu.registers.SP);
        assert_equal("DBR after reset", 0x00, cpu.registers.DBR);
        assert_equal("PBR after reset", 0x00, cpu.registers.PBR);
        assert_equal("D after reset", 0x0000, cpu.registers.D);
        assert_true("Emulation mode after reset", cpu.registers.E);
        assert_true("M flag set after reset", cpu.getFlag(FLAG_MEMORY_WIDTH));
        assert_true("X flag set after reset", cpu.getFlag(FLAG_INDEX_WIDTH));
    }
    
    void testLDAImmediate8Bit() {
        printTestHeader("Test LDA Immediate (8-bit)");
        
        cpu.reset();
        // Create a simple program: LDA #$42
        vector<uint8> rom(0x10000, 0xEA);
        rom[0x8000] = 0xA9;         // LDA immediate
        rom[0x8001] = 0x42;         // Value to load
        rom[0x8002] = 0xEA;         // NOP
        
        memory.loadROM(rom);
        cpu.registers.PC = 0x8000;
        cpu.registers.PBR = 0x00;
        
        // Execute LDA #$42
        int cycles = cpu.executeInstruction();
        
        assert_equal("A register low byte", 0x42, cpu.registers.A & 0xFF);
        assert_equal("PC after LDA immedaite 8-bit", 0x8002, cpu.registers.PC);
        assert_true("Zero flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("Negative flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test with zero value
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("A register with zero", 0x00, cpu.registers.A & 0xFF);
        assert_true("Zero flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test with negative value (bit 7 set)
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x80;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("A register with negative", 0x80, cpu.registers.A & 0xFF);
        assert_true("Negative flag set", cpu.getFlag(FLAG_NEGATIVE));
    }
    
    void testLDAImmediate16Bit() {
        printTestHeader("Test LDA Immediate (16-bit)");
        
        cpu.reset();
        
        // Switch to 16-bit mode for accumulator
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);
        
        // Create program: LDA #$1234
        vector<uint8> rom(0x10000, 0xEA);
        rom[0x8000] = 0xA9;         // LDA Immediate
        rom[0x8001] = 0x34;         // Low Byte
        rom[0x8002] = 0x12;         // High byte
        
        memory.loadROM(rom);
        cpu.registers.PC = 0x8000;
        cpu.registers.PBR = 0x00;
        
        cpu.executeInstruction();
        
        assert_equal("A register full 16-bit", 0x1234, cpu.registers.A);
        assert_equal("PC after LDA immediate 16-bit", 0x8003, cpu.registers.PC);
        assert_true("Zero flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("Negative flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }
    
    void testNOP() {
        printTestHeader("Test NOP Instruction");
        cpu.reset();
        
        vector<uint8> rom(0x10000, 0xEA);
        rom[0x8000] = 0xEA;         // NOP
        
        memory.loadROM(rom);
        cpu.registers.PC = 0x8000;
        
        uint16 pcBefore = cpu.registers.PC;
        uint16 aBefore = cpu.registers.A;
        uint16 xBefore = cpu.registers.X;
        
        cpu.executeInstruction();
        
        assert_equal("PC incremented by 1", pcBefore + 1, cpu.registers.PC);
        assert_equal("A unchanged", aBefore, cpu.registers.A);
        assert_equal("X unchanged", xBefore, cpu.registers.X);
    }
    
    void testFlags() {
        printTestHeader("Test Flag Operations");
        
        cpu.reset();
        
        // Test settings and clearing flags
        cpu.setFlag(FLAG_CARRY, true);
        assert_true("Carry flag set", cpu.getFlag(FLAG_CARRY));
        
        cpu.setFlag(FLAG_CARRY, false);
        assert_true("Carry flag cleared", !cpu.getFlag(FLAG_CARRY));
        
        cpu.setFlag(FLAG_ZERO, true);
        assert_true("Zero flag set", cpu.getFlag(FLAG_ZERO));
        
        cpu.setFlag(FLAG_OVERFLOW, true);
        assert_true("Overflow flag set", cpu.getFlag(FLAG_OVERFLOW));
        
        // Test multiple flags at once
        cpu.setFlag(FLAG_CARRY, true);
        cpu.setFlag(FLAG_ZERO, true);
        assert_true("Multiple flags set", cpu.getFlag(FLAG_CARRY) && cpu.getFlag(FLAG_ZERO));
    }
    
    void testMemoryReadWrite() {
        printTestHeader("Test Memory Read/Write");
        memory.reset();
        
        // Test WRAM write/read
        memory.write(0x7E0000, 0x42);
        assert_equal("WRAM read after write", 0x42, memory.read(0x7E0000));
        
        memory.write(0x7E0100, 0xAB);
        assert_equal("WRAM read at different address", 0xAB, memory.read(0x7E0100));
        
        // Test low WRAM mirror
        memory.write(0x0000, 0x55);
        assert_equal("Low WRAM write/read", 0x55, memory.read(0x0000));
        
        // Test ROM read
        vector<uint8> rom(0x10000);
        rom[0x0000] = 0x99;
        rom[0x1000] = 0x88;
        memory.loadROM(rom);
        
        // ROM should be readable
        uint8 romValue = memory.read(0x808000);     // Bank 0x80, offset 0x8000
        cout << " Rom read value 0x" << hex << (int)romValue << dec << endl;
    }
    
    void testTransferInstructions8Bit() {
        printTestHeader("Test Transfer Instructions (8-bit mode)");
        
        cpu.reset();
        
        // Test TAX (0xAA) - 8-bit mode
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit index mode
        
        std::vector<uint8> rom(0x10000, 0xEA);
        rom[0x8000] = 0xAA;  // TAX
        memory.loadROM(rom);
        cpu.registers.PC = 0x8000;
        cpu.executeInstruction();
        
        assert_equal("TAX transfers low byte only in 8-bit", 0x34, cpu.registers.X);
        assert_true("TAX updates N flag correctly", !cpu.getFlag(FLAG_NEGATIVE));
        assert_true("TAX updates Z flag correctly", !cpu.getFlag(FLAG_ZERO));
        
        // Test TAY (0xA8) - 8-bit mode
        cpu.registers.A = 0x5678;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xA8;  // TAY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TAY transfers low byte only in 8-bit", 0x78, cpu.registers.Y);
        
        // Test TXA (0x8A) - 8-bit A mode
        cpu.registers.X = 0xABCD;
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit memory mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x8A;  // TXA
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TXA transfers to low byte, preserves high", 0x12CD, cpu.registers.A);
        assert_true("TXA updates N flag", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TYA (0x98) - 8-bit A mode
        cpu.registers.Y = 0x00FF;
        cpu.registers.A = 0x5500;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x98;  // TYA
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TYA transfers to low byte, preserves high", 0x55FF, cpu.registers.A);
        
        // Test with zero value
        cpu.registers.A = 0x1200;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xAA;  // TAX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TAX with zero", 0x00, cpu.registers.X);
        assert_true("Zero flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test with negative value (bit 7 set)
        cpu.registers.A = 0x1280;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TAX with negative", 0x80, cpu.registers.X);
        assert_true("Negative flag set", cpu.getFlag(FLAG_NEGATIVE));
    }
    
    void testTransferInstructions16Bit() {
        printTestHeader("Test Transfer Instructions (16-bit mode)");
        
        cpu.reset();
        
        // Test TAX (0xAA) - 16-bit mode
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_INDEX_WIDTH, false);  // 16-bit index mode
        
        std::vector<uint8> rom(0x10000, 0xEA);
        rom[0x8000] = 0xAA;  // TAX
        memory.loadROM(rom);
        cpu.registers.PC = 0x8000;
        cpu.executeInstruction();
        
        assert_equal("TAX transfers full 16-bit in 16-bit mode", 0x1234, cpu.registers.X);
        assert_true("TAX N flag clear for positive", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TAY (0xA8) - 16-bit mode
        cpu.registers.A = 0xABCD;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xA8;  // TAY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TAY transfers full 16-bit", 0xABCD, cpu.registers.Y);
        assert_true("TAY N flag set for negative", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TXA (0x8A) - 16-bit A mode
        cpu.registers.X = 0x5678;
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);  // 16-bit memory mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x8A;  // TXA
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TXA transfers full 16-bit", 0x5678, cpu.registers.A);
        
        // Test TYA (0x98) - 16-bit A mode
        cpu.registers.Y = 0x9ABC;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x98;  // TYA
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TYA transfers full 16-bit", 0x9ABC, cpu.registers.A);
        
        // Test zero in 16-bit mode
        cpu.registers.A = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xAA;  // TAX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TAX with zero 16-bit", 0x0000, cpu.registers.X);
        assert_true("Zero flag set in 16-bit", cpu.getFlag(FLAG_ZERO));
    }
    
    void testStackPointerTransfers() {
        printTestHeader("Test Stack Pointer Transfers");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test TSX (0xBA) - Transfer SP to X, 8-bit mode
        cpu.registers.SP = 0x01F5;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xBA;  // TSX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TSX transfers full SP value", 0x01F5, cpu.registers.X);
        assert_true("TSX flags based on low byte in 8-bit", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TSX - 16-bit mode
        cpu.registers.SP = 0x1234;
        cpu.setFlag(FLAG_INDEX_WIDTH, false);  // 16-bit
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TSX in 16-bit mode", 0x1234, cpu.registers.X);
        assert_true("TSX N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TXS (0x9A) - Transfer X to SP
        cpu.registers.X = 0xABCD;
        cpu.registers.P = 0xFF;  // Set all flags
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x9A;  // TXS
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TXS sets SP", 0xABCD, cpu.registers.SP);
        assert_equal("TXS does not affect flags", 0xFF, cpu.registers.P);
        
        // Test with different value
        cpu.registers.X = 0x0100;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TXS sets SP to X value", 0x0100, cpu.registers.SP);
    }
    
    void testDirectPageTransfers() {
        printTestHeader("Test Direct Page Transfers");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test TCD (0x5B) - Transfer A to D
        cpu.registers.A = 0x2000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x5B;  // TCD
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TCD transfers A to D", 0x2000, cpu.registers.D);
        assert_true("TCD updates N flag", !cpu.getFlag(FLAG_NEGATIVE));
        assert_true("TCD updates Z flag", !cpu.getFlag(FLAG_ZERO));
        
        // Test TCD with zero
        cpu.registers.A = 0x0000;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TCD with zero", 0x0000, cpu.registers.D);
        assert_true("TCD Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test TCD with negative (bit 15 set)
        cpu.registers.A = 0x8000;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TCD with negative", 0x8000, cpu.registers.D);
        assert_true("TCD N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TDC (0x7B) - Transfer D to A
        cpu.registers.D = 0x1234;
        cpu.registers.A = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x7B;  // TDC
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TDC transfers D to A", 0x1234, cpu.registers.A);
        assert_true("TDC updates flags", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test TCS (0x1B) - Transfer A to SP
        cpu.registers.A = 0x01FF;
        cpu.registers.P = 0xFF;  // Set all flags
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x1B;  // TCS
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TCS transfers A to SP", 0x01FF, cpu.registers.SP);
        assert_equal("TCS does not affect flags", 0xFF, cpu.registers.P);
        
        // Test TSC (0x3B) - Transfer SP to A
        cpu.registers.SP = 0x0180;
        cpu.registers.A = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x3B;  // TSC
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("TSC transfers SP to A", 0x0180, cpu.registers.A);
        assert_true("TSC updates flags", !cpu.getFlag(FLAG_ZERO));
    }
    
    void testRegisterIncrement8Bit() {
        printTestHeader("Test Register Increment (8-bit mode)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test INX (0xE8) - Basic increment
        cpu.registers.X = 0x05;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE8;  // INX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX basic increment", 0x06, cpu.registers.X);
        assert_true("INX N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        assert_true("INX Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test INX - Wrap from 0xFF to 0x00
        cpu.registers.X = 0xFF;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX wrap to zero", 0x00, cpu.registers.X);
        assert_true("INX Z flag set on wrap", cpu.getFlag(FLAG_ZERO));
        assert_true("INX N flag clear on wrap", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INX - Result is negative (bit 7 set)
        cpu.registers.X = 0x7F;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX to negative", 0x80, cpu.registers.X);
        assert_true("INX N flag set", cpu.getFlag(FLAG_NEGATIVE));
        assert_true("INX Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test INY (0xC8) - Basic increment
        cpu.registers.Y = 0x0A;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC8;  // INY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INY basic increment", 0x0B, cpu.registers.Y);
        
        // Test INY - Wrap from 0xFF
        cpu.registers.Y = 0xFF;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INY wrap to zero", 0x00, cpu.registers.Y);
        assert_true("INY Z flag set", cpu.getFlag(FLAG_ZERO));
    }

    void testRegisterIncrement16Bit() {
        printTestHeader("Test Register Increment (16-bit mode)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test INX - 16-bit mode
        cpu.registers.X = 0x1234;
        cpu.setFlag(FLAG_INDEX_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE8;  // INX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX 16-bit increment", 0x1235, cpu.registers.X);
        assert_true("INX N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INX - Wrap from 0xFFFF to 0x0000
        cpu.registers.X = 0xFFFF;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX 16-bit wrap", 0x0000, cpu.registers.X);
        assert_true("INX Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test INX - Negative in 16-bit (bit 15 set)
        cpu.registers.X = 0x7FFF;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INX 16-bit to negative", 0x8000, cpu.registers.X);
        assert_true("INX N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INY - 16-bit mode
        cpu.registers.Y = 0xABCD;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC8;  // INY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INY 16-bit increment", 0xABCE, cpu.registers.Y);
    }

    void testRegisterDecrement8Bit() {
        printTestHeader("Test Register Decrement (8-bit mode)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test DEX (0xCA) - Basic decrement
        cpu.registers.X = 0x10;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xCA;  // DEX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX basic decrement", 0x0F, cpu.registers.X);
        assert_true("DEX N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        assert_true("DEX Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test DEX - Decrement to zero
        cpu.registers.X = 0x01;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX to zero", 0x00, cpu.registers.X);
        assert_true("DEX Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test DEX - Wrap from 0x00 to 0xFF
        cpu.registers.X = 0x00;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX wrap to 0xFF", 0xFF, cpu.registers.X);
        assert_true("DEX N flag set", cpu.getFlag(FLAG_NEGATIVE));
        assert_true("DEX Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test DEY (0x88) - Basic decrement
        cpu.registers.Y = 0x80;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x88;  // DEY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEY basic decrement", 0x7F, cpu.registers.Y);
        assert_true("DEY N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test DEY - Wrap from 0x00
        cpu.registers.Y = 0x00;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEY wrap to 0xFF", 0xFF, cpu.registers.Y);
        assert_true("DEY N flag set", cpu.getFlag(FLAG_NEGATIVE));
    }

    void testRegisterDecrement16Bit() {
        printTestHeader("Test Register Decrement (16-bit mode)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test DEX - 16-bit mode
        cpu.registers.X = 0x1000;
        cpu.setFlag(FLAG_INDEX_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xCA;  // DEX
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX 16-bit decrement", 0x0FFF, cpu.registers.X);
        assert_true("DEX N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test DEX - Decrement to zero
        cpu.registers.X = 0x0001;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX 16-bit to zero", 0x0000, cpu.registers.X);
        assert_true("DEX Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test DEX - Wrap from 0x0000 to 0xFFFF
        cpu.registers.X = 0x0000;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEX 16-bit wrap", 0xFFFF, cpu.registers.X);
        assert_true("DEX N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test DEY - 16-bit mode
        cpu.registers.Y = 0x8000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x88;  // DEY
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEY 16-bit decrement", 0x7FFF, cpu.registers.Y);
        assert_true("DEY N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }

    void testAccumulatorIncDec() {
        printTestHeader("Test Accumulator INC/DEC");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test INC A (0x1A) - 8-bit mode
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x1A;  // INC A
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC A preserves high byte", 0x1235, cpu.registers.A);
        assert_true("INC A N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INC A - Wrap in 8-bit
        cpu.registers.A = 0x12FF;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC A wrap in 8-bit", 0x1200, cpu.registers.A);
        assert_true("INC A Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test INC A - 16-bit mode
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC A 16-bit", 0x1235, cpu.registers.A);
        
        // Test DEC A (0x3A) - 8-bit mode
        cpu.registers.A = 0x5610;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x3A;  // DEC A
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEC A preserves high byte", 0x560F, cpu.registers.A);
        
        // Test DEC A - Decrement to zero
        cpu.registers.A = 0x5601;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEC A to zero", 0x5600, cpu.registers.A);
        assert_true("DEC A Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test DEC A - Wrap in 8-bit
        cpu.registers.A = 0x5600;
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEC A wrap in 8-bit", 0x56FF, cpu.registers.A);
        assert_true("DEC A N flag set", cpu.getFlag(FLAG_NEGATIVE));
    }

    void testMemoryIncDec() {
        printTestHeader("Test Memory INC/DEC");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        memory.reset();
        
        // Test INC Direct Page (0xE6) - 8-bit
        memory.write(0x0010, 0x42);
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.D = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE6;  // INC Direct Page
        rom[0x8001] = 0x10;  // Address $10
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC memory value", 0x43, memory.read(0x0010));
        assert_true("INC memory N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INC - Wrap in memory
        memory.write(0x0010, 0xFF);
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC memory wrap", 0x00, memory.read(0x0010));
        assert_true("INC memory Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test DEC Direct Page (0xC6) - 8-bit
        memory.write(0x0020, 0x10);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC6;  // DEC Direct Page
        rom[0x8001] = 0x20;  // Address $20
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEC memory value", 0x0F, memory.read(0x0020));
        
        // Test DEC - Wrap in memory
        memory.write(0x0020, 0x00);
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("DEC memory wrap", 0xFF, memory.read(0x0020));
        assert_true("DEC memory N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test INC Absolute (0xEE) - 16-bit mode
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);  // 16-bit mode
        memory.write16(0x1000, 0x1234);
        cpu.registers.DBR = 0x00;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xEE;  // INC Absolute
        rom[0x8001] = 0x00;  // Low byte
        rom[0x8002] = 0x10;  // High byte
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("INC memory 16-bit", 0x1235, memory.read16(0x1000));
    }

    void testAND_Operation() {
        printTestHeader("Test AND Operation (8-bit)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test AND Immediate (0x29) - Basic masking
        cpu.registers.A = 0x12FF;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x29;  // AND Immediate
        rom[0x8001] = 0x0F;  // Mask lower 4 bits
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND basic mask", 0x120F, cpu.registers.A);
        assert_true("AND N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        assert_true("AND Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test AND - Result is zero
        cpu.registers.A = 0x120F;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xF0;  // AND with 0xF0
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND result zero", 0x1200, cpu.registers.A);
        assert_true("AND Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test AND - Result is negative (bit 7 set)
        cpu.registers.A = 0x12FF;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x80;  // AND with 0x80
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND result negative", 0x1280, cpu.registers.A);
        assert_true("AND N flag set", cpu.getFlag(FLAG_NEGATIVE));
        assert_true("AND Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test AND - All bits set
        cpu.registers.A = 0x12AA;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xFF;  // AND with 0xFF (identity)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND with 0xFF", 0x12AA, cpu.registers.A);
        assert_true("AND N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test AND - Clear specific bits
        cpu.registers.A = 0x12F5;  // 11110101
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x0F;  // AND with 00001111
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND clear upper bits", 0x1205, cpu.registers.A);  // 00000101
    }

    void testORA_Operation() {
        printTestHeader("Test ORA Operation (8-bit)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test ORA Immediate (0x09) - Basic OR
        cpu.registers.A = 0x560F;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x09;  // ORA Immediate
        rom[0x8001] = 0xF0;  // OR with 0xF0
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA basic", 0x56FF, cpu.registers.A);
        assert_true("ORA N flag set", cpu.getFlag(FLAG_NEGATIVE));
        assert_true("ORA Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test ORA - OR with zero (identity)
        cpu.registers.A = 0x5642;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;  // OR with 0x00
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA with zero", 0x5642, cpu.registers.A);
        assert_true("ORA N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test ORA - Set specific bits
        cpu.registers.A = 0x5605;  // 00000101
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x0A;  // OR with 00001010
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA set bits", 0x560F, cpu.registers.A);  // 00001111
        
        // Test ORA - Result negative
        cpu.registers.A = 0x5600;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x80;  // OR with 0x80
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA result negative", 0x5680, cpu.registers.A);
        assert_true("ORA N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test ORA - All bits
        cpu.registers.A = 0x5600;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xFF;  // OR with 0xFF
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA all bits", 0x56FF, cpu.registers.A);
    }

    void testEOR_Operation() {
        printTestHeader("Test EOR Operation (8-bit)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test EOR Immediate (0x49) - Basic XOR
        cpu.registers.A = 0x78AA;  // 10101010
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x49;  // EOR Immediate
        rom[0x8001] = 0xFF;  // XOR with 11111111
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR flip all bits", 0x7855, cpu.registers.A);  // 01010101
        assert_true("EOR N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test EOR - XOR with itself = zero
        cpu.registers.A = 0x7842;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x42;  // XOR with same value
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR with self", 0x7800, cpu.registers.A);
        assert_true("EOR Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test EOR - XOR with zero (identity)
        cpu.registers.A = 0x78A5;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;  // XOR with 0x00
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR with zero", 0x78A5, cpu.registers.A);
        assert_true("EOR N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test EOR - Toggle specific bits
        cpu.registers.A = 0x78F0;  // 11110000
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xFF;  // XOR with 11111111
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR toggle", 0x780F, cpu.registers.A);  // 00001111
        
        // Test EOR - Partial toggle
        cpu.registers.A = 0x78AA;  // 10101010
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x0F;  // XOR with 00001111
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR partial toggle", 0x78A5, cpu.registers.A);  // 10100101
        
        // Test EOR - Result negative
        cpu.registers.A = 0x787F;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xFF;  // XOR with 0xFF
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR result negative", 0x7880, cpu.registers.A);
        assert_true("EOR N flag set", cpu.getFlag(FLAG_NEGATIVE));
    }

    void testLogicOperations16Bit() {
        printTestHeader("Test Logic Operations (16-bit)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test AND - 16-bit mode
        cpu.registers.A = 0xAAAA;
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x29;  // AND Immediate
        rom[0x8001] = 0x0F;  // Low byte
        rom[0x8002] = 0xF0;  // High byte (0xF00F)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND 16-bit", 0xA00A, cpu.registers.A);
        assert_true("AND 16-bit N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test AND - 16-bit result zero
        cpu.registers.A = 0x0F0F;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0xF0;
        rom[0x8002] = 0xF0;  // AND with 0xF0F0
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND 16-bit zero", 0x0000, cpu.registers.A);
        assert_true("AND 16-bit Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test ORA - 16-bit mode
        cpu.registers.A = 0x0F0F;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x09;  // ORA Immediate
        rom[0x8001] = 0xF0;
        rom[0x8002] = 0xF0;  // OR with 0xF0F0
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA 16-bit", 0xFFFF, cpu.registers.A);
        assert_true("ORA 16-bit N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test EOR - 16-bit mode
        cpu.registers.A = 0xAAAA;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x49;  // EOR Immediate
        rom[0x8001] = 0xFF;
        rom[0x8002] = 0xFF;  // XOR with 0xFFFF
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR 16-bit flip", 0x5555, cpu.registers.A);
        assert_true("EOR 16-bit N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test EOR - 16-bit to zero
        cpu.registers.A = 0x1234;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x34;
        rom[0x8002] = 0x12;  // XOR with self
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR 16-bit zero", 0x0000, cpu.registers.A);
        assert_true("EOR 16-bit Z flag set", cpu.getFlag(FLAG_ZERO));
        
        // Test bit masking patterns
        cpu.registers.A = 0xF0F0;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x29;  // AND Immediate
        rom[0x8001] = 0x0F;
        rom[0x8002] = 0x0F;  // AND with 0x0F0F
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND 16-bit mask", 0x0000, cpu.registers.A);
    }

    // Optional: Test with different addressing modes
    void testLogicAddressingModes() {
        printTestHeader("Test Logic Operations - Different Addressing Modes");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        memory.reset();
        
        // Test AND Direct Page (0x25)
        memory.write(0x0010, 0x0F);
        cpu.registers.A = 0x56FF;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.D = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x25;  // AND Direct Page
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("AND Direct Page", 0x560F, cpu.registers.A);
        
        // Test ORA Absolute (0x0D)
        memory.write(0x1000, 0xF0);
        cpu.registers.A = 0x560F;
        cpu.registers.DBR = 0x00;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x0D;  // ORA Absolute
        rom[0x8001] = 0x00;  // Low byte
        rom[0x8002] = 0x10;  // High byte
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("ORA Absolute", 0x56FF, cpu.registers.A);
        
        // Test EOR Direct Page (0x45)
        memory.write(0x0020, 0xFF);
        cpu.registers.A = 0x78AA;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x45;  // EOR Direct Page
        rom[0x8001] = 0x20;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("EOR Direct Page", 0x7855, cpu.registers.A);
    }
    
    void testCMP_Equal() {
        printTestHeader("Test CMP - Equal Values");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test CMP Immediate (0xC9) - Equal values
        cpu.registers.A = 0x1242;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC9;  // CMP Immediate
        rom[0x8001] = 0x42;  // Compare with 0x42
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CMP equal - A unchanged", 0x1242, cpu.registers.A);
        assert_true("CMP equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CMP equal - C flag set (no borrow)", cpu.getFlag(FLAG_CARRY));
        assert_true("CMP equal - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CMP with zero
        cpu.registers.A = 0x1200;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP with zero - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CMP with zero - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CMP with zero - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }

    void testCMP_GreaterThan() {
        printTestHeader("Test CMP - Greater Than");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test A > operand (0x50 > 0x30)
        cpu.registers.A = 0x1250;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC9;  // CMP Immediate
        rom[0x8001] = 0x30;  // Compare with 0x30
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CMP greater - A unchanged", 0x1250, cpu.registers.A);
        assert_true("CMP greater - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("CMP greater - C flag set (A >= operand)", cpu.getFlag(FLAG_CARRY));
        assert_true("CMP greater - N flag clear (0x50-0x30=0x20)", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test with result having bit 7 clear
        cpu.registers.A = 0x127F;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x7E;  // 0x7F - 0x7E = 0x01
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP result positive - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CMP result positive - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }

    void testCMP_LessThan() {
        printTestHeader("Test CMP - Less Than");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test A < operand (0x30 < 0x50)
        cpu.registers.A = 0x1230;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC9;  // CMP Immediate
        rom[0x8001] = 0x50;  // Compare with 0x50
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CMP less than - A unchanged", 0x1230, cpu.registers.A);
        assert_true("CMP less than - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("CMP less than - C flag clear (borrow)", !cpu.getFlag(FLAG_CARRY));
        assert_true("CMP less than - N flag set (0x30-0x50=0xE0)", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test 0x00 - 0x01 = 0xFF (borrow)
        cpu.registers.A = 0x1200;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x01;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP 0-1 - C flag clear", !cpu.getFlag(FLAG_CARRY));
        assert_true("CMP 0-1 - N flag set (result=0xFF)", cpu.getFlag(FLAG_NEGATIVE));
        assert_true("CMP 0-1 - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        
        // Test boundary: 0x7F - 0x80 (positive - negative)
        cpu.registers.A = 0x127F;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x80;  // 0x7F - 0x80 = 0xFF (borrow)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP 0x7F-0x80 - C flag clear", !cpu.getFlag(FLAG_CARRY));
        assert_true("CMP 0x7F-0x80 - N flag set", cpu.getFlag(FLAG_NEGATIVE));
    }

    void testCPX_CPY_Operations() {
        printTestHeader("Test CPX and CPY");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test CPX Immediate (0xE0) - Equal
        cpu.registers.X = 0x10;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE0;  // CPX Immediate
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CPX equal - X unchanged", 0x10, cpu.registers.X);
        assert_true("CPX equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CPX equal - C flag set", cpu.getFlag(FLAG_CARRY));
        
        // Test CPX - Greater than
        cpu.registers.X = 0x50;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x30;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPX greater - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("CPX greater - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CPX greater - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CPX - Less than
        cpu.registers.X = 0x30;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x50;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPX less - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("CPX less - C flag clear", !cpu.getFlag(FLAG_CARRY));
        assert_true("CPX less - N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CPY Immediate (0xC0) - Equal
        cpu.registers.Y = 0x42;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC0;  // CPY Immediate
        rom[0x8001] = 0x42;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CPY equal - Y unchanged", 0x42, cpu.registers.Y);
        assert_true("CPY equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CPY equal - C flag set", cpu.getFlag(FLAG_CARRY));
        
        // Test CPY - Greater than
        cpu.registers.Y = 0x80;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x40;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPY greater - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CPY greater - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CPY - Less than
        cpu.registers.Y = 0x40;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x80;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPY less - C flag clear", !cpu.getFlag(FLAG_CARRY));
        assert_true("CPY less - N flag set", cpu.getFlag(FLAG_NEGATIVE));
    }

    void testComparisons16Bit() {
        printTestHeader("Test Comparisons (16-bit mode)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test CMP - 16-bit equal
        cpu.registers.A = 0x1234;
        cpu.setFlag(FLAG_MEMORY_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC9;  // CMP Immediate
        rom[0x8001] = 0x34;  // Low byte
        rom[0x8002] = 0x12;  // High byte (0x1234)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("CMP 16-bit equal - A unchanged", 0x1234, cpu.registers.A);
        assert_true("CMP 16-bit equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CMP 16-bit equal - C flag set", cpu.getFlag(FLAG_CARRY));
        
        // Test CMP - 16-bit greater (0x5000 > 0x3000)
        cpu.registers.A = 0x5000;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;
        rom[0x8002] = 0x30;  // 0x3000
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP 16-bit greater - Z flag clear", !cpu.getFlag(FLAG_ZERO));
        assert_true("CMP 16-bit greater - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CMP 16-bit greater - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CMP - 16-bit less (0x3000 < 0x5000)
        cpu.registers.A = 0x3000;
        cpu.registers.PC = 0x8000;
        rom[0x8001] = 0x00;
        rom[0x8002] = 0x50;  // 0x5000
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP 16-bit less - C flag clear", !cpu.getFlag(FLAG_CARRY));
        assert_true("CMP 16-bit less - N flag set", cpu.getFlag(FLAG_NEGATIVE));
        
        // Test CPX - 16-bit
        cpu.registers.X = 0xABCD;
        cpu.setFlag(FLAG_INDEX_WIDTH, false);  // 16-bit mode
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE0;  // CPX Immediate
        rom[0x8001] = 0xCD;
        rom[0x8002] = 0xAB;  // 0xABCD
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPX 16-bit equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CPX 16-bit equal - C flag set", cpu.getFlag(FLAG_CARRY));
        
        // Test CPY - 16-bit greater
        cpu.registers.Y = 0x8000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC0;  // CPY Immediate
        rom[0x8001] = 0x00;
        rom[0x8002] = 0x40;  // 0x4000
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPY 16-bit greater - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CPY 16-bit greater - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }

    // Optional: Test with different addressing modes
    void testComparisonAddressingModes() {
        printTestHeader("Test Comparison - Different Addressing Modes");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        memory.reset();
        
        // Test CMP Direct Page (0xC5)
        memory.write(0x0010, 0x42);
        cpu.registers.A = 0x5642;
        cpu.setFlag(FLAG_MEMORY_WIDTH, true);  // 8-bit mode
        cpu.registers.D = 0x0000;
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xC5;  // CMP Direct Page
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CMP Direct Page equal - Z flag set", cpu.getFlag(FLAG_ZERO));
        assert_true("CMP Direct Page equal - C flag set", cpu.getFlag(FLAG_CARRY));
        
        // Test CPX Direct Page (0xE4)
        memory.write(0x0020, 0x10);
        cpu.registers.X = 0x20;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xE4;  // CPX Direct Page
        rom[0x8001] = 0x20;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_true("CPX Direct Page greater - C flag set", cpu.getFlag(FLAG_CARRY));
        assert_true("CPX Direct Page - N flag clear", !cpu.getFlag(FLAG_NEGATIVE));
    }
    
    void testBranchTaken() {
        printTestHeader("Test Branch Taken (Forward)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test BEQ (0xF0) - Branch if Equal (Z=1), taken
        cpu.setFlag(FLAG_ZERO, true);  // Condition is true
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xF0;  // BEQ
        rom[0x8001] = 0x05;  // Offset +5
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        // PC should be: 0x8002 (after fetching) + 0x05 = 0x8007
        assert_equal("BEQ taken - PC updated", 0x8007, cpu.registers.PC);
        
        // Test BNE (0xD0) - Branch if Not Equal (Z=0), taken
        cpu.setFlag(FLAG_ZERO, false);  // Z=0, condition true
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xD0;  // BNE
        rom[0x8001] = 0x10;  // Offset +16
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BNE taken - PC updated", 0x8012, cpu.registers.PC);
        
        // Test BCS (0xB0) - Branch if Carry Set (C=1), taken
        cpu.setFlag(FLAG_CARRY, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xB0;  // BCS
        rom[0x8001] = 0x08;  // Offset +8
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BCS taken - PC updated", 0x800A, cpu.registers.PC);
        
        // Test BCC (0x90) - Branch if Carry Clear (C=0), taken
        cpu.setFlag(FLAG_CARRY, false);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x90;  // BCC
        rom[0x8001] = 0x0F;  // Offset +15
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BCC taken - PC updated", 0x8011, cpu.registers.PC);
        
        // Test BMI (0x30) - Branch if Minus (N=1), taken
        cpu.setFlag(FLAG_NEGATIVE, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x30;  // BMI
        rom[0x8001] = 0x20;  // Offset +32
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BMI taken - PC updated", 0x8022, cpu.registers.PC);
        
        // Test BPL (0x10) - Branch if Plus (N=0), taken
        cpu.setFlag(FLAG_NEGATIVE, false);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x10;  // BPL
        rom[0x8001] = 0x7F;  // Offset +127 (max positive)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BPL taken - PC updated", 0x8081, cpu.registers.PC);
    }

    void testBranchNotTaken() {
        printTestHeader("Test Branch Not Taken");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test BEQ - Not taken (Z=0)
        cpu.setFlag(FLAG_ZERO, false);  // Condition is false
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xF0;  // BEQ
        rom[0x8001] = 0x05;  // Offset (ignored)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        // PC should continue to next instruction: 0x8002
        assert_equal("BEQ not taken - PC at next instruction", 0x8002, cpu.registers.PC);
        
        // Test BNE - Not taken (Z=1)
        cpu.setFlag(FLAG_ZERO, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xD0;  // BNE
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BNE not taken - PC at next instruction", 0x8002, cpu.registers.PC);
        
        // Test BCS - Not taken (C=0)
        cpu.setFlag(FLAG_CARRY, false);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0xB0;  // BCS
        rom[0x8001] = 0x20;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BCS not taken - PC at next instruction", 0x8002, cpu.registers.PC);
        
        // Test BCC - Not taken (C=1)
        cpu.setFlag(FLAG_CARRY, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x90;  // BCC
        rom[0x8001] = 0x20;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BCC not taken - PC at next instruction", 0x8002, cpu.registers.PC);
        
        // Test BMI - Not taken (N=0)
        cpu.setFlag(FLAG_NEGATIVE, false);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x30;  // BMI
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BMI not taken - PC at next instruction", 0x8002, cpu.registers.PC);
        
        // Test BPL - Not taken (N=1)
        cpu.setFlag(FLAG_NEGATIVE, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x10;  // BPL
        rom[0x8001] = 0x10;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BPL not taken - PC at next instruction", 0x8002, cpu.registers.PC);
    }

    void testBackwardBranch() {
        printTestHeader("Test Backward Branch (Negative Offset)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test BNE with negative offset (backward branch)
        cpu.setFlag(FLAG_ZERO, false);  // Z=0, branch taken
        cpu.registers.PC = 0x8010;
        rom[0x8010] = 0xD0;  // BNE
        rom[0x8011] = 0xFC;  // Offset -4 (0xFC in two's complement)
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        // PC should be: 0x8012 (after fetching) + (-4) = 0x800E
        assert_equal("BNE backward - PC updated", 0x800E, cpu.registers.PC);
        
        // Test BEQ with offset -1
        cpu.setFlag(FLAG_ZERO, true);
        cpu.registers.PC = 0x8020;
        rom[0x8020] = 0xF0;  // BEQ
        rom[0x8021] = 0xFF;  // Offset -1
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BEQ offset -1", 0x8021, cpu.registers.PC);
        
        // Test BCC with offset -128 (max negative)
        cpu.setFlag(FLAG_CARRY, false);
        cpu.registers.PC = 0x8100;
        rom[0x8100] = 0x90;  // BCC
        rom[0x8101] = 0x80;  // Offset -128
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        // PC should be: 0x8102 + (-128) = 0x8082
        assert_equal("BCC max negative offset", 0x8082, cpu.registers.PC);
        
        // Test with offset -2
        cpu.setFlag(FLAG_ZERO, false);
        cpu.registers.PC = 0x8050;
        rom[0x8050] = 0xD0;  // BNE
        rom[0x8051] = 0xFE;  // Offset -2
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BNE offset -2", 0x8050, cpu.registers.PC);
    }

    void testAllBranchInstructions() {
        printTestHeader("Test All Branch Instructions");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test BVS (0x70) - Branch if Overflow Set (V=1)
        cpu.setFlag(FLAG_OVERFLOW, true);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x70;  // BVS
        rom[0x8001] = 0x05;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BVS taken", 0x8007, cpu.registers.PC);
        
        // Test BVS - Not taken
        cpu.setFlag(FLAG_OVERFLOW, false);
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BVS not taken", 0x8002, cpu.registers.PC);
        
        // Test BVC (0x50) - Branch if Overflow Clear (V=0)
        cpu.setFlag(FLAG_OVERFLOW, false);
        cpu.registers.PC = 0x8000;
        rom[0x8000] = 0x50;  // BVC
        rom[0x8001] = 0x08;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BVC taken", 0x800A, cpu.registers.PC);
        
        // Test BVC - Not taken
        cpu.setFlag(FLAG_OVERFLOW, true);
        cpu.registers.PC = 0x8000;
        memory.loadROM(rom);
        cpu.executeInstruction();
        
        assert_equal("BVC not taken", 0x8002, cpu.registers.PC);
    }

    void testLoopWithBranch() {
        printTestHeader("Test Loop with Branch (Real Program)");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Create a simple loop program:
        // loop:
        //     INX             ; 0x8000: E8
        //     CPX #$05        ; 0x8001: E0 05
        //     BNE loop        ; 0x8003: D0 FC (offset -4)
        // done:
        //     NOP             ; 0x8005: EA
        
        int pc = 0x8000;
        rom[pc++] = 0xE8;  // INX
        rom[pc++] = 0xE0;  // CPX Immediate
        rom[pc++] = 0x05;  // Compare with 5
        rom[pc++] = 0xD0;  // BNE
        rom[pc++] = 0xFB;  // Offset -5 (back to INX)
        rom[pc++] = 0xEA;  // NOP (after loop)
        
        memory.loadROM(rom);
        cpu.registers.X = 0x00;
        cpu.setFlag(FLAG_INDEX_WIDTH, true);  // 8-bit mode
        cpu.registers.PC = 0x8000;
        
        // Execute the loop
        // Iteration 1: X=0->1, CMP, Z=0, BNE taken to 0x8000
        cpu.executeInstruction();  // INX: X=1
        assert_equal("Loop iter 1 - X=1", 0x01, cpu.registers.X);
        
        cpu.executeInstruction();  // CPX #$05
        assert_true("Loop iter 1 - Z=0", !cpu.getFlag(FLAG_ZERO));
        
        cpu.executeInstruction();  // BNE (taken)
        assert_equal("Loop iter 1 - Branch back to INX", 0x8000, cpu.registers.PC);
        
        // Iteration 2: X=1->2
        cpu.executeInstruction();  // INX: X=2
        cpu.executeInstruction();  // CPX #$05
        cpu.executeInstruction();  // BNE (taken)
        assert_equal("Loop iter 2 - Branch back", 0x8000, cpu.registers.PC);
        
        // Iteration 3: X=2->3
        cpu.executeInstruction();  // INX: X=3
        cpu.executeInstruction();  // CPX #$05
        cpu.executeInstruction();  // BNE (taken)
        
        // Iteration 4: X=3->4
        cpu.executeInstruction();  // INX: X=4
        cpu.executeInstruction();  // CPX #$05
        cpu.executeInstruction();  // BNE (taken)
        
        // Iteration 5: X=4->5, CMP, Z=1, BNE not taken
        cpu.executeInstruction();  // INX: X=5
        assert_equal("Loop final - X=5", 0x05, cpu.registers.X);
        
        cpu.executeInstruction();  // CPX #$05
        assert_true("Loop final - Z=1 (equal)", cpu.getFlag(FLAG_ZERO));
        
        cpu.executeInstruction();  // BNE (not taken)
        assert_equal("Loop exit - PC at NOP", 0x8005, cpu.registers.PC);
        
        // Verify we're at the NOP after the loop
        cpu.executeInstruction();  // NOP
        assert_equal("After loop - PC advanced", 0x8006, cpu.registers.PC);
    }

    // Optional: Test signed offset conversion
    void testSignedOffsetConversion() {
        printTestHeader("Test Signed Offset Conversion");
        
        cpu.reset();
        std::vector<uint8> rom(0x10000, 0xEA);
        memory.loadROM(rom);
        
        // Test various signed offsets
        struct OffsetTest {
            uint8 offset_byte;
            int8 expected_signed;
            uint16 start_pc;
            uint16 expected_pc;
        };
        
        OffsetTest tests[] = {
            {0x00, 0, 0x8000, 0x8002},      // +0
            {0x01, 1, 0x8000, 0x8003},      // +1
            {0x7F, 127, 0x8000, 0x8081},    // +127 (max positive)
            {0xFF, -1, 0x8000, 0x8001},     // -1
            {0xFE, -2, 0x8000, 0x8000},     // -2
            {0x80, -128, 0x8100, 0x8082},   // -128 (max negative)
        };
        
        for (const auto& test : tests) {
            cpu.setFlag(FLAG_ZERO, true);  // Make sure BEQ is taken
            cpu.registers.PC = test.start_pc;
            rom[test.start_pc] = 0xF0;  // BEQ
            rom[test.start_pc + 1] = test.offset_byte;
            memory.loadROM(rom);
            cpu.executeInstruction();
            
            assert_equal("Offset conversion", test.expected_pc, cpu.registers.PC);
        }
    }

};

int main() {
    try {
        CPUTester tester;
        tester.runAllTests();
        return 0;
    } catch(exception& e) {
        cerr << COLOR_RED << "Exception: " << e.what() << COLOR_RESET << endl;
        return 1;
    }
}
