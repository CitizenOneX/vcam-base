#include "RealSenseCam.h"

HRESULT RealSenseCam::Init()
{
	// TODO work out how to get this logging into Debug Output console in VS2019
	rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
	rs2::log(RS2_LOG_SEVERITY_DEBUG, "Starting Init()");

	// Set up the persistent objects used by the RealSenseCam
	m_pPipeline = new rs2::pipeline();
	//m_pColorizer = new rs2::colorizer(); // TODO we won't need this in the end when we use actual RGB values aligned to depth
	m_pPointCloud = new rs2::pointcloud(); // Declare pointcloud object, for calculating pointclouds and texture mappings
	m_pPoints = new rs2::points(); // We want the points object to be persistent so we can display the last cloud when a frame drops

	// set up the config object for the desired device/streams pipeline
	rs2::config* pCfg = new rs2::config();
	pCfg->disable_all_streams();
	pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
	pCfg->enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);  // TODO won't need this with RGB
	pCfg->enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);  // remember color streams go mental if OpenMP is enabled in RS2 build

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

		// initialise the align-to-depth object (used to map RGB pixels to Depth pixels)
		m_pAlignToDepth = new rs2::align(RS2_STREAM_DEPTH);

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

void RealSenseCam::GetCamFrame(BYTE* frameBuffer, int frameSize)
{
	if (m_pPipeline != NULL)
	{
		// Block program until frames arrive if we need to, but take the most recent and discard older frames
		rs2::frameset frames = m_pPipeline->wait_for_frames();

		// align the color frame to the depth frame (so we end up with the smaller depth frame with color mapped onto it)
		// TODO color frames will only be reenabled after I rebuild realsense with OpenMP set to FALSE, since it results
		// in 100% CPU utilisation when handling color frames by the looks
		//frames = m_pAlignToDepth->process(frames);
		
		// now pull the individual frames and process
		//auto depth = frames.get_depth_frame();

		// colorize the depth data with the default color map
		//auto colorized_depth = m_pColorizer->colorize(depth);
		//auto colorized_depth = depth;

		auto ir = frames.get_infrared_frame();
		//auto color = frames.get_color_frame();
		
		//m_pPointCloud->map_to(ir);
		//m_pPoints = &m_pPointCloud->calculate(depth);

		// TODO For now just copy the colorized depth frame over to the framebuffer
		// wait, this could have been a single memcpy...
		//for (int i = 0; i < min(frameSize, colorized_depth.get_data_size()); i++)
		//{
		//	frameBuffer[i] = ((BYTE*)colorized_depth.get_data())[i];
		//}
		//memcpy(frameBuffer, colorized_depth.get_data(), min(frameSize, colorized_depth.get_data_size()));
		memcpy(frameBuffer, ir.get_data(), min(frameSize, ir.get_data_size()));
		//memcpy(frameBuffer, color.get_data(), min(frameSize, color.get_data_size()));
		//memcpy(frameBuffer, depth.get_data(), min(frameSize, depth.get_data_size()));

		// TODO - plenty:
		// fetch RGB and Depth frames as point cloud
		// project colorized point cloud to the 2D frame
		// refer to rs-gl sample for OpenGL-accelerated implementation; just copy the rendered texture back to CPU memory rather than use a gl window
		// https://github.com/IntelRealSense/librealsense/tree/master/examples/gl
	}
}