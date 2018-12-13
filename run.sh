#!/bin/sh

source logger.sh
qsub stencil_1024.job
qsub stencil_4096.job
qsub stencil_8000.job
