#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <librealsense2/rs.hpp>

enum RealSenseCamType
{
	IR,
	Color,
	ColorizedDepth,
	ColorAlignedDepth,
	PointCloud,
	PointCloudIR,
	PointCloudColor
};

class RealSenseCam
{
public:
	HRESULT Init(RealSenseCamType type);
	void UnInit();
	void GetCamFrame(BYTE* frameBuffer, int frameSize);

private:
	RealSenseCamType m_Type;		// which type of stream to make
	rs2::pipeline *m_pPipeline;
	rs2::align *m_pAlignToDepth;	// Define the align object. It will be used to align RGB to depth TODO: necessary, if using point cloud map_to?
	rs2::pointcloud *m_pPointCloud;	// RS2 pointcloud helper
	rs2::points* m_pPoints;			// persist the points between frames in case we want to display again
	rs2::colorizer* m_pColorizer;	// Helper to colorize depth images - not needed when RGB colors are used

	// helper functions for mapping RS frames to output directshow frames (includes inverting etc.)
	void invert8bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
	void invert24bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
};
