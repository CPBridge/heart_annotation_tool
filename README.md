# Heart Annotation Tool

This repository contains two small annotation tools for annotating fetal heart ultrasound videos with certain variables of interest. It relates to work performed towards my DPhil thesis (see [my website](https://chrispbridge.wordpress.com/) for more information).

## Purpose

There are two tools:
* `heart_annotations` - For annotating heart videos with 'global' information in each frame. Specfically, the heart visibility, heart centre location, heart_radius, viewing plane, orientation, and cardiac phase variables.  
* `substructure_annotations` - For annotating the locations of cardiac structures.

## Dependencies

* A C++ compiler conforming to the C++11 standard or later.
* The [OpenCV](http://opencv.org) library. Tested on version 4.2 but most fairly recent
versions should be compatible. If you are using GNU/Linux, there will probably
be a suitable packaged version in your distribution's repository.
* The [Boost](http://www.boost.org) program options, system and filesystem libraries. Again there are likely to be suitable packaged versions on your GNU/Linux distribution (probably packed along with the other boost libraries). You will also need the development headers, which are sometimes packaged separately.

## Build

First ensure you have installed the dependencies as above.

Compile the two files `heart_annotations.cpp` and `substructure_annotations.cpp` using your C++ compiler, making sure to specify C++11 if necessary, and link against the OpenCV libraries and the Boost program options, filesystem and system libraries.

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
* **Return** - Move forwards one frame and save annotations for the current frame to the buffer.
* **Backscape** - Move backwards one frame and save annotations for the current frame to the buffer.
* **p** - (Play) Move forwards one frame and do *not* store annotations (useful for viewing the video whilst not annotating).
* **r** - (Rewind) Move backwards one frame and do *not* store annotations.

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

#### Create A Structures List file

Before you begin annotating the locations of structures of interest in a video, you first need to define which structures you are interested in and which of the cardiac views they are present in.

This information is stored in a simple text file that you can create in any text editor. The file should consist of a list of structures with each structure appearing on one line of the file. At the start of each line, the structure's name appears, which should contain no spaces. Then after this, the following information should appear, separated by spaces:

- **Fourier Model Order**: This is not relevant for the annotation stage, but is relevant to later processes in my [fetal_heart_analysis](https://github.com/CPBridge/fetal_heart_analysis) repository. This is a non-negative integer that represents the order of the Fourier model that is used to model the trajectory of this structure over the cardiac cycle. The higher the number, the complex a path can be modelled. 0 means that no movement is modelled (the structure is assumed to be stationary over the cardiac cycle), 1 means that the movement is modelled as an ellipse, and so on.
- **Systole Only**: Again, this is ignored for the annotation stage, but is used later for processes. It describes whether the structure appears only during systole (0) -- valve-like structures are typically not visible during disatole -- or throughout the entire cardiac cycle (1).
- **Views**: A space-separated list of all the view classes in which the structure appears, represented by the positive integer representing the class (1 four-chamber view, 2 left ventricular output tract view, 3 three vessels view). Views must appear in increasing order and each structure must belong to at least one view!

An example structure lists file:

```
crux 3 0 1
apex 0 0 1 2
base 0 0 1
mitral_valve_end 3 0 1 2
mitral_valve_centre 3 1 1
tricuspid_valve_end 3 0 1
tricuspid_valve_centre 3 1 1
aortic_valve 3 0 2
aorta_3V 1 0 3
pulmonary_valve 1 0 3
trachea 0 0 3
SVC 1 0 3
spine 0 0 1 2 3
descending_aorta 0 0 1 2 3
```

#### Opening a Video

You can open the structures annotation tool from the command line. You need to provide the name of the video using the `-v` flag, the directory to store the resulting 'structure track files' (`.stk` files) using the `-t` flag, the location of the directory containing the heart track files (`.tk` files using the `-d` flag, and the structures list file (as above) with the `-s` flag. The names of the `.tk` and `.stk` files within their respective directories is automatically chosen to match the name of the video file (but changing the extension). The heart track file **must already exist** within the directory because the structure annotations process depends upon the information from the whole heart annotation process. The loaded heart track file is not changed by the structures annotation tool. By contrast, the structures annotation file (`.stk`) need not already exist. If it does already exist the existing file will be loaded for editing, if it does not already exist it will be created. For example:

```bash
./substructure_annotations -v /path/to/a/video.avi -t /another/path/to/structure/tracks/ -d /yet/another/path/to/heart/tracks/ -s /path/to/a/structure/list/file.txt
```

#### Annotating a Frame

Annotation consists of labelling the following information for each structure in each frame of the video:

- **Visibility** : Each structure can be labelled as *present*, *not present*, or *obscured*.
- **Position** : The position of each structure in each frame. This is displayed on the screen via the **base** of the arrow that represents a given structure.
- **Orientation** : The orientation of each structure within the image. This is displayed on screen via the direction of the arrow that represents a structure.

Within a frame you can use the keyboard and mouse to manipulate these variables in the following way:

* **Arrow Keys** - Move the current structure centre in the chosen direction. (Hold ctrl to move faster.)
* **c/a** - Rotate the current structure clockwise (c) or anticlockwise (a). (Hold ctrl to move faster.)
* **,/.** - Use the comma and dot keys (easier to think of them as the < and > keys) to cycle forwards and backwards through the strcutures. The currently active structure is displayed in a different colour. Change the annotation view to four-chamber (1) / left-ventricular outflow (2) / three vessels (3).
* **Delete** - Cycles between not visible, visible, and obscured for the current structure.
on the diplay indicating the anatomical left and right sides of the heart (applies to the whole video, not just the current frame).
* **Left mouse click** : Move the current structure to the chosen position.
* **Middle mouse click** : Select a structure close the chosen position.
* **Right mouse click** : Change the current structure's orientation to point to the chosen position.

#### Moving Between Frames And Exiting

This works in the same way as in the `heart_annotations` tool, with the same shortcuts. In addition there is an optional motion prediction mode toggled via the **m** key. When it is turned on, the structure locations for the next frame are predicted using a motion estimate.

## Using Structure Track Files

There are Python functions in the `heart_annotation_python_utilities.py` file that read the structure list and structure track files.

- `readStructureList` reads the structure list file.
- `readStructure` reads the annotations for a given structure from an `.stk` file.

Read their built-in Python docstrings for more information.

## Licence

This software is licensed under the GNU Public License. See the licence file for more information.

## Author

This software was written by Christopher Bridge at the Insitute of Biomedical Engineering, University of Oxford.
