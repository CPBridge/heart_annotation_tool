How to interpret the .stk files for cariac structure annotations:

Line 1:
-------

This is a header line that gives human readable names for the columns later in the file.

Line 2:
-------

The first number on this line are the number of structures in this file.
The second and third numbers on this line are respectively the x and y dimensions of the image (in that order), in pixels.

Beyond:
-------

After this, each structure appears in turn, separated by an empty line. The first line of each structure (after the empty line) gives the index of the struture (they appear in order starting at 0), and the name of the structure. Then there are a set of lines, one per frame of the video, whose columns contain the following information:

* **Column 1 (Frame Number)**: The frame number that this line refers to. The first frame in the video is frame 0,
and frames appear in order thereafter, so this line is mostly for sanity checking.
* **Column 2 (Labelled)**: A Boolean variable indicating whether the user has labelled/annotated this structure in this frame (0 not labelled, 1 labelled).
Note that if labelled is 0, all other columns should be ignored for this line as they are not meaningful.
* **Column 3 (Present)**: Categorical variable indicating whether the structure is visible in this frame. (0 not visible, 1 visible, 2 obscured).
Note that if this column is 0, all other columns in this line should be ignored as they are not necessarily meaningful.
* **Column 4 (Y)**: The y location of the structure, measured in pixels from the top of the image.
* **Column 5 (X)**: The x location of the structure, measured in pixels from the left of the image.
* **Column 6 (Orientation)**: The orientation of the structure, measured in degrees anticlockwise from the positive x axis (mathematical convention).
