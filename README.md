# Connect2plus2

This project uses computer vision to track moves on a connect 4 board, followed by a move reccomendation.

Sample frame captures can be found in the Frames folder.

To prevent saving frames from impacting performance as much as possible, the image that is saved as a frame is not MUTEX locked to prevent other threads from modifying it during the saving process. As a result, the display thread will occasionally not have written text to the image before it has been saved. This creates a flicker in the time displayed, as well as the move detection and reccomendation in the frames.

Timing graphs for the two main threads can be found in the Timing Graphs folder. 

Two of these graphs represent the time at which each thread completed its task measured from the start of the program. The image thread has some significant spikes when a move is played, as the board processing thread locks it's resource to process a move. This blocking period is almost entirely due to a self imposed wait timer to filter false moves, rather than computation starving. Eliminating this debounce would remove these spikes, however introduce the risk of measuring a move twice. These wait periods have been removed as outliers in the jitter graph, to make the majority of cases easier to see. Jitter graphs are processed as the difference between completion times, used to measure how long each iteration takes.
