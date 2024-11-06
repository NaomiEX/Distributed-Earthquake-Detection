#ifndef SENSORS_H
#define SENSORS_H

#include "constants.h"
#include <stdbool.h>
#include "structures.h"


int sensor_main(MPI_Comm comm, int dims[]);
void* communicateWithNeighbors(void* pArg);
void check_requests(MPI_Comm cart_comm, sensorReading* reading, MPI_Datatype reading_mpi_type_in, int cart_rank);
int is_match(sensorReading reading1, sensorReading reading2);
void check_if_alive();
void communicate_machine_details();
void create_all_alerts(alert* alerts, int num_neighbors);
alert create_alert(int rank, int idx);

#endif
