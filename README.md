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
    key=yeBOfetLDTkP

    [application_N]
    port=9810
    host=127.0.0.10
    ...

*Request format for `resize` command*

    {
      "command" : "resize",
      "arguments" : { argument properties ... },
      "digest" : "command-specific message digest"
    }


_Arguments_:
- `source` - source image path, e.g. `"/path/to/source/image.png"`
- `output` - output image path, e.g. `"/path/to/output/image.png"`
- `width` - width of thumbnail, e.g. 120
- `height` - height of thumbnail, e.g. 120
- `fx` - scale factor for horizontal axis, e.g.: 0.5
- `fy` - scale factor for vertical axis, e.g.: 0.5
- `interpolation` - Interpolation method (_see output of_ `imresize --help` _command_)

The message digest should be built by formula:
    digest = SHA1(application_name + arguments + key)

where
- `SHA1()` function computes SHA-1 hash,
- `application_name` is the name of application defined in the configuration file,
- `arguments` is a command-specific serialized string,
- `key` represents private key, which is configured per application in the configuration file:

Serialized arguments for `resize` command:

    source + output + width + height + round(fx * 1000) / 1000 + round(fy * 1000) / 1000

*Request example: make 50%-thumbnail from source.png and save it to thumbnail.png*

    {
      "command" : "resize",
      "arguments" : {
        "source" : "source.png",
        "output" : "thumbnail.png",
        "fx" : 0.5,
        "fy" : 0.5
      },
      "digest" : "f9a28c6f5b71d054a899eb8ca44a5c5903bef61e"
    }

where `digest` is computed as

     sha1('application_1' + 'source.png' + 'thumbnail.png'
     + '0' + '0' + '0.5' + '0.5' + 'yeBOfetLDTkP');


*Response format of `resize` command*

    {
      "error" : "ERROR_CODE",
      "response" : "MESSAGE"
    }

where `ERROR_CODE` is `"0"` on success, or `"1"` otherwise; `MESSAGE` is `"OK"` on success, or
error message on error, e.g.:

    {"error":"0","response":"OK"}


*Request format for `meta` command*

    {
      "command"    : "meta",
      "subcommand" : "(subcommand name)",
      "digest"     : ""
    }

Possible `subcommand` values:
- `"version"` - request version full name
- `"features"` - ImTools features
- `"copyright"` - copyright string
- `"all"` - `"version"`, `"features"` and `"copyright"` separated by new line.

The message digest should be built by formula:

    digest = SHA1(application_name + "version" + key)

*Example meta request*

    {
      "command"    : "meta",
      "arguments"  : {
        "subcommand" : "version"
      },
      "digest"     : "c66b55ed158b157e2559f81554de3237c20831a3"
    }


*Request format for `merge` command*

    {
      "command"    : "merge",
      "arguments": { ... },
      "digest"     : "(SHA-1)"
    }

Supported arguments:
- `old_image` - required; old image
- `new_image` - required; new image
- `input_images` - required; array of input images
- `output_images` - required; array of output images (size must be equal to size of `input_images`)
- `strict` - optional; value > 0 turns some warnings into fatal errors
- `min_threshold` - Min. noise suppression threshold (see `immerge -h`)
- `max_threshold` - Max. noise suppression threshold (see `immerge -h`)

*Example meta request*

    {
      "command": "merge",
      "arguments": {
        "old_image": "old.jpg",
        "new_image": "new.jpg",
        "input_images": [
          "in.jpg"
        ],
        "output_images": [
          "out.jpg"
        ],
        "strict": 2
      },
      "digest": "f6bca39b56b85cac90398b69e2b10378591508c2"
    }


where `digest` is computed as

    sha1(application_name + old_image + new_image + size(output_images)
        + strict + private_key)
       = sha1('application_1' + 'old.jpg' + 'new.jpg' + '.' + '1' + '2' + 'yeBOfetLDTkP')
       = 'f6bca39b56b85cac90398b69e2b10378591508c2'

*Request format for `diff` command*

    {
      "command"    : "diff",
      "arguments": { ... },
      "digest"     : "(SHA-1)"
    }

Supported arguments:
- `old_image` - path to "old" image
- `new_image` - path to "new" image
- `out_image` - path to output image file where the difference will be stored

*Example meta request*

    {
      "command": "diff",
      "arguments": {
        "old_image": "old.jpg",
        "new_image": "new.jpg",
        "out_image": "diff.jpg",
      },
      "digest": "b89d207f0aeaedab3d044ac968f3d9d502cfcb3c"
    }

where `digest` is computed as

    sha1(application_name + old_image + new_image + private_key)
       = sha1('application_1' + 'old.jpg' + 'new.jpg' + 'yeBOfetLDTkP')
       = 'b89d207f0aeaedab3d044ac968f3d9d502cfcb3c'


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
