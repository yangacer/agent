#Requirements

- boost 1.52+
- openssl

#Installation

    mkdir build
    cd build
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/include -DCMAKE_INSTALL_PREFIX=/path/for/install
    make zlib # Due to library dependencies, we have to build zlib first.
    make install  # administrator privilege may be required

#Examples

- test/basic.cpp

- test/tube.cpp

#Features

- Form-multipart upload.

- Chunked notification (no matter what transfer encoding is).

- Concurrent execution.

- Support HTTPS (without needing of cert file).


#Contributors

[Michael Yang](https://github.com/flachesis).


