#Requirements

- boost 1.52+
- openssl

#Installation

    mkdir build
    cd build
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/include -DCMAKE_INSTALL_PREFIX=/path/for/install
    # Due to library dependencies, we have to build zlib first.
    make zlib
    # Now we have zlib s.t. configuration of libarchive can be found it
    # properly
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/include -DCMAKE_INSTALL_PREFIX=/path/for/install 
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


