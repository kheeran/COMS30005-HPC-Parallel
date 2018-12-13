#!/bin/sh

cd output_1024
source log.sh
cat stencil.out
cd ..
cd output_4096
source log.sh
cat stencil.out
cd ..
cd output_8000
source log.sh
cat stencil.out
cd ..
