#include "RealSenseCam.h"

HRESULT RealSenseCam::Init()
{
	// TODO might use data from the profile at initialisation, e.g. frame dimensions etc.
	m_pColorizer = new rs2::colorizer();
	m_pPipeline = new rs2::pipeline();

	m_pCfg = new rs2::config();
	//m_pCfg->enable_stream(RS2_STREAM_DEPTH);
	m_pCfg->enable_stream(RS2_STREAM_INFRARED);
	m_pProfile = &m_pPipeline->start();//*m_pCfg);

	//m_pAlignToDepth = new rs2::align(RS2_STREAM_DEPTH);

	return *m_pProfile ? S_OK : E_FAIL;
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
	// Block program until frames arrive
	rs2::frameset frames = m_pPipeline->wait_for_frames();

	// align the color frame to the depth frame (so we end up with the smaller depth frame with color mapped onto it)
	//frames = m_pAlignToDepth->process(frames);
	//auto depth = frames.get_depth_frame();
	// colorize the depth data with the default color map
	//auto colorized_depth = m_pColorizer->colorize(depth);
	auto ir = frames.get_infrared_frame();

	// TODO For now just copy the colorized depth frame over to the framebuffer
	// wait, this could have been a single memcpy...
	//for (int i = 0; i < min(frameSize, colorized_depth.get_data_size()); i++)
	//{
	//	frameBuffer[i] = ((BYTE*)colorized_depth.get_data())[i];
	//}
	//memcpy(frameBuffer, colorized_depth.get_data(), min(frameSize, colorized_depth.get_data_size()));
	memcpy(frameBuffer, ir.get_data(), min(frameSize, ir.get_data_size()));

	// TODO - plenty:
	// fetch RGB and Depth frames as point cloud
	// project colorized point cloud to the 2D frame

}