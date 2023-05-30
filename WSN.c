#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>

#define ALERT 2

int base_station(MPI_Comm world_comm, MPI_Comm comm);
int sensor(MPI_Comm world_comm, MPI_Comm comm, int nrows , int ncols);
int rows, cols;

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Comm new_comm;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc == 3) {
		rows = atoi (argv[1]);
		cols = atoi (argv[2]);
		if( (rows*cols) != size-1) {
			if( my_rank ==0) printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", rows, cols, rows*cols,size-1);
			MPI_Finalize(); 
			return 0;
		}
    }
    else{
    	rows = cols = 0;
    }
    MPI_Comm_split( MPI_COMM_WORLD,rank == 0, 0, &new_comm); // color will either be 0 or 1 
    if (rank == 0) 
	base_station( MPI_COMM_WORLD, new_comm );
    else
	sensor( MPI_COMM_WORLD, new_comm , rows , cols);
    MPI_Finalize();
    return 0;
}

/* This is the base */
int base_station(MPI_Comm world_comm, MPI_Comm comm)
{
	int        i, size, nslaves, firstmsg;
	float sentinel;
	time_t rawtime;
	struct tm *info;
	MPI_Status status;
	MPI_Comm_size(world_comm, &size );
	nslaves = size - 1;
	double * buf;
	buf = (double*)malloc(8 * sizeof(double));
	sentinel = 1.0;

	for(i = 0; i<10, i++;){
		MPI_Bcast(&sentinel, 1, MPI_FLOAT, 0, world_comm);
		MPI_Recv( buf, 6, MPI_DOUBLE, MPI_ANY_SOURCE, ALERT, world_comm, &status);
		rawtime = (time_t)buf[0]
		info = localtime( &rawtime );
   		printf("time：%s (%d,%d)'s height:%f top:%f bottom:%f left:%f right:%f", asctime(info),(int)buf[1],(int)buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);	
	}
	sentinel = 0.0;
	MPI_Bcast(&sentinel, 1, MPI_FLOAT, 0, world_comm);
    return 0;
}

/* This is the sensor */
int sensor(MPI_Comm world_comm, MPI_Comm comm, int nrows , int ncols)
{
	int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, worldSize;
	MPI_Comm comm2D;
	MPI_Status status;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	double buf[8];
	float sentinel;
	time_t now;
	float height;
	int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	MPI_Request send_request[4];
    MPI_Request receive_request[4];
    MPI_Status send_status[4];
    MPI_Status receive_status[4]
    float * recvVal[4];
    int i , count;

    
    MPI_Comm_size(world_comm, &worldSize); // size of the world communicator
  	MPI_Comm_size(comm, &size); // size of the sensor communicator
	MPI_Comm_rank(comm, &my_rank);  // rank of the sensor communicator

	if (nrows != 0 && ncols != 0) {
		dims[0] = nrows; /* number of rows */
		dims[1] = ncols; /* number of columns */
	} else {
		dims[0]=dims[1]=0;
		nrows=ncols=(int)sqrt(size);
	}
	
	
	MPI_Dims_create(size, ndims, dims);
    	if(my_rank==0)
		printf("Sensor Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",my_rank,size,dims[0],dims[1]);

    /* create cartesian mapping */
	wrap_around[0] = 0;
	wrap_around[1] = 0; /* periodic shift is .false. */
	reorder = 0;
	ierr =0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
	
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

	MPI_Cart_shift( comm2D, 0, 1, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, 1, 1, &nbr_j_lo, &nbr_j_hi );

	do{
		time(&now);
		srand((unsigned)time(now));
		height = 4000 ＋ 1.0 * rand() / RAND_MAX * 4000;
		MPI_Bcast(&sentinel, 1, MPI_FLOAT, 0, world_comm);
		if (height > 6000.0)
		{
			MPI_Isend(&height, 1, MPI_FLOAT, nbr_i_lo, 1, comm2D, &send_request[0]);
			MPI_Isend(&height, 1, MPI_FLOAT, nbr_i_hi, 1, comm2D, &send_request[1]);
			MPI_Isend(&height, 1, MPI_FLOAT, nbr_j_lo, 1, comm2D, &send_request[2]);
			MPI_Isend(&height, 1, MPI_FLOAT, nbr_j_hi, 1, comm2D, &send_request[3]);
		
			
			MPI_Irecv(&recvVal[0], 1, MPI_FLOAT, nbr_i_lo, 1, comm2D, &receive_request[0]);  	// top
			MPI_Irecv(&recvVal[1], 1, MPI_FLOAT, nbr_i_hi, 1, comm2D, &receive_request[1]);  	// bottom
			MPI_Irecv(&recvVal[2], 1, MPI_FLOAT, nbr_j_lo, 1, comm2D, &receive_request[2]);		// left
			MPI_Irecv(&recvVal[3], 1, MPI_FLOAT, nbr_j_hi, 1, comm2D, &receive_request[3]);		// right
		
			MPI_Waitall(4, send_request, send_status);
			MPI_Waitall(4, receive_request, receive_status);

			count = 0;
			for (i = 0; i < 4; i++;){
				if (abs(height - recvVal[i]) <= 100){
					count += 1;
				}
			}
			if (count >= 2){
				buf[0] = (double)now;
				buf[1] = (double)coord[0];
				buf[2] = (double)coord[1];
				buf[3] = (double)height;
				for (i = 0; i < 4; i++;){
					buf[4+i] = (double)recvVal[i];
				} 
				MPI_Send(buf, strlen(buf) + 1, MPI_DOUBLE, 0, ALERT, world_comm);
			}

		}


	} while(sentinel != 0.0)

	
/*
	printf("Global rank (within slave comm): %d. Cart rank: %d. Coord: (%d, %d).\n", my_rank, my_cart_rank, coord[0], coord[1]);
	fflush(stdout);


	sprintf( buf, "Hello from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm );

	sprintf( buf, "Goodbye from slave %d at Coordinate: (%d, %d)\n", my_rank, coord[0], coord[1]);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);
	
	sprintf(buf, "Slave %d at Coordinate: (%d, %d) is exiting\n", my_rank, coord[0], coord[1]);
	MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, worldSize-1, MSG_PRINT_ORDERED, world_comm);
	MPI_Send(buf, 0, MPI_CHAR, worldSize-1, MSG_EXIT, world_comm);
*/
    MPI_Comm_free( &comm2D );
	return 0;
}