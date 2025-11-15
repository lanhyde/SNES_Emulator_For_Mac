//
//  EmulatorBridge.mm
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

#import "EmulatorBridge.h"
#import "../Core/CPU/CPU65c816.hpp"
#import "../Core/Memory/Memory.hpp"
#include <vector>

// SNES native resolution
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 224;

@interface EmulatorBridge() {
    CPU65c816* cpu;
    Memory* memory;
    std::vector<uint8_t>* frameBuffer;
    BOOL running;
}
@end

@implementation EmulatorBridge
-(instancetype)init {
    self = [super init];
    if (self) {
        cpu = new CPU65c816();
        memory = new Memory();
        cpu->setMemory(memory);
        
        // Allocate frame buffer (RGB, 3 bytes per pixel)
        frameBuffer = new std::vector<uint8_t>(SCREEN_WIDTH * SCREEN_HEIGHT * 3);
        
        // Fill with a test pattern initially
        [self fillTestPattern];
        
        running = NO;
    }
    return self;
}

-(void)dealloc {
    delete cpu;
    delete memory;
    delete frameBuffer;
}

-(BOOL)loadROMFromPath:(NSString *)path error:(NSError **)error {
    NSData* data = [NSData dataWithContentsOfFile:path];
    if (!data) {
        if (error) {
            *error = [NSError errorWithDomain:@"EmulatorError" code:1 userInfo:@{NSLocalizedDescriptionKey:@"Failed to read ROM file"}];
        }
        return NO;
    }
    return [self loadROMFromData:data error:error];
}

-(BOOL)loadROMFromData:(NSData *)data error:(NSError **)error {
    if (data.length == 0) {
        if (error) {
            *error = [NSError errorWithDomain:@"EmulatorError" code:2 userInfo:@{NSLocalizedDescriptionKey:@"ROM data is empty"}];
        }
        return NO;
    }
    
    // Convert NSData to std::vector
    const uint8_t* bytes = (const uint8_t*)data.bytes;
    std::vector<uint8_t> romData(bytes, bytes + data.length);
    
    bool success = memory->loadROM(romData);
    if (!success) {
        if (error) {
            *error = [NSError errorWithDomain:@"EmulatorError" code:3 userInfo:@{NSLocalizedDescriptionKey:@"Failed to load ROM"}];
        }
        return NO;
    }
    [self reset];
    return YES;
}

-(void)reset {
    cpu->reset();
    memory->reset();
    [self fillTestPattern];
}

-(void)runFrame {
    if (!running) return;
    // SNES runs at ~60Hz
    // At ~3.58MHz CPU speed, that's roughly 59,666 cycles per frame
    const int CYCLES_PER_FRAME = 59666;
    
    int cyclesRun = 0;
    while (cyclesRun < CYCLES_PER_FRAME) {
        int cycles = cpu->executeInstruction();
        cyclesRun += cycles;
    }
    // TODO: Update frame buffer with actual PPU output
    // For now, we'll keep the test pattern
}

-(void)step {
    cpu->executeInstruction();
}

-(const uint8_t*)getFrameBuffer {
    return frameBuffer->data();
}

-(NSInteger)frameBufferWidth {
    return SCREEN_WIDTH;
}

-(NSInteger)frameBufferHeight {
    return SCREEN_HEIGHT;
}

-(BOOL)isRunning {
    return running;
}

-(void)pause {
    running = NO;
}

-(void)resume {
    running = YES;
}

-(NSString*)getCPUState {
    // Format CPU registers for debugging
    return [NSString stringWithFormat:@"A: $%04X  X: $%04X  Y: $%04X\n"
    @"SP: $%04X  PC: $%04X  P: $%02X\n"
    @"DBR: $%02X  PBR: $%02X  D: $%04X\n"
    @"E: %d  Cycles: %llu",
    cpu->registers.A,
    cpu->registers.X,
    cpu->registers.Y,
    cpu->registers.SP,
    cpu->registers.PC,
    cpu->registers.P,
    cpu->registers.DBR,
    cpu->registers.PBR,
    cpu->registers.D,
    cpu->registers.E ? 1 : 0,
    cpu->totalCycles];
}

// Helper: Fill frame buffer with a colorful test pattern
-(void)fillTestPattern {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for(int x = 0; x < SCREEN_WIDTH; ++x) {
            int offset = (y * SCREEN_WIDTH + x) * 3;
            // Create a gradient pattern
            (*frameBuffer)[offset + 0] = (uint8_t)((x * 255) / SCREEN_WIDTH);           // R
            (*frameBuffer)[offset + 1] = (uint8_t)((y * 255) / SCREEN_HEIGHT);          // G
            (*frameBuffer)[offset + 2] = (uint8_t)(((x + y) * 255) / (SCREEN_WIDTH + SCREEN_HEIGHT));       // B
        }
    }
}
@end
