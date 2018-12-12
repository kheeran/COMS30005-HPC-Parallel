#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

// Define output file name
#define OUTPUT_FILE "stencil.pgm"

void stencil(const int nx, const int ny, float * restrict  image, float * restrict  tmp_image);
void init_image(const int nx, const int ny, float * restrict  image, float * restrict  tmp_image);
void output_image(const char * file_name, const int nx, const int ny, float * restrict image);
double wtime(void);

int main(int argc, char *argv[]) {

  // Check usage
  if (argc != 4) {
    fprintf(stderr, "Usage: %s nx ny niters\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int rank;               /* 'rank' of process among it's cohort */
  int size;               /* size of cohort, i.e. num processes started */
  int flag;               /* for checking whether MPI_Init() has been called */
  enum bool {FALSE,TRUE}; /* enumerated type: false = 0, true = 1 */


  // Initiliase problem dimensions from command line arguments

  int nx = atoi(argv[1]);
  int ny = atoi(argv[2]);
  int niters = atoi(argv[3]);

  double partY = ceil(ny/rank)

  // Allocate the image
  float * restrict image = malloc(sizeof(float)*(nx+2)*(ny+2));
  float * restrict tmp_image = malloc(sizeof(float)*(nx+2)*(ny+2));

  // float * restrict image_send = malloc(sizeof(float)*(nx+2)*(ny/partY+2));
  // float * restrict tmp_image_send = malloc(sizeof(float)*(nx+2)*(ny/partY+2));
  //
  // float * restrict image_receive = malloc(sizeof(float)*(nx+2)*(ny/partY+2));
  // float * restrict tmp_image_receive = malloc(sizeof(float)*(nx+2)*(ny/partY+2));
  //
  // float * restrict halo_send = malloc(sizeof(float)*(nx+2));
  // float * restrict halo_receive = malloc(sizeof(float)*(nx+2));

  // Set the input image
  init_image(nx, ny, image, tmp_image);

  /* initialise our MPI environment */
  MPI_Init( &argc, &argv );

  MPI_Initialized(&flag);
  if ( flag != TRUE ) {
    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
  }

  MPI_Comm_size( MPI_COMM_WORLD, &size );

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  // Call the stencil kernel
  double tic = wtime();

  if (rank == 0) {
    for (int t = 0; t < niters; ++t) {

      stencil(nx, ny, image, tmp_image);
      stencil(nx, ny, tmp_image, image);
    }
  } else if (rank == size-1){
    for (int t = 0; t < niters; ++t) {
      stencil(nx, ny, image, tmp_image);
      stencil(nx, ny, tmp_image, image);
    }
  } else {
    for (int t = 0; t < niters; ++t) {
      stencil(nx, ny, image, tmp_image);
      stencil(nx, ny, tmp_image, image);
    }
  }

  double toc = wtime();

  printf("Processing on rank %d\n", rank );
  // Output
  printf("------------------------------------\n");
  printf(" runtime: %lf s\n", toc-tic);
  printf("------------------------------------\n");

  output_image(OUTPUT_FILE, nx, ny, image);
  free(image);

  MPI_Finalize();

  return EXIT_SUCCESS;
}

void stencil(const int nx, const int ny, float * restrict  image, float * restrict  tmp_image) {

  for (int i = 1; i < ny+1; ++i) {
    for (int j = 1; j < nx+1; ++j) {
      tmp_image[j+i*(nx+2)] = image[j+i*(nx+2)] * 0.6f;
      tmp_image[j+i*(nx+2)] += image[j  +(i-1)*(nx+2)] * 0.1f;
      tmp_image[j+i*(nx+2)] += image[j  +(i+1)*(nx+2)] * 0.1f;
      tmp_image[j+i*(nx+2)] += image[j-1+i*(nx+2)] * 0.1f;
      tmp_image[j+i*(nx+2)] += image[j+1+i*(nx+2)] * 0.1f;
    }
  }
}

// Create the input image
void init_image(const int nx, const int ny, float * restrict  image, float * restrict  tmp_image) {
  // Zero everything
  for (int j = 0; j < ny+2; ++j) {
    for (int i = 0; i < nx+2; ++i) {
      image[j+i*(ny+2)] = 0.0;
      tmp_image[j+i*(ny+2)] = 0.0;
    }
  }

  // Checkerboard
  for (int j = 0; j < 8; ++j) {
    for (int i = 0; i < 8; ++i) {
      for (int jj = j*(ny)/8; jj < (j+1)*(ny)/8; ++jj) {
        for (int ii = i*(nx)/8; ii < (i+1)*(nx)/8; ++ii) {
          if ((i+j)%2)
          image[(jj+1)+(ii+1)*(ny+2)] = 100.0;
        }
      }
    }
  }
}

// Routine to output the image in Netpbm grayscale binary image format
void output_image(const char * file_name, const int nx, const int ny, float * restrict image) {

  // Open output file
  FILE *fp = fopen(file_name, "w");
  if (!fp) {
    fprintf(stderr, "Error: Could not open %s\n", OUTPUT_FILE);
    exit(EXIT_FAILURE);
  }

  // Ouptut image header
  fprintf(fp, "P5 %d %d 255\n", nx, ny);

  // Calculate maximum value of image
  // This is used to rescale the values
  // to a range of 0-255 for output
  float maximum = 0.0;
  for (int j = 1; j < ny+1; ++j) {
    for (int i = 1; i < nx+1; ++i) {
      if (image[j+i*(ny+2)] > maximum)
        maximum = image[j+i*(ny+2)];
    }
  }

  // Output image, converting to numbers 0-255
  for (int j = 1; j < (ny+1); ++j) {
    for (int i = 1; i < (nx+1); ++i) {
      fputc((char)(255.0*image[j+i*(ny+2)]/maximum), fp);
    }
  }

  // Close the file
  fclose(fp);

}

// Get the current time in seconds since  wtime(void) {
double wtime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec*1e-6;
}
