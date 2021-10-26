#include "RealSenseCam.h"
#include "Projection.hpp"
#include <cassert>
//#include "Projection.hpp"

RealSenseCam::RealSenseCam() : m_AlignToDepth(RS2_STREAM_DEPTH), m_Type(RealSenseCamType::PointCloudColor), mOutputWidth(640), mOutputHeight(480)
{
	// TODO these are only currently needed for Pointcloud types, not video/depth/IR/colorized depth
	m_pApp = new window(1280, 720, "RealSense Pointcloud Example");
	m_pViewState = new glfw_state();
}

RealSenseCam::~RealSenseCam()
{
	if (m_pApp) delete m_pApp;
	if (m_pViewState) delete m_pViewState;
}

HRESULT RealSenseCam::Init(RealSenseCamType type)
{
	m_Type = type;

	// TODO work out how to get this logging into Debug Output console in VS2019
	rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
	rs2::log_to_file(rs2_log_severity::RS2_LOG_SEVERITY_ALL,
		"librealsense.log");
	rs2::log(RS2_LOG_SEVERITY_DEBUG, "Starting Init()");

	// Set up the persistent objects used by the RealSenseCam
	//m_pPipeline = new rs2::pipeline();
	// set up the config object for the desired device/streams pipeline
	rs2::config Cfg;
	//pCfg->disable_all_streams(); // TODO this should be safe enough since I'm enabling the ones I need after

	switch (m_Type)
	{
	case RealSenseCamType::IR:
		Cfg.enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);  // TODO won't need this with RGB
		mOutputWidth = 320;
		mOutputHeight = 240;
		break;
	case RealSenseCamType::Color:
		Cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		mOutputWidth = 320;
		mOutputHeight = 240;
		break;
	case RealSenseCamType::ColorizedDepth:
		Cfg.enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		mOutputWidth = 320;
		mOutputHeight = 240;
		break;
	case RealSenseCamType::ColorAlignedDepth:
		Cfg.enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		Cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams cause the CPU to go mental if OpenMP is enabled in RS2 build
		//m_AlignToDepth.  (RS2_STREAM_DEPTH); // FIXME if we initialise in the header, how do we specify
		mOutputWidth = 320;
		mOutputHeight = 240;
		break;
	case RealSenseCamType::PointCloud:
		Cfg.enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		// Create a simple OpenGL window for rendering:
		//m_App = new window(1280, 720, "RealSense Pointcloud Example");
		//m_PointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		//m_Points = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		//m_ViewState = new glfw_state();
		mOutputWidth = 640;
		mOutputHeight = 480;
		break;
	case RealSenseCamType::PointCloudIR:
		Cfg.enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		Cfg.enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);  // TODO won't need this with RGB
		//m_App = new window(1280, 720, "RealSense Pointcloud Example");
		//m_PointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		//m_Points = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		//m_ViewState = new glfw_state();
		mOutputWidth = 640;
		mOutputHeight = 480;
		break;
	case RealSenseCamType::PointCloudColor:
		Cfg.enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		Cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		//m_App = new window(1280, 720, "RealSense Pointcloud Example");
		//m_pPointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		//m_pPoints = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		//m_pViewState = new glfw_state();
		// register callbacks to allow manipulation of the pointcloud
		register_glfw_callbacks(*m_pApp, *m_pViewState);
		mOutputWidth = 640;
		mOutputHeight = 480;
		break;
	default:
		break;
	}

	// now try to resolve the config and start!
	//if (pCfg->can_resolve(((std::shared_ptr<rs2_pipeline>) * m_pPipeline)))
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
	//if (m_Pipe != NULL)
	//{
		m_Pipe.stop();
	//}
}

void RealSenseCam::GetCamFrame(BYTE* frameBuffer, int frameSize)
{
	// just make sure that we've correctly set the output frame size
	assert(frameSize == mOutputWidth * mOutputHeight * 3);

	//if (m_pPipeline != NULL)
	//{
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
			//render_pointcloud_to_buffer(frameBuffer, frameSize, mOutputWidth, mOutputHeight, m_pViewState, m_pPoints, NULL);
			// TODO copy to framebuffer
		}
		break;
		case RealSenseCamType::PointCloudIR:
		{
			auto depth = frames.get_depth_frame();
			auto ir = frames.get_infrared_frame();
			m_PointCloud.map_to(ir);
			m_Points = m_PointCloud.calculate(depth);
			// TODO copy to framebuffer
		}
		break;
		case RealSenseCamType::PointCloudColor:
		{
			auto color = frames.get_color_frame();
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

			// TODO copy to framebuffer
			// Upload the color frame to OpenGL
			m_pViewState->tex.upload(color);

			// Draw the pointcloud
			//draw_pointcloud(m_pApp->width(), m_pApp->height(), *m_pViewState, m_Points);
			render_pointcloud_to_buffer(frameBuffer, frameSize, (float)mOutputWidth, (float)mOutputHeight, m_pApp->width(), m_pApp->height(), *m_pViewState, m_Points);
		}
		break;
		default:
			break;
		}

		// TODO - plenty:
		// fetch RGB and Depth frames as point cloud
		// project colorized point cloud to the 2D frame
		// refer to rs-gl sample for OpenGL-accelerated implementation; just copy the rendered texture back to CPU memory rather than use a gl window
		// https://github.com/IntelRealSense/librealsense/tree/master/examples/gl
	//}
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
