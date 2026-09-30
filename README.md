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
meson setup build
meson compile -C build
sudo meson install -C build
```

Required dependencies:

* gtk+ version >=3.0
* gtkmm version >=3.10
* glibmm version >= 2.18.0
* gstreamer 1.0 (gstreamer1.0-x, gst-plugins-base, gst-plugins-good ...)
* enchant version >=1.4.0 (spell check)
* libxml++ >=2.20
* meson >= 0.60 and ninja (to build)

To install into your home directory instead, without sudo, use `meson setup build --prefix=$HOME/.local`
and run `~/.local/bin/subtitleeditor`. The default GStreamer sinks can be set with
`-Ddefault-video-sink=…` and `-Ddefault-audio-sink=…`.

## Developement
After building, it is possible to run Subtitle Editor without installing it:
```
SE_DEV=1 build/src/subtitleeditor
```
To update the translations files when strings or files get added, one needs to run the following:
```
./prepare-po.sh # lists the files that can contain translatable strings in po/POTFILES.in
meson compile -C build subtitleeditor-pot # generates a new pot file
meson compile -C build subtitleeditor-update-po # applies the pot file to the individual translations
```

To prepare a release:
- bump the version in meson.build
- packages should be built with `meson setup --buildtype=release`; the default `debugoptimized` includes the messages of the `--debug-*` options
- check the log (git log) to create release notes in GitHub
- draft a release, when it’s ok, add a new bumped tag
