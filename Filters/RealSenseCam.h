#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <librealsense2/rs.hpp>

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

// forward declaration here so we can keep a pointer in this class
struct glfw_state;
class window;

class RealSenseCam
{
public:
	HRESULT Init(RealSenseCamType type);
	void UnInit();
	void GetCamFrame(BYTE* frameBuffer, int frameSize);

private:
	RealSenseCamType m_Type;			// which type of stream to make (IR, color, point cloud etc)
	rs2::pipeline m_Pipe;
	//rs2::pipeline *m_pPipeline;
	rs2::align *m_pAlignToDepth;		// Define the align object. It will be used to align RGB to depth TODO: necessary, if using point cloud map_to?
	rs2::pointcloud *m_pPointCloud;		// RS2 pointcloud helper
	rs2::points* m_pPoints;				// persist the points between frames in case we want to display again
	rs2::colorizer* m_pColorizer;		// Helper to colorize depth images - not needed when RGB colors are used
	window* m_pApp;						// TODO Very temporary - spawn a new window for opengl rendering
	glfw_state* m_pViewState;			// An object to manage view state for 3d point cloud projections
	int mOutputWidth, mOutputHeight;	// Dimensions of the output video frame (can be different to input frame size for point cloud types)
										// Needs to match what gets provided in output media sample frame buffer!


	// helper functions for mapping RS frames to output directshow frames (includes inverting etc.)
	void invert8bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
	void invert24bppToRGB(BYTE* frameBuffer, int frameSize, rs2::video_frame frame);
};
