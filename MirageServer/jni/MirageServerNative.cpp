/*
 * Matcher.cpp
 *
 *  Created on: Jan 20, 2013
 *      Author: diego
 */
#include <jni.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include "TargetImage.h"
using namespace std;
using namespace cv;

#define MAX_ITEM 200
#define MAX_KEY 500
#define INF 10000
#define EPSILON 0.0001
#define _DEBUG true

template<class T, class U>
bool compare(const pair<T, U> &a, const pair<T, U> &b) {
	return a.first > b.first;
}
#ifdef __cplusplus
extern "C" {
#endif

static vector<TargetImage> targetImages;

/**
 * Convert data stored in an array into keypoints and descriptor
 */
void readKeyAndDesc(vector<KeyPoint> &trainKeys, Mat &trainDes, float *mdata, int &count) {
	// doc du lieu
	int keyNum, octave, classId;
	float x, y, angle, size, response;
	keyNum = mdata[count++];

	for (int i = 0; i < keyNum; ++i) {
		angle = mdata[count++];
		classId = mdata[count++];
		octave = mdata[count++];
		x = mdata[count++];
		y = mdata[count++];
		response = mdata[count++];
		size = mdata[count++];
		KeyPoint p(x, y, size, angle, response, octave, classId);
		trainKeys.push_back(p);
	}

	int rows, cols, type;
	uchar *data;
	rows = mdata[count++];
	cols = mdata[count++];
	type = mdata[count++];
	int matSize = rows * cols;

	data = new uchar[matSize];
	for (int i = 0; i < matSize; ++i) {
		data[i] = mdata[count++];
	}

	trainDes = Mat(rows, cols, CV_8U, data);
}

/**KeyPoint
 * Read database from an array
 */
void readDatabase(vector<vector<KeyPoint> > &queryKeys, vector<Mat> &queryDes,vector<Size>& queryDims, float *mdata, int &count) {
	int querySize;
	//scanf("%d", &querySize);
	//ss >> querySize;
	querySize = mdata[count++];
	for (int i = 0; i < querySize; ++i) {
		vector < KeyPoint > qK;
		Mat qD;
		Size qS;
		int ID;
		TargetImage targetimage;
		ID = mdata[count++];
		qS.width = mdata[count++];
		qS.height = mdata[count++];
		readKeyAndDesc(qK, qD, mdata, count);
		queryKeys.push_back(qK);
		queryDes.push_back(qD);
		queryDims.push_back(qS);
		targetimage.setId(ID);
                targetimage.setSize(qS);
                targetimage.setDescriptor(qD);
                targetimage.setKeypoints(qK);
                targetImages.push_back(targetimage);

	}
}
/**KeyPoint
 * Read database from an array
 */
void readDB(float *mdata, int &count) {
        int querySize;
        //scanf("%d", &querySize);
        //ss >> querySize;
        querySize = mdata[count++];
        for (int i = 0; i < querySize; ++i) {
                vector < KeyPoint > qK;
                Mat qD;
                Size qS;
                int ID;
                TargetImage targetimage;
                ID = mdata[count++];
                qS.width = mdata[count++];
                qS.height = mdata[count++];
                readKeyAndDesc(qK, qD, mdata, count);

                targetimage.setId(ID);
                targetimage.setSize(qS);
                targetimage.setDescriptor(qD);
                targetimage.setKeypoints(qK);
                targetImages.push_back(targetimage);

        }
}

/**
 *  Checks the homography of the Homography
 */
bool niceHomography(const Mat& H)
{
    const double det = (H.at<double>(0,0) * H.at<double>(1,1)) - (H.at<double>(1,0) * H.at<double>(0,1));
    if (det < 0)
      return false;
//
    const double N1 = sqrt((H.at<double>( 0, 0) * H.at<double>( 0, 0)) + (H.at<double>( 1, 0) * H.at<double>( 1, 0)));
    if (N1 > 4 || N1 < 0.1)
      return false;
//
    const double N2 = sqrt((H.at<double>( 0, 1) * H.at<double>( 0, 1)) + (H.at<double>( 1, 1) * H.at<double>( 1, 1)));
    if (N2 > 4 || N2 < 0.1)
      return false;
//
    const double N3 = sqrt((H.at<double>( 2, 0) * H.at<double>( 2, 0)) + (H.at<double>( 2, 1) * H.at<double>( 2, 1)));
    if (N3 > 0.002)
      return false;

  return true;
}

inline bool refineMatchesWithHomography
    (
    const std::vector<cv::KeyPoint>& queryKeypoints,
    const std::vector<cv::KeyPoint>& trainKeypoints,
    float reprojectionThreshold,
    std::vector<cv::DMatch>& matches,
    cv::Mat& homography
    )
{
    const unsigned int minNumberMatchesAllowed = 15;

    if (matches.size() < minNumberMatchesAllowed)
        return false;

    // Prepare data for cv::findHomography
    std::vector<cv::Point2f> srcPoints(matches.size());
    std::vector<cv::Point2f> dstPoints(matches.size());

    for (size_t i = 0; i < matches.size(); i++)
    {
        srcPoints[i] = trainKeypoints[matches[i].trainIdx].pt;
        dstPoints[i] = queryKeypoints[matches[i].queryIdx].pt;
    }

    // Find homography matrix and get inliers mask
    std::vector<unsigned char> inliersMask(srcPoints.size());
    homography = cv::findHomography(srcPoints,
                                    dstPoints,
                                    CV_FM_RANSAC,
                                    reprojectionThreshold,
                                    inliersMask);
    std::vector<cv::DMatch> inliers;
    for (size_t i=0; i<inliersMask.size(); i++)
    {
        if (inliersMask[i])
            inliers.push_back(matches[i]);
    }


    matches.swap(inliers);
    return (matches.size() > minNumberMatchesAllowed) && niceHomography(homography);
}

inline void showimage(string title, Mat& img) {
	Mat im_out;
	resize(img, im_out, Size((img.cols / img.rows) * 640, 640), 0, 0,
			INTER_LINEAR);
	imshow(title, im_out);
	waitKey(5);
}
inline void extractFeatures(const Mat& img, Mat& des, vector<KeyPoint>& keys)
{
        // detect image keypoints

        ORB sfd1(1000);
        FREAK sde(true,true,22,8);
        sfd1.detect(img, keys);
        //cerr << "Train keys size start " << keys.size() << endl;
        int s = 500;
        // select only the appropriate number of keypoints
        while(keys.size() > 1000) {
                //cerr << "Train keys size " << keys.size() << endl;
                keys.clear();
                ORB sfd(s+500);
                s += 500;
                sfd1.detect(img, keys);
        }

        // compute image descriptor
        sde.compute(img, keys, des);
}inline void ratiotest(vector<DMatch> &matches,const Mat& queryDes,int j){
  double max_dist = 0; double min_dist = 100;

   //-- Quick calculation of max and min distances between keypoints
   for( int i = 0; i < queryDes.rows; i++ )
   { double dist = matches[i].distance;
     if( dist < min_dist ) min_dist = dist;
     if( dist > max_dist ) max_dist = dist;
   }

   //printf("-- Max dist %d: %f \n", j,max_dist );
   //printf("-- Min dist %d: %f \n", j,min_dist );

   //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
   std::vector< DMatch > good_matches;

   for( int i = 0; i < queryDes.rows; i++ )
   { if( matches[i].distance < 4*min_dist )
      { good_matches.push_back( matches[i]); }
   }
   matches.swap(good_matches);
}
inline void drawHomography(Mat& img, const std::vector<KeyPoint>& keypoints_object,const std::vector<KeyPoint>& keypoints_scene, const Size& dim, const vector<DMatch>& good_matches) {

  Mat img_scene= img.clone();

  //-- Localize the object
  std::vector<Point2f> obj;
  std::vector<Point2f> scene;

  for( int i = 0; i < good_matches.size(); i++ )
  {
    //-- Get the keypoints from the good matches
    obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
    scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
  }

  Mat H = findHomography( obj, scene, CV_FM_RANSAC,5 );
//
//    //-- Get the corners from the image_1 ( the object to be "detected" )
  std::vector<Point2f> obj_corners(4);
  obj_corners[0] = cvPoint(0,0); obj_corners[1] = cvPoint( dim.width, 0 );
  obj_corners[2] = cvPoint( dim.width, dim.height); obj_corners[3] = cvPoint( 0, dim.height);
  std::vector<Point2f> scene_corners(4);
//
//
//
  perspectiveTransform( obj_corners, scene_corners, H);
//
//    //-- Draw lines between the corners (the mapped object in the scene - image_2 )
//
  line( img_scene, scene_corners[0], scene_corners[1] , Scalar(0, 255, 0), 10 );
  line( img_scene, scene_corners[1] , scene_corners[2], Scalar( 0, 255, 0), 10 );
  line( img_scene, scene_corners[2] , scene_corners[3], Scalar( 0, 255, 0), 10 );
  line( img_scene, scene_corners[3] , scene_corners[0], Scalar( 0, 255, 0), 10 );

  //-- Show detected matches

  showimage( "Good Matches & Object detection", img_scene );

}
/**
* Match the query image to images in database. The best matches are returned
*/
inline void matchtest(Mat& m_grayImg, const vector<KeyPoint> &trainKeys, const Mat &trainDes,vector<pair<float, int> > &result) {
    cv::FlannBasedMatcher bf(new flann::LshIndexParams(20,10,2));

    // train the query image
    int size = targetImages.size();
    int num_matches_thresh = 6;
    for(int i = 0; i < size; ++i) {
           // compute match score for each image in the database
           vector<DMatch> matches;
           vector<DMatch> refinedmatches;
           bf.match(targetImages[i].getDescriptor(),trainDes, matches);

           if(matches.size()< static_cast<size_t>(num_matches_thresh))
               continue;
          // Construct point-point correspondences for homography estimation
          Mat src_points(1, static_cast<int>(matches.size()), CV_32FC2);
          Mat dst_points(1, static_cast<int>(matches.size()), CV_32FC2);
          for (size_t i = 0; i < matches.size(); ++i)
          {
              const DMatch& m = matches[i];

              Point2f p = trainKeys[m.queryIdx].pt;
              p.x -= train.width * 0.5f;
              p.y -= features1.img_size.height * 0.5f;
              src_points.at<Point2f>(0, static_cast<int>(i)) = p;

              p = features2.keypoints[m.trainIdx].pt;
              p.x -= features2.img_size.width * 0.5f;
              p.y -= features2.img_size.height * 0.5f;
              dst_points.at<Point2f>(0, static_cast<int>(i)) = p;
          }

    }
}
/**
* Match the query image to images in database. The best matches are returned
*/
inline void match(Mat& m_grayImg, const vector<KeyPoint> &trainKeys, const Mat &trainDes,vector<pair<float, int> > &result) {

      cv::FlannBasedMatcher bf(new flann::LshIndexParams(20,10,2));
      //BFMatcher bf(NORM_HAMMING,true);


      // train the query image
      int size = targetImages.size();
      for(int i = 0; i < size; ++i) {
              // compute match score for each image in the database
              vector<DMatch> matches;
              vector<DMatch> refinedmatches;
              bf.match(targetImages[i].getDescriptor(),trainDes, matches);
              ratiotest(matches,targetImages[i].getDescriptor(),i);

              //Find homography transformation and detect good matches
              cv::Mat m_roughHomography;
              cv::Mat m_refinedHomography;

              bool homographyFound = refineMatchesWithHomography(
                                      targetImages[i].getKeypoints(),trainKeys,

                                      2,
                                      matches,
                                      m_roughHomography);
              if(homographyFound){
                  //Testing the homography

                  Mat m_warpedImg;
                  cv::warpPerspective(m_grayImg, m_warpedImg, m_roughHomography, targetImages[i].getSize(), cv::INTER_LINEAR);

                  //Shoe Warped Image
                  //showimage("Title",m_warpedImg);

                  //Extract Warped Image Keys
                  Mat warpDes;
                  vector<KeyPoint> warpKeys;
                  extractFeatures(m_grayImg,warpDes,warpKeys);

                  //Match
                  bf.match(targetImages[i].getDescriptor(),warpDes, refinedmatches);
                  ratiotest(refinedmatches,targetImages[i].getDescriptor(),i);
                  homographyFound = refineMatchesWithHomography(
                      targetImages[i].getKeypoints(),warpKeys,

                                          2,
                                          refinedmatches,
                                          m_refinedHomography);
                  if(homographyFound){
                                drawHomography(m_grayImg,targetImages[i].getKeypoints(),trainKeys,targetImages[i].getSize(),matches);
                                pair <float, int> p(matches.size(), targetImages[i].getId());
                                result.push_back(p);
                  }
              }
      }

      // sort in descending
      std::sort(result.begin(), result.end(), compare<float, int>);
}

/**
 * Get min value of two number
 */
inline int min(int a, int b) {
	return a > b ? b : a;
}
JNIEXPORT void JNICALL Java_com_server_Matcher_load(JNIEnv *env,jclass obj){
      cerr << "Loading... " << endl;


      FILE * pFile;
      long lSize;
      char * buffer;
      size_t sresult;

      pFile = fopen("Data.txt", "rb");
      if (pFile == NULL) {
              fputs("File error", stderr);
              exit(1);
      }

      // obtain file size:
      fseek(pFile, 0, SEEK_END);
      lSize = ftell(pFile);
      rewind(pFile);

      // allocate memory to contain the whole file:
      buffer = (char*) malloc(sizeof(char) * lSize);
      if (buffer == NULL) {
              fputs("Memory error", stderr);
              exit(2);
      }

      // copy the file into the buffer:
      sresult = fread(buffer, 1, lSize, pFile);
      if (sresult != lSize) {
              fputs("Reading error", stderr);
              exit(3);
      }

      /* the whole file is now loaded in the memory buffer. */

      // terminateknn
      //fclose (pFile);
      int dataSize, count = 0;
      char *endPtr;
      dataSize = strtol(buffer, &endPtr, 10);
      float *mdata = new float[dataSize];
      // read data as an array of float number
      for (int i = 0; i < dataSize; ++i) {
              mdata[i] = strtod(endPtr, &endPtr);
      }

      readDB(mdata, count);
      fclose(pFile);
      delete[] mdata;
      cerr << "Loading Done" << endl;

}

JNIEXPORT jintArray JNICALL Java_com_server_Matcher_recognition(JNIEnv *env,
		jclass obj, jstring path) {

	const char *nativeString = (*env).GetStringUTFChars(path, 0);

	cerr << "Start " << endl;
	cerr << "File: " << nativeString << endl;

	// read image from file
	vector < KeyPoint > trainKeys;
	Mat trainDes, img = imread(nativeString, 0);
	vector<pair<float, int> > result;

	// detect image keypoints
	extractFeatures(img, trainDes, trainKeys);

	//Load trainIG
	TargetImage trainTI;



	//Change to add the homography and the debug
	cerr << "Matching begin" << endl;
        match(img,trainKeys, trainDes, result);
	int size = min(result.size(), MAX_ITEM);
	// print out the best result
	printf("Size: %d\n", result.size());

	jintArray resultArray;
	resultArray = (*env).NewIntArray(size);
	if (resultArray == NULL) {
		return NULL; /* out of memory error thrown */
	}

	jint fill[size];

	for (int i = 0; i < size; ++i) {
		fill[i] = result[i].second;
		cout << result[i].first << " " << result[i].second << endl;
	}
	trainDes.release();
	trainKeys.clear();

	(*env).SetIntArrayRegion(resultArray, 0, size, fill);
	(*env).ReleaseStringUTFChars(path, nativeString);
	return resultArray;

}

#ifdef __cplusplus
}
#endif
