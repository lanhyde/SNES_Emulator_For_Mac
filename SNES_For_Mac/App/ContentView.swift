//
//  ContentView.swift
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/13.
//

import SwiftUI

struct ContentView: View {
    @StateObject private var emulatorManager = EmulatorManager()
    var body: some View {
        VStack {
            EmulatorView(emulator: emulatorManager.emulator)
                .aspectRatio(256.0 / 224.0, contentMode: .fit)
                .border(Color.gray, width: 2)
                .frame(maxWidth: 768, maxHeight: 672)
            
            if emulatorManager.showDebug {
                Text(emulatorManager.cpuState)
                    .font(.system(.body, design: .monospaced))
                    .padding()
                    .background(Color.black.opacity(0.8))
                    .foregroundColor(.green)
                    .cornerRadius(8)
            }
            
            HStack(spacing: 20) {
                Button(action: {
                    emulatorManager.loadROM()
                }) {
                    Label("Load ROM", systemImage: "folder")
                }
                
                Button(action: {
                    emulatorManager.reset()
                }) {
                    Label("Reset", systemImage: "arrow.counterclockwise")
                }
                
                Button(action: {
                    emulatorManager.step()
                }) {
                    Label("Step", systemImage: "arrow.right")
                }
                .disabled(emulatorManager.isRunning)
                
                Toggle("Debug", isOn: $emulatorManager.showDebug)
            }
            .padding()
        }
        .padding()
        .frame(minWidth: 800, minHeight: 700)
    }
}

#Preview {
    ContentView()
}
