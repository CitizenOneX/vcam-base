#include "RealSenseCam.h"

HRESULT RealSenseCam::Init()
{
	// TODO work out how to get this logging into Debug Output console in VS2019
	rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
	rs2::log(RS2_LOG_SEVERITY_DEBUG, "Starting Init()");

	// TODO might use data from the profile at initialisation, e.g. frame dimensions etc.
	m_pColorizer = new rs2::colorizer();
	m_pPipeline = new rs2::pipeline();

	m_pCfg = new rs2::config();
	m_pCfg->disable_all_streams();
	m_pCfg->enable_stream(RS2_STREAM_DEPTH, 320, 240, RS2_FORMAT_Z16, 30);
	m_pCfg->enable_stream(RS2_STREAM_INFRARED, 320, 240, RS2_FORMAT_Y8, 30);
	// TODO rebuild librealsense without OpenMP, and link with Release lib to avoid 100% CPU utilisation when using color stream
	//m_pCfg->enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_ANY, 30);

	if (m_pCfg->can_resolve(((std::shared_ptr<rs2_pipeline>)*m_pPipeline)))
	{
		m_pProfile = &m_pPipeline->start(*m_pCfg);  
		// TODO I think this profile isn't the same as what is returned by get_active_profile()...
		// Could be that I'm not handling the std::shared_ptr<rs2_pipeline_profile>() smart pointer business properly
		
		// Debug logging to work out which streams we got in our profile
		OutputDebugStringA("Pipeline Profile: \n");
		rs2::pipeline_profile activeProfile = m_pPipeline->get_active_profile();

		OutputDebugStringA("- Device: ");
		OutputDebugStringA(activeProfile.get_device().get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME));
		OutputDebugStringA("\n");

		if (*m_pProfile)
		{
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

			m_pAlignToDepth = new rs2::align(RS2_STREAM_DEPTH);

			return S_OK;
		}
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
		frames = m_pAlignToDepth->process(frames);
		auto depth = frames.get_depth_frame();

		// colorize the depth data with the default color map
		auto colorized_depth = m_pColorizer->colorize(depth);
		//auto colorized_depth = depth;
		//auto ir = frames.get_infrared_frame();

		// TODO For now just copy the colorized depth frame over to the framebuffer
		// wait, this could have been a single memcpy...
		//for (int i = 0; i < min(frameSize, colorized_depth.get_data_size()); i++)
		//{
		//	frameBuffer[i] = ((BYTE*)colorized_depth.get_data())[i];
		//}
		memcpy(frameBuffer, colorized_depth.get_data(), min(frameSize, colorized_depth.get_data_size()));
		//memcpy(frameBuffer, ir.get_data(), min(frameSize, ir.get_data_size()));
		//memcpy(frameBuffer, depth.get_data(), min(frameSize, depth.get_data_size()));

		// TODO - plenty:
		// fetch RGB and Depth frames as point cloud
		// project colorized point cloud to the 2D frame
		// refer to rs-gl sample for OpenGL-accelerated implementation; just copy the rendered texture back to CPU memory rather than use a gl window
		// https://github.com/IntelRealSense/librealsense/tree/master/examples/gl
	}
}