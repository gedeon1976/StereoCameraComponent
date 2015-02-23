#include "StereoCamera.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// interface realization(implementation) for the API of this component
//////////////////////////////////////////////////////////////////////////////////////////////


// Initialize method
void StereoCamera::Init()
{
	cameraGlobalStatus = StereoHeadState::STEREO_NOT_CALIBRATED;	
	
}

// get the calibration status
int StereoCamera::getStereoCameraState()
{
	return cameraGlobalStatus;
}

// calibrate the cameras
void StereoCamera::calibrateStereoCamera(string &leftSettingsFile, string &rightSettingsFile) {

	int value = 0;
	leftInputSettingsFile.assign(leftSettingsFile);
	rightInputSettingsFile.assign(rightSettingsFile);

	// call the calibration
	calibrateCameras(leftInputSettingsFile, rightInputSettingsFile);

	// set state to calibrated
	value = StereoHeadState::STEREO_CALIBRATED;
	setStereoCameraState(value);

}

// get the intrinsic parameters
void StereoCamera::getIntrinsicParameters(vector<cv::Mat> &IntrinsicParameters)
{
	readIntrinsicParameters(IntrinsicParameters);
}

// get some useful camera parameters
void StereoCamera::getCameraUsefulParameters(cameraParameters &cameraParameters)
{
	readCameraUsefulParameters(cameraParameters);
}

// get the distortion parameters
void StereoCamera::getDistortionParameters(vector<cv::Mat> &DistortionMatrices)
{
	readDistortionParameters(DistortionMatrices);
}

// get the transforms between cameras
void StereoCamera::getStereoTransforms(vector<cv::Mat> &StereoTransforms){

	findStereoTransform(StereoTransforms);

}

// get the projection matrix for each camera 
void StereoCamera::getProjectionMatrices(vector<cv::Mat> &ProjectionMatrices){

	findProjectionMatricesFrom_E_Matrix(ProjectionMatrices);

}

// get the vergence angle
double StereoCamera::getVergenceAngle(){

	double Angle=0;
	return Angle;
}


//get the fundamental matrix relationship
double StereoCamera::getFundamentalMatrix(cv::Mat &FundamentalMatrix)
{
	double Error = 0;

	// get from image points from a couple of images from the calibration process
	findFundamentalMatrix(FundamentalMatrix);

	return Error;
}

// get the esential matrix relationship
double StereoCamera::getEsentialMatrix(cv::Mat &EsentialMatrix)
{
	double Error = 0;
	findEssentialMatrix(EsentialMatrix);
	return Error;
}

// get the scale factor for triangulation
void StereoCamera::getScaleFactor(double &ScaleFactor){

	readAssymetricalCirclesData();
	estimateScaleFactor(ScaleFactor);

	// save estimated scale factor
	scaleFactorValue = ScaleFactor;
}

// get the path to a given file
bool StereoCamera::getPathForThisFile(string &fileName, string &pathFound)
{
	bool found;
	found = getFilePath(fileName, pathFound);
	return found;
}

// perform a test for the results from the calibration
void StereoCamera::testCalibrationProcess(){

	trackTestPointer();
}


//////////////////////////////////////////////////////////////////////////////////////////
// internal methods implementation
//////////////////////////////////////////////////////////////////////////////////////////


StereoCamera::~StereoCamera()
{

}

// set the state
void StereoCamera::setStereoCameraState(int value)
{
	cameraGlobalStatus = value;
}

// calibrate the cameras
void StereoCamera::calibrateCameras(string &leftSettingsFile, string &rightSettingsFile)
{
	// start the calibration process

	int value = StereoHeadState::STEREO_NOT_CALIBRATED;	

	// read the settings files for the left and right cameras
	leftCamera.readSettings(leftSettingsFile);
	rightCamera.readSettings(rightSettingsFile);

	// run the calibration process for each camera

	// store a call to the member function used for camera calibration. 
	// This calling will be used as the thread callable function parameter with its arguments
	std::function<void(CameraCalibration,const string&)>threadCalibrateFunction = &CameraCalibration::getImagesAndFindPatterns;

	//create two threads for camera calibration	
	
	const string leftCameraName("leftCamera");
	const string rightCameraName("rightCamera");

	std::thread threadForLeftCalibration(threadCalibrateFunction, leftCamera, leftCameraName);
	std::thread threadForRightCalibration(threadCalibrateFunction, rightCamera, rightCameraName);

		
	threadForLeftCalibration.join();
	threadForRightCalibration.join();

	// read the results from xml files
	string myPath;
	string FileName("Calibration_Results_Left_Camera.xml");	

	// get the path to the results file
	boost::filesystem::path currentPath = boost::filesystem::current_path();
	string resultsPath = currentPath.generic_string();
	
	boost::filesystem::path p{"/"};
	getFileGivenPath(FileName,resultsPath,myPath);
	pathToSetupFiles = boost::filesystem::path(myPath);
	parentPath = pathToSetupFiles.parent_path();
		
	string leftResultsFile(parentPath.generic_string() + p.generic_string() + "Calibration_Results_Left_Camera.xml");
	string rightResultsFile(parentPath.generic_string() + p.generic_string() + "Calibration_Results_Right_Camera.xml");

	leftCamera.readResults(leftResultsFile);
	rightCamera.readResults(rightResultsFile);

	
}


// read the intrinsic parameters from the calibration results 
void StereoCamera::readIntrinsicParameters(vector<cv::Mat> &intrinsicParameters)
{
	Mat K_left = Mat::eye(3,3,CV_64F);
	Mat K_right = Mat::eye(3,3,CV_64F);
		
	leftCamera.getIntrinsicMatrix(K_left);
	rightCamera.getIntrinsicMatrix(K_right);

	intrinsicParameters.push_back(K_left);
	intrinsicParameters.push_back(K_right);
	
}

// read Assymetrical circles Patterns from the two images
void StereoCamera::readAssymetricalCirclesData(){

	leftCamera.getInfoFromCirclePatterns(leftPatternCalibrationData);
	rightCamera.getInfoFromCirclePatterns(rightPatternCalibrationData);

}

// read some useful parameters from the calibration results
void StereoCamera::readCameraUsefulParameters(cameraParameters &cameraUsefulParameters)
{
	leftCamera.getCameraUsefulParameters(CameraUsefulParametersLeft);
	rightCamera.getCameraUsefulParameters(CameraUsefulParametersRight);

	cameraUsefulParameters.push_back(CameraUsefulParametersLeft);
	cameraUsefulParameters.push_back(CameraUsefulParametersRight);
}

// read the distortion parameters from the calibration results
void StereoCamera::readDistortionParameters(vector<cv::Mat> &DistortionMatrices)
{
	Mat D_left = Mat::zeros(8, 1, CV_64F); 
	Mat D_right = Mat::zeros(8, 1, CV_64F);
	
	leftCamera.getDistortionMatrix(D_left);
	rightCamera.getDistortionMatrix(D_right);

	DistortionMatrices.push_back(D_left);
	DistortionMatrices.push_back(D_right);
	
}

