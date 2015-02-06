#!/bin/bash
set -eu
cd $(dirname $0)

export NDK_PROJECT_PATH=$PWD

ndk-build -B NDK_APPLICATION_MK=jni/Application.mk 2>&1 | grep -v warning
go-bindata libs/armeabi/...
go build 2>&1 | grep -v warning
cp -v airfps study-diary
