#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#define MASTER 0

// Define output file name
#define OUTPUT_FILE "stencil.pgm"

void stencil(const int rank, const int partX, const int nx, const int ny, float * restrict  image, float * restrict  tmp_image);
void stencile(const int rank, const int partX, const int partXe, const int nx, const int ny, float * restrict  image, float * restrict  tmp_image);
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
  int flag;
  int partX;              // size of the partitions
  int partXe;             // size of the last partition
  MPI_Status status;
  int tag = 0;
  enum bool {FALSE,TRUE}; /* enumerated type: false = 0, true = 1 */

  // Initiliase problem dimensions from command line arguments

  int nx = atoi(argv[1]);
  int ny = atoi(argv[2]);
  int niters = atoi(argv[3]);

  // Allocate the image
  float * image = malloc(sizeof(float)*(nx+2)*(ny+2));
  float * tmp_image = malloc(sizeof(float)*(nx+2)*(ny+2));

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

  // Determining the size of each partition
  double nnx = nx;
  double ssize = size;
  partX = ceil(nnx/ssize);
  if (nx%partX != 0) {
    partXe = nx%partX;
  } else {
    partXe = partX;
  }

  //Creating variables for the indeces of the MPI_Comm_size
  int tophalo = rank*partX;
  int toprow = rank*partX + (ny+2);
  int botrow = (rank+1)*partX;
  int bothalo = (rank+1)*partX + 2*(ny+2);

  // Call the stencil kernel
  double tic = wtime();

  if (rank == MASTER) {
    for (int t = 0; t < niters; ++t) {
      stencil(rank, partX, nx, ny, image, tmp_image);
      // Send botrow to next rank
      MPI_Send(&tmp_image[botrow], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD);
      // Receive bothalo from next rank
      MPI_Recv(&tmp_image[bothalo], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD, &status);
      stencil(rank, partX, nx, ny, tmp_image, image);
      // Send botrow to next rank
      MPI_Send(&image[botrow], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD);
      // Receive bothalo from next rank
      MPI_Recv(&image[bothalo], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD, &status);

    }
    // Receive from all ranks
    if (size>2){
      for (int i = 1; i<size-1; ++i){
        // MPI_Recv(&image[i*partX+(1*(ny+2))], (ny+2)*partX, MPI_FLOAT, i, tag, MPI_COMM_WORLD, &status);
      }
    }
    if (size>1){
      //Receive from last processor
      // MPI_Recv(&image[(size-1)*partX+(ny+2)], (ny+2)*partXe, MPI_FLOAT, size-1, tag, MPI_COMM_WORLD, &status);
    }

  } else if (rank < size-1){
    for (int t = 0; t < niters; ++t) {
      stencil(rank, partX, nx, ny, image, tmp_image);
      // Send botrow to next rank
      MPI_Send(&tmp_image[botrow], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD);
      // Receive topHalo from previous rank
      MPI_Recv(&tmp_image[tophalo], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD, &status);
      // Send toprow to previous rank
      MPI_Send(&tmp_image[toprow], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD);
      // Receive bothalo from next rank
      MPI_Recv(&tmp_image[bothalo], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD, &status);

      stencil(rank, partX, nx, ny, tmp_image, image);
      // Send botrow to next rank
      MPI_Send(&image[botrow], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD);
      // Receive topHalo from previous rank
      MPI_Recv(&image[tophalo], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD, &status);
      // Send toprow to previous rank
      MPI_Send(&image[toprow], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD);
      // Receive bothalo from next rank
      MPI_Recv(&image[bothalo], ny+2, MPI_FLOAT, rank+1, tag, MPI_COMM_WORLD, &status);

    }
    // Sending the completed section
    // MPI_Send(&image[rank*partX+(1*(ny+2))], (ny+2)*partX, MPI_FLOAT, MASTER , tag, MPI_COMM_WORLD);

  } else if (rank == size-1){

    for (int t = 0; t < niters; ++t) {
      stencile(rank, partX, partXe, nx, ny, image, tmp_image);
      // Receive tophalo from previous rank
      MPI_Recv(&tmp_image[tophalo], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD, &status);
      // Send toprow to previous rank
      MPI_Send(&tmp_image[toprow], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD);

      stencile(rank, partX, partXe, nx, ny, tmp_image, image);
      // Receive tophalo from previous rank
      MPI_Recv(&image[tophalo], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD, &status);
      // Send toprow to previous rank
      MPI_Send(&image[toprow], ny+2, MPI_FLOAT, rank-1, tag, MPI_COMM_WORLD);

    }
    // Sending the completed section
    // MPI_Send(&image[toprow], partXe*(ny+2), MPI_FLOAT, MASTER, tag, MPI_COMM_WORLD);
    // MPI_Send(&image[rank*partX+(1*(ny+2))], (ny+2)*partXe, MPI_FLOAT, MASTER , tag, MPI_COMM_WORLD);


  } else {
    printf("Error on processor %d: this rank has not been coded for\n", rank );
    return EXIT_FAILURE;
  }

  double toc = wtime();

  printf("Timing on rank %d:\n", rank );
  // Output
  printf("------------------------------------\n");
  printf(" runtime: %lf s\n", toc-tic);
  printf("------------------------------------\n");

  if (rank == MASTER) {
    output_image(OUTPUT_FILE, nx, ny, image);
    free(image);
    free(tmp_image);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}

void stencil(const int rank, const int partX, const int nx, const int ny, float * restrict  image, float * restrict  tmp_image) {

  for (int i = (rank*partX) + 1; i < ((rank+1)*partX) + 1; ++i) {
    for (int j = 1; j < ny + 1; ++j) {
      tmp_image[j+i*(ny+2)] = image[j+i*(ny+2)] * 0.6f;
      tmp_image[j+i*(ny+2)] += image[j  +(i-1)*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j  +(i+1)*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j-1+i*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j+1+i*(ny+2)] * 0.1f;
    }
  }
}

void stencile(const int rank, const int partX, const int partXe, const int nx, const int ny, float * restrict  image, float * restrict  tmp_image) {

  for (int i = (rank*partX) + 1; i < (rank*partX + partXe) + 1; ++i) {
    for (int j = 1; j < ny + 1; ++j) {
      tmp_image[j+i*(ny+2)] = image[j+i*(ny+2)] * 0.6f;
      tmp_image[j+i*(ny+2)] += image[j  +(i-1)*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j  +(i+1)*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j-1+i*(ny+2)] * 0.1f;
      tmp_image[j+i*(ny+2)] += image[j+1+i*(ny+2)] * 0.1f;
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
