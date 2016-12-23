# Heart Annotation Tool

This repository contains two small annotation tools for annotating fetal heart ultrasound videos with certain variables of interest. It relates to work performed towards my DPhil thesis (see [my website](https://chrispbridge.wordpress.com/) for more information).

## Purpose

There are two tools:
* `heart_annotations` - For annotating heart videos with 'global' information in each frame. Specfically, the heart visibility, heart centre location, heart_radius, viewing plane, orientation, and cardiac phase variables.  
* `substructure_annotations` - For annotating the locations of cardiac structures.

## Dependencies

* A C++ compiler conforming to the C++11 standard or later.
* The [OpenCV](http://opencv.org) library. Tested on version 3.1.0 but most fairly recent
versions should be compatible. If you are using GNU/Linux, there will probably
be a suitable packaged version in your distribution's repository.
* The [Boost](http://www.boost.org) program options library. Again there is likely to be a suitable packaged version on your GNU/Linux distribution (probably packed along with the other boost libraries). You will also need the development headers, which are sometimes packaged separately.

## Build

First ensure you have installed the dependencies as above.

Compile the two files `heart_annotations.cpp` and `substructure_annotations.cpp` using your C++ compiler, making sure to specify C++11 if necessary, and link against the OpenCV libraries and the Boost program options library.

There is Makefile in the `build/` directory to simplify this process for users with GNU/Linux operating systems or similar. To use this, issue one of the following commands from the `build/` directory.

To build both tools:

```bash
$ make
```
To build just `heart_annotations`:
```bash
$ make heart_annotations
```

To build just `substructure_annotations`:

```bash
$ make substructure_annotations
```

To remove any/all compiled software, just use:

```bash
$ make clean
```

## Usage: Heart Annotations

This tool allows you to annotate a `.avi` video, and store the results in a `.tk` track file (a plain text file with `.tk` extension).

#### Opening a Video

To open a video in the annotation tool from the command line, you need to provide two inputs: the path to the video file (the `-v` option), and the location of the directory to store the track in (the `-t` option). These options can appear in any order. For example, to annotate a video at `/path/to/a/video.avi` and store the results in `/another/path/to/tracks/`, issue the following:

```bash
$ ./heart_annotations -v /path/to/a/video.avi -t /another/path/to/tracks/
```
This will store the output in a `.tk` file in the specified directory and the name of this file will be the same as the video (minus the path and `.avi` extension). For example, in the above case, the output file would be called `/another/path/to/tracks/video.tk`. If there is already a file of that name in the directory, the software will read in this file and allow you to edit a previous set of annotations and save the changes, overwriting the previous file.

If you do not provide an output directory, the current working directory is used.

#### Annotating a Frame

When you open the tool, you will see the first frame of the video appear with a circle in the middle. The different variables are displayed as follows:

* The colour of the circle represents the view label: cyan is the four-chamber (4C) view, green is the left-ventricular outflow tract (LVOT) view, and yellow is the three vessels (3V) view.
* The radius of the heart is represented by the radius of the circle.
* The location of the heart centre is the location of the centre of the circle (the end of the radial line).
* The orientation of the heart is shown by the direction of the radial line.
* The cardiac phase is shown by the arrow head moving up and down the radial (this will not appear until some annotations are made, see below). If the arrow is moving radially outwards, that is systole, and if it is moving radial inwards that is diastole. When the arrow reaches the two ends of the line, that frame is the end-systole/end-diastole frame, as relevant.
* The heart visibility is represented by the presence and appearance of the cirlce. If the circle is present and bright, the heart is visible. If it is present but darker in colour, the heart is obscured. If it is not visible at all, the heart is invisible.

Within a frame you can use the keyboard to manipulate the variables using the following keys:

* **Arrow Keys** - Move the annotation centre in the chosen direction. (Hold ctrl to move faster.)
* **c/a** - Rotate the annotation clockwise (c) or anticlockwise (a). (Hold ctrl to move faster.)
* **1/2/3** - Change the annotation view to four-chamber (1) / left-ventricular outflow (2) / three vessels (3).
* **s/d** - Mark this frame as an end-systole frame (s) or an end-diastole (d) frame. The same key will also remove a previous labelling. (Note that a single frame can only hold one of these two labels and any subsequent label of the other type will override it.)
* **Delete** - Cycles between not visible, visible, and obscured.
* **+/-** - Increase or decrease the radius annotation (applies to the whole video, not just the current frame).
* **h** - Toggle between the two 'flips'. These are indicated by the 'L' and 'R'
on the diplay indicating the anatomical left and right sides of the heart (applies to the whole video, not just the current frame).

#### Moving Through Frames and Storing Annotations

The tool stores an array of the annotations for each frame in the video in memory (the 'buffer'). This also remembers which frames you have previously annotated, and which you have not.

You can move through the frames with the following keys:
* Return - Move forwards one frame and save annotations for the current frame to the buffer.
* Backscape - Move backwards one frame and save annotations for the current frame to the buffer.
* p - (Play) Move forwards one frame and do *not* store annotations (useful for viewing the video whilst not annotating).
* r - (Rewind) Move backwards one frame and do *not* store annotations.

Whenever you use return or backspace to move to a frame that has no previously stored annotation, the initial value for that annotation will be copied from the value that was just stored in the frame that was previously being annotated. This does not apply the diastole and systole frame labellings, which are reset in the new frame. In this way annotations are propogated through the video, allowing you to make lots of similar annotations quickly in sequences where the heart orientation/position/view does not change by just repeatedly tapping or holding down return/backspace. However if you move to a frame where there *is* a previous annotation stored in the buffer, this previous annotation will be restored instead of propogating the annotation from the neighbouring frame.

Occasionally you may want to propogate annotations through sequences of frames even when those frames *do* have previously stored annotations in the buffer. This may happen for example when correcting a mistake you have made over a number of frames. You can do this by activating *overwrite mode* by pressing the **o** key. When this mode is active, annotations will always be propogated from one frame to the next when you press return or enter. Use this with caution however, as it is easy to mistakenly overwrite previously annotated frames. You can see when you are in overwrite mode as "OVERWRITE MODE" will appear in yellow text in the bottom right of the image, and return to normal behaviour by pressing **o** again.

#### Cardiac Phase Annotations

The values for the cardiac phase are not directly annotated but instead are inferred from your end-diastole (ED) and end-systole (ES) labels by interpolating. To do this, annotate the ED/ES frames, and then tap the **z** key to calculate the circular-valued cardiac values. These can then be seen by the arrowhead moving in and out along the orientation line (in = diastole, out = systole), but only after they have been calculated for the first time. Note that if you then make subsequent changes to the ED/ES frames, you will need to tap **z** again to update the cardiac phase values.

As well as interpolating values for the cardiac phase, the routine also extrapolates estimated positions for ED and ES frames by assuming a consant phase rate. Therefore you do not need to label every single ED/ES frame in video in order to hit the strongly, although it is stringly recommended that you manually annotate as many as you can. You can see the automatically selected ED/ED frames appear with the text in brackets "ED"/"ES". If the routine has insufficient annotated frames, you will see a warning message output to the terminal and the values will not be updated.

#### Exiting

When you are finished, you can exit the application in two ways:

* **Esc** - Exit the software and save the annotations to file. If an existing annotation file was read in when the software started, it will be overwritten.
* **q** - Exit the software and do not save changes. Any annotations performed in this session will be lost. Any file read in at the start will be left unedited from its original state.

Alternatively, you will exit automatically when you hit Return on the last frame.

## Using Track Files

The track files that are created by the tool are simple text files that follow the format described on [this page](reference/trackfiles.md) (.tk files) and [this page](reference/structtrackfiles.md) (.stk files).

In both cases, there are C++ functions in `thesisUtilities.cpp` to read the information from a trackfile. These should be self-explanatory to use.

Furthermore, Python functions to read the files are provided in `python-utilities/heart_annotation_python_utilities.py`. The function `readHeartTrackFile` takes a filename and returns the track data as a numpy array in which each row corresponds to a frame and each column corresponds to one of the variables.

```python
# Import the module
import heart_annotation_python_utilities as hapu

# An example path to a trackfile of interested
filename = 'path/to/testvideo.tk'

# Read in the file
data_table,image_dimensions,image_flip,heart_radius = hapu.readHeartTrackFile(filename)

# Now the data can be accessed with the column index variables
# For example, to access all the variables relating to the first frame (index 0), use
data_table[0,hapu.tk_frameCol] # the frame number
data_table[0,hapu.tk_labelledCol] # the Boolean 'labelled' variable
data_table[0,hapu.tk_presentCol] # indicates whether the variable is present
data_table[0,hapu.tk_yposCol] # y location of heart centre
data_table[0,hapu.tk_xposCol] # x location of the heart centre
data_table[0,hapu.tk_oriCol] # heart orientation
data_table[0,hapu.tk_viewCol] # heart view
data_table[0,hapu.tk_phasePointsCol] # flags for end-diastole/systole frames
data_table[0,hapu.tk_cardiacPhaseCol] # the circular cardiac phase variable
```

## Usage: substructure_annotations

Instructions for this coming soon...

## Licence

This software is licensed under the GNU Public License. See the licence file for more information.

## Author

This software was written by Christopher Bridge at the Insitute of Biomedical Engineering, University of Oxford.
