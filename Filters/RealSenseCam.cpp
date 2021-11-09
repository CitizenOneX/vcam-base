#include "RealSenseCam.h"

#include <cassert>

// rather than add to library list in program settings, just add the library dependencies here
#pragma comment(lib, "d3d11")           // direct3D library
#pragma comment(lib, "d3dcompiler")     // shader compiler


RealSenseCam::RealSenseCam() : m_AlignToDepth(RS2_STREAM_DEPTH), m_Type(RealSenseCamType::PointCloudColor), m_InputDepthWidth(320), m_InputDepthHeight(240), m_InputTexWidth(640), m_InputTexHeight(480), m_OutputWidth(640), m_OutputHeight(480)
{
}

RealSenseCam::~RealSenseCam()
{
}

HRESULT RealSenseCam::Init(RealSenseCamType type)
{
	m_Type = type;
	// clip out all points more distant than this in meters
	float clippingDistanceZ = 1.3f;

	// TODO work out how to get this logging into Debug Output console in VS2019
	rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
	rs2::log_to_file(rs2_log_severity::RS2_LOG_SEVERITY_ALL, "librealsense.log");
	rs2::log(RS2_LOG_SEVERITY_DEBUG, "Starting Init()");

	// set up the Realsense config object for the desired device/streams pipeline
	rs2::config Cfg;

	switch (m_Type)
	{
	case RealSenseCamType::IR:
		m_InputTexWidth = 320;
		m_InputTexHeight = 240;
		m_OutputWidth = 320;
		m_OutputHeight = 240;
		Cfg.enable_stream(RS2_STREAM_INFRARED, m_InputTexWidth, m_InputTexHeight, RS2_FORMAT_Y8, 30);
		break;
	case RealSenseCamType::Color:
		m_InputTexWidth = 640;
		m_InputTexHeight = 480;
		m_OutputWidth = 640;
		m_OutputHeight = 480;
		// remember color streams go mental if OpenMP is enabled in RS2 build
		Cfg.enable_stream(RS2_STREAM_COLOR, m_InputTexWidth, m_InputTexHeight, RS2_FORMAT_RGB8, 30);
		break;
	case RealSenseCamType::ColorizedDepth:
		m_InputDepthWidth = 320;
		m_InputDepthHeight = 240;
		m_OutputWidth = 320;
		m_OutputHeight = 240;
		Cfg.enable_stream(RS2_STREAM_DEPTH, m_InputDepthWidth, m_InputDepthHeight, RS2_FORMAT_Z16, 30);
		break;
	case RealSenseCamType::ColorAlignedDepth:
		m_InputDepthWidth = 320;
		m_InputDepthHeight = 240;
		m_InputTexWidth = 640;
		m_InputTexHeight = 480;
		m_OutputWidth = 320;
		m_OutputHeight = 240;
		Cfg.enable_stream(RS2_STREAM_DEPTH, m_InputDepthWidth, m_InputDepthHeight, RS2_FORMAT_Z16, 30);
		// remember color streams cause the CPU to go mental if OpenMP is enabled in RS2 build
		Cfg.enable_stream(RS2_STREAM_COLOR, m_InputTexWidth, m_InputTexHeight, RS2_FORMAT_ANY, 30);
		break;
	case RealSenseCamType::PointCloud:
		m_InputDepthWidth = 320;
		m_InputDepthHeight = 240;
		m_InputTexWidth = 320;
		m_InputTexHeight = 240;
		m_OutputWidth = 640;
		m_OutputHeight = 480;
		Cfg.enable_stream(RS2_STREAM_DEPTH, m_InputDepthWidth, m_InputDepthHeight, RS2_FORMAT_Z16, 30);
		m_Renderer.Init(m_InputDepthWidth, m_InputDepthHeight, m_InputTexWidth, m_InputTexHeight, m_OutputWidth, m_OutputHeight, clippingDistanceZ);
		break;
	case RealSenseCamType::PointCloudIR:
		m_InputDepthWidth = 320;
		m_InputDepthHeight = 240;
		m_InputTexWidth = 320;
		m_InputTexHeight = 240;
		m_OutputWidth = 640;
		m_OutputHeight = 480;
		Cfg.enable_stream(RS2_STREAM_DEPTH, m_InputDepthWidth, m_InputDepthHeight, RS2_FORMAT_Z16, 30);
		Cfg.enable_stream(RS2_STREAM_INFRARED, m_InputTexWidth, m_InputTexHeight, RS2_FORMAT_Y8, 30);
		// No need for AlignTo - IR is automatically aligned with depth
		m_Renderer.Init(m_InputDepthWidth, m_InputDepthHeight, m_InputTexWidth, m_InputTexHeight, m_OutputWidth, m_OutputHeight, clippingDistanceZ);
		break;
	case RealSenseCamType::PointCloudColor:
		m_InputDepthWidth = 320;
		m_InputDepthHeight = 240;
		m_InputTexWidth = 640;
		m_InputTexHeight = 480;
		m_OutputWidth = 640;
		m_OutputHeight = 480;
		Cfg.enable_stream(RS2_STREAM_DEPTH, m_InputDepthWidth, m_InputDepthHeight, RS2_FORMAT_Z16, 30);
		Cfg.enable_stream(RS2_STREAM_COLOR, m_InputTexWidth, m_InputTexHeight, RS2_FORMAT_RGBA8, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		m_Renderer.Init(m_InputDepthWidth, m_InputDepthHeight, m_InputTexWidth, m_InputTexHeight, m_OutputWidth, m_OutputHeight, clippingDistanceZ);
		break;
	default:
		assert(false);
	}

	// now try to resolve the config and start!
	if (Cfg.can_resolve(((std::shared_ptr<rs2_pipeline>)m_Pipe)))
		{
		m_Pipe.start(Cfg);

		// Debug logging to work out which devices/streams we got in our profile
		OutputDebugStringA("Pipeline Profile: \n");
		rs2::pipeline_profile activeProfile = m_Pipe.get_active_profile();

		OutputDebugStringA("- Device: ");
		OutputDebugStringA(activeProfile.get_device().get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME));
		OutputDebugStringA("\n");

		// now log each of the streams
		auto streamProfiles = activeProfile.get_streams();

		for each (auto streamProfile in streamProfiles)
		{
			OutputDebugStringA("- Stream: ");
			OutputDebugStringA(rs2_stream_to_string(streamProfile.stream_type()));
			OutputDebugStringA(", ");
			char buffer[4];
			OutputDebugStringA(_itoa(streamProfile.fps(), buffer, 10));
			OutputDebugStringA(" fps, ");
			OutputDebugStringA(rs2_format_to_string(streamProfile.format()));
			OutputDebugStringA("\n");
		}

		return S_OK;
	}

	return E_FAIL;
}

