/* 
   159735 Parallel Programming
   Startup program for sequential implementation of simulation by ray
   tracing of gravitational lensing.
 */
#include <ctime>

#include <iostream>
#include <string>

#include <cmath>
#include <cuda.h>
#include "lenses.h"
#include "arrayff.hxx"
#define N (2048*2048)
#define THREADS_PER_BLOCK 512
// Global variables! Not nice style, but we'll get away with it here.

// Boundaries in physical units on the lens plane
const float WL  = 2.0;
const float XL1 = -WL;
const float XL2 =  WL;
const float YL1 = -WL;
const float YL2 =  WL;

// Used to time code. OK for single threaded programs but not for
// multithreaded programs. See other demos for hints at timing CUDA
// code.
double diffclock(clock_t clock1,clock_t clock2)
{
  double diffticks = clock1 - clock2;
  double diffms = (diffticks * 1000) / CLOCKS_PER_SEC;
  return diffms; // Time difference in milliseconds
}


__device__ void shootParallel(float& xs, float& ys, float xl, float yl, 
     float* xlens, float* ylens, float* eps, int nlenses)
{
  float dx, dy, dr;
  xs = xl;
  ys = yl;
  for (int p = 0; p < nlenses; ++p) {
    dx = xl - xlens[p];
    dy = yl - ylens[p];
    dr = dx * dx + dy * dy;
    xs -= eps[p] * dx / dr;
    ys -= eps[p] * dy / dr;
  }

}


__global__ void pixel_add(int npixx, int npixy, int lens_scale, int nlenses, float* xlens, float* ylens, float* eps, float *lensim)
{
  int index = threadIdx.x + blockIdx.x * blockDim.x;
  const float rsrc = 0.1;      // radius
  const float ldc  = 0.5;      // limb darkening coefficient
  const float xsrc = 0.0;      // x and y centre on the map
  const float ysrc = 0.0;
  const float rsrc2 = rsrc * rsrc;
  if (index <= npixx * npixy) {
    //what ix and iy  does this thread work on
    int ix = index % npixx;
    int iy = index / npixy; 
    float xl, yl, xs, ys, sep2, mu;
    float xd, yd, dx, dy, dr;

    yl = YL1 + iy * lens_scale;
    xl = XL1 + ix * lens_scale;
    shootParallel(xs, ys, xl, yl, xlens, ylens, eps, nlenses);
    xd = xs - xsrc;
    yd = ys - ysrc;
    sep2 = xd * xd + yd * yd;
    if (sep2 < rsrc2) {
      mu = sqrt(1 - sep2 / rsrc2);
      lensim[index] = 1.0 - (ldc * (1 - mu));
    }
  }
  
}
    

int main(int argc, char* argv[]) 
{
  // Set up lensing system configuration - call example_1, _2, _3 or
  // _n as you wish. The positions and mass fractions of the lenses
  // are stored in these arrays
  float* xlens;
  float* ylens;
  float* eps;
  const int nlenses = set_example_1(&xlens, &ylens, &eps);
  std::cout << "# Simulating " << nlenses << " lens system" << std::endl;
  size_t size = nlenses * sizeof(float);
 
 // std::cout << "# Building " << npixx << "X" << npixy << " lens image" << std::endl;

   const float lens_scale = 0.005;
// Size of the lens image
   const int npixx = static_cast<int>(floor((XL2 - XL1) / lens_scale)) + 1;
   const int npixy = static_cast<int>(floor((YL2 - YL1) / lens_scale)) + 1;

  // Put the lens image in this array
  Array<float, 2> lensim(npixy, npixx);
  size_t size_lensim = (npixy*npixy) * sizeof(float)

  float *dev_xlens, *dev_ylens, *dev_eps, *dev_lensim,
  cudaMalloc(&dev_xlens, size);
  cudaMalloc(&dev_ylens, size);
  cudaMalloc(&dev_eps, size);
  cudaMalloc(&dev_lensim, size_lensim);

  cudaMemcpy(dev_xlens, xlens, size, cudaMemcpyHostToDevice);
  cudaMemcpy(dev_ylens, ylens, size, cudaMemcpyHostToDevice);
  cudaMemcpy(dev_eps, eps, size, cudaMemcpyHostToDevice);
  cudaMemcpy(dev_lensim, lensim.buffer, size_lensim, cudaMemcpyHostToDevice);


  clock_t tstart = clock();
  pixel_add<<< N/THREADS_PER_BLOCK, THREADS_PER_BLOCK >>>(npixx, npixy, lens_scale, nlenses, dev_xlens, dev_ylens, dev_eps, dev_lensim);
 
  int numuse = 0;


  // Copy result from device memory into host memory
  cudaMemcpy(lensim.buffer, dev_lensim, size, cudaMemcpyDeviceToHost);



  clock_t tend = clock();
  double tms = diffclock(tend, tstart);
  std::cout << "# Time elapsed: " << tms << " ms " << numuse << std::endl;

  // Write the lens image to a FITS formatted file. You can view this
  // image file using ds9
  dump_array<float, 2>(lensim, "lens.fit");

  delete[] xlens;
  delete[] ylens;
  delete[] eps;
}
