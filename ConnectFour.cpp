#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <sched.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "GetBoard.h"
#include <string>
#include "connect4/Solver.hpp"
#include "connect4/Position.hpp"
#include "connect4/TranspositionTable.hpp"
#include "connect4/OpeningBook.hpp"


cv::VideoCapture cap;
cv::Mat latestImage;
#define NUM_THREADS (4)
#define NUM_CPUS (4)

#define moveWaitThreshold 0.5

#define MY_SCHEDULER SCHED_FIFO

int numberOfProcessors = NUM_CPUS;

unsigned int idx = 0, jdx = 1;
unsigned int seqIterations = 47;
unsigned int reqIterations = 10000000;
volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;

auto tLastMove = std::chrono::high_resolution_clock::now();
int lastMoveCol = 0;
typedef struct {
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;

pthread_mutex_t boardLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t moveLock = PTHREAD_MUTEX_INITIALIZER;


std::array<std::array<char, 7>, 6> capturedBoard;
std::array<std::array<char, 7>, 6> currentBoard;

ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>> boardBuffer =
        ATLAS::CircularBuffer<boardBufferSize, std::array<std::array<char, 7>, 6>>();

bool thread1SetupDone = false;
bool thread2SetupDone = false;
bool thread3SetupDone = false;
bool thread4SetupDone = false;

std::string gameString = "";

volatile int strongAnswer = 0;
volatile int weakAnswer = 0;

int tick = 0;

/**
This is the image recognition thread. It constantly looks at the camera feed
to determine the state of the board. This board is then returned as 2 dimensional 7x6 array
of characters with an 'X' denoting positions with a piece and an 'O' denoting empty positions.
**/

[[noreturn]] void *thread1(void *args) {
    std::cout << "Image thread running" << std::endl;
    cap >> latestImage;
    capturedBoard = getBoard(latestImage);
    currentBoard = capturedBoard;
    for (int i = 0; i < boardBufferSize; ++i) {
        boardBuffer.insert(capturedBoard);
    }
    thread1SetupDone = true;

    while (true) {
        if (pthread_mutex_trylock(&boardLock) == 0) {
            cap >> latestImage;
            capturedBoard = getBoard(latestImage);
            pthread_mutex_unlock(&boardLock);
        }

    }
}

int moveCount = 0;

/**
This is the move detection thread. It constantly looks at the capturedBoard object
and boardBuffer to determine if a move has been played. If a change is detected, 
the change is added to the buffer. When the buffer is in agreement on  the change,
it reports that a move has been made.
**/

[[noreturn]] void *thread2(void *args) {
    std::cout << "Move thread running" << std::endl;
    thread2SetupDone = true;
    while (true) {
        auto T1 = std::chrono::high_resolution_clock::now();
        if (not isBoardEqual(currentBoard, capturedBoard)) {
            if (pthread_mutex_trylock(&boardLock) == 0) {
                boardBuffer.insert(capturedBoard);
                if (isBufferEqual(boardBuffer)) {
                    int moveLoc = getMoveLocation(currentBoard, capturedBoard);
                    if (moveLoc == 0) {
                        continue;
                    }
                    auto tMove = std::chrono::high_resolution_clock::now();
                    auto tDelta = std::chrono::duration_cast<std::chrono::milliseconds>(tMove - tLastMove).count();
                    if (tDelta > (5000 * moveWaitThreshold)) {
                        std::cout << "Move Made! " << moveLoc << std::endl;
                        strongAnswer = 0;
                        weakAnswer = 0;
                        lastMoveCol = moveLoc;
                        gameString = gameString + std::to_string(moveLoc);
                        while (strongAnswer == 0 and std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::high_resolution_clock::now() - tMove).count() < 250) {
                        }
                        if (strongAnswer != 0) {
                            std::cout << "Next move Strong: " << std::to_string(strongAnswer) << std::endl;
                        }
                        else {
                            std::cout << "Next move Weak: " << std::to_string(weakAnswer) << std::endl;
                        }
                        currentBoard = capturedBoard;
                        tLastMove = tMove;
                    }
                    auto T2 = std::chrono::high_resolution_clock::now();
                    //std::cout << "TIME: " << std::chrono::duration_cast<std::chrono::nanoseconds>(T2 - T1).count() << std::endl;

                }
                pthread_mutex_unlock(&boardLock);
            }
        }

    }
}

/**
Strong solver thread. This will return the best possible move the player can make, using number of remaining moves
needed to win as a metric. This takes several several seconds to solve early in the game, so the weak solution is given
if it takes more than half a second to respond.
**/

