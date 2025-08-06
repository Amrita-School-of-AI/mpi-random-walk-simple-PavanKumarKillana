#include <iostream>
#include <cstdlib> // For atoi, rand, srand
#include <ctime>   // For time
#include <mpi.h>

void walker_process();
void controller_process();

int domain_size;
int max_steps;
int world_rank;
int world_size;

int main(int argc, char **argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 3)
    {
        if (world_rank == 0)
        {
            std::cerr << "Usage: mpirun -np <p> " << argv[0] << " <domain_size> <max_steps>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    domain_size = atoi(argv[1]);
    max_steps = atoi(argv[2]);

    if (world_rank == 0)
    {
        // Rank 0 is the controller
        controller_process();
    }
    else
    {
        // All other ranks are walkers
        walker_process();
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}

void walker_process()
{
    // Seed the random number generator.
    // Using rank ensures each walker gets a different sequence of random numbers.
    srand(time(NULL) + world_rank);

    // 1. Initialize the walker's position to 0.
    int position = 0;
    int steps = 0;

    // 2. Loop for a maximum of `max_steps`.
    while (steps < max_steps)
    {
        // 3. In each step, randomly move left (-1) or right (+1).
        int move = (rand() % 2 == 0) ? -1 : 1;
        position += move;
        steps++;

        // 4. Check if the walker has moved outside the domain [-domain_size, +domain_size].
        if (position > domain_size || position < -domain_size)
        {
            // 5a. Print a message with keyword "finished"
            std::cout << "Rank " << world_rank << ": Walker finished in " << steps << " steps." << std::endl;

            // 5b. Send an integer message to the controller to signal completion.
            int finish = 1;
            MPI_Send(&finish, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            // 5c. Break the loop.
            return;
        }
    }

    // 5a (if finished by max_steps): still print message
    std::cout << "Rank " << world_rank << ": Walker finished in " << steps << " steps (max steps reached)." << std::endl;
    int finish = 1;
    MPI_Send(&finish, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

void controller_process()
{
    // 1. Determine number of walkers
    int walkers = world_size - 1;
    int received = 0;

    // 2. Loop to receive messages from each walker
    while (received < walkers)
    {
        int msg;
        MPI_Status status;

        // 3. Wait for a message from any walker
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        // 4. Increment received count
        received++;
    }

    // 4. Print final summary message
    std::cout << "Controller: All " << walkers << " walkers have finished." << std::endl;
}
