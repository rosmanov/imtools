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

    # layman -f -o https://bitbucket.org/osmanov/gentoo-overlay/raw/master/repository.xml -a osmanov-overlay
    # emerge -va media-gfx/imtools

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

    # [application_example]
    # Port on which to accept WebSocket requests.
    # port=9902
    #
    # Hostname or IP address on which to accept WebSocket requests.
    # Empty means all addresses.
    # host=
    #
    # Change to this directory at the start (for relative paths).
    # chdir=
    #
    # Whether to allow absolute paths.
    # allow_absolute_paths=false

    [application_1]
    port=9809
    chdir=/home/ruslan/projects/imtools/bin
    allow_absolute_paths=false

    [application_N]
    port=9810
    host=127.0.0.10
    ...

*Request format for `imresize` command*

    {
      "command" : "imresize",
      "arguments" : { argument properties ... }
      }
    }


_Arguments_:
- `source` - source image path, e.g. `"/path/to/source/image.png"`
- `output` - output image path, e.g. `"/path/to/output/image.png"`
- `width` - width of thumbnail, e.g. 120
- `height` - height of thumbnail, e.g. 120
- `fx` - scale factor for horizontal axis, e.g.: 0.5
- `fy` - scale factor for vertical axis, e.g.: 0.5
- `interpolation` - Interpolation method (_see output of_ `imresize --help` _command_)

*Request example: make 50%-thumbnail from source.png and save it to thumbnail.png*

    {
      "command" : "imresize",
      "arguments" : {
        "source" : "source.png",
        "output" : "thumbnail.png",
        "fx" : 0.5,
        "fy" : 0.5
      }
    }

[Nginx](http://nginx.org) configuration:

    location /websocket {
      proxy_pass http://backend;
      proxy_http_version 1.1;
      proxy_set_header Upgrade $http_upgrade;
      proxy_set_header Connection "upgrade";
      proxy_read_timeout 3600; # 60-minute timeout for reads
      proxy_send_timeout 3600; # 60-minute timeout for writes
    }

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
