struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D ssaoInput;
SamplerState g_sampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float2 dim;
    ssaoInput.GetDimensions(dim.x, dim.y);

    float2 texelSize = 1.0 / dim.xy;
    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += ssaoInput.Sample(g_sampler, input.texCoord + offset).r;
        }
    }
    result /= (4.0 * 4.0);
    return float4(result, result, result, 0);
}
