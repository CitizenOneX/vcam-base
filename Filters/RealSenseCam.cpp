#include "RealSenseCam.h"

HRESULT RealSenseCam::Init(RealSenseCamType type)
{
	m_Type = type;

	// TODO work out how to get this logging into Debug Output console in VS2019
	rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
	rs2::log(RS2_LOG_SEVERITY_DEBUG, "Starting Init()");

	// Set up the persistent objects used by the RealSenseCam
	m_pPipeline = new rs2::pipeline();
	// set up the config object for the desired device/streams pipeline
	rs2::config* pCfg = new rs2::config();
	pCfg->disable_all_streams();

	switch (m_Type)
	{
	case IR:
		pCfg->enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);  // TODO won't need this with RGB
		break;
	case Color:
		pCfg->enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		break;
	case ColorizedDepth:
		pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		m_pColorizer = new rs2::colorizer(); // TODO we won't need this in the end when we use actual RGB values aligned to depth
		break;
	case ColorAlignedDepth:
		pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		pCfg->enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		m_pAlignToDepth = new rs2::align(RS2_STREAM_DEPTH);
		break;
	case PointCloud:
		pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		m_pPointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		m_pPoints = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		break;
	case PointCloudIR:
		pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		pCfg->enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);  // TODO won't need this with RGB
		m_pPointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		m_pPoints = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		break;
	case PointCloudColor:
		pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
		pCfg->enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build
		m_pPointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
		m_pPoints = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops
		break;
	default:
		break;
	}

	// now try to resolve the config and start!
	if (pCfg->can_resolve(((std::shared_ptr<rs2_pipeline>)*m_pPipeline)))
	{
		m_pPipeline->start(*pCfg);  

		// Debug logging to work out which devices/streams we got in our profile
		OutputDebugStringA("Pipeline Profile: \n");
		rs2::pipeline_profile activeProfile = m_pPipeline->get_active_profile();

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
	if (m_pPipeline != NULL)
	{
		m_pPipeline->stop();
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
void RealSenseCam::invert8bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame)
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
void RealSenseCam::invert24bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame)
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

void RealSenseCam::GetCamFrame(BYTE* frameBuffer, int frameSize)
{
	if (m_pPipeline != NULL)
	{
		// Block program until frames arrive if we need to, but take the most recent and discard older frames
		rs2::frameset frames = m_pPipeline->wait_for_frames();

		switch (m_Type)
		{
		case IR:
			{
				// IR is 1 byte per pixel so we need to copy to R, G and B
				// might as well invert while we're there
				auto ir = frames.get_infrared_frame();
				invert8bppToRGB(frameBuffer, frameSize, ir);
			}
			break;
		case Color:
			{
				auto color = frames.get_color_frame();
				invert24bppToRGB(frameBuffer, frameSize, color);
			}
			break;
		case ColorizedDepth:
			{
				auto colorized_depth = m_pColorizer->colorize(frames.get_depth_frame());
				invert24bppToRGB(frameBuffer, frameSize, colorized_depth);
			}
			break;
		case ColorAlignedDepth:
			{
				// align the color frame to the depth frame (so we end up with the smaller depth frame with color mapped onto it)
				// TODO color frames will only be reenabled after I rebuild realsense with OpenMP set to FALSE, since it results
				// in 100% CPU utilisation when handling color frames by the looks
				frames = m_pAlignToDepth->process(frames);
				auto color = frames.get_color_frame();
				invert24bppToRGB(frameBuffer, frameSize, color);
			}
			break;
		case PointCloud:
			{
				auto depth = frames.get_depth_frame();
				m_pPoints = &m_pPointCloud->calculate(depth);
				// TODO copy to framebuffer
			}
			break;
		case PointCloudIR:
			{
				auto depth = frames.get_depth_frame();
				auto ir = frames.get_infrared_frame();
				m_pPointCloud->map_to(ir);
				m_pPoints = &m_pPointCloud->calculate(depth);
				// TODO copy to framebuffer
			}
			break;
		case PointCloudColor:
			{
				auto depth = frames.get_depth_frame();
				auto color = frames.get_color_frame();
				m_pPointCloud->map_to(color);
				m_pPoints = &m_pPointCloud->calculate(depth);
				// TODO copy to framebuffer
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
	}
}