void RealSenseCam::UnInit()
{
	// uninit the point cloud renderer if it was initialized
	if (m_Type == RealSenseCamType::PointCloud || m_Type == RealSenseCamType::PointCloudIR || m_Type == RealSenseCamType::PointCloudColor)
	{
		m_Renderer.UnInit();
	}

	// stop the realsense pipeline
	m_Pipe.stop();
}

void RealSenseCam::GetCamFrame(BYTE* frameBuffer, int frameSize)
{
	// just make sure that we've correctly set the output frame size
	assert(frameSize == m_OutputWidth * m_OutputHeight * 3);

	// Block program until frames arrive if we need to, but take the most recent and discard older frames
	rs2::frameset frames = m_Pipe.wait_for_frames();

	switch (m_Type)
	{
	case RealSenseCamType::IR:
	{
		// IR is 1 byte per pixel so we need to copy to R, G and B
		// might as well invert while we're there
		auto ir = frames.get_infrared_frame();
		invert8bppToRGB(frameBuffer, frameSize, ir);
	}
	break;
	case RealSenseCamType::Color:
	{
		auto color = frames.get_color_frame();
		invert24bppToRGB(frameBuffer, frameSize, color);
	}
	break;
	case RealSenseCamType::ColorizedDepth:
	{
		auto colorized_depth = m_Colorizer.colorize(frames.get_depth_frame());
		invert24bppToRGB(frameBuffer, frameSize, colorized_depth);
	}
	break;
	case RealSenseCamType::ColorAlignedDepth:
	{
		// align the color frame to the depth frame (so we end up with the smaller depth frame with color mapped onto it)
		// TODO color frames will only be reenabled after I rebuild realsense with OpenMP set to FALSE, since it results
		// in 100% CPU utilisation when handling color frames by the looks
		frames = m_AlignToDepth.process(frames);
		auto color = frames.get_color_frame();
		invert24bppToRGB(frameBuffer, frameSize, color);
	}
	break;
	case RealSenseCamType::PointCloud:
	{
		auto depth = frames.get_depth_frame();
		m_Points = m_PointCloud.calculate(depth);
		rs2_error* e = nullptr;
		int pointsCount = rs2_get_frame_points_count((rs2_frame*)m_Points, &e);
		if (e != NULL)
		{
			OutputDebugStringA("Error calculating points: \n");
			OutputDebugStringA(rs2_get_error_message(e));
		}
		// Upload the vertices to Direct3D
		// Draw the pointcloud and copy to the framebuffer
		m_Renderer.RenderFrame(frameBuffer, frameSize, pointsCount, (const float*)m_Points.get_vertices(), (const float*)m_Points.get_texture_coordinates(), NULL, 0); // FIXME no color frame
	}
	break;
	case RealSenseCamType::PointCloudIR:
	{
		auto depth = frames.get_depth_frame();
		auto ir = frames.get_infrared_frame();
		m_PointCloud.map_to(ir);
		m_Points = m_PointCloud.calculate(depth);
		rs2_error* e = nullptr;
		int pointsCount = rs2_get_frame_points_count((rs2_frame*)m_Points, &e);
		if (e != NULL)
		{
			OutputDebugStringA("Error calculating points: \n");
			OutputDebugStringA(rs2_get_error_message(e));
		}
		// Upload the vertices to Direct3D
		// Draw the pointcloud and copy to the framebuffer
		m_Renderer.RenderFrame(frameBuffer, frameSize, pointsCount, (const float*)m_Points.get_vertices(), (const float*)m_Points.get_texture_coordinates(), ir.get_data(), ir.get_data_size());
	}
	break;
	case RealSenseCamType::PointCloudColor:
	{
		rs2::video_frame color = frames.get_color_frame();
		m_PointCloud.map_to(color);
		auto depth = frames.get_depth_frame();
		m_Points = m_PointCloud.calculate(depth);
		rs2_error* e = nullptr;
		int pointsCount = rs2_get_frame_points_count((rs2_frame*)m_Points, &e);
		if (e != NULL)
		{
			OutputDebugStringA("Error calculating points: \n");
			OutputDebugStringA(rs2_get_error_message(e));
		}

		// Upload the vertices to Direct3D
		// Draw the pointcloud and copy to the framebuffer
		m_Renderer.RenderFrame(frameBuffer, frameSize, pointsCount, (const float*)m_Points.get_vertices(), (const float*)m_Points.get_texture_coordinates(), color.get_data(), color.get_data_size());
	}
	break;
	default:
		break;
	}
}

