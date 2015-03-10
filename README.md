# ImTools

Some tools for image manipulation by means of the *OpenCV* library.

## Installation

### Manual

At the root of the project run:
```
cmake [OPTIONS] . # <-- note the dot
make
```

*OPTIONS*:
- `-DIMTOOLS_DEBUG=ON|OFF` - whether to turn on/off debug mode (no optimization, lots of output). Default: OFF.
- `-DIMTOOLS_DEBUG_PROFILER=ON|OFF - Enable profiling info in debug messages. Default: OFF.`
- `-DIMTOOLS_THREADS=ON|OFF` - whether to enable threading (some operations will run in parallel). Default: ON.
- `-DIMTOOLS_THREADS_BOOST=ON|OFF - use boost libraries for threading. Default: OFF.`
- `-DIMTOOLS_EXTRA=ON|OFF` - whether to build extra tools. Default: OFF.
- `-DIMTOOLS_SERVER=ON|OFF - whether to build WebSocket server. Default: OFF.`

As a result, `bin` directory will contain the binaries.

### Gentoo

```text
# layman -f -o https://bitbucket.org/osmanov/gentoo-overlay/raw/master/repository.xml -a osmanov-overlay
# emerge -va media-gfx/imtools
```

## Main tools

### immerge

A tool to compute difference between two images and apply the difference
to a number of similar images.

Can be useful to update a logo on a number of images.

### imresize

Thumbnail maker.

### imserver

WebSocket server which can be used for real-time image processing on a Web site.

*Sample configuration*
```
[application_1]
port=9909
host=127.0.0.10

[application_N]
port=9910
#host=
...
```

[Nginx](http://nginx.org) configuration:
```nginx
location /websocket {
  proxy_pass http://backend;
  proxy_http_version 1.1;
  proxy_set_header Upgrade $http_upgrade;
  proxy_set_header Connection "upgrade";
  proxy_read_timeout 3600; # 60-minute timeout for reads
  proxy_send_timeout 3600; # 60-minute timeout for writes
}
```

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

## Building documentation

To build documentation run `./build-docs.sh`.
