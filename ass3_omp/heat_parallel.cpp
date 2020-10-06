#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include <omp.h>

// Load the multidimensional array template
#include "arrayff.hxx"

// Load the template functions to draw the hot and cold spots
#include "draw.hxx"

int main(int argc, char* argv[]) 
{
  const float tol = 0.00001;
  const int npix = atoi(argv[1]);
  const int npixx = npix;
  const int npixy = npix;
  const int ntotal = npixx * npixy;
 
  Array<float, 2> h(npixy, npixy), g(npixy, npixx);

  const int nrequired = npixx * npixy;
  const int ITMAX = 1000000;

  int iter = 0;
  int nconverged = 0;

  fix_boundaries2(h);
  dump_array<float, 2>(h, "plate_parellel0.fit");
  omp_set_num_threads(atoi(argv[2]));
  const int nthrds = omp_get_num_threads();
  double T0 = omp_get_wtime();


  #pragma omp parallel num_threads(nthrds) private(nconverged) shared(iter)
  {
      int ID = omp_get_thread_num();
      int npixy_pstart = npixy * ID / nthrds;
      int npixy_pend_loop2 = npixy * (ID+1) / nthrds;
      int npixy_pend1_loop1 = (npixy-2) * (ID+1) / nthrds;
      do { 
          for (int y = npixy_pstart+1; y <= npixy_pend1_loop1; ++y) {
            for (int x = 1; x < npixx-1; ++x) {
              g(y, x) = 0.25 * (h(y, x-1) + h(y, x+1) + h(y-1, x) + h(y+1,x));
            }
          }
        fix_boundaries2(g);
        nconverged = 0;
        #pragma omp barrier

        for (int y = npixy_pstart; y < npixy_pend_loop2; ++y) {
          for (int x = 0; x < npixx; ++x) {
            float dhg = std::fabs(g(y, x) - h(y, x));
            h(y, x) = g(y, x);
            if (dhg < tol) 
               ++nconverged;
          }
        }
        #pragma omp atomic update
        ++iter;
      } while (nconverged < nrequired && iter < ITMAX);
  }
  double T1 = omp_get_wtime() - T0;
  dump_array<float, 2>(h, "plate_parellel1.fit");
  std::cout << "Required " << iter << " iterations" << std::endl;
  std::cout << "time spent on iterations is: " << T1  << "s" << std::endl;
}
