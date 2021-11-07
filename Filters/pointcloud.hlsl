Texture2D colorTex: register(t0);
SamplerState colorTexSampler : register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    matrix worldViewProj;
}

/* vertex attributes go here to input to the vertex shader */
struct vs_in {
    float3 position_local : SV_POSITION;
    float2 color_tex_uv : TEXCOORD0;
};

/* outputs from vertex shader go here. can be interpolated to pixel shader */
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
    float2 color_tex_uv : TEXCOORD0;
};

vs_out vs_main(vs_in input) {
    // TODO if calculating point cloud geometry, use SV_VertexID %/div width 
    // to compute x,y position of vertex #ID, then we only need to pass depth frame
    // And then fill a separate color buffer as well (or texcoord and texture)

    // for now though just convert the inputs to float4s and transform vertex position
    vs_out output = (vs_out)0;          // zero the memory first
    output.position_clip = mul(float4(input.position_local, 1.0), worldViewProj);
    output.color_tex_uv = input.color_tex_uv;
    return output;
}

float4 ps_main(vs_out input) : SV_TARGET {
    // sample the texture to assign the color
    return colorTex.Sample(colorTexSampler, input.color_tex_uv); // must return an RGBA colour
}
