Subtitle Editor
===============

Subtitle Editor is a GTK+3 tool to edit subtitles for GNU/Linux/*BSD.

It can be used for new subtitles or as a tool to transform, edit,
correct and refine existing subtitle. This program also shows sound
waves, which makes it easier to synchronise subtitles to voices.

Subtitle Editor is free software released under the GNU General Public
License (GPL3).

## Features

* Really easy to use
* Multiple document interface
* Undo/Redo
* Internationalization support
* Drag-and-drop
* Video player integrated in the main window (based on GStreamer)
* Can play preview with external video player (using MPlayer or other)
* Can be used for timing
* Generate and display a waveform
* Generate and display keyframes
* Can be used for translating
* Shows subtitles over the video

## Editing

* Style Editor
* Spell checking
* Text correction (Space around punctuation, capitalize, empty subtitle ...)
* Errors checking (Overlapping, too short or long duration ...)
* Framerate conversion
* Edit times and frames
* Scale subtitles
* Split or joint subtitles
* Split or joint documents
* Edit text and adjust time (start, end)
* Move subtitle
* Find and replace (Support regular expressions)
* Sort subtitles
* Type writer effect
* Lots of timing and editing tools

## Supported Formats

* Adobe Encore DVD
* Advanced Sub Station Alpha
* Burnt-in timecode (BITC)
* MicroDVD
* MPL2
* MPsub (MPlayer subtitle)
* SBV
* SubRip
* Sub Station Alpha
* SubViewer 2.0
* Timed Text Authoring Format (TTAF)
* Plain-Text

## Installation

Source tarballs, installing from a source tarball:

```bash
./autogen.sh
./configure
make
sudo make install
```

Required dependencies:

* gtk+ version >=3.0
* gtkmm version >=3.10
* glibmm version >= 2.18.0
* gstreamer 1.0 (gstreamer1.0-x, gst-plugins-base, gst-plugins-good ...)
* gstreamermm version >= 1.0
* enchant version >=1.4.0 (spell check)
* libxml++ >=2.20

## Developement
After building with `make`, it is possible to run Subtitle Editor before installing it from the directory where it was built with:
```
 SE_DEV=1 ./src/subtitleeditor
```
To update the translations files when strings get added, one needs to run the following in the `./po` directory:
```
intltool-update --pot
```
