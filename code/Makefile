all: stencil
stencil: stencil.c
	mpiicc -std=c11 -Ofast -xAVX -Wall $^ -o $@