// get the path for a given file
bool StereoCamera::getFilePath(string &fileName,string &pathFound)
{
	bool found = false;

	// look for current path
	boost::filesystem::path directory;
	boost::filesystem::path currentPath = boost::filesystem::current_path();

	// get the number of elements of the path
	int pathElementsSize = 0;
	for (boost::filesystem::path::iterator it = currentPath.begin(); it != currentPath.end(); ++it){
		pathElementsSize = pathElementsSize + 1;
	}

	// built the directory for search 2 levels up
	int levelUp = 2;
	boost::filesystem::path::iterator itToBuildPath = currentPath.begin();
	for (int i = 0; i < (pathElementsSize - levelUp); i++){
		directory /= *itToBuildPath;
		++itToBuildPath;
	}

	boost::filesystem::path& path = directory;
	const boost::filesystem::path file = fileName;
	const boost::filesystem::recursive_directory_iterator end;
	const boost::filesystem::recursive_directory_iterator dir_iter(directory);

	const auto it = std::find_if(dir_iter,
		end,
		[&file](const boost::filesystem::directory_entry& e)
	{
		return e.path().filename() == file;
	});

	if (it != end){

		path = it->path();
		pathFound = path.generic_string();		// make the path portable
		found = true;
	}
	return found;
}

// search for a given file given a path
bool StereoCamera::getFileGivenPath(string &fileName, string &givenPath, string &pathFound)
{
	bool found = false;

	// look for current path
	boost::filesystem::path directory;
	boost::filesystem::path currentPath(givenPath);

	// get the number of elements of the path
	int pathElementsSize = 0;
	for (boost::filesystem::path::iterator it = currentPath.begin(); it != currentPath.end(); ++it){
		pathElementsSize = pathElementsSize + 1;
	}

	// built the directory for search 0 levels up
	int levelUp = 0;
	boost::filesystem::path::iterator itToBuildPath = currentPath.begin();
	for (int i = 0; i < (pathElementsSize - levelUp); i++){
		directory /= *itToBuildPath;
		++itToBuildPath;
	}

	boost::filesystem::path& path = directory;
	const boost::filesystem::path file = fileName;
	const boost::filesystem::recursive_directory_iterator end;
	const boost::filesystem::recursive_directory_iterator dir_iter(directory);

	const auto it = std::find_if(dir_iter,
		end,
		[&file](const boost::filesystem::directory_entry& e)
	{
		return e.path().filename() == file;
	});

	if (it != end){

		path = it->path();
		pathFound = path.generic_string();		// make the path portable
		found = true;
	}
	return found;
}

// read the images used from calibration from the path where it were saved
void StereoCamera::getImageUsedFromCalibration(vector<cv::Mat> &leftImageList, vector<cv::Mat> &rightImageList)
{
	// read the jpg files
	bool foundLeft,foundRight;
	cv::Mat leftCalibrationImage, rightCalibrationImage;
	string myPath;

	for (int k = 1; k <= leftCamera.getHowManyImagesWereUsedperCamera();k++){

		// build the string to be looked for
		string FileNameLeft("Image" + string(std::to_string(k)) + "leftCamera.jpg");
		string FileNameRight("Image" + string(std::to_string(k)) + "rightCamera.jpg");

		// find and read the images
		foundLeft = getFilePath(FileNameLeft, myPath);
		if (foundLeft){
			leftCalibrationImage = cv::imread(myPath, IMREAD_COLOR);
			leftCalibrationImageList.push_back(leftCalibrationImage);
		}

		foundRight = getFilePath(FileNameRight, myPath);
		if (foundRight){
			rightCalibrationImage = cv::imread(myPath, IMREAD_COLOR);
			rightCalibrationImageList.push_back(rightCalibrationImage);
		}
	}	
	

}

// find matches on the left and right images
void StereoCamera::findMatches() {
	
	// get the images
	bool fileFound;
	string pathToFile;
	Mat imageLeft, imageRight;	

	matchesLeft.clear();
	matchesRight.clear();
	leftCalibrationImageList.clear();
	rightCalibrationImageList.clear();

	getImageUsedFromCalibration(leftCalibrationImageList, rightCalibrationImageList);

	// choose the middle image pair to find the matches
	int choosedOne = std::abs(std::round((leftCalibrationImageList.size() + rightCalibrationImageList.size()) / 4));

	imageLeft = leftCalibrationImageList.at(choosedOne);
	imageRight = rightCalibrationImageList.at(choosedOne);

	// here we are going to use the A-KAZE detector and descriptor
	// described in the article
	// Fast explicit diffussion for accelerated features in nonlinear scale spaces
	// BMVC.2013  Pablo Alcantarilla et al

	vector<KeyPoint> keyPointsLeft, KeyPointsRigth;
	Mat descriptorsLeft, descriptorsRigth;
	
	cv::Ptr<AKAZE> akaze = AKAZE::create();

	akaze->detectAndCompute(imageLeft, noArray(), keyPointsLeft, descriptorsLeft);
	akaze->detectAndCompute(imageRight, noArray(), KeyPointsRigth, descriptorsRigth);

	// matcher
	BFMatcher matcherBruteForce(NORM_HAMMING);
	vector<vector<DMatch> > matches;
	matcherBruteForce.knnMatch(descriptorsLeft, descriptorsRigth, matches, 2);

	// find correct matches
	for (size_t i = 0; i < matches.size(); i++)
	{
		DMatch currentMatch = matches[i][0];
		float distance1 = matches[i][0].distance;
		float distance2 = matches[i][1].distance;

		if (distance1 < match_ratio*distance2)
		{
			matchesLeft.push_back(keyPointsLeft[currentMatch.queryIdx]);
			matchesRight.push_back(KeyPointsRigth[currentMatch.trainIdx]);
			good_matches.push_back(currentMatch);
		}
	}

	
	// draw the results
	imageMatches;
	drawMatches(imageLeft, keyPointsLeft, imageRight, KeyPointsRigth, good_matches, imageMatches, DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	imshow("Matching witk AKAZE Features", imageMatches);

	// write image to disk
	string imageMatchesName("AKAZEmatches.jpg");
	fileFound = getPathForThisFile(imageMatchesName,pathToFile);
	if (fileFound){
		boost::filesystem::remove(pathToFile);
	}
	imwrite("AKAZEmatches.jpg", imageMatches);	
}


// sort the matches using the Hat Matrix method, see Ola Millnert Master thesis Feb'2006
void StereoCamera::SortMatchesUsingHatMatrix(vector<sortMatch> &sortedMatches){

	// get the size of the matches
	int matchesNumber = good_matches.size();

	// get the matches
	vector<cv::Point3f> leftNormPoints, rightNormPoints;
	vector<cv::Point2f> leftPoints, rightPoints;
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);

	normalizePoints(KLeft, leftPoints, leftNormPoints);
	normalizePoints(KRight, rightPoints, rightNormPoints);

	// build the A matrix from Ah = 0
	vector<sortMatch> outliernessValues;
	cv::Mat A, Hat_Matrix,tmpMatrix, tmpMatrix2;
	cv::Mat row_A(1,9,CV_64F);

	for (int i = 0; i < matchesNumber; i++){

		row_A.at<double>(0, 0) = rightNormPoints.at(i).x*leftNormPoints.at(i).x;
		row_A.at<double>(0, 1) = rightNormPoints.at(i).x*leftNormPoints.at(i).y;
		row_A.at<double>(0, 2) = rightNormPoints.at(i).x*leftNormPoints.at(i).z;

		row_A.at<double>(0, 3) = rightNormPoints.at(i).y*leftNormPoints.at(i).x;
		row_A.at<double>(0, 4) = rightNormPoints.at(i).y*leftNormPoints.at(i).y;
		row_A.at<double>(0, 5) = rightNormPoints.at(i).y*leftNormPoints.at(i).z;

		row_A.at<double>(0, 6) = rightNormPoints.at(i).z*leftNormPoints.at(i).x;
		row_A.at<double>(0, 7) = rightNormPoints.at(i).z*leftNormPoints.at(i).y;
		row_A.at<double>(0, 8) = rightNormPoints.at(i).z*leftNormPoints.at(i).z;

		// add new (correspondence)row to matrix
		A.push_back(row_A);	
	}

	// build the hat Matrix Hat_Matrix = A*(Atransp*A)inv*ATransp
	gemm(A, A, 1.0, cv::noArray(), 0.0, tmpMatrix,cv::GEMM_1_T);
	tmpMatrix2 = tmpMatrix.inv(cv::DECOMP_SVD);

	gemm(A,tmpMatrix2,1.0,cv::noArray(),0.0,tmpMatrix);
	gemm(tmpMatrix, A, 1.0, cv::noArray(), 0.0,Hat_Matrix, cv::GEMM_2_T);

	sortMatch currentMatch;
	for (int i = 0; i < Hat_Matrix.size().width; i++){
		for (int j = 0; j < Hat_Matrix.size().height; j++){
			if (i == j){
				currentMatch.currentIndex = 0;
				currentMatch.originalIndex = i;
				currentMatch.outliernessMeasure = Hat_Matrix.at<double>(i, j);
				currentMatch.leftPoint = leftNormPoints.at(i);
				currentMatch.rightPoint = rightNormPoints.at(i);
				outliernessValues.push_back(currentMatch);
			}
		}
	}

	// sort the matches using a lambda function 
	// the sort value is the diagonal of the Hat Matrix
	// low values are better matches, larges ones are taken as outliers
	std::sort(outliernessValues.begin(), outliernessValues.end(), 
		[](const sortMatch &a, const sortMatch &b){
		return b.outliernessMeasure > a.outliernessMeasure; });

	// save the sorted matches
	sortedMatches = outliernessValues;

}


