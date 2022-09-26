//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4x4 gTexTransform;
};

Texture2D    gDiffuseMap : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static float4 test = float4(1.f, 1.f, 1.f, 1.f );

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : NORMAL;
	float2 TexC : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : NORMAL;
	float2 TexC : TEXCOORD;
};


float4 Lighting(VertexOut pin) {
	//단위벡터어로 맹글어
	float3 vCameraPosition = float3(1.f, 1.f, 1.f);
	//월드좌표 = 카메라 좌표 겟또
	vCameraPosition = mul(float4(vCameraPosition,1.f), gWorldViewProj);

	//이게 최종적인 현재 색상
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

	//
	float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);


	return diffuseAlbedo;

}

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = texC;//mul(texC, gMatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	//
	//float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
	
	//노말벡터 정규화
	pin.Color = normalize(pin.Color);

	//위치, 정규화된 노멀, 텍스쳐까지 넘김
	float4 tmp = Lighting(pin);

	
	
	return tmp;
	//return diffuseAlbedo;
	//return pin.Color;
}


