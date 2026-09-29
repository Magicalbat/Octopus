
static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct VertexOut {
    float3 color;
    float4 sv_pos : SV_Position;
};

[shader("vertex")]
VertexOut vertMain(uint vid: SV_VertexID) {
    VertexOut out = {
        colors[vid],
        float4(positions[vid], 0.0, 1.0),
    };

    return out;
}

[shader("fragment")]
float4 fragMain(VertexOut vert) : SV_Target {
    return float4(vert.color, 1.0);
}
