//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txDepth : register(t1);
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
matrix LightView;
float4 info;
};



//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
	float4 Norm : NORMAL0;
	float4 OPos : POSITION;
	float4 WorldPos : POSITION1;
	
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT VS(VS_INPUT input)
	{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	output.WorldPos = pos = mul(pos, World);

	output.Pos = mul(pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;
	output.Norm = normalize(mul(input.Norm, World));
	output.OPos = output.Pos;


	return output;
	}

PS_INPUT VS_screen(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	output.Pos = pos;
	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm.xyz = input.Norm;
	output.Norm.w = 1;




	return output;
}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
	float4 wpos = input.WorldPos;
	wpos = mul(wpos, LightView);
	wpos = mul(wpos, Projection);
	float shadowlight = 1.0;//1 .. no shadow
	float pixeldepth = wpos.z / wpos.w;
	float2 texdpos = wpos.xy / wpos.w;
	texdpos.x = texdpos.x*0.5 + 0.5;
	texdpos.y = texdpos.y* (-0.5) + 0.5;

	float4 depth = txDepth.SampleLevel(samLinear, texdpos, 0);

	float pxd = depth.x / depth.y;
	if (pixeldepth > (pxd + 0.000001))
		shadowlight = 0.3;

	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float3 lightposition = float3(10000, 10000, 10000);
	
	float3 d = normalize(lightposition - input.Pos);
	float3 norm = normalize(input.Norm.xyz);
	float light = saturate(dot(norm, d));
	light *= 0.7;
	light += 0.3;
	color.rgb *= light * shadowlight;

	float3 cam = float3(View._14, View._24, View._34);

	float3 dc = normalize(cam - input.Pos);
	float3 refl = reflect(norm, d);
	float spec = saturate(dot(refl, d));
	spec = pow(spec, 5);
	color.rgb += spec;
	
	if (info.r)
	{
		color.r += (input.Pos.y + 700) / 1700;
		color.g += (input.Pos.x  ) / 8000;
		color.b += (input.Pos.x ) / 8000;
	}
	// color.a = 1;

	return color;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSsky(PS_INPUT input) : SV_Target
{
	//return float4(0,1,0,1);
	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	 color.a = 1;
	 return color;
}
//--------------------------------------------------------------------------------------
float4 PS_Shadow(PS_INPUT input) : SV_Target
{
float4 texx;
float4 pos = input.OPos;
float depth = pos.z / pos.w;
texx = float4(pos.z, pos.w, depth, 1);

return texx;
}

float2 PixelOffsets[9] =
{
	{ -0.004, -0.004 },
	{ -0.003, -0.003 },
	{ -0.002, -0.002 },
	{ -0.001, -0.001 },
	{ 0.000, 0.000 },
	{ 0.001, 0.001 },
	{ 0.002, 0.002 },
	{ 0.003, 0.003 },
	{ 0.004, 0.004 },
};

static const float BlurWeights[9] =
{
	0.026995,
	0.064759,
	0.120985,
	0.176033,
	0.199471,
	0.176033,
	0.120985,
	0.064759,
	0.026995,
};

float4 PS_screen(PS_INPUT input) : SV_Target
{
	//return float4(1,0,0,1);
	//float4 texx = txDiffuse.SampleLevel(samLinear, input.Tex , 0);
	//return float4(texx.rgb, 1);

	//float4 bloom = float4(0,0,0,0);
	//float span = 7;
	////float span = 6 + 5 * sin(5*g_time.x);
	//int tt = abs(span);
	//for (float i = -tt; i <= tt; i=i+0.3)
	//	{
	//	float ofs = i;
	//	bloom += txDiffuse.Sample(samLinear, input.Tex + float2(0,ofs / 100)) / (2.0 * tt + 1.0);
	//	}
	//bloom *= bloom;
	//bloom = bloom*0.7;
	////return bloom;
	//float4 result= txDiffuse.Sample(samLinear, input.Tex) + bloom;
	//result.a = 1;
	//return result;
	//

	float4 glow = txDiffuse.SampleLevel(samLinear, input.Tex,5);

	float4 glowsum = float4(0, 0, 0, 0);
	float t = 0.002 * 2;
	for (int xx = -10; xx < 10; xx++)
		for (int yy = -10; yy < 10; yy++)
		{
			float g = txDiffuse.SampleLevel(samLinear, input.Tex + float2(t*xx,t*yy), 1).r;

			g = saturate(g - 0.5)*0.4;
			//float distance = sqrt(xx*xx + yy*yy);
			float distance = xx*xx + yy*yy;
			g = g *(196 - distance) / 196.;
			g = pow(g, 4) * 25;
			glowsum += g;

		}
	glowsum.a = 1;
	glowsum = saturate(glowsum);
	//return glowsum;
	float4 tex;
	if (!info.r)
		tex = txDiffuse.SampleLevel(samLinear, input.Tex, 0);
	else
		tex = txDiffuse.SampleLevel(samLinear, input.Tex, 4);


	float2 coord = input.Tex - float2(0.5, 0.5);
	coord *= 2.0;
	float dist = length(coord);

	
	tex += glowsum / 2;
	
	float3 bwcolor = tex.rgb;

	float aver = (bwcolor.r + bwcolor.g + bwcolor.b) / 3.0;
	bwcolor.r = aver;
	bwcolor.g = aver;
	bwcolor.b = aver;
	dist = pow(dist, 0.8);
	float3 result = bwcolor * dist + tex.rgb * (1.0 - dist);
	return float4(result, 1);

	//tex.a = 1;
	//return tex;


	//float4 texture_color;
	//float4 glow_color = float4(0, 0, 0, 0);
	//t = 0.02;
	//glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, 0), 4);
	//glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(t, 0), 4);
	//glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, t), 4);
	//glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(-t, 0), 4);
	//glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, -t), 4);
	//glow_color /= 4.;
	//
	//
	//glow_color = saturate(glow_color*2 - 0.7);
	//texture_color.a = 1;
	//texture_color.rgb += glow_color.rgb;
	//return texture_color;



	//for (int i = 0; i < 5; i++)
	//	{
	//	float3 col = txDiffuse.Sample(samLinear, input.Tex + float2(0.01,0)*i);
	//	texture_color.rgb += col * (5-i) * 0.2;
	//	}
	//return texture_color;
}