// get the median from a given vector of values
void StereoCamera::getMedian(vector<double>vectorInput, double &medianValue){

	// the first step: order the values
	double median=0;
	std::sort(vectorInput.begin(), vectorInput.end());

	int vectorSize = vectorInput.size();
	double medianType = std::fmod(vectorSize, 2);
	int isEven = std::round(medianType);
	
	// the second step: calculate the median
	switch (isEven){
	case 0: // vector numbers is even
		median = 0.5*(vectorInput.at(vectorSize / 2) + vectorInput.at(0 + vectorSize / 2));
		break;
	case 1: // vector numbers is odd
		median = vectorInput.at((vectorSize+1)/2);
		break;
	}

	// return the median value
	medianValue = median;

}

// find the fundamental matrix
void StereoCamera::findFundamentalMatrix(cv::Mat &F_MatrixExtern) {

	// find matches between an image pair
	cv::Size imageSize;
	vector<cv::Point2f> leftPoints, rightPoints;
	double epsilon = 0.000001;
	int attempsCounter = 0;
	int matchesNumber = 8;
	int maxAttempsLimit = 10;
	bool foundMinimumMatchesNumber = false;

	// find the matches
	findMatches();

	// select the RANSAC method to calculate the matrix
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);

	// check a minimum number of matches, in our case at least 8 matches
	while (!foundMinimumMatchesNumber){
		if (matchesLeft.size() >= matchesNumber){

			F_Matrix = findFundamentalMat(leftPoints, rightPoints, FM_RANSAC, 3, 0.999);
			F_Matrix.copyTo(F_MatrixExtern);
			foundMinimumMatchesNumber = true;
		}
		else{
			findMatches();
		}
		attempsCounter = attempsCounter + 1;
		if (attempsCounter >= maxAttempsLimit){

			cout << "Max number of attemps trying to find a minimum number of matches has been reached...\n" << endl;
			cout << "Try again using another image pair \n" << endl;
			exit(0);
		}
	}
	

	// Draw the epipolar lines
	cv::Mat imageLeft, imageRight;

	// choose the middle image pair to check F results
	int choosedOne = std::abs(std::round((leftCalibrationImageList.size() + rightCalibrationImageList.size()) / 4));

	imageLeft = leftCalibrationImageList.at(choosedOne);
	imageRight = rightCalibrationImageList.at(choosedOne);
	imageSize = imageLeft.size();

	vector<cv::Point3f> leftEpipolarLines, rightEpipolarLines;
	computeCorrespondEpilines(leftPoints, 1, F_Matrix, rightEpipolarLines);
	computeCorrespondEpilines(rightPoints, 2, F_Matrix, leftEpipolarLines);

	int linesNumber = leftPoints.size();
	double aL, bL, cL, aR, bR, cR;
	double x1L, y1L, x2L, y2L;
	double x1R, y1R, x2R, y2R;
	cv::Point point1L, point2L, point1R, point2R;
	for (int i = 0; i < linesNumber; i++){

		// find the imtercepts
		aL = leftEpipolarLines.at(i).x; aR = rightEpipolarLines.at(i).x;
		bL = leftEpipolarLines.at(i).y; bR = rightEpipolarLines.at(i).y;
		cL = leftEpipolarLines.at(i).z; cR = rightEpipolarLines.at(i).z;

		// calculate points
		if (std::abs(bL) > epsilon){
			x1L = 0; y1L = -cL/bL; x2L = imageSize.width; y2L = -(aL*x2L)/bL - (cL/bL);
		}
		else{
			x1L = -cL/aL; y1L = 0; y2L = imageSize.height; x2L = -(bL*y2L)/aL - (cL/aL);
		}

		if (std::abs(bR) > epsilon){
			x1R = 0; y1R = -cR/bR; x2R = imageSize.width; y2R = -(aR*x2R)/bR - (cR/bR);
		}
		else{
			x1R = -cR/aR; y1R = 0; y2R = imageSize.height; x2R = -(bR*y2R)/aR - (cR/aR);
		}

		point1L.x = x1L; point1L.y = y1L;
		point2L.x = x2L; point2L.y = y2L;
		point1R.x = x1R; point1R.y = y1R;
		point2R.x = x2R; point2R.y = y2R;

		line(imageLeft, point1L, point2L,Scalar(0,0,255));
		line(imageRight, point1R, point2R,Scalar(0, 0, 255));
	}

	// show epipolar lines
	cv::imshow("Left Epipolar Lines",imageLeft);
	cv::imshow("Right Epipolar Lines", imageRight);

	// print the matrix
	string F_Name("Fundamental Matrix");
	printMatrix(F_Matrix, F_Name);

	// probe F
	cout << "det(F): " << cv::determinant(F_Matrix) << '\n' << endl;
}


