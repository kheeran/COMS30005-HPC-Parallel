all: stencil
stencil: stencil.c
	icc -std=c11 -g -pg -qopt-report-phase=vec -qopt-report=5 -Wall -fast -xAVX $^ -o $@