[[noreturn]] void *thread3(void *args) {
    std::cout << "Strong solver thread running" << std::endl;

    GameSolver::Connect4::Solver solver;

    bool weak = false;
    std::string opening_book = "connect4/7x6.book";
    solver.loadBook(opening_book);
    GameSolver::Connect4::Position P;
    std::string previousGameString = "";
    thread3SetupDone = true;

    while (true) {
        if (not(previousGameString == gameString)) {
            //if (pthread_mutex_trylock(&moveLock) == 0) {
            for (int column = 1; column <= 7; column++) {
                std::string copyGameString = gameString;
                copyGameString.std::string::append(std::to_string(column));
                //std::cout << "T1 " << P.play(copyGameString) << " T2 " << copyGameString.size() << std::endl;
                if (P.play(copyGameString) != copyGameString.size()) {
                    //std::cerr << "Next Move: " << copyGameString << ": Invalid move " << (P.nbMoves() + 1) << " \""
                              //<< gameString << "\"" << std::endl;
                } else {
                    solver.reset();
                    int score = solver.solve(P, weak);
                    strongAnswer = column;
                    //std::cout << "Next Move (" << column << "): " << copyGameString << " score: " << score
                              //<< " nodes iterated: " << solver.getNodeCount();
                    std::cout << "Strong Answer: " << strongAnswer<< std::endl;
                }
            }
            previousGameString = gameString;
            //pthread_mutex_unlock(&moveLock);
            //}
        }
    }

}

/**
Weak solver thread. This solution executes extremely quickly but only guarenteess 
that the opponent will not win on the next move. It is used as a reccomendation if the 
strong solver takes more than half a second to return a result.
**/

[[noreturn]] void *thread4(void *args) {
    std::cout << "Weak solver thread running" << std::endl;

    GameSolver::Connect4::Position P;
    std::string previousGameString = "";
    std::srand(std::time(nullptr));
    thread4SetupDone = true;

    while (true) {
        if (not(previousGameString == gameString)) {
            auto T1 = std::chrono::high_resolution_clock::now();
            for (int i = 1; i <= 24; i++) {
                int column = 1 + std::rand()/((RAND_MAX + 1u)/7);
                P.play(gameString);
                if (P.canPlay(column)) {
                    weakAnswer = column;
                    break;
                }
            }
            if (weakAnswer == 0) {
                for (int i = 1; i <= 7; i++) {
                    int column = i;
                    P.play(gameString);
                    if (P.canPlay(column)) {
                        weakAnswer = column;
                        break;
                    }
                }
                if (weakAnswer == 0) {
                    throw;
                }
            }
            previousGameString = gameString;
        }

    }

}

void print_scheduler(void) {
    int schedType, scope;

    schedType = sched_getscheduler(getpid());

    switch (schedType) {
        case SCHED_FIFO:
            printf("Pthread Policy is SCHED_FIFO\n");
            break;
        case SCHED_OTHER:
            printf("Pthread Policy is SCHED_OTHER\n");
            break;
        case SCHED_RR:
            printf("Pthread Policy is SCHED_RR\n");
            break;
        default:
            printf("Pthread Policy is UNKNOWN\n");
    }

    pthread_attr_getscope(&main_attr, &scope);

    if (scope == PTHREAD_SCOPE_SYSTEM)
        printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
        printf("PTHREAD SCOPE PROCESS\n");
    else
        printf("PTHREAD SCOPE UNKNOWN\n");

}

/**
This function initializes all other threads, done in a SCHED_FIFO scheduling system
**/

