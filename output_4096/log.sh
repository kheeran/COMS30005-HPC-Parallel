#!/bin/bash

echo "OUTPUT LOGGED"
echo "#LOG"  >> outputlog.md
echo "" >> outputlog.md
echo "" >> outputlog.md
echo "## ICC fast" >> outputlog.md
cat stencil.out >> outputlog.md
echo "" >> outputlog.md
echo "------------------------------------------------------" >> outputlog.md
