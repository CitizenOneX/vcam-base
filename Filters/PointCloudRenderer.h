#pragma once

#ifndef UNICODE
#define UNICODE
#endif 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>          // D3D interface
#include <DirectXMath.h>    // matrix/vector math

// TODO why am I getting rs2 to calculate point cloud and return a flat list of 3d vertices; losing the RGB-D structure?
// why not just load the depth and colour textures natively and compute vertices and color texture sampling in a shader?
// well, intrinsics/extrinsics/distortion models are all taken care of by rs2 point cloud calculator, for starters
class PointCloudRenderer
{
public:
	PointCloudRenderer();
	~PointCloudRenderer();

	// TODO really here I just need to know the vertex structure (if we're going with that)
	// TODO just uses a default camera position, lookat, up - for now
	HRESULT Init(int inputDepthWidth, int inputDepthHeight, int inputTexWidth, int inputTexHeight, int outputWidth, int outputHeight, float clippingDistanceZ);

	void UnInit();

	// TODO vertex structures with colour? Separate streams? 
	// TODO really can pass current depth and color frames and let the shader work out the points, not librealsense
	// TODO pass near/far clipping, other thresholding?
	void RenderFrame(BYTE* outputFrameBuffer, const int outputFrameLength, const unsigned int pointsCount, const float* pointsXyz, const float* texUvs, const void* color_frame_data, const int color_frame_size);

private:
	// D3D globals
	ID3D11Device* device_ptr = NULL;
	ID3D11DeviceContext* device_context_ptr = NULL;
	ID3D11Texture2D* target_ptr = NULL;				// render target texture
	ID3D11Texture2D* depth_stencil_ptr = NULL;		// render target depth stencil texture
	ID3D11Texture2D* staging_ptr = NULL;			// staging copy of render target to pass back to CPU
	ID3D11RenderTargetView* render_target_view_ptr = NULL;
	ID3D11VertexShader* vertex_shader_ptr = NULL;
	ID3D11PixelShader* pixel_shader_ptr = NULL;
	ID3D11InputLayout* input_layout_ptr = NULL;
	ID3D11Buffer* vertex_buffer_ptr = NULL;
	ID3D11Buffer* constant_buffer_ptr = NULL;
	ID3D11Texture2D* color_tex_ptr = NULL;			// RGB(A)?8-formatted data per point (comes off the sensor as RGB8)
	ID3D11ShaderResourceView* tex_view_ptr = NULL;
	ID3D11SamplerState* sampler_state_ptr = NULL;
	ID3D11DepthStencilState* depth_stencil_state_ptr = NULL;
	ID3D11DepthStencilView* depth_stencil_view_ptr = NULL;
	DirectX::XMMATRIX world; 
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMVECTOR eyePos;
	DirectX::XMVECTOR lookAtPos;
	DirectX::XMVECTOR upVector;

	UINT m_InputDepthWidth;
	UINT m_InputDepthHeight;
	UINT m_InputTexWidth;
	UINT m_InputTexHeight;
	UINT m_OutputWidth;
	UINT m_OutputHeight;
	float m_ClippingDistanceZ;
	float* m_BackgroundColor;

	void convert32bppToRGB(BYTE* frameBuffer, int frameSize, BYTE* pData, int pixelCount);
};