// find the esential matrix
void StereoCamera::findEssentialMatrix(cv::Mat &EsentialMatrix) {
	
	cv::Mat tmpResult, normalized_E;
	cv::Mat K_left = Mat::eye(3, 3, CV_64F);
	cv::Mat K_right = Mat::eye(3, 3, CV_64F);

	// save the K matrices	
	leftCamera.getIntrinsicMatrix(K_left);
	rightCamera.getIntrinsicMatrix(K_right);

	K_left.copyTo(KLeft);
	K_right.copyTo(KRight);

	// print internal matrices
	string KL_Name("kLeft Matrix");
	printMatrix(K_left, KL_Name);

	string KR_Name("kRight Matrix");
	printMatrix(K_right, KR_Name);

	gemm(K_right,F_Matrix,1.0,noArray(),0.0,tmpResult,GEMM_1_T);
	gemm(tmpResult,K_left,1.0,noArray(),0.0,E_Matrix);
	E_Matrix.copyTo(EsentialMatrix);
	
	// print the matrix
	string E_Name("Essential Matrix");
	printMatrix(E_Matrix, E_Name);

	cv::Mat w, u, vt;
	cv::SVD::compute(E_Matrix,w,u,vt);

	printMatrix(w,string("w"));
	printMatrix(w, string("u"));
	printMatrix(w, string("vt"));

	cv::normalize(E_Matrix, normalized_E, E_Matrix.at<double>(2, 2));
	string ENorm_Name("Essential Matrix Normalized");
	printMatrix(normalized_E, ENorm_Name);

	cv::SVD::compute(normalized_E, w, u, vt);
	printMatrix(w, string("w"));
	printMatrix(w, string("u"));
	printMatrix(w, string("vt"));

	normalized_E.copyTo(E_Matrix);

	// get an average value for the focal lengths and principal points
	double focalLength = (CameraUsefulParametersLeft.focalLength + CameraUsefulParametersRight.focalLength) / 2;
	cv::Point2d principalPoint((CameraUsefulParametersLeft.principalPointX + CameraUsefulParametersRight.principalPointX) / 2,
		(CameraUsefulParametersLeft.principalPointY + CameraUsefulParametersRight.principalPointY) / 2);

	// save these values to further calculus
	averageFocalLength = focalLength;
	averagePrincipalPoint = principalPoint;

	// Ola Millnert method to find the best E is applied here
	// from the Master Thesis titled
	// Range determination for mobile robots using an omnidirectional camera  Feb 2006

	// 1. sort the matches according their outlierness measure using the Hat Matrix
	//	Hat Matrix is a method for outliers diagnosis in measurements

	vector<sortMatch> sortedMatches, sortedMatchesWithOutliers;
	SortMatchesUsingHatMatrix(sortedMatches);

	// Reject the outliers whose values are greater that hii > 2p/N, hii is the diagonals of Hat Matrix
	// See Dagmar Blatna 


	// set size of correspondences and parameters estimated for the E matrix
	int N = sortedMatches.size();
	int p = 8;								// 8 parameters for E calculus

	double ThresholdOutliers = 2*static_cast<double>(p)/N;
	
	for (auto i :sortedMatchesWithOutliers){

		double hii = i.outliernessMeasure;
		if (hii < ThresholdOutliers){
			sortedMatches.push_back(i);
		}
	}
	
	// 2. Build the N-9 E matrices 

	vector<cv::Mat> E_matrices;

	vector<cv::Point2f> leftPoints, rightPoints, leftPointsNormalized, rightPointsNormalized;
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);

	// get the best points
	int NMatches = 9;
	vector<cv::Point2f> leftNormalPoints, rightNormalPoints;
	vector<cv::Point3f> leftNormalizedPoints, rightNormalizedPoints;

	const int N_E_Matrices = sortedMatches.size() - NMatches;
	int startPosition = 0 ;

	for (int k = 0; k < N_E_Matrices; k++){

		// select the next 9 correspondences
		leftNormalizedPoints.clear();
		rightNormalizedPoints.clear();

		for (int i = 0; i < NMatches; i++){

			leftNormalizedPoints.push_back(sortedMatches.at(i + startPosition).leftPoint);
			rightNormalizedPoints.push_back(sortedMatches.at(i + startPosition).rightPoint);
		}

		// get E using normalized points xnorm= Kinv*x
		convertPointsFromHomogeneous(leftNormalizedPoints, leftNormalPoints);
		convertPointsFromHomogeneous(rightNormalizedPoints, rightNormalPoints);

		cv::Mat E = findEssentialMat(leftNormalPoints, rightNormalPoints, focalLength, principalPoint, RANSAC, 0.99,3.0);
		E_matrices.push_back(E);

		// increase the start Position
		startPosition = startPosition + 1;
	}

	// find the matrix E directly
	/*convertPointsFromHomogeneous(leftNormalizedPoints, leftPoints);
	convertPointsFromHomogeneous(rightNormalizedPoints, rightPoints);

	cv::Mat E_openCV = findEssentialMat(leftPoints,rightPoints,focalLength,principalPoint,RANSAC,0.999);
	E_openCV.copyTo(EsentialMatrix);
	E_openCV.copyTo(E_Matrix);
	
	string ECV_Name("Essential Matrix OpenCV");
	printMatrix(E_openCV, ECV_Name);

	cv::SVD::compute(E_openCV, w, u, vt);

	printMatrix(w, string("w"));
	printMatrix(w, string("u"));
	printMatrix(w, string("vt"));*/

	// 3. Find the best E matrix

	cv::Mat best_E;
	Essential_MatrixData currentEssential_MatrixData;
	vector<Essential_MatrixData> E_matricesData;

	for (int k = 0; k < E_matrices.size();k++){

		////////////////////////////////////////////////
		// a. find the residuals for each E matrix found

		cv::Mat pointsLeft(3,1,CV_64F);
		cv::Mat pointsRight(3, 1, CV_64F);
		cv::Mat tmpMatrix, residualsMat;
		double residualsValue = 0;
		vector<double> residuals;

		for (int i = 0; i < sortedMatches.size(); i++){

			pointsLeft.at<double>(0, 0) = sortedMatches.at(i).leftPoint.x;
			pointsLeft.at<double>(1, 0) = sortedMatches.at(i).leftPoint.y;
			pointsLeft.at<double>(2, 0) = sortedMatches.at(i).leftPoint.z;

			pointsRight.at<double>(0, 0) = sortedMatches.at(i).rightPoint.x;
			pointsRight.at<double>(1, 0) = sortedMatches.at(i).rightPoint.y;
			pointsRight.at<double>(2, 0) = sortedMatches.at(i).rightPoint.z;

			gemm(pointsRight, E_matrices.at(k), 1.0, cv::noArray(), 0.0, tmpMatrix, cv::GEMM_1_T);
			gemm(tmpMatrix, pointsLeft,1.0,cv::noArray(),0.0,residualsMat);

			// save the residuals for each correspondence
			residualsValue = residualsMat.at<double>(0, 0);
			residuals.push_back(residualsValue);
		}
		// save the residuals for each E matrix
		currentEssential_MatrixData.matrixIndex = k;
		currentEssential_MatrixData.residuals = residuals;

		// save the current essential Matrix
		E_matrices.at(k).copyTo(currentEssential_MatrixData.E_Matrix);		

		/////////////////////////////////////////////////////
		// b. check residuals that overcome a given threshold

		vector<double> residualsSquare;
		for (auto i : residuals){
			residualsSquare.push_back(pow(i, 2));
		}
		
		double medianValue;
		getMedian(residualsSquare, medianValue);

		// set the first threshold to filter correspondences greater that it
		double threshold_Ri = 2.5*(1.4826*(1 + (5 / (N - p - 1)))*sqrt(medianValue));
		currentEssential_MatrixData.thresholdForResiduals = threshold_Ri;

		bool isRiValid = false;
		vector<bool> validRis;
		// filter the residuals
		for (int i = 0; i < residuals.size();i++){
			if (std::abs(residuals.at(i)) < threshold_Ri){
				isRiValid = true;
				validRis.push_back(isRiValid);
			}
			else{
				isRiValid = false;
				validRis.push_back(isRiValid);
			}
		}

		// save the validity status
		currentEssential_MatrixData.validResiduals = validRis;

		//////////////////////////////////////////////////////////////////////////////////////////////////
		// c.  find the variance for the E Matrix using the Residuals that overcome the threshold filter

		vector<double> goodResiduals;
		double sumValues = 0;
		double currentValue;
		for (int i = 0; i < residuals.size(); i++){
			
			if (validRis.at(i)){

				currentValue = residuals.at(i);
				goodResiduals.push_back(currentValue);
				sumValues = sumValues + residuals.at(i);
			}
		}

		// get the mean value for the good residuals
		double goodResidualsMean = sumValues / goodResiduals.size();

		// calculate the Vi variance of good residual for each E matrix
		double varianceVi;
		double summatoryValue = 0;

		for (auto i : goodResiduals){
			summatoryValue = summatoryValue + pow((i-goodResidualsMean), 2);
		}

		varianceVi = summatoryValue / (goodResiduals.size() -1);
		currentEssential_MatrixData.EvarianceValidResiduals = varianceVi;

		// save the data for this essential Matrix
		E_matricesData.push_back(currentEssential_MatrixData);

		// clear the residuals values on preparation for next E matrix
		residuals.clear();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// c. Find the median of the variances for all the E matrices
	double medianOf_Ek_Variances;
	vector<double> Ek_Variances;

	for (auto i:E_matricesData){			
		Ek_Variances.push_back(i.EvarianceValidResiduals);
	}
	getMedian(Ek_Variances, medianOf_Ek_Variances);

	// Set a Second threshold to filter out the bad E matrices
	double threshold_Vi = 2.5*(1.4826*(1 +(5/(N-p))))*medianOf_Ek_Variances;

	vector<cv::Mat> good_EMatrices;
	for (auto i:E_matricesData){

		if (i.EvarianceValidResiduals < threshold_Vi){

			good_EMatrices.push_back(i.E_Matrix);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// d. Choose the best E matrix with lowest QE
	double qE;
	E_MatricesCandidates currentMatrixCandidate;
	vector<E_MatricesCandidates> finalCandidates;
	for (auto i:good_EMatrices){

		// compute SVD(E_matrix) and solves for qE measure
		cv::SVD::compute(i,w,u,vt);
		qE = w.at<double>(0,0) - w.at<double>(1,0);
		currentMatrixCandidate.qE = qE;
		i.copyTo(currentMatrixCandidate.E_matrix);
		finalCandidates.push_back(currentMatrixCandidate);		
	}

	// find the best E matrix according to qE
	std::sort(finalCandidates.begin(), finalCandidates.end(),
		[](const E_MatricesCandidates &a, const E_MatricesCandidates &b){
		return a.qE < b.qE; });

	// Finally get the best E Matrix
	finalCandidates.front().E_matrix.copyTo(best_E);

	// copy the best E matrix
	best_E.copyTo(EsentialMatrix);
	best_E.copyTo(E_Matrix);

	string Enorm_Name("Essential Matrix using normalized points x_norm = Kinverted*x");
	printMatrix(best_E, Enorm_Name);

	cv::SVD::compute(best_E, w, u, vt);

	// check SVD values, 2 have to be equal and the last must be zero
	printMatrix(w, string("w"));
	//printMatrix(u, string("u"));
	//printMatrix(vt, string("vt"));

	cout << "Det(E)= " << cv::determinant(best_E) << "\n" << endl;

}


// get the rotation angles from a rotation matrix
void StereoCamera::getRotationAnglesFromMatrix(cv::Mat &rotationMatrix,double &Alpha,double &Beta,double &Gamma){

	// the method used here is from the notes from the web of Steve LaValle curse on Planning Algorithms
	double R11, R21, R31, R32, R33;

	R11 = rotationMatrix.at<double>(0,0);
	R21 = rotationMatrix.at<double>(1,0);
	R31 = rotationMatrix.at<double>(2,0);
	R32 = rotationMatrix.at<double>(2,1);
	R33 = rotationMatrix.at<double>(2,2);

	Alpha = std::atan2(R21, R11);
	Beta = std::atan2(-R31,hypot(R32,R33));
	Gamma = std::atan2(R32,R33);

	// print the matrix
	string R_Name("Rotation Matrix");
	printMatrix(rotationMatrix, R_Name);

	// angles in degrees
	cout << "Alpha degrees "<< (Alpha * 180) / CV_PI << '\n' << endl;
	cout << "Beta degrees " << (Beta * 180) / CV_PI  << '\n' << endl;
	cout << "Gamma degrees " << (Gamma * 180) / CV_PI << '\n' << endl;
}

// get the traslation bewteen two cameras
void StereoCamera::getTraslationFromMatrix(cv::Mat &TraslationMatrix, double &X_shift, double &Y_shift, double &Z_shift){

	X_shift = TraslationMatrix.at<double>(0,0);
	Y_shift = TraslationMatrix.at<double>(1,0);
	Z_shift = TraslationMatrix.at<double>(2,0);

	// print the matrix
	string T_Name("traslation Matrix");
	printMatrix(TraslationMatrix, T_Name);
}

// find the transforms between the left and right cameras
void StereoCamera::findStereoTransform(vector<cv::Mat> &RotationAndTraslation) {

	
	cv::Mat Rotation, traslation;
	cv::Mat RotationVector;
	vector<cv::Point2f> leftPoints, rightPoints;

	// get an average value for the focal lengths and principal points
	double focalLength = (CameraUsefulParametersLeft.focalLength + CameraUsefulParametersRight.focalLength) / 2;
	cv::Point2d principalPoint((CameraUsefulParametersLeft.principalPointX + CameraUsefulParametersRight.principalPointX)/2,
								(CameraUsefulParametersLeft.principalPointY + CameraUsefulParametersRight.principalPointY)/2);

	// perform a cheirality check to know the positive depth from points
	// some explanation about the esential matrix relationships can be found on the Zisermman book section 9.6
	// there is a section about the possibles poses from a given Esential Matrix
	// So it seems that openCV 3.0 has support to probe the correct pose option
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);
	recoverPose(E_Matrix, leftPoints, rightPoints, Rotation, traslation, focalLength, principalPoint);

	RotationAndTraslation.push_back(Rotation);
	RotationAndTraslation.push_back(traslation);

	// get axis of rotation
	double Alpha, Beta, Gamma;
	getRotationAnglesFromMatrix(Rotation, Alpha, Beta, Gamma);

	// get the traslation
	double X_shift, Y_shift, Z_shift;
	getTraslationFromMatrix(traslation,X_shift, Y_shift, Z_shift);
}

// get a pair of correct matrices P,P' from the essential matrix
void StereoCamera::findProjectionMatricesFrom_E_Matrix(vector<cv::Mat> &ProjectionMatrices){

	// get the K matrices
	cv::Mat K_left = Mat::eye(3, 3, CV_64F);
	cv::Mat K_right = Mat::eye(3, 3, CV_64F);

	// build the real P,P' matrices to perform which is the good one
	cv::Mat P_realLeft(3, 4, CV_64F);
	cv::Mat P_real_recovered(3, 4, CV_64F);
	cv::Mat P_recovered(3, 4, CV_64F);

	// Find the correct P matrix
	cv::Mat triangulatedPointsRecovered;
	vector<cv::Point2f> leftPoints, rightPoints;

	// Get the matched points
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);

	vector<cv::Point2f> leftNormalPoints, rightNormalPoints;
	vector<cv::Point3f> leftNormalizedPoints, rightNormalizedPoints;
	leftCamera.getIntrinsicMatrix(K_left);
	rightCamera.getIntrinsicMatrix(K_right);

	normalizePoints(K_left, leftPoints, leftNormalizedPoints);
	normalizePoints(K_right, rightPoints, rightNormalizedPoints);
	convertPointsFromHomogeneous(leftNormalizedPoints, leftNormalPoints);
	convertPointsFromHomogeneous(rightNormalizedPoints, rightNormalPoints);

	// refine the matches
	vector<cv::Point2f> refinedMatchesLeft, refinedMatchesRight;
	correctMatches(F_Matrix, leftPoints, rightPoints, refinedMatchesLeft, refinedMatchesRight);

	// build the normalized projection Cameras
	// make the first projection matrix P = [I 0]
	PLeft = Mat::eye(3, 4, CV_64F);
	ProjectionMatrices.push_back(PLeft);

	// Find the R and t from E matrix using recoverPose function
	// build the alternatives for the second projection matrix 
	// according to section 9.6 from Zisserman book on 2nd edition
	// it is implemented at the recoverPose OpenCV function
	cv::Mat R_recovered, t_recovered, mask;

	recoverPose(E_Matrix, leftNormalPoints, rightNormalPoints, R_recovered, t_recovered,
		averageFocalLength,averagePrincipalPoint,mask);

	// build the projection matrices options
	build_Projection_Matrix(P_recovered, R_recovered,t_recovered);

	string testPRight("P_Recovered");
	printMatrix(P_recovered, testPRight);	

	// save the right projection Matrix
	ProjectionMatrices.push_back(P_recovered);
	P_recovered.copyTo(PRight);
	cout << " P_recovered choosed" << '\n' << endl;

	// test points depth
	vector<cv::Point3f> testTriangulatedPoints;
	triangulatePoints(PLeft, P_recovered, leftNormalPoints, rightNormalPoints, triangulatedPointsRecovered);

	// convert from homogeneous
	cv::convertPointsFromHomogeneous(triangulatedPointsRecovered.reshape(4, 1), testTriangulatedPoints);

	
}

