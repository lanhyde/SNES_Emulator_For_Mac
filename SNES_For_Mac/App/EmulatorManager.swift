//
//  EmulatorManager.swift
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/14.
//

import SwiftUI
import Combine
import UniformTypeIdentifiers

class EmulatorManager: ObservableObject {
    let emulator: EmulatorBridge
    @Published var isRunning = false
    @Published var showDebug = true
    @Published var cpuState = ""
    
    private var timer: Timer?
    
    init() {
        emulator = EmulatorBridge()
        updateCPUState()
    }
    
    func loadROM() {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.allowedContentTypes = [.data]
        panel.message = "Select a SNES ROM file"
        
        if panel.runModal() == .OK, let url = panel.url {
            do {
                try emulator.loadROM(fromPath: url.path)
            } catch {
                print("Failed to load ROM: \(error)")
            }
        }
    }
    
    func reset() {
        emulator.reset()
        updateCPUState()
    }
    
    func toggleRunning() {
        if isRunning {
            pause()
        } else {
            resume()
        }
    }
    
    func resume() {
        isRunning = true
        emulator.resume()
        
        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 60.0, repeats: true) { [weak self] _ in
            self?.emulator.runFrame()
            self?.updateCPUState()
        }
    }
    
    func pause() {
        isRunning = false
        emulator.pause()
        timer?.invalidate()
        timer = nil
        updateCPUState()
    }
    
    func step() {
        emulator.step()
        updateCPUState()
    }
    
    func updateCPUState() {
        cpuState = emulator.getCPUState()
    }
}
