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
};

//int main() {
//    try {
//        CPUTester tester;
//        tester.runAllTests();
//        return 0;
//    } catch(exception& e) {
//        cerr << COLOR_RED << "Exception: " << e.what() << COLOR_RESET << endl;
//        return 1;
//    }
//}