// Estimate the Scale Factor for 3D triangulation
void StereoCamera::estimateScaleFactor(double &ScaleFactor){

	// ABSOLUTE ROTATION METHOD From Lourakis'13 article titled:
	// Accurate Scale Factor Estimation in 3D reconstruction

	// get the K matrices
	//cv::Mat K_left = Mat::eye(3, 3, CV_64F);
	//cv::Mat K_right = Mat::eye(3, 3, CV_64F);	

	// build the Mi positions for each circle from the calibration Patterns
	vector<circlesDataPerImage> Mi_Left,Mi_Right;
	
	Mi_Left = leftPatternCalibrationData;
	Mi_Right = rightPatternCalibrationData;

	// build the Ni positions for each circle from the left and right image pair
	
	// start with the image pair No 5
	vector<cv::Point2f> inputLeft, inputRight;
	vector<cv::Point3f> Mis_3D;
	vector<circlePatternInfo> leftPattern, rightPattern;

	leftPattern = Mi_Left.at(4).circlesData;
	rightPattern = Mi_Right.at(4).circlesData;

	// get the circles positions in (x,y) and in 3D
	for (int i = 0; i < leftPattern.size();i++){

		inputLeft.push_back(leftPattern.at(i).circlePosition);
		inputRight.push_back(rightPattern.at(i).circlePosition);
		Mis_3D.push_back(leftPattern.at(i).circle3DPosition);
	}

	// take the found Projection matrices P,P' and triangulate the circles points
	cv::Mat triangulateCirclesPatternPoint_3D;
	vector<cv::Point2f> leftTestPoint, rightTestPoint;
	vector<cv::Point3f> leftNormalizedPoint, rightNormalizedPoint;

	normalizePoints(KLeft, inputLeft, leftNormalizedPoint);
	normalizePoints(KRight, inputRight, rightNormalizedPoint);

	convertPointsFromHomogeneous(leftNormalizedPoint, leftTestPoint);
	convertPointsFromHomogeneous(rightNormalizedPoint, rightTestPoint);
	
	triangulatePoints(PLeft, PRight, leftTestPoint, rightTestPoint, triangulateCirclesPatternPoint_3D);

	// converts from homogeneous to obtain the Nis Points 
	vector<cv::Point3f> Nis_3D;
	cv::convertPointsFromHomogeneous(triangulateCirclesPatternPoint_3D.reshape(4, 1),Nis_3D);

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// 3. Obtain the centroids for the Mi's and Ni's Positions
	cv::Scalar M_mean = cv::mean(Mis_3D, cv::noArray());
	cv::Scalar N_mean = cv::mean(Nis_3D, cv::noArray());

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// 4. Get the relative positions
	cv::Point3f M_mean3D, N_mean3D;
	M_mean3D.x = M_mean.val[0];
	M_mean3D.y = M_mean.val[1];
	M_mean3D.z = M_mean.val[2];

	N_mean3D.x = N_mean.val[0];
	N_mean3D.y = N_mean.val[1];
	N_mean3D.z = N_mean.val[2];

	vector<cv::Point3f> Mis_3Drelative, Nis_3Drelative;
	for (int i = 0; i < Mis_3D.size();i++){

		Mis_3Drelative.push_back(Mis_3D.at(i) - M_mean3D);
		Nis_3Drelative.push_back(Nis_3D.at(i) - N_mean3D);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// 5. Build the cross covariance matrix and get the Scale factor
	cv::Mat CrossCovarianceMatrix = cv::Mat::zeros(3,3,CV_32F);
	cv::Mat CrossCovTemp(3, 3, CV_32F);
	double norm_Mi = 0;
	double norm_Ni = 0;
		
	for (int i = 0; i < Mis_3Drelative.size();i++){

		cv::Mat Mi_R(Mis_3Drelative.at(i));
		cv::Mat Ni_R(Nis_3Drelative.at(i));
		gemm(Ni_R, Mi_R, 1.0, cv::noArray(), 0.0, CrossCovTemp, cv::GEMM_2_T);

		if (CrossCovarianceMatrix.type() != CrossCovTemp.type()){
			CrossCovarianceMatrix.convertTo(CrossCovarianceMatrix, CrossCovTemp.type());
		}
		cv::add(CrossCovarianceMatrix,CrossCovTemp,CrossCovarianceMatrix,cv::noArray());

		// get relative points L2 norm
		norm_Mi = norm_Mi + cv::norm(Mis_3Drelative.at(i));
		norm_Ni = norm_Ni + cv::norm(Nis_3Drelative.at(i));
	}
	
	// print covariance Matrix
	cout << "Cross Covariance Matrix \n" << CrossCovarianceMatrix << endl;

	cv::Mat u, w, vt;
	cv::SVD::compute(CrossCovarianceMatrix,w,u,vt);

	cv::Mat R,t;
	double lambda;

	// Find R between point reference frames
	gemm(vt,u,1.0,cv::noArray(),0.0,R,cv::GEMM_1_T + cv::GEMM_2_T);

	// print R matrix
	cout << "Rotation Matrix \n" << R << endl;

	// get the scale factor
	lambda = sqrt(norm_Mi / norm_Ni);

	// get the traslation between point reference frames
	cv::Mat partial_t;
	cv::Mat N_meanTmp(N_mean3D);
	cv::Mat M_meanTmp(M_mean3D);

	// t = N_mean - lambda*R*M_mean;
	gemm(R, M_meanTmp, lambda, cv::noArray(), 0.0,partial_t);
	t = N_mean - partial_t;

	// print traslation t
	cout << "traslation t between Mi and Ni points \n" << t << endl;

	// return Scale factor
	ScaleFactor = lambda;
}


// find  a 3d point position
void StereoCamera::find3DPoint() {
	// TODO - implement StereoCamera::find3DPoint
	throw "Not yet implemented";
}

// build a normalized projection Matrix P = [R t]
void StereoCamera::build_Projection_Matrix(cv::Mat &P, cv::Mat R, cv::Mat T){

	// get the size of the matrix
	cv::Size matrixSize = P.size();

	// fill the Rotation Part
	for (int i = 0; i < matrixSize.height;i++){
		for (int j = 0; j < matrixSize.width - 1;j++){

			P.at<double>(i, j) = R.at<double>(i,j);
		}
	}

	// fill the Traslation Part
	for (int i = 0; i < matrixSize.height;i++){
		
		P.at<double>(i, matrixSize.width - 1) = T.at<double>(i,0);
	}
	
}

// test a 3D point: all the values must be positive to be correct
//					positive because are in the front of the camera 
//					for details see Zisserman book  on multiple view geometry section 9.6
bool StereoCamera::test3DPoint(vector<cv::Point3f> pointsToTest){

	bool isCorrect = false;
	bool xValue, yValue, zValue;

	for (int i = 0; i < pointsToTest.size(); i++){
	
		zValue = std::signbit(pointsToTest.at(i).z);

		if (!zValue){
			isCorrect = true;
		}
		else{
			isCorrect = false;
			break;
		}
	
	}	
	return isCorrect;
}

// normalize points with a K matrix x_norm = Kinverted*x
void StereoCamera::normalizePoints(cv::Mat K, vector<cv::Point2f> &inputPoints, vector<cv::Point3f> &normalizedPoints){

	cv::Mat tmpMatrix,tmpMatrix2, K_inverted;
	cv::Point3f currentNormalizedPoint;
	cv::Point3f currentNormPoint;
	vector<cv::Point3f> tmpNormalizedPoints;
	convertPointsToHomogeneous(inputPoints, tmpNormalizedPoints);
	tmpMatrix = cv::Mat(3,1,CV_64F);
	K_inverted = K.inv(DECOMP_SVD);

	int pointsSize = inputPoints.size();
	for (int i = 0; i < pointsSize; i++){		

		tmpMatrix.at<double>(0,0) = tmpNormalizedPoints.at(i).x;
		tmpMatrix.at<double>(1,0) = tmpNormalizedPoints.at(i).y;
		tmpMatrix.at<double>(2,0) = tmpNormalizedPoints.at(i).z;

		gemm(K_inverted, tmpMatrix, 1.0, cv::noArray(), 0.0, tmpMatrix2);
		currentNormalizedPoint.x = tmpMatrix2.at<double>(0,0);
		currentNormalizedPoint.y = tmpMatrix2.at<double>(1,0);
		currentNormalizedPoint.z = tmpMatrix2.at<double>(2,0);
		normalizedPoints.push_back(currentNormalizedPoint);

		// save the normalized point
		currentNormPoint.x = currentNormalizedPoint.x;
		currentNormPoint.y = currentNormalizedPoint.y;
		currentNormPoint.z = currentNormalizedPoint.z;
		
	}
}

// perform a tracking test for verification of
//			F,E,P and P' matrices
void StereoCamera::trackTestPointer(){

	int cameraID_Left, cameraID_Right;
	string cameraName_Left("LeftCamera"), cameraName_Right("RightCamera");

	cv::Mat DistortionCoeffsLeft = Mat::zeros(8, 1, CV_64F);
	cv::Mat DistortionCoeffsRight = Mat::zeros(8, 1, CV_64F);

	cv::Mat K_left = Mat::eye(3, 3, CV_64F);
	cv::Mat K_right = Mat::eye(3, 3, CV_64F);

	// get the matrix K and distortion coefficients
	leftCamera.getIntrinsicMatrix(K_left);
	leftCamera.getDistortionMatrix(DistortionCoeffsLeft);

	rightCamera.getIntrinsicMatrix(K_right);
	rightCamera.getDistortionMatrix(DistortionCoeffsRight);

	// get the corresponding input device IDs
	leftCamera.getCameraID(cameraID_Left);
	rightCamera.getCameraID(cameraID_Right);

	// rectify the stereo head 
	// to get a correct alignment and perspective of correct results
	cameraData camL, camR;
	leftCamera.getCameraUsefulParameters(camL);
	rightCamera.getCameraUsefulParameters(camR);
	cv::Size imgSize = cv::Size(camL.imageWidth,camL.imageHeight);

	cv::Mat R1, R2, P1, P2, Q, R, t;
	vector<cv::Mat> stereoTransform;
	getStereoTransforms(stereoTransform);

	stereoTransform.at(0).copyTo(R);
	stereoTransform.at(1).copyTo(t);
 	
	stereoRectify(K_left, DistortionCoeffsLeft,
				  K_right,DistortionCoeffsRight,
				  imgSize, R, t, R1, R2, P1, P2, Q, 0, 1,imgSize);

	// check for image displacement
	bool isVerticalStereo = fabs(P2.at<double>(1,3)) > fabs(P2.at<double>(0,3));


	// Use F matrix to perform uncalibrated rectification
	cv::Mat H1, H2;
	vector<cv::Point2f> leftPoints, rightPoints;
	KeyPoint::convert(matchesLeft, leftPoints);
	KeyPoint::convert(matchesRight, rightPoints);

	stereoRectifyUncalibrated(leftPoints,rightPoints,F_Matrix,imgSize,H1,H2,3);
	R1 = K_left.inv(cv::DECOMP_SVD)*H1*K_left;
	R2 = K_right.inv(cv::DECOMP_SVD)*H1*K_right;
	K_left.copyTo(P1);
	K_right.copyTo(P2);

	// compute and display rectification

	cv::Mat mapX_L,mapY_L;
	cv::Mat mapX_R, mapY_R;
	initUndistortRectifyMap(KLeft,DistortionCoeffsLeft,R1,P1,imgSize,CV_16SC2,mapX_L,mapY_L);
	initUndistortRectifyMap(KRight, DistortionCoeffsRight, R2, P2, imgSize, CV_16SC2, mapX_R, mapY_R);

	cv::Mat canvas;
	double sf;
	int w, h;

	// create the matrix to show the rectification
	sf = 600. / MAX(imgSize.width, imgSize.height);
	w = std::round(imgSize.width*sf);
	h = std::round(imgSize.height*sf);

	if (!isVerticalStereo){

		canvas.create(h,w*2, CV_8UC3);
	}
	else{
		canvas.create(h*2, w, CV_8UC3);
	}

	// perform the remapping
	// choose the middle image pair used to find the matches to perform the rectification
	cv::Mat leftImg, rightImg;
	int choosedOne = std::abs(std::round((leftCalibrationImageList.size() + rightCalibrationImageList.size()) / 4));

	leftImg = leftCalibrationImageList.at(choosedOne);
	rightImg = rightCalibrationImageList.at(choosedOne);

	cv::Mat dstImgL, dstImgR, cImg, canvasPart;	

	remap(leftImg, dstImgL, mapX_L, mapY_L, cv::INTER_LINEAR,BORDER_CONSTANT,Scalar(0,0,0));
	remap(rightImg, dstImgR, mapX_R, mapY_R, cv::INTER_LINEAR,BORDER_CONSTANT, Scalar(0, 0, 0));
	
	//cvtColor(dstImg, cImg, cv::COLOR_GRAY2BGR);
	//canvasPart = !isVerticalStereo? canvas(Rect(w*k,0,w,h)):canvas(Rect(0,h*k,w,h));
	//resize(cImg, canvasPart,canvasPart.size(),0,0,cv::INTER_AREA);


	// sow rectified image
	cv::imshow("rectified left",dstImgL);
	cv::imshow("rectified right", dstImgR);


	// start tracking
	TrackerPoint trackerL(cameraID_Left, cameraName_Left, K_left, DistortionCoeffsLeft);
	TrackerPoint trackerR(cameraID_Right,cameraName_Right,K_right, DistortionCoeffsRight);	

	// register(connect) the signals to the slots(receivers) functions
	trackerL.registerSignal(boost::bind(&StereoCamera::getLeftPoint, this, _1));
	trackerR.registerSignal(boost::bind(&StereoCamera::getRightPoint,this, _1));

	// register(connect) the signal to results evaluation
	trackerL.registerEvaluateSignal(boost::bind(&StereoCamera::evaluateResults, this));		

	// create threads for reading data
	std::function<void(TrackerPoint)> threadTrackingFunction = &TrackerPoint::startTracking;
	
	std::thread leftTrackerThread(threadTrackingFunction,trackerL);
	std::thread rightTrackerThread(threadTrackingFunction,trackerR);

	leftTrackerThread.join();
	rightTrackerThread.join();	

}

// print the contest of a given Matrix
void StereoCamera::printMatrix(cv::Mat Matrix, string matrixName){

	// get the size of the matrix
	string rowContent, rowItem;

	cv::Size matrixSize = Matrix.size();

	cout << matrixName << " content is: " << '\n' << endl;
	for (int i = 0; i < matrixSize.height;i++){
		for (int j = 0; j < matrixSize.width;j++){

			rowItem.assign(std::to_string(Matrix.at<double>(i, j)));
			rowContent.append(rowItem + string("  "));
		}
		cout << rowContent << '\n' << endl;
		rowContent.clear();
	}

}

// get the left point from tracking test
void StereoCamera::getLeftPoint(cv::Point2f leftPoint){

	// update current point position
	leftTrackedPoint = leftPoint;
	cout << "Left Position is: " << leftPoint.x << " " << leftPoint.y << "\n" << endl;
}

// get the right point from tracking test
void StereoCamera::getRightPoint(cv::Point2f rightPoint){

	// update current point position
	rightTrackedPoint = rightPoint;
	cout << "Right Position is: " << rightPoint.x << " " << rightPoint.y << "\n" << endl;

}

// evaluate results from calibration
void StereoCamera::evaluateResults(void){

	// get the K matrices
	cv::Mat K_left = Mat::eye(3, 3, CV_64F);
	cv::Mat K_right = Mat::eye(3, 3, CV_64F);
	cv::Mat P_realLeft(3,4, CV_64F);
	cv::Mat P_realRight(3,4, CV_64F);
	
	vector<cv::Point3f> inputLeft, inputRight;

	// take the found Projection matrices P,P' and triangulate the current point
	cv::Mat triangulateTrackedPoint_3D;
	vector<cv::Point2f> leftPoint, rightPoint, leftTestPoint,rightTestPoint;
	vector<cv::Point3f> leftNormalizedPoint, rightNormalizedPoint;

	leftPoint.push_back(leftTrackedPoint);
	rightPoint.push_back(rightTrackedPoint);

	normalizePoints(KLeft, leftPoint, leftNormalizedPoint);
	normalizePoints(KRight, rightPoint, rightNormalizedPoint);

	convertPointsFromHomogeneous(leftNormalizedPoint, leftTestPoint);
	convertPointsFromHomogeneous(rightNormalizedPoint, rightTestPoint);		

	// build the matrices P =K*PNormalized
	//                    P' = K'*P'Normalized	
	gemm(KLeft,PLeft,1.0,cv::noArray(),0.0,P_realLeft);
	gemm(KRight,PRight,1.0,cv::noArray(),0.0, P_realRight);

	// scaling factor
	double lambda = scaleFactorValue;

	triangulatePoints(PLeft, PRight, leftTestPoint, rightTestPoint, triangulateTrackedPoint_3D);

	// converts from homogeneous
	vector<cv::Point3f> trackedPoint_3D;
	cv::convertPointsFromHomogeneous(triangulateTrackedPoint_3D.reshape(4,1), trackedPoint_3D);
	
	// prints X,Y,Z values
	cout << "the current position of tracked point is: \n"
		<< "X " << lambda*trackedPoint_3D.front().x << "\n"
		<< "Y " << lambda*trackedPoint_3D.front().y << "\n"
		<< "Z " << lambda*trackedPoint_3D.front().z << "\n" << endl;

	// convert to homogeneous points
	cv::convertPointsToHomogeneous(leftPoint, inputLeft);
	cv::convertPointsToHomogeneous(rightPoint, inputRight);

	// call linear LS triangulation
	vector<cv::Point3f> point3D;
	linearLSTriangulation(inputLeft,P_realLeft,inputRight,P_realRight,point3D);

	// prints X,Y,Z values
	cout << "the current position of tracked point using linear LS triangulation is: \n"
		<< "X " << lambda*point3D.front().x << "\n"
		<< "Y " << lambda*point3D.front().y << "\n"
		<< "Z " << lambda*point3D.front().z << "\n" << endl;

		
}

// perform a linear triangulation
// from More that technical web blog
void StereoCamera::linearLSTriangulation(vector<cv::Point3f> PointLeft, cv::Mat P1, vector<cv::Point3f> PointRight, cv::Mat P2, vector<cv::Point3f> &triangulatedPoint){

	// build matrix A for homogeneous equation system Ax = 0
	// it assumes X = (x,y,z,1), for linear LS method
	// which turns it into AX = B system,
	// where A is a 4x3, X is 3x1 and B is 4x1

	cv::Point3f Point1 = PointLeft.front();
	cv::Point3f Point2 = PointRight.front();
	cv::Point3f point3D;

	int x1 = Point1.x;int y1 = Point1.y;
	int x2 = Point2.x;int y2 = Point2.y;

	cv::Mat A =(Mat_<double>(4,3) << x1*P1.at<double>(2, 0) - P1.at<double>(0, 0), x1*P1.at<double>(2, 1) - P1.at<double>(0, 1), x1*P1.at<double>(2, 2) - P1.at<double>(0, 2),
									 y1*P1.at<double>(2, 0) - P1.at<double>(1, 0), y1*P1.at<double>(2, 1) - P1.at<double>(1, 1), y1*P1.at<double>(2, 2) - P1.at<double>(1, 2),
									 x2*P2.at<double>(2, 0) - P2.at<double>(0, 0), x2*P2.at<double>(2, 1) - P2.at<double>(0, 1), x2*P2.at<double>(2, 2) - P2.at<double>(0, 2),
									 y2*P2.at<double>(2, 0) - P2.at<double>(1, 0), y2*P2.at<double>(2, 1) - P2.at<double>(1, 1), y2*P2.at<double>(2, 2) - P2.at<double>(1, 2));


	cv::Mat B = (Mat_<double>(4, 1) << -x1*P1.at<double>(2, 3) - P1.at<double>(0, 3),
									   -y1*P1.at<double>(2, 3) - P1.at<double>(1, 3), 
							           -x2*P2.at<double>(2, 3) - P2.at<double>(0, 3), 
							           -y2*P2.at<double>(2, 3) - P2.at<double>(1, 3));

	cv::Mat X;
	solve(A, B, X, cv::DECOMP_SVD);

	// return the results
	point3D.x = X.at<double>(0,0);
	point3D.y = X.at<double>(1,0);
	point3D.z = X.at<double>(2,0);

	triangulatedPoint.push_back(point3D);
	
}










