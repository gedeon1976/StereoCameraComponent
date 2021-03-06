/*! \brief

	This is the settings class used
	for reading the xml camera calibration results files 

*/

#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

/// this structure save the points found for the asymetric circle pattern
struct circlePatternInfo
{
	int circleID;
	cv::Point2f circlePosition;
	cv::Point3f circle3DPosition;
};

/// This structure save all the circles found per each calibration image
struct circlesDataPerImage
{
	int cameraID;
	int imageID;
	std::vector<circlePatternInfo> circlesData;
};

class Results
{
public:
	string calibrationTime;				///< time of calibration 
	int nrFrames;						///< number of images used for the calibration
	int imageWidth;						///< image width
	int imageHeight;					///< image height
	int boardWidth;						///< width of board pattern (in number of patterns)
	int boardHeight;					///< height of board pattern (in number of patterns)

	float StereoBaseline;				///< The separation in mm between the cameras
	float Mx;							///< Pixels per/mm in the sensor in the x direction
	float My;							///< Pixels per/mm in the sensor in the y direction
	float sensorSizeWidth;				///< image sensor width in mm
	float sensorSizeHeight;				///< image sensor height in mm

	float squareSize;					///< size of the pattern in mm
	int flagValue;						///< flag value used to calculate the calibration
	Mat intrinsicCameraMatrix;			///< OpenCV matrix containing the intrinsic parameters found
	Mat distortionCoefficients;			///< OpenCV matrix containing the distortion parameters found
	float avgReprojectionError;			///< average reprojection error for all the images used
	Mat perViewReprojectionErrors;		///< error reprojection error per image
	Mat extrinsicParameters;			///< rotation and traslation for each used image in the calibration
	Mat imagePoints;					///< image points used for the calibration in all images
	Mat circleData;						///< It contains all the data from the asymetrical circles pattern
	
public:
	Results();
	~Results();

	/// write the results of the corresponding calibration to a xml file
	/// @param[in,out] fs It contains the setting class parameters
	void write(FileStorage& fs) const;
	
	/// read the results xml file
	/// @param[in] node It contains the readings from the results file 
	void read(const FileNode& node);

	/// interpretate the different values from the xml results file
	void interpretate();
};

