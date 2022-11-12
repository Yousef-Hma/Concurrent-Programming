/* ------------------------------------------------------------------------------------------------
    Author : 14296919
    Concurrent Programming CW - Advanced Computational Engineering (EEEE4115)
 ------------------------------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <time.h>

/* Check if a number is a power of 2 or not.
   If n is power of 2, return 1, else return 0.
 */
int powerOfTwo(int n)
{
    if(n==0)
        return 0;

    while(n != 1)
    {
        n = n/2;
        if(n%2 != 0 && n != 1)
            return 0;
    }
    return 1;
}

int main(int argc, char* argv[])
{
    int Rank, Size, TermCount;

    // Initiate/Start MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank); // CPU Identifier
    MPI_Comm_size(MPI_COMM_WORLD, &Size); // Number of CPUs

    // Size Check
    if (powerOfTwo(Size) != 1)
    {
        if( Rank == 0) fprintf(stderr, "Not a power of 2");
        MPI_Finalize();
        return 1;
    }

    // Declarations and initiations
    int BuffVal = 0, MasterCPU = 0, Steps = 1, MsgID_UP = 1, MsgID_DP = 1000;
    int Level = Rank + 1;
    int OutputArray[Size];
    int RandArray[Size];

    // Initiate Randomisation
    time_t T;
    srand(time(&T));

// ------------------------------ Master CPU ----------------------------------------
    if(Rank == MasterCPU)
    {

        printf("Enter the number of terms: "); //Prompt user to input number of terms
        fflush(stdout);
        scanf("%d", &TermCount);

        int i;

        for(i=0; i<Size; i++)
            RandArray[i]=  0 ; // Add padding

        for(i=0; i<TermCount; i++)
            RandArray[i]=  rand()%10;  //Populate array with random 1-digit numbers

        // Display the starting values
        printf("Initial elements of array: ");
        for(i=0 ; i<Size; i++)
            printf("%d ",RandArray[i]);

        fflush(stdout);
    }
// ---------------------------------------------------------------------------------

    // Broadcast the number of terms to all CPUs
    MPI_Bcast(&TermCount, 1, MPI_INT, MasterCPU, MPI_COMM_WORLD);

    // Scatter the data of the array to CPUs using Element_Index = CPU_Rank
    MPI_Scatter(RandArray, 1, MPI_INT, &BuffVal, 1, MPI_INT, 0, MPI_COMM_WORLD);


// ----------------------------------- Up Phase ------------------------------------

    for(int Position_UP = 2; Position_UP <= Size; Position_UP=Position_UP*2 )
    {
        // Updated elements (Receivers)
        if(Level%Position_UP == 0)
        {
            int recvData;
            int Source = Rank - Steps;

            // Non-blocking Receive
            MPI_Request recv_request;
            MPI_Irecv(&recvData,1,MPI_INT,Source,MsgID_UP,MPI_COMM_WORLD,&recv_request);

            MPI_Wait(&recv_request, MPI_STATUS_IGNORE); // Wait until data is received
            BuffVal = BuffVal + recvData; // Use received data to update
        }
        else if(Level <= Size - Steps)// Updating element (Senders)
        {
            int destination = Rank + Steps;

            // Non-blocking Send
            MPI_Request send_request;
            MPI_Isend(&BuffVal,1,MPI_INT,destination,MsgID_UP,MPI_COMM_WORLD,&send_request);
        }
        Steps = Steps*2;
        MsgID_UP++; // Increment to obtain new unique message ID
    }

// ---------------------------------------------------------------------------------


// ----------------------------------- Down Phase ----------------------------------

    if( Rank != MasterCPU )
    {
        for(int Position_DP = Size/2; Position_DP >= 2; Position_DP=Position_DP/2)
        {
            int temp = Position_DP/2; // Dummy variable to store iteration condition

            if(Level%Position_DP == 0) // Updating elements (Senders)
            {
                int destination = Rank + Position_DP/2;

                if(destination < Size) // Ensure that destination is within the sequence of terms
                {
                    // Non-blocking Send
                    MPI_Request send_request;
                    MPI_Isend(&BuffVal,1,MPI_INT,destination,MsgID_DP,MPI_COMM_WORLD,&send_request);
                }
            }
            else if( Level%temp == 0 && Level != 1) // Updated elements (Receivers)
            {
                int recvData;
                int Source = Rank - Position_DP/2;

                if(Source > 0) // Ensure that source is within the sequence of terms
                {
                    // Non-blocking Receive
                    MPI_Request recv_request;
                    MPI_Irecv(&recvData,1,MPI_INT,Source,MsgID_DP,MPI_COMM_WORLD,&recv_request);

                    MPI_Wait(&recv_request, MPI_STATUS_IGNORE); // Wait until data is received
                    BuffVal = BuffVal + recvData; // Use received data to update
                }

            }
            MsgID_DP++; // Increment to obtain new unique message ID
        }
    }

// ---------------------------------------------------------------------------------

    // Gather the data from CPUs using Element_Index = CPU_Rank and store it in the Output Array
    MPI_Gather(&BuffVal,1,MPI_INT,OutputArray,1,MPI_INT,MasterCPU,MPI_COMM_WORLD);


// ------------------------------ Master CPU ----------------------------------------

    if (Rank == MasterCPU)
    {
        printf("\nFinal elements of array :  "); // Display the final elements of the array
        for (int i = 0; i < Size; ++i)
            printf("%d ",OutputArray[i]);
        fflush(stdout);

    }
// ---------------------------------------------------------------------------------

    MPI_Finalize();

    return 0;
}
