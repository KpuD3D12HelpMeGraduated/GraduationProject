cbuffer TEST_B0 : register(b0)
{
    float4x4 gWorldViewProj;
};

cbuffer TEST_B1 : register(b1)
{
    float4 offset1;
};

Texture2D tex_0 : register(t0);
SamplerState sam_0 : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    output.pos = mul(float4(input.pos, 1.f), gWorldViewProj);
    output.normal = input.normal;
    output.uv = input.uv;

    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 color = tex_0.Sample(sam_0, input.uv);
    return color;
}