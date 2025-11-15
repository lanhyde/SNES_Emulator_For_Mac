//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#ifndef EmulatorBridge_h
#define EmulatorBridge_h
#import <Foundation/Foundation.h>

#ifdef __cplusplus
class CPU65c816;
class Memory;
#endif

NS_ASSUME_NONNULL_BEGIN

@interface EmulatorBridge: NSObject
// Initialize the emulator
-(instancetype)init;

// Load a ROM file
-(BOOL)loadROMFromPath:(NSString*)path error:(NSError**)error;
-(BOOL)loadROMFromData:(NSData*)data error:(NSError**)error;

// Emulator control
-(void)reset;
-(void)runFrame;            // Run one frame worth of cycles
-(void)step;                // Execute one instruction

// Get frame buffer for rendering
// Returns pointer to RGB pixel data (SNES native is 256x224)
-(const uint8_t *)getFrameBuffer;
-(NSInteger)frameBufferWidth;
-(NSInteger)frameBufferHeight;

// Emulator state
-(BOOL)isRunning;
-(void)pause;
-(void)resume;

// Debug info
-(NSString*)getCPUState;
@end

NS_ASSUME_NONNULL_END
#endif
