//
//  Shader.metal
//  SNES_For_Mac
//
//  Created by Haide Lan on 2025/11/14.
//

#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]], constant VertexIn* vertices [[buffer(0)]]) {
    VertexOut out;
    out.position = float4(vertices[vertexID].position, 0.0, 1.0);
    out.texCoord = vertices[vertexID].texCoord;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]], texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);
    float4 color = colorTexture.sample(textureSampler, in.texCoord);
    return color;
}
