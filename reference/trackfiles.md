How to interpret the .tk files for whole-heart annotations:

Line 1:
-------

This is a header line that gives human readable names for the columns later in the file.

Line 2:
-------

The two numbers on this line are the x and y dimensions of the image (in that order), in pixels.

Line 3:
-------

The first value on this line is a Boolean variable representing the 'flip' convention
of the scanning. 1 means that the cross section is being viewed from above (looking from
fetal head to fetal toes). 0 means that the cross section is being viewed from below
(from fetal toes to fetal head).

The second value on this line is the radius of the heart in the video, measured in pixels.

Line 4 Until The End :
----------------------

These lines contain the actual frame-by-frame annotations. There is one line per
frame. The columns are as follows:

* **Column 1 (Frame Number)**: The frame number that this line refers to. The first frame in the video is frame 0,
and frames appear in order thereafter, so this line is mostly for sanity checking.
* **Column 2 (Labelled)**: A Boolean variable indicating whether the user has labelled/annotated this frame (0 not labelled, 1 labelled).
Note that if labelled is 0, all other columns should be ignored as they are not meaningful.
* **Column 3 (Present)**: Categorical variable indicating whether the heart is visible in this frame. (0 not visible, 1 visible, 2 obscured).
Note that if this column is 0, all other columns should be ignored as they are not necessarily meaningful.
* **Column 4 (Centre Y)**: The y location of the heart centre, measured in pixels from the top of the image.
* **Column 5 (Centre X)**: The x location of the heart centre, measured in pixels from the left of the image.
* **Column 6 (Orientation)**: The orientation of the heart, measured in degrees anticlockwise from the positive x axis (mathematical convention).
* **Column 7 (View)**: View category label (1 four chamber, 2 left ventricular outflow, 3 three vessels).
* **Column 8 (Phase Points)**: This is used to label the frames that the user annotated as end-diastole or end-systole.
(0 not end-diastole or end-systole, 1 auto labelled end-systole, 2 manual labelled end-systole, 3 auto labelled end-diastole, 4 manual labelled end-diastole).
* **Column 9 (Cardiac Phase Value)**: Circular-valued cardiac phase value (in radians) interpolated from the labels in the previous column. 0 represents end-diastole and pi represents end-systole.
