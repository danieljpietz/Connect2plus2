//
// Created by danieljpietz on 12/5/20.
//

#ifndef REALTIMEPROJECT_GETBOARD_H
#define REALTIMEPROJECT_GETBOARD_H

#include <opencv2/opencv.hpp>
#include "CircularBuffer.h"

#define boardBufferSize 2

bool isBufferEqual(ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>> buffer);

bool isBoardEqual(std::array<std::array<char, 7>, 6> board1, std::array<std::array<char, 7>, 6> board2);

int getMoveLocation(std::array<std::array<char, 7>, 6> board1, std::array<std::array<char, 7>, 6> board2);

void printBoard(std::array<std::array<char, 7>, 6> board);

std::array<std::array<char, 7>, 6> getBoard(cv::VideoCapture cap);

#endif //REALTIMEPROJECT_GETBOARD_H
