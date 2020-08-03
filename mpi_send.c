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
const long a = 1664525;
const long c = 1013904223;
const int k = 4;
typedef unsigned long ULONG;

int main(int argc, const char * argv[]) {

    ULONG m = (ULONG) pow(2.0, 32.0);
    ULONG N = m - (ULONG) pow(2,30);
    ULONG A = 1;
    ULONG sum = 0;
    ULONG C = 0;
    // double rX = 0;
    // double rY = 0;
    int rX[k], rY[k];
    int Distance[k][3];

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
//    cout << "sum is: " << sum << endl;
//    cout << "A is: " << A << endl;
    // in case sum is still a huge number, to calculate C need mod before multiple c
    // C = (c*sum) mod m = [c(sum mod m)] mod m
    C = (c * (sum % m)) % m;
//        cout << "C is: " << C << endl;
    printf("C is: %lu\n", C);
    //generate random number for all k proccessors
    N = 8500002;
    // double Distance = 0;
    double totalCount = 0;
    srand(12345);

    // initial the random numbers and counter
    for(int i=0; i<k; i++){
        Distance[i][0] = (double)rand();
        Distance[i][1] = (double)rand();
        Distance[i][2] = 0;
        // Distance[i][0] = (double) ((A*rand() + C) % m) / pow(2,31) - 1;
        // Distance[i][1] = (double) ((A*rand() + C) % m) / pow(2,31) - 1;
        // Distance[i][2] = sqrt(pow(Distance[i][0] ) + pow(Distance[i][1], 2));

        //xSeed[i] = rand();
        //ySeed[i] = rand();
        //inCircle[i] = 0;
        // rX[i] = (double) ((A*rand() + C) % m) / pow(2,31) - 1;
        // rY[i] = (double) ((A*rand() + C) % m) / pow(2,31) - 1;
        // Distance[i] = sqrt(pow(rX, 2) + pow(rY, 2));
    }


        if(world_rank == 0) {
            for(int i=1; i<k; i++) {
                MPI_Send(&Distance[i], 1, MPI_INT, 0, i, MPI_COMM_WORLD);
            }
            for(int j=0; j<N/k; j++) {
                double rX = (double) ((A*Distance[0][0] + C) % m) / pow(2,31) - 1;
                double rY = (double) ((A*Distance[0][1] + C) % m) / pow(2,31) - 1;
                double length = sqrt(pow(rX, 2) + pow(rY, 2));
                if(length <= 1) {
                    Distance[0][2] ++;
                    //inCircle[0] ++;
                }

                // rX = (double) ((A*xSeed[0] + C) % m) / pow(2,31) - 1;
                // rY = (double) ((A*ySeed + C) % m) / pow(2,31) - 1;
                // Distance = sqrt(pow(rX, 2) + pow(rY, 2));      
            }
            totalCount += Distance[0][2];
            for(int i=1; i<k; i++) {
                MPI_Recv(&Distance[i], 1, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // add all the counts from master and all slave processess
                totalCount += Distance[i][2];

            }
        }

        else {
            for(int i=1; i<k; i++) {
              MPI_Recv(&Distance[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
              for(int j=0; j<N/k; j++) {
                double rX = (double) ((A*Distance[i][0] + C) % m) / pow(2,31) - 1;
                double rY = (double) ((A*Distance[i][1] + C) % m) / pow(2,31) - 1;
                double length = sqrt(pow(rX, 2) + pow(rY, 2));
                if(length <= 1) {
                    Distance[i][2] ++;
                    //inCircle[0] ++;
                } 
              }
              MPI_Send(&Distance[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            
        }




    //     for(int j=0; j<N; j++) {
    //         int rSeed = rand();
    // //      cout << "rSeed is: " << rSeed << endl;
    //         rX = (double) ((A*rSeed + C) % m) / pow(2,31) - 1;
    // //      cout << " rX is: " << rX << endl;
    //         rSeed = rand();
    // //      cout << "rSeed is: " << rSeed << endl;
    //         rY = (double) ((A*rSeed + C) % m) / pow(2,31) - 1;
    // //            cout << " rY is: " << rY << endl;
    //         Distance = sqrt(pow(rX, 2) + pow(rY, 2));
    //         if(Distance <= 1) {
    //             inCircle ++;
    //         }

    //     }
    // }
//  cout << "inCircle is: " << inCircle << endl;
    double PI = (4 * totalCount / N) / k ;
    printf("PI is: %f\n", PI);
//  cout << "PI is: " << PI << endl;
//  printf("Hello, World!\n");
    
    return 0;
}
