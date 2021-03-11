//
// Created by liuyang on 2021/3/9.
//

#ifndef VIDEOPLAYER_FACETRACK_H
#define VIDEOPLAYER_FACETRACK_H

#include <seeta/FaceLandmarker.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

using namespace cv;
using namespace seeta;

class CascadeDetectorAdapter : public DetectionBasedTracker::IDetector {
public:
    CascadeDetectorAdapter(cv::Ptr <cv::CascadeClassifier> detector) :
            IDetector(),
            Detector(detector) {
        CV_Assert(detector);
    }

    void detect(const cv::Mat &Image, std::vector <cv::Rect> &objects) {

        Detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize,
                                   maxObjSize);

    }

    virtual ~CascadeDetectorAdapter() {

    }

private:
    CascadeDetectorAdapter();

    cv::Ptr <cv::CascadeClassifier> Detector;
};


class FaceTrack {
public:
    FaceTrack(const char *faceModel, const char *landMarkerModel);

    ~FaceTrack();

    void stop();

    void run();

    void process(Mat src, cv::Rect &face, std::vector <SeetaPointF> &points);

private:
    DetectionBasedTracker *tracker;
    FaceLandmarker *landmarker;
};

#endif //VIDEOPLAYER_FACETRACK_H
