//
// Created by danieljpietz on 12/5/20.
//

#include "GetBoard.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

using namespace cv;
using namespace std;

std::vector<Mat3b> buffer;

/** 
Returns the average of a vector of images. 
Used for noise reduction when determining the state of the board.
**/

Mat3b getMean(const vector<Mat3b> &images) {
    if (images.empty()) return Mat3b();

    // Create a 0 initialized image to use as accumulator
    Mat m(images[0].rows, images[0].cols, CV_64FC3);
    m.setTo(Scalar(0, 0, 0, 0));

    // Use a temp image to hold the conversion of each input image to CV_64FC3
    // This will be allocated just the first time, since all your images have
    // the same size.
    Mat temp;
    for (int i = 0; i < images.size(); ++i) {
        // Convert the input images to CV_64FC3 ...
        images[i].convertTo(temp, CV_64FC3);

        // ... so you can accumulate
        m += temp;
    }

    // Convert back to CV_8UC3 type, applying the division to get the actual mean
    m.convertTo(m, CV_8U, 1. / images.size());
    return m;
}

double avgCircleRadius = 10;

/**
Determines the state of the board given a picture and hole pixel locations.
A hole is marked as occupied if any pixel component is greater than 50.
**/

int iter = 0;

std::array<std::array<char, 7>, 6> getBoardArray(std::array<std::array<Point, 7>, 6> board, Mat src) {
    std::array<std::array<char, 7>, 6> currentBoard;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            auto center = board.at(i).at(j);
            auto radius = avgCircleRadius / 2;

            Mat mask = Mat::zeros(src.rows, src.cols, CV_8UC1);
            circle(mask, center, radius, Scalar(255, 255, 255), FILLED, 8, 0);

            Scalar mm = mean(src, mask);
            if (mm[0] < 50 and mm[1] < 50 and mm[2] < 50) {
                currentBoard[i][j] = 'O';
            } else {
                currentBoard[i][j] = 'X';
                circle(src, center, radius, Scalar(255, 255, 255), FILLED, 8, 0);
            }
        }
    }
    return currentBoard;
}

/**
Checks two board arrays to see if they are equal
**/

bool isBoardEqual(std::array<std::array<char, 7>, 6> board1, std::array<std::array<char, 7>, 6> board2) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            if (board1.at(i).at(j) != board2.at(i).at(j)) {
                return false;
            }
        }
    }
    return true;
}

/**
Checks a board buffer to see if all boards are equal
**/

bool isBufferEqual(ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>> buffer) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            for (int k = 1; k < boardBufferSize; ++k) {
                if (not isBoardEqual(buffer.at(k - 1), buffer.at(k))) {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
Compares two boards to see where a move has been made. Only checks for one move at at time
**/

int getMoveLocation(std::array<std::array<char, 7>, 6> board1, std::array<std::array<char, 7>, 6> board2) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 6; j >= 0; --j) {
            if (board1.at(i).at(j) != board2.at(i).at(j)) {
                return j + 1;
            }
        }
    }
    return 0;
}

/**
Prints the board with 'X' denoting occupied holes and 'O' denoting empty holes.
**/

void printBoard(std::array<std::array<char, 7>, 6> board) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            std::cout << " " << board.at(i).at(j) << " ";
        }
        std::cout << std::endl;
    }
}

cv::Mat lastAvg;
#define bufferSize 15
bool firstIter = true;
int bufferIndex = 0;

/**
Entry point for getting the state of the board from a picture. Alogrithm is as follows.

1. Detect all circles around 30 pixels in diameter
2. Use the top left and bottom right circles as respective hole locations on board
3. Assume even placement of holes, and snap all other circles to fit 7 holes horizontally and 6 vertically
4. Using the average diameter of circles found as a mask, find the average color within each hole to check if it is occupied.
5. Return an array containing occupied holes.
**/

std::array<std::array<Point, 7>, 6> currentBoardArr;

std::array<std::array<char, 7>, 6> getBoard(cv::Mat src) {

    std::array<std::array<Point, 7>, 6> board;

    if (firstIter) {
        for (int i = 0; i < bufferSize; ++i) {
            buffer.push_back(src);
        }

        src = getMean(buffer);
        Mat gray;
        cvtColor(src, gray, COLOR_BGR2GRAY);
        medianBlur(gray, gray, 1);
        std::vector<Vec3f> circles;
        std::vector<int> radii;
        HoughCircles(gray, circles, HOUGH_GRADIENT, 1,
                     gray.rows / 64,  // change this value to detect circles with different distances to each other
                     100, 30, 9, 25 // change the last two parameters
                // (min_radius & max_radius) to detect larger circles
        );

        int minX, minY, maxX, maxY = 0;

        minX = circles[0][0];
        minY = circles[0][1];
        maxX = circles[0][0];
        maxY = circles[0][1];

        for (size_t i = 0; i < circles.size(); i++) {
            Vec3i c = circles[i];
            radii.push_back(c[2]);
            Point center = Point(c[0], c[1]);

            if (center.x <= minX) {
                minX = center.x;
            } else if (center.x >= maxX) {
                maxX = center.x;
            }

            if (center.y <= minY) {
                minY = center.y;
            } else if (center.y >= maxY) {
                maxY = center.y;
            }

            //circle(src, center,radius, color,FILLED, 8,0);
            //circle( src, center, 1, Scalar(0,100,100), 3, LINE_AA);
            //circle( src, center, radius, Scalar(255,0,255), 3, LINE_AA);
        }
        for (int i = 0; i < radii.size(); ++i) {
            avgCircleRadius += radii[i] / radii.size();
        }


        for (size_t i = 0; i < circles.size(); i++) {
            Vec3i c = circles[i];
            Point center = Point(c[0], c[1]);
            int boardX = ceil(7 * (((float) center.x - minX) / (maxX - minX))) - 1;
            if (boardX == -1) {
                ++boardX;
            }
            int boardY = ceil(6 * (((float) center.y - minY) / (maxY - minY))) - 1;
            if (boardY == -1) {
                ++boardY;
            }
            board.at(boardY).at(boardX) = center;
        }
        currentBoardArr = board;

    firstIter = false;

    } else {
        buffer[bufferIndex++ % bufferSize] = src;
    }
    return getBoardArray(currentBoardArr, src);
}