int main(int argc, char *argv[]) {
    std::cout << "Welcome to Connect 4" << std::endl;
    cap = cv::VideoCapture(0);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    int rc, idx;
    int i;
    cpu_set_t threadcpu;
    int coreid;

    numberOfProcessors = get_nprocs_conf();

    printf("This system has %d processors with %d available\n", numberOfProcessors, get_nprocs());
    printf("The test thread created will be run on a specific core based on thread index\n");

    mainpid = getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    print_scheduler();
    rc = sched_getparam(mainpid, &main_param);
    main_param.sched_priority = rt_max_prio;

    if (MY_SCHEDULER != SCHED_OTHER) {
        if (rc = sched_setscheduler(getpid(), MY_SCHEDULER, &main_param) < 0)
            perror("******** WARNING: sched_setscheduler");
    }

    print_scheduler();


    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);



    ///////
    //////
    i = 0;
    CPU_ZERO(&threadcpu);

    coreid = i % numberOfProcessors;
    printf("Setting thread %d to core %d\n", i, coreid);

    CPU_SET(coreid, &threadcpu);

    rc = pthread_attr_init(&rt_sched_attr[i]);
    rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
    rc = pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    rt_param[i].sched_priority = rt_max_prio - i - 1;
    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    threadParams[i].threadIdx = i;

    pthread_create(&threads[i],               // pointer to thread descriptor
                   &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                   thread1,              // thread function entry point
                   (void *) &(threadParams[i]) // parameters to pass in
    );

    while (!thread1SetupDone) {
        std::cout << "Thread 1 spin";
    }
    std::cout << "Thread 1 Setup Done" << std::endl;
    //////
    //////
    i = 1;
    CPU_ZERO(&threadcpu);

    coreid = i % numberOfProcessors;
    printf("Setting thread %d to core %d\n", i, coreid);

    CPU_SET(coreid, &threadcpu);

    rc = pthread_attr_init(&rt_sched_attr[i]);
    rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
    rc = pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    rt_param[i].sched_priority = rt_max_prio - i - 1;
    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    threadParams[i].threadIdx = i;

    pthread_create(&threads[i],               // pointer to thread descriptor
                   &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                   thread2,              // thread function entry point
                   (void *) &(threadParams[i]) // parameters to pass in
    );


    //////
    //////
    i = 2;
    CPU_ZERO(&threadcpu);

    coreid = i % numberOfProcessors;
    printf("Setting thread %d to core %d\n", i, coreid);

    CPU_SET(coreid, &threadcpu);

    rc = pthread_attr_init(&rt_sched_attr[i]);
    rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
    rc = pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    rt_param[i].sched_priority = rt_max_prio - i - 1;
    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    threadParams[i].threadIdx = i;

    pthread_create(&threads[i],               // pointer to thread descriptor
                   &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                   thread3,              // thread function entry point
                   (void *) &(threadParams[i]) // parameters to pass in
    );

    //while (!thread3SetupDone) {
       // std::cout << "thread 3 spion";
    //}
    std::cout << "Thread 3 Setup Done" << std::endl;

    //////
    //////

    i = 3;
    CPU_ZERO(&threadcpu);

    coreid = i % numberOfProcessors;
    printf("Setting thread %d to core %d\n", i, coreid);

    CPU_SET(coreid, &threadcpu);

    rc = pthread_attr_init(&rt_sched_attr[i]);
    rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
    rc = pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    rt_param[i].sched_priority = rt_max_prio - i - 1;
    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    threadParams[i].threadIdx = i;

    pthread_create(&threads[i],               // pointer to thread descriptor
                   &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                   thread4,              // thread function entry point
                   (void *) &(threadParams[i]) // parameters to pass in
    );
    //////
    //////

    //while (!thread4SetupDone) {
        //std::cout << "Thread 4 spin" << std::endl;
    //}
    std::cout << "Thread 4 Setup Done\nAll Threads Running\n" << std::endl;

    std::cout << "\033[1;31mWELCOME TO CONNECT 4!\033[0m\n" << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    cv::Mat dispImage = latestImage;
    while (true) {
        if (latestImage.empty())
            continue;
        latestImage.copyTo(dispImage);
        currentTime = std::chrono::high_resolution_clock::now();

        std::string seconds = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count());
        std::string mili = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() % 1000);
        std::string displayString = seconds + ":" + mili;
        cv::putText(dispImage, //target image
                    displayString, //text
                    cv::Point(10, dispImage.rows / 8), //top-left position
                    cv::FONT_HERSHEY_DUPLEX,
                    1.0,
                    CV_RGB(118, 185, 0), //font color
                    1);

        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - tLastMove).count() < 2 and
        std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count() > 2) {
            cv::putText(dispImage, //target image
                        "Move Detected at " + std::to_string(lastMoveCol), //text
                        cv::Point(dispImage.cols / 2, dispImage.rows / 8), //top-left position
                        cv::FONT_HERSHEY_DUPLEX,
                        1.0,
                        CV_RGB(255, 0, 0), //font color
                        1);

            cv::putText(dispImage, //target image
                        "Recommended move: " +
                        std::to_string(strongAnswer == 0 ? weakAnswer : strongAnswer), //text
                        cv::Point(dispImage.cols / 4, 1.5*dispImage.rows / 8), //top-left position
                        cv::FONT_HERSHEY_DUPLEX,
                        1.0,
                        CV_RGB(255, 0, 0), //font color
                        1);


        }

        imshow( "Frame", dispImage );
        char c=(char)cv::waitKey(25);
        if(c==27)
            break;

    }

    for (i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("\nTEST COMPLETE\n");
}
