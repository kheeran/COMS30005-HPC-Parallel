all: stencil
stencil: stencil.c
	mpiicc -std=c11 -Wall -Ofast -xAVX $^ -o $@
