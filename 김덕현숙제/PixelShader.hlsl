cbuffer TEST_B0 : register(b0)
{
    float4x4 gWorldViewProj;
    float4x4 gTexTransform;
    float4x4 gMatTransform;
    float4x4 gBoneTransforms[96];
};

cbuffer TEST_B1 : register(b1)
{
    float4 offset1;
};

Texture2D tex_0 : register(t0);
SamplerState sam_0 : register(s0);

struct VS_IN
{
    //float3 pos : POSITION;
    //float4 normal : NORMAL;
    //float2 uv : TEXCOORD;

    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv    : TEXCOORD;
    float3 TangentL : TANGENT;
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices  : BONEINDICES;
};

struct VS_OUT
{
    //float4 pos : SV_Position;
    //float4 normal : NORMAL;
    //float2 uv : TEXCOORD;

    float4 pos    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH   : POSITION1;
    float3 PosW    : POSITION2;
    float3 normal : NORMAL;
    float3 TangentW : TANGENT;
    float2 uv    : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0.0f;

    //애니메이션
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = input.BoneWeights.x;
    weights[1] = input.BoneWeights.y;
    weights[2] = input.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        // Assume no nonuniform scaling when transforming normals, so 
        // that we do not have to use the inverse-transpose.

        posL += weights[i] * mul(float4(input.pos, 1.0f), gBoneTransforms[input.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(input.normal, (float3x3)gBoneTransforms[input.BoneIndices[i]]);
        tangentL += weights[i] * mul(input.TangentL.xyz, (float3x3)gBoneTransforms[input.BoneIndices[i]]);
    }

    input.pos = posL;
    input.normal = normalL;
    input.TangentL.xyz = tangentL;

    output.pos = mul(float4(input.pos, 1.f), gWorldViewProj);
    //output.pos += offset0;
    output.normal = input.normal;

    float4 texC = mul(float4(input.uv, 0.0f, 1.0f), gTexTransform);
    output.uv = mul(texC, gMatTransform).xy;

    return output;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 color = tex_0.Sample(sam_0, input.uv);
    return color;
}