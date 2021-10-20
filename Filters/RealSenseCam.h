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
	rs2::align *m_pAlignToDepth;	// Define the align object. It will be used to align RGB to depth TODO: necessary, if using point cloud map_to?
	rs2::pointcloud *m_pPointCloud;	// RS2 pointcloud helper
	rs2::points* m_pPoints; // persist the points between frames in case we want to display again

	//rs2::colorizer* m_pColorizer;	// TODO Helper to colorize depth images - not needed when RGB colors are used
};
