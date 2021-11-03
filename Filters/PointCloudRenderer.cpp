#include "PointCloudRenderer.h"

#include <d3dcompiler.h>    // shader compiler
#include <DirectXMath.h>    // matrix/vector math
#include <cassert>

PointCloudRenderer::PointCloudRenderer() : m_InputWidth(0), m_InputHeight(0), m_OutputWidth(0), m_OutputHeight(0)
{
}

PointCloudRenderer::~PointCloudRenderer()
{
}

HRESULT PointCloudRenderer::Init(int inputWidth, int inputHeight, int outputWidth, int outputHeight)
{
    m_InputWidth = inputWidth;
    m_InputHeight = inputHeight;
    m_OutputWidth = outputWidth;
    m_OutputHeight = outputHeight;

    // Set up Direct3D Device and Device Context
    {
        D3D_FEATURE_LEVEL feature_level;
        UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        HRESULT hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            flags,
            NULL,
            0,
            D3D11_SDK_VERSION,
            &device_ptr,
            &feature_level,
            &device_context_ptr);
        assert(S_OK == hr && device_ptr && device_context_ptr);

        // Create the render target texture (which will be copied back to caller's output frame)
        D3D11_TEXTURE2D_DESC desc_target = {};
        desc_target.Width = outputWidth;
        desc_target.Height = outputHeight;
        desc_target.ArraySize = 1;
        desc_target.SampleDesc.Count = 1;
        desc_target.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // .... DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc_target.BindFlags = D3D11_BIND_RENDER_TARGET;
        desc_target.Usage = D3D11_USAGE_DEFAULT;
        hr = device_ptr->CreateTexture2D(&desc_target, nullptr, &target_ptr);
        assert(SUCCEEDED(hr));

        // Create the Staging texture, we resource-copy GPU->GPU from target to staging, then read from staging
        // at our leisure
        D3D11_TEXTURE2D_DESC desc_staging = {};
        desc_staging.Width = outputWidth;
        desc_staging.Height = outputHeight;
        desc_staging.ArraySize = 1;
        desc_staging.SampleDesc.Count = 1;
        desc_staging.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // .... DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc_staging.BindFlags = 0;
        desc_staging.Usage = D3D11_USAGE_STAGING;
        desc_staging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        hr = device_ptr->CreateTexture2D(&desc_staging, nullptr, &staging_ptr);
        assert(SUCCEEDED(hr));

        // create and set the render target view
        hr = device_ptr->CreateRenderTargetView(target_ptr, nullptr, &render_target_view_ptr);
        assert(SUCCEEDED(hr));
        device_context_ptr->OMSetRenderTargets(1, &render_target_view_ptr, nullptr);
    }

    // Compile the Shaders
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3DCOMPILE_DEBUG; // add more debug output
#endif
        ID3DBlob* vs_blob_ptr = NULL, * ps_blob_ptr = NULL, * error_blob = NULL;

        // COMPILE VERTEX SHADER
        HRESULT hr = D3DCompileFromFile(
            L"pointcloud.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "vs_main",
            "vs_5_0",
            flags,
            0,
            &vs_blob_ptr,
            &error_blob);
        if (FAILED(hr)) {
            if (error_blob) {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            if (vs_blob_ptr) { vs_blob_ptr->Release(); }
            assert(false);
        }

        // COMPILE PIXEL SHADER
        hr = D3DCompileFromFile(
            L"pointcloud.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "ps_main",
            "ps_5_0",
            flags,
            0,
            &ps_blob_ptr,
            &error_blob);
        if (FAILED(hr)) {
            if (error_blob) {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            if (ps_blob_ptr) { ps_blob_ptr->Release(); }
            assert(false);
        }

        hr = device_ptr->CreateVertexShader(
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            NULL,
            &vertex_shader_ptr);
        assert(SUCCEEDED(hr));

        hr = device_ptr->CreatePixelShader(
            ps_blob_ptr->GetBufferPointer(),
            ps_blob_ptr->GetBufferSize(),
            NULL,
            &pixel_shader_ptr);
        assert(SUCCEEDED(hr));

        // set up input layout for vertex buffer
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            /*
            { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            */
        };
        hr = device_ptr->CreateInputLayout(
            inputElementDesc,
            ARRAYSIZE(inputElementDesc),
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            &input_layout_ptr);
        assert(SUCCEEDED(hr));
    }


    // Create dynamic vertex buffer - sized to input width x height
    {
        int arrayElementCount = m_InputWidth * m_InputHeight * 3;
        float* vertex_data_array = new float[arrayElementCount];
        ZeroMemory(vertex_data_array, arrayElementCount * sizeof(float));

        // create vertex buffer to store the vertex data
        D3D11_BUFFER_DESC vertex_buff_descr = {};
        vertex_buff_descr.ByteWidth = arrayElementCount * sizeof(float);
        vertex_buff_descr.Usage = D3D11_USAGE_DYNAMIC;
        vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertex_buff_descr.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = vertex_data_array;
        HRESULT hr = device_ptr->CreateBuffer(&vertex_buff_descr, &sr_data, &vertex_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    // constant buffer for world view projection matrix 
    {
        ID3D11Buffer* constant_buffer_ptr = NULL;
        struct VS_CONSTANT_BUFFER
        {
            DirectX::XMMATRIX mWorldViewProj;
        };

        VS_CONSTANT_BUFFER VsConstData = {};

        // Set up WVP matrix, camera details
        DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
        static DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(0.5f, 0.5f, 1.0f, 0.0f);
        static DirectX::XMVECTOR lookAtPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); //Look at center of the world
        static DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); //Positive Y Axis = Up
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(eyePos, lookAtPos, upVector);
        float fovRadians = DirectX::XM_PIDIV2; // 90 degree FOV
        float aspectRatio = static_cast<float>(m_OutputWidth) / static_cast<float>(m_OutputHeight);
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nearZ, farZ);
        VsConstData.mWorldViewProj = DirectX::XMMatrixTranspose(world * viewMatrix * projectionMatrix);

        // create the constant buffer descriptor
        D3D11_BUFFER_DESC constant_buff_descr;
        constant_buff_descr.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
        constant_buff_descr.Usage = D3D11_USAGE_DYNAMIC;        // TODO only if the camera details are dynamic
        constant_buff_descr.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constant_buff_descr.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constant_buff_descr.MiscFlags = 0;
        constant_buff_descr.StructureByteStride = 0;

        // Fill in the subresource data
        D3D11_SUBRESOURCE_DATA sr_data;
        sr_data.pSysMem = &VsConstData;
        sr_data.SysMemPitch = 0;
        sr_data.SysMemSlicePitch = 0;

        // Create the buffer
        HRESULT hr = device_ptr->CreateBuffer(
            &constant_buff_descr,
            &sr_data,
            &constant_buffer_ptr);
        assert(SUCCEEDED(hr));

        // Set the constant buffer
        device_context_ptr->VSSetConstantBuffers(0, 1, &constant_buffer_ptr);
    }

    return S_OK;;
}

