texture preWarpBuffer;

sampler2D preWarpBufferSampler =
sampler_state
{
    Texture = <preWarpBuffer>;
    AddressU = Border;
    AddressV = Border;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
}; 

bool left;
float frustumOffset;
float barrelFactor;

float4 FrustumWarp( float2 Tex : TEXCOORD0) : COLOR
{
    float2 v = float2(Tex);
	//float s = 1.0 - ((!left)?v.x:(1.0 - v.x)) * (1.0 - frustumOffset);
	float s = 1 + ((left)?v.x:(1.0 - v.x)) * frustumOffset;
	float2 warped = float2(
        v.x, 
        0.5 + (v.y - 0.5) / s);

    return tex2D(preWarpBufferSampler, warped);
}

technique RenderFrustumWarp
{
    pass p0
    {
        VertexShader = null;
        PixelShader = compile ps_3_0 FrustumWarp();
        ZEnable = false;
    }
}

float4 BarrelWarp( float2 Tex : TEXCOORD0) : COLOR
{
    float2 v = float2(Tex);
    v *= 2.0; // [0,1]-based -> [-1,1]
    v -= 1.0;
    float2 warped = float2(
        barrelFactor * v.x / (v.y * v.y + barrelFactor), 
        barrelFactor * v.y / (v.x * v.x + barrelFactor));
    warped += 1.0; // [-1,1] back into [0,1]
    warped *= 0.5;

    return tex2D(preWarpBufferSampler, warped);
}

technique RenderBarrelWarp
{
    pass p0
    {
        VertexShader = null;
        PixelShader = compile ps_3_0 BarrelWarp();
        ZEnable = false;
    }
}
