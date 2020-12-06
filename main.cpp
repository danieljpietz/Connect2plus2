#include <iostream>
#include <cmath>
#include <algorithm>    // std::sort
#include <array>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "GetBoard.h"

using namespace cv;

void checkBoard(std::array<std::array<Point, 7>, 6> board, Mat image) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            Vec3b color = image.at<Vec3b>(board.at(i).at(j));
            if (color[0] < 120 and color[1] < 120 and color[2] > 120) {
                std::cout << " R ";
                continue;
            }

            if (color[0] < 150 and color[1] > 80 and color[2] > 80) {
                std::cout << " Y ";
                continue;
            }
            std::cout << " X ";
        }

        std::cout << std::endl;
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    Mat src;
    VideoCapture cap(1);
    String window_capture_name = "Video Capture";
    namedWindow(window_capture_name);
    cap >> src;
    Mat mask;
    cv::Mat dstImage = cv::Mat::zeros(src.size(), src.type());
    inRange(src, Scalar(0, 0, 0), Scalar(255, 255, 255), mask);
    src.copyTo(dstImage, mask);
    Mat gray;
    cvtColor(dstImage, gray, COLOR_BGR2GRAY);
    std::vector<Vec3f> circles;
    HoughCircles(gray, circles, HOUGH_GRADIENT, 1,
                 gray.rows / 32,  // change this value to detect circles with different distances to each other
                 100, 30, 1, 30 // change the last two parameters
            // (min_radius & max_radius) to detect larger circles
    );
    std::cout << "XXXXX" << std::endl;

    int minX, minY, maxX, maxY = 0;

    minX = circles[0][0];
    minY = circles[0][1];
    maxX = circles[0][0];
    maxY = circles[0][1];



    for (size_t i = 0; i < circles.size(); i++) {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        center.x = 5*((float)center.x/5);
        center.y = 5*((float)center.y/5);
        // circle center
        Vec3b color = src.at<Vec3b>(center);
        //circle(src, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
        // circle outline
        int radius = c[2];
        if (color[0] < 120 and color[1] < 120 and color[2] > 120) {
            circle(src, center, radius, Scalar(255, 0, 255), 3, LINE_AA);
        }

        if (center.x <= minX ) {
            minX = center.x;
        }
        else if (center.x >= maxX) {
            maxX = center.x;
        }

        if (center.y <= minY ) {
            minY = center.y;
        }
        else if (center.y >= maxY) {
            maxY = center.y;
        }
    }

    std::array<std::array<Point, 7>,  6> board;

    for (size_t i = 0; i < circles.size(); i++) {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        Point centerNotRounded = Point(c[0], c[1]);
        center.x = 5*((float)center.x/5);
        center.y = 5*((float)center.y/5);
        int boardX = ceil(7 * (((float)center.x - minX)/(maxX - minX))) - 1;
        if(boardX == -1) {
            ++boardX;
        }
        int boardY = ceil(6 * (((float)center.y - minY)/(maxY - minY))) - 1;
        if(boardY == -1) {
            ++boardY;
        }
        std::cout << boardX << "," << boardY << std::endl;
        board.at(boardY).at(boardX) = centerNotRounded;
    }

    std::cout << minX << std::endl;
    std::cout << maxX << std::endl;

    checkBoard(board, src);

        imshow(window_capture_name, src);

        waitKey();

        return 0;


}
