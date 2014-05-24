# ImTools

Some tools for image manipulation by means of the *OpenCV* library.

## Installation

At the root of the project run:
```
cmake [OPTIONS] . # <-- note the dot
make
```

*OPTIONS*:
- `-DIMTOOLS_DEBUG=ON|OFF` - whether to turn on/off debug mode (no optimization, lots of output). Default: OFF.
- `-DIMTOOLS_THREADS=ON|OFF` - whether to enable threading (some operations will run in parallel). Default: ON.
- `-DIMTOOLS_EXTRA=ON|OFF` - whether to build extra tools. Default: OFF.

As a result, `bin` directory will contain the binaries.

## Main tools

### immerge

A tool to compute difference between two images and apply the difference
to a number of similar images.

Can be useful to update a logo on a number of images.

## Extra tools

These are small programs designed for testing functions declared in
`src/imtools.hxx` file.

### imdiff

Tests `imtools::diff()` function.

Computes difference between two images of the same size. Generates a binary image
with the changed pixels highlighted. The result can be used as input for
`imboundboxes`.

### imboundboxes

Tests `imtools::bound_boxes()` function.

Outlines "significant" groups of non-zero pixels with bounding boxes. Result is
shown via standard OpenCV GUI API.

Information about the bounding boxes is written to standard output. This
information can be used as input for `imtpl`.

### imtpl

Tests `imtools::match_template()` function.

Accepts two images:
- original
- template - reasonably modified part of original

On _original_ image finds a rectangle which best matches _template_.
Outputs coordinates of the top left corner.

### immpatch

Tests `imtools::patch()` function.

Copies one image over another at specified location.
