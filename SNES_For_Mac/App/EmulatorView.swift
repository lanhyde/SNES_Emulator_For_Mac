//
//  EmulatorView.swift
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

import SwiftUI
import MetalKit

struct EmulatorView: NSViewRepresentable {
    let emulator: EmulatorBridge
    @State private var renderer: MetalRenderer?
    
    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        
        let renderer = MetalRenderer(emulator: emulator)
        mtkView.delegate = renderer
        
        DispatchQueue.main.async {
            self.renderer = renderer
        }
        
        return mtkView
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
        
    }
}
