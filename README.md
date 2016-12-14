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

It expects two inputs: the path to the video file (the `-v` option), and the location of the directory to store the track in (the `-t` option). These options can appear in any order. For example, to annotate a video at `/path/to/a/video.avi` and store the results in `/another/path/to/tracks/`, issue the following:

```bash
$ ./heart_annotations -v /path/to/a/video.avi -t /another/path/to/tracks/
```
This will store the output in `.tk` in the specified directory and the name of this file will be the same as the video (minus the path and `.avi` extension). For example, in the above case, the output file would be called `/another/path/to/tracks/video.tk`. If there is already a file of that name in the directory, the software will read in this file and allow you to edit a previous set of annotations and save the changes, overwriting the previous file.

When you open the tool, you will see the first frame of the video appear with a circle in the middle. The different variables are displayed as follows:

* The colour of the circle represents the view label: cyan is the four-chamber (4C) view, green is the left-ventricular outflow tract (LVOT) view, and yellow is the three vessels (3V) view.
* The radius of the heart is represented by the radius of the circle.
* The location of the heart centre is the location of the centre of the circle (the end of the radial line).
* The orientation of the heart is shown by the direction of the radial line.
* The cardiac phase is shown by the arrow head moving up and down the radial (this will not appear until some annotations are made, see below). If the arrow is moving radially outwards, that is systole, and if it is moving radial inwards that is diastole. When the arrow reaches the two ends of the line, that frame is the end-systole/end-diastole frame, as relevant.
* The heart visibility is represented by the presence and appearance of the cirlce. If the circle is present and bright, the heart is visible. If it is present but darker in colour, the heart is obscured. If it is not visible at all, the heart is invisible.

You can use the keyboard to move through the frames and manipulate the variables.
