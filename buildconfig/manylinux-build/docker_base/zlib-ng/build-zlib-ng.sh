#!/bin/bash
set -e -x

cd $(dirname `readlink -f "$0"`)

ZLIB_NG_VER=2.2.3
ZLIB_NG_NAME="zlib-ng-$ZLIB_NG_VER"
curl -sL --retry 10 https://github.com/zlib-ng/zlib-ng/archive/refs/tags/${ZLIB_NG_VER}.tar.gz > ${ZLIB_NG_NAME}.tar.gz

sha512sum -c zlib-ng.sha512
tar -xf ${ZLIB_NG_NAME}.tar.gz
cd ${ZLIB_NG_NAME}

cmake . $PG_BASE_CMAKE_FLAGS -DZLIB_COMPAT=1
make
make install
