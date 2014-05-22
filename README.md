# ImTools

Some tools for image manipulation by means of the *OpenCV* library.

## Main tools

### immerge

A tool to compute difference between two images and apply the difference
to a number of similar images.

Can be useful to update a logo on a number of images.

## Extra tools

These are small programs created specially for testing functions declared in
`src/imtools.hxx` header file.

### imboundboxes

Tests `imtools::bound_boxes()` function.

### imdiff

Tests `imtools::diff()` function.

### immpatch

Tests `imtools::patch()` function.

### imtpl

Tests `imtools::match_template()` function.
