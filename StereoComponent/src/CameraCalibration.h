/*! \brief

	This is the camera calibration class
	it is based on the OpenCV 2.4.9 Tutorials
	about camera calibration but written 
	in a class compact form with multithread capacity

*/

#pragma once

// include settings structure class
#include "Settings.h"
#include "Results.h"
#include "commonStereoComponent.h"

// c++11 headers
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

// include boost libraries
#include <boost\filesystem.hpp>

typedef cameraUsefulData cameraData;



class CameraCalibration {



public:
	CameraCalibration();
	//CameraCalibration(const CameraCalibration &camera);	

	~CameraCalibration();

	int readSettings(string &inputSettingsFile);

	// read the results from the calibration process and save it into a FileStorage structure
	int readResults(string &outputResultsFile);

	void getImagesAndFindPatterns(const string &cameraName);

	double computeReprojectionErrors(vector<vector<Point3f>>& objectPoints, 
									vector<vector<Point2f>>& imagePoints, 
									vector<Mat>& rvecs, vector<Mat>& tvecs, 
									Mat& cameraMatrix, Mat& distCoeffs, vector<float>& perViewErrors);

	void calcBoardCornerPositions(Size boardSize, float squareSize, 
									vector<Point3f>& corners, Settings::Pattern patternType);

	bool runCalibration(Settings& s, Size& imageSize, Mat& cameraMatrix, 
						Mat& distCoeffs, vector<vector<Point2f>> imagePoints, 
						vector<Mat>& rvecs, vector<Mat>& tvecs, 
						vector<float>& reprojErrs, double& totalAvgErr);

	void saveCameraParams(Settings& s, Size& imageSize, Mat& cameraMatrix, 
						Mat& distCoeffs, vector<Mat>& rvecs, vector<Mat>& tvecs, 
						vector<float>& reprojErrs,
						vector<vector<Point2f>>& imagePoints, double totalAvgErr);

	bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, 
						Mat& distCoeffs, vector<vector<Point2f>> imagePoints);

	// get the intrinsic calibration matrix found for this camera
	void getIntrinsicMatrix(Mat &intrinsicMatrix);

	void getCameraUsefulParameters(cameraData &cameraUsefulParameters);

	// get the distortion parameters found for this camera
	void getDistortionMatrix(Mat &distortionParametersMatrix);

	// get the images used for the calibration process
	void getImagesUsedForCalibration(vector<capturedFrame> &imageList);

	// get the number of images used for calibration
	int getHowManyImagesWereUsedperCamera();

	// get file path
	bool getPathForThisFile(string &Filename, string &pathFound);

	// get camera ID
	void getCameraID(int &cameraID);


private:

	Settings s;
	Results calibrationResults;
	string  inputSettingsFile;
	vector<vector<Point2f> > imagePoints;
	Mat cameraMatrix, distCoeffs;
	Mat intrinsicK_Matrix, distortionCoefficients;
	Mat savedImage;
	Size imageSize;
	int mode;
	clock_t prevTimestamp;
	const Scalar RED, GREEN;
	const char ESC_KEY = 27;
	cameraData cameraUsefulParameters;	
	int frameCounter;
	int imageCounter;
	boost::filesystem::path currentPath, resultsPath;

	
};