void PointCloudRenderer::UnInit()
{
    if (render_target_view_ptr) render_target_view_ptr->Release();
    if (vertex_shader_ptr) vertex_shader_ptr->Release();
    if (pixel_shader_ptr) pixel_shader_ptr->Release();
    if (input_layout_ptr) input_layout_ptr->Release();
    if (vertex_buffer_ptr) vertex_buffer_ptr->Release();
    if (staging_ptr) staging_ptr->Release();
    if (target_ptr) target_ptr->Release();
    if (device_context_ptr) device_context_ptr->Release();
    if (device_ptr) device_ptr->Release();
}

void PointCloudRenderer::RenderFrame(BYTE* outputFrameBuffer, const int outputFrameLength, const float* pointsXyz, const unsigned int pointsCount)
{
    assert(outputFrameBuffer != NULL && pointsXyz != NULL && (pointsCount == m_InputWidth * m_InputHeight) && (outputFrameLength == m_OutputWidth * m_OutputHeight * 3));

    // copy/set/map the updated vertex data into the vertex buffer
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;

        //  Disable GPU access to the vertex buffer data.
        HRESULT hr = device_context_ptr->Map(vertex_buffer_ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        assert(SUCCEEDED(hr));
        //  Update the vertex buffer here.
        memcpy(mappedResource.pData, pointsXyz, (unsigned long long)pointsCount * 3 * sizeof(float));
        //  Reenable GPU access to the vertex buffer data.
        device_context_ptr->Unmap(vertex_buffer_ptr, 0);
    }

    // Direct3D rendering goes here:
#if defined( DEBUG ) || defined( _DEBUG )
    // clear the back buffer to cornflower blue for the new frame
    float background_colour[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
#else
    // clear the back buffer to black for the new frame
    float background_colour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
#endif // DEBUG

    device_context_ptr->ClearRenderTargetView(render_target_view_ptr, background_colour);

    // TODO move to Init()
    D3D11_VIEWPORT viewport = {
      0.0f,
      0.0f,
      static_cast<float>(m_OutputWidth),
      static_cast<float>(m_OutputHeight),
      0.0f,
      1.0f };

    // TODO can this just be done once in Init or is it the sort of thing that we need to check context each frame and re-set?
    device_context_ptr->RSSetViewports(1, &viewport);

    // set the output merger
    device_context_ptr->OMSetRenderTargets(1, &render_target_view_ptr, NULL);

    // set the input assembler
    UINT vertex_stride = 3 * sizeof(float);
    UINT vertex_offset = 0;
    device_context_ptr->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    device_context_ptr->IASetInputLayout(input_layout_ptr);
    device_context_ptr->IASetVertexBuffers(0, 1, &vertex_buffer_ptr, &vertex_stride, &vertex_offset);

    // set the shaders
    device_context_ptr->VSSetShader(vertex_shader_ptr, NULL, 0);
    device_context_ptr->PSSetShader(pixel_shader_ptr, NULL, 0);

    // draw the points
    UINT vertex_count = m_InputWidth * m_InputHeight;
    device_context_ptr->Draw(vertex_count, 0);

    // flush the DirectX to the render target
    device_context_ptr->Flush();

    // Copy render target texture to the staging texture
    device_context_ptr->CopyResource(staging_ptr, target_ptr);
    
    // Map/memcpy/Unmap the staging data to main memory
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = device_context_ptr->Map(staging_ptr, 0, D3D11_MAP_READ, 0, &mappedResource);
        assert(SUCCEEDED(hr));
        // pData is 32bit, outputFrameBuffer is 24bit, and one of them is BGR I think?
        convert32bppToRGB(outputFrameBuffer, outputFrameLength, (BYTE*)mappedResource.pData, m_OutputWidth * m_OutputHeight);
        device_context_ptr->Unmap(staging_ptr, 0);
    }
}

/// <summary>
/// assuming the 32bits per pixel is an RGBA value
/// then replicate it in the R, G and B bytes of the output frame buffer.
/// Also flip the bytes around to(from?) BGR.
/// </summary>
/// <param name="frameBuffer">output buffer, 24bpp</param>
/// <param name="frameSize">output buffer size in bytes</param>
/// <param name="pData">32bpp RGBA render target from Direct3D</param>
/// <param name="pixelCount">The number of pixels in the output images (same for both)</param>
void PointCloudRenderer::convert32bppToRGB(BYTE* frameBuffer, int frameSize, BYTE* pData, int pixelCount)
{
    for (int i = 0; i < pixelCount; ++i)
    {
        frameBuffer[3 * i] = pData[4 * i + 2];
        frameBuffer[3 * i + 1] = pData[4 * i + 1];
        frameBuffer[3 * i + 2] = pData[4 * i + 0];
    }
}