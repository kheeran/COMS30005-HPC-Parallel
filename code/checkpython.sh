#!/bin/bash
echo "WHAT SIZE? (1024, 4096 OR 8000)"
read size
read size2
python check.py --ref-stencil-file stencil_"$size"_"$size2"_100.pgm --stencil-file stencil.pgm
