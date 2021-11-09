Texture2D colorTex: register(t0);
SamplerState colorTexSampler : register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

/* outputs from vertex shader go here. can be interpolated to pixel shader */
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
    float2 color_tex_uv : TEXCOORD0;
};

float4 main(vs_out input) : SV_TARGET {
    // sample the texture to assign the color
    return colorTex.Sample(colorTexSampler, input.color_tex_uv); // must return an RGBA colour
}
