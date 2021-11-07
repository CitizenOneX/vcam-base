#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <librealsense2/rs.hpp>
#include "PointCloudRenderer.h"

enum class RealSenseCamType
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
	RealSenseCam();
	~RealSenseCam();
	HRESULT Init(RealSenseCamType type);
	void UnInit();
	void GetCamFrame(BYTE* frameBuffer, int frameSize);

private:
	RealSenseCamType m_Type;			// which type of stream to make (IR, color, point cloud etc)
	rs2::pipeline m_Pipe;
	rs2::align m_AlignToDepth;			// Define the align object. It will be used to align RGB to depth TODO: necessary, if using point cloud map_to?
	rs2::pointcloud m_PointCloud;		// RS2 pointcloud helper
	rs2::points m_Points;				// persist the points between frames in case we want to display again
	rs2::colorizer m_Colorizer;			// Helper to colorize depth images - not needed when RGB colors are used
	int m_InputDepthWidth, m_InputDepthHeight;	// Dimensions of the depth input frame
	int m_InputTexWidth, m_InputTexHeight;	// Dimensions of the color/IR texture input frame
	int m_OutputWidth, m_OutputHeight;	// Dimensions of the output video frame (can be different to input frame size for point cloud types)
										// Needs to match what gets provided in output media sample frame buffer!
	PointCloudRenderer m_Renderer;		// Custom class that uses Direct3D to project point cloud data to a texture and copy back to the frame

	// helper functions for mapping RS frames to output directshow frames (includes inverting etc.)
	void invert8bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
	void invert24bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
};
