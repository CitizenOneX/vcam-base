#pragma once

#ifndef UNICODE
#define UNICODE
#endif 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>          // D3D interface

// TODO why am I getting rs2 to calculate point cloud and return a flat list of 3d vertices; losing the RGB-D structure?
// why not just load the depth and colour textures natively and compute vertices and color texture sampling in a shader?
class PointCloudRenderer
{
public:
	PointCloudRenderer();
	~PointCloudRenderer();

	// TODO really here I just need to know the vertex structure (if we're going with that)
	// TODO could also maybe pass thresholding, clipping values here if they're only set once
	// TODO currently just sets all points to a predefined color - no per-point color yet
	// TODO just uses a default camera position, lookat, up - for now
	HRESULT Init(int inputWidth, int inputHeight, int outputWidth, int outputHeight);

	void UnInit();

	// TODO vertex structures with colour? Separate streams? 
	// TODO really can pass current depth and color frames and let the shader work out the points, not librealsense
	// TODO pass near/far clipping, other thresholding?
	void RenderFrame(BYTE* outputFrameBuffer, const int outputFrameLength, const float* pointsXyz, const unsigned int pointsCount);

private:
	// D3D globals
	ID3D11Device* device_ptr = NULL;
	ID3D11DeviceContext* device_context_ptr = NULL;
	ID3D11Texture2D* target_ptr = NULL;
	ID3D11Texture2D* staging_ptr = NULL;
	ID3D11RenderTargetView* render_target_view_ptr = NULL;
	ID3D11VertexShader* vertex_shader_ptr = NULL;
	ID3D11PixelShader* pixel_shader_ptr = NULL;
	ID3D11InputLayout* input_layout_ptr = NULL;
	ID3D11Buffer* vertex_buffer_ptr = NULL;

	UINT m_InputWidth;
	UINT m_InputHeight;
	UINT m_OutputWidth;
	UINT m_OutputHeight;

	void convert32bppToRGB(BYTE* frameBuffer, int frameSize, BYTE* pData, int pixelCount);
};

