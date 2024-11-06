#ifndef BASE_H
#define BASE_H

#include "constants.h"
#include <stdbool.h>
#include "sensors.h"
#include "structures.h"


// Main function for base station simulation
int base_station_main(MPI_Comm world_comm, MPI_Comm comm, int *dims, int size, int max_iterations, int sentinel, int caas);

// Main function for simulating balloon seismic sensors
void* simulate_balloon_func(void *pArg);

// Main function for logging a sample report
void log_report(alert* raised_alert, sensorReading balloon_seismic_reading, int iteration, bool matched, float comm_time, FILE *log_file,  int alert_count);

// Main function for logging the summary of a sample report
void summary(FILE *log_file, int iterations, int true_alert, int false_alert, float total_comm_time, int message_num);

// Auxiliary function for base station simulation
int base_aux(MPI_Comm world_comm, MPI_Comm comm, FILE *log_file, int global_arr_size, int max_iterations, int sentinel, int caas);

//Main function to check if a node's readings match with another node's readings based on pre-defined thresholds
bool matched(float lat1, float long1, float mag1, float depth1, float lat2, float long2, float mag2, float depth2);

#endif
