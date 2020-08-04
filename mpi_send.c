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

int main(int argc, const char * argv[]) {
    const ULONG a = 1664525;
    const ULONG c = 1013904223;
    const ULONG m = (ULONG) pow(2.0, 32.0);
    const ULONG sidelen = sqrt(m);
    ULONG N = m - pow(2,30);
    const int k = 4;
 
    ULONG A = 1;
    ULONG sum = 0;
    ULONG C = 0;
    int Seed[k];
    int inCircle[k];
    // double rX = 0;
    // double rY = 0;

    MPI_Init(NULL, NULL);
    // world_rank is process id
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // world_size is total processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    /* Calculate C with two formula
     (ab) mod m=[a(b mod m)] mod m
     (a+b) mod m=[(a mod m)+(b mod m)] mod m
     
     1) C=c(ak−1+ak−2+···+a1+a0) mod m
     2) C = [c((ak−1+ak−2+···+a1+a0) mod m)] mod m
     3) sum = (ak−1+ak−2+···+a1+a0) mod m
     4) sum = [(ak-1 mod m) + (ak-2 mod m) + ... + (a0 mod m) ] mod m
     */
    // first calculate the sum
    for(int i=0; i<k; i++) {
        A = (A * a) % m;
        ULONG C_temp = 1;
        for( int j=0; j<=i; j++) {
            C_temp = (C_temp * a) % m;
        }
        sum += C_temp;
    }
    sum = sum % m;
    printf("sum is: %lu\n", sum);
    printf("A is: %lu\n", A);
    // in case sum is still a huge number, to calculate C need mod before multiple c
    // C = (c*sum) mod m = [c(sum mod m)] mod m
    C = (c * (sum % m)) % m;
//        cout << "C is: " << C << endl;
    printf("C is: %lu\n", C);
    //generate random number for all k proccessors
     N = 100;
    // double Distance = 0;
    double totalCount = 0;
    srand(12345);

    // initial the random numbers and counter
    for(int i=0; i<k; i++){
        Seed[i] = rand();
        inCircle[i] = 0;
    }


    if(world_rank == 0) {
        for(int i=1; i<k; i++) {
            MPI_Send(&Seed[i], 1, MPI_INT, 0, i, MPI_COMM_WORLD);
        }
        for(int j=0; j<N/k; j++) {
            ULONG i_random = (A*Seed[0] + C) % m;
            //put interge in range(0, 65536)
            ULONG rX = i_random % sidelen;
            ULONG rY = i_random / sidelen;
            // rescale x and y in (-1, 1)
            rX = 2 * rX / sidelen -1;
            rY = 2 * rY / sidelen -1;
            double length = sqrt(pow(rX, 2) + pow(rY, 2));
            if(length <= 1) {
                inCircle[0]++ ;
            }   
        }
        totalCount += inCircle[0];
        for(int i=1; i<k; i++) {
            MPI_Recv(&Seed[i], 1, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // add all the counts from master and all slave processess
            totalCount += inCircle[i];

        }
    }

    else {
        for(int i=1; i<k; i++) {
            MPI_Recv(&Seed[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
            for(int j=0; j<N/k; j++) {
                ULONG i_random = (A*Seed[0] + C) % m;
                //put interge in range(0, 65536)
                ULONG rX = i_random % sidelen;
                ULONG rY = i_random / sidelen;
                // rescale x and y in (-1, 1)
                rX = 2 * rX / sidelen -1;
                rY = 2 * rY / sidelen -1;
                // double rX = (double) ((A*Distance[0][0] + C) % m) / pow(2,31) - 1;
                // double rY = (double) ((A*Distance[0][1] + C) % m) / pow(2,31) - 1;
                double length = sqrt(pow(rX, 2) + pow(rY, 2));
                if(length <= 1) {
                    inCircle[i]++ ;
                } 
            }

            MPI_Send(&Seed[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
            
    }

    MPI_Finalize();

    double PI = (4 * totalCount / N) / k ;
    printf("PI is: %f\n", PI);
    
    return 0;
}
