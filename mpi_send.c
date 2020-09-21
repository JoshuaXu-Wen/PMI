//
//  main.c
//  Parallel
//
//  Created by xuwen on 2020/7/29.
//  Copyright © 2020 Joshua Xu. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <mpi.h>

typedef unsigned long ULONG;



int main(int argc, char** argv) {
    const ULONG a = 1664525;
    const ULONG c = 1013904223;
    const ULONG m = (ULONG) pow(2, 32);
    const ULONG sidelen = sqrt(m);
    const double N = m - pow(2,30); // total random numbers, any large number that less than m
 
    ULONG A = 1;
    ULONG sum = 0;
    ULONG C = 0;
    ULONG totalCount = 0;
    ULONG inCircle_0;
    ULONG inCircle_1;
    double T0 = 0, T1 = 0;
    srand(12345);


    MPI_Status Stat;
    MPI_Init(&argc, &argv);
    // world_rank is process id
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // world_size is total processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    //ULONG Seed[world_size];
    ULONG Seed = 0;
    //int inCircle[world_size];

    //printf("total proccessors: %d\n", world_size);
        /* Calculate C with two formula
        (ab) mod m=[a(b mod m)] mod m
        (a+b) mod m=[(a mod m)+(b mod m)] mod m
     
        1) C=c(ak−1+ak−2+···+a1+a0) mod m
        2) C = [c((ak−1+ak−2+···+a1+a0) mod m)] mod m
        3) sum = (ak−1+ak−2+···+a1+a0) mod m
        4) sum = [(ak-1 mod m) + (ak-2 mod m) + ... + (a0 mod m) ] mod m
        */
        // first calculate the sum
    for(int i=0; i<world_size; i++) {
        A = (A * a) % m;
        ULONG C_temp = 1;
        for( int j=0; j<=i; j++) {
            C_temp = (C_temp * a) % m;
        }
        sum += C_temp;
    }
    sum = sum % m;
        // in case sum is still a huge number, to calculate C need mod before multiple c
        // C = (c*sum) mod m = [c(sum mod m)] mod m
    C = (c * (sum % m)) % m;
    fprintf(stdout, "C is: %lu\n", C);

    if(world_rank == 0) {
        T0 = MPI_Wtime();

        //generate random number for all proccessors
        // initial the random numbers and counter
        for(int i=1; i<world_size; i++){
            Seed = rand();
            fprintf(stdout, "Seed[%d] on master process is %lu\n", i, Seed);
            MPI_Send(&Seed, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        inCircle_0 = 0;
        Seed = rand();
        for(int j=0; j<N/world_size; j++) {
            //put interge in range(0, 65536)
            ULONG i_random = (A*Seed + C) % m;
            double rX = i_random % sidelen;
            double rY = i_random / sidelen;
            // rescale x and y in (-1, 1)
            rX = 2 * rX / sidelen - 1;
            rY = 2 * rY / sidelen - 1;
            double length = sqrt(pow(rX, 2) + pow(rY, 2));
            //printf("length on master process is: %f\n", length);

            if(length <= 1) {
                inCircle_0++ ;
            } 
            // use the exist random number as seed to generate a new random number
            Seed = i_random;

        }
        totalCount += inCircle_0;
        //fprintf(stdout, "inCircle_0 is: %lu\n", inCircle_0);

        for(int i=1; i<world_size; i++) {
            MPI_Recv(&inCircle_1, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &Stat);
            // add all the counts from master and all slave processess
            //fprintf(stdout, "inCircle_1 from process %d on Master process: is %lu\n", i, inCircle_1);
            totalCount += inCircle_1;

        }
        //fprintf(stdout, "totalCount is: %lu\n",  totalCount);
        double PI = 4 * totalCount / N ;
        T0 = MPI_Wtime() - T0;
        printf("total time on master process is %f\n", T0);
        fprintf(stdout, "PI is: %f\n", PI);
    }

    else {
        T1 = MPI_Wtime();
        inCircle_1 = 0;
        MPI_Recv(&Seed, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Stat); 

        //printf("Seed[%d] on process %d is %lu\n", world_rank, world_rank, Seed);
        for(int j=0; j<N/world_size; j++) {
            ULONG i_random = (A*Seed + C) % m; 
            //put interge in range(0, 65536)
            double rX = i_random % sidelen;
            double rY = i_random / sidelen;
            // rescale x and y in (-1, 1)
            rX = 2 * rX / sidelen - 1;
            rY = 2 * rY / sidelen - 1;
            double length = sqrt(pow(rX, 2) + pow(rY, 2));
            //fprintf(stdout, "length is: %f\n", length);
            if(length <= 1) {
                inCircle_1++;
            }
                // use the exist random number as seed to generate a new random number
            Seed = i_random;

        }
        //fprintf(stdout, "inCircle_1 on process %d is: %lu\n", world_rank, inCircle_1);
        MPI_Send(&inCircle_1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        T1 = MPI_Wtime() - T1;
        printf("time on slave process %d is %f\n", world_rank, T1);
    }

    MPI_Finalize();

    
    return 0;
}
