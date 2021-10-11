#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <librealsense2/rs.hpp>

class RealSenseCam
{
public:
	HRESULT Init();
	void UnInit();
	void GetCamFrame(BYTE* frameBuffer, int frameSize);
private:
	rs2::pipeline *m_pPipeline;
	rs2::pipeline_profile *m_pProfile;
	rs2::config *m_pCfg;
	rs2::colorizer *m_pColorizer;             // Helper to colorize depth images
	// Define the align object. It will be used to align to depth viewport.
	rs2::align *m_pAlignToDepth;

};