/// <summary>
/// assuming the 8bits per pixel is an IR intensity value
/// then replicate it in the R, G and B bytes of the output frame buffer
/// as we invert the image
/// </summary>
/// <param name="frameBuffer">output buffer, 24bpp</param>
/// <param name="frameSize">output buffer size in bytes</param>
/// <param name="frame">input video frame</param>
void RealSenseCam::invert8bppToRGB(BYTE * frameBuffer, int frameSize, rs2::video_frame frame)
{
	int pixelCount = frame.get_height() * frame.get_width();
	auto data = (BYTE*)frame.get_data();
	for (int i = 0; i < pixelCount; ++i)
	{
		BYTE val = data[(pixelCount - i) - 1];
		frameBuffer[3 * i] = val;
		frameBuffer[3 * i + 1] = val;
		frameBuffer[3 * i + 2] = val;
	}
}

/// <summary>
/// assuming the 24bits per pixel is an RGB value
/// then replicate it in the R, G and B bytes of the output frame buffer
/// as we invert the image. Don't flip the bytes around to BGR though.
/// </summary>
/// <param name="frameBuffer">output buffer, 24bpp</param>
/// <param name="frameSize">output buffer size in bytes</param>
/// <param name="frame">input video frame</param>
void RealSenseCam::invert24bppToRGB(BYTE * frameBuffer, int frameSize, rs2::video_frame frame)
{
	int pixelCount = frame.get_height() * frame.get_width();
	auto data = (BYTE*)frame.get_data();
	for (int i = 0; i < pixelCount; ++i)
	{
		frameBuffer[3 * i] = data[3 * (pixelCount - i) - 1];
		frameBuffer[3 * i + 1] = data[3 * (pixelCount - i) - 2];
		frameBuffer[3 * i + 2] = data[3 * (pixelCount - i) - 3];
	}
}
