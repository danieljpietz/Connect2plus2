#if 0

#include <iostream>
#include <cmath>
#include <array>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include "CircularBuffer.h"
#include "GetBoard.h"

using namespace cv;

#define boardBufferSize 10

void printBoard(std::array<std::array<char, 7>, 6> board) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            std::cout << " " << board.at(i).at(j) << " ";
        }
        std::cout << std::endl;
    }
}

std::array<std::array<char, 7>, 6> getBoard(std::array<std::array<Point, 7>, 6> board, Mat image) {
    std::array<std::array<char, 7>, 6> currentBoard;
    medianBlur(image, image, 1);
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 7; ++j) {
            Vec3b color = image.at<Vec3b>(board.at(i).at(j));
            if (color[0] < 100 and color[1] < 100 and color[2] > 100) {
                currentBoard[i][j] = 'R';
                continue;
            }

            if (color[0] < 180 and color[1] > 70 and color[2] > 70) {
                currentBoard[i][j] = 'X';
                continue;
            }
            currentBoard[i][j] = 'X';
        }
    }
    return currentBoard;
}

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

int getMoveLocation(std::array<std::array<char, 7>, 6> board1, std::array<std::array<char, 7>, 6> board2) {
    for (int i = 0; i < 6; ++i) {
        for (int j = 6; j >= 0; --j) {
            if (board1.at(i).at(j) == 'X' and board2.at(i).at(j) != 'X') {
                return j;
            }
        }
    }
    return 0;
}

void draw(Mat& src) {
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
    int minX, minY, maxX, maxY = 0;

    for (size_t i = 0; i < circles.size(); i++) {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        // circle center
        Vec3b color = src.at<Vec3b>(center);
        //circle(src, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
        // circle outline
        int radius = c[2];
        if (color[0] < 120 and color[1] < 120 and color[2] > 120) {
            circle(src, center, radius, Scalar(0, 0, 255), 3, LINE_AA);
        }

        if (color[0] < 150 and color[1] > 80 and color[2] > 80) {
            circle(src, center, radius, Scalar(255, 255, 255), 3, LINE_AA);
            continue;
        }
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    Mat src;
    VideoCapture cap(0);
    String window_capture_name = "Video Capture";
    cap >> src;



    medianBlur(src, src, 1);
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
    int minX, minY, maxX, maxY = 0;

    minX = circles[0][0];
    minY = circles[0][1];
    maxX = circles[0][0];
    maxY = circles[0][1];


    for (size_t i = 0; i < circles.size(); i++) {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        // circle center
        Vec3b color = src.at<Vec3b>(center);
        //circle(src, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
        // circle outline
        int radius = c[2];
        cvtColor(src, src, COLOR_BGR2HSV);
        if ((0 <= color[0] and color[0] <= 180)) {
            circle(src, center, radius, Scalar(0, 0, 255), 3, LINE_AA);
        }

        if ((23 <= color[0] and color[0] <= 74)) {
            circle(src, center, radius, Scalar(255, 255, 255), 3, LINE_AA);

        }

        cvtColor(src, src, COLOR_HSV2BGR);

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
    }

    imshow(window_capture_name, src);
    waitKey();

    std::array<std::array<Point, 7>, 6> board;

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
        std::cout << boardX << "," << boardY << std::endl;
        board.at(boardY).at(boardX) = center;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto t3 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>> boardBuffer =
            ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>>();
    std::array<std::array<char, 7>, 6> currentBoard, capturedBoard;
    cap >> src;
    currentBoard = getBoard(board, src);

    for (int i = 0; i < boardBufferSize; ++i) {
        boardBuffer.insert(currentBoard);
    }

    while (true) {
        capturedBoard = getBoard(board, src);
        if (not isBoardEqual(currentBoard, capturedBoard)) {
            boardBuffer.insert(capturedBoard);
            if (isBufferEqual(boardBuffer)) {
                std::cout << "Move Made! " << getMoveLocation(currentBoard, capturedBoard) << std::endl;
                currentBoard = capturedBoard;
                std::cout << "CURRENT\n";
                printBoard(currentBoard);
                std::cout << "Captured\n";
                printBoard(capturedBoard);
            }
        }
        cap >> src;
    }

    std::cout << t3 / 10 << std::endl;


    //waitKey();

    return 0;


}

#endif