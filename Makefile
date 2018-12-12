all: stencil
stencil: stencil.c
	mpicc -std=c11 -Wall -Ofast $^ -o $@
