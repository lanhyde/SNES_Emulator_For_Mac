//
//  MetalRenderer.swift
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

import Metal
import MetalKit
import SwiftUI

class MetalRenderer: NSObject, MTKViewDelegate {
    var device: MTLDevice!
    var commandQueue: MTLCommandQueue!
    var pipelineState: MTLRenderPipelineState!
    var texture: MTLTexture?
    var vertexBuffer: MTLBuffer!
    
    let emulator: EmulatorBridge
    // Vertex data for a full-screen quad
    struct Vertex {
        var position: SIMD2<Float>
        var texCoord: SIMD2<Float>
    }
    
    let vertices: [Vertex] = [
        Vertex(position: SIMD2<Float>(-1, -1), texCoord: SIMD2<Float>(0, 1)),
        Vertex(position: SIMD2<Float>(1, -1), texCoord: SIMD2<Float>(1, 1)),
        Vertex(position: SIMD2<Float>(-1, 1), texCoord: SIMD2<Float>(0, 0)),
        Vertex(position: SIMD2<Float>(1, 1), texCoord: SIMD2<Float>(1, 0)),
    ]
    
    init(emulator: EmulatorBridge) {
        self.emulator = emulator
        super.init()
        setupMetal()
    }
    
    func setupMetal() {
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal is not supported on this device")
        }
        self.device = device
        self.commandQueue = device.makeCommandQueue()
        
        // Create vertex buffer
        let vertexBufferSize = vertices.count * MemoryLayout<Vertex>.stride
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertexBufferSize, options: [])
        
        setupPipeline()
        
        // Create texture for SNES screen
        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: Int(emulator.frameBufferWidth()), height: Int(emulator.frameBufferHeight()), mipmapped: false)
        textureDescriptor.usage = [.shaderRead]
        texture = device.makeTexture(descriptor: textureDescriptor)
    }
    
    func setupPipeline() {
        let library = device.makeDefaultLibrary()
        let vertexFunction = library?.makeFunction(name: "vertexShader")
        let fragmentFunction = library?.makeFunction(name: "fragmentShader")
        
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            fatalError("Failed to create pipeline state: \(error)")
        }
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        
    }
    
    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
            return
        }
        // Update texture with frame buffer data
        updateTexture()
        
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        renderEncoder.endEncoding()
        
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
    
    func updateTexture() {
        guard let texture = texture else { return }
        
        let frameBuffer = emulator.getFrameBuffer()
        let width = Int(emulator.frameBufferWidth())
        let height = Int(emulator.frameBufferHeight())
        
        // Convert RGB to RGBA
        var rgbaData = [UInt8](repeating: 0, count: width * height * 4)
        for i in 0..<(width * height) {
            let srcOffset = i * 3
            let dstOffset = i * 4
            rgbaData[dstOffset] = frameBuffer[srcOffset]
            rgbaData[dstOffset + 1] = frameBuffer[srcOffset + 1]
            rgbaData[dstOffset + 2] = frameBuffer[srcOffset + 2]
            rgbaData[dstOffset + 3] = 255
        }
        
        let region = MTLRegionMake2D(0, 0, width, height)
        texture.replace(region: region, mipmapLevel: 0, withBytes: rgbaData, bytesPerRow: width * 4)
    }
}
