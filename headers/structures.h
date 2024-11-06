#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <mpi.h>
#include "constants.h"

typedef struct dateTime {
    int year;
    int month;
    int date;
    int hour;
    int minute;
    int second;
} dateTime;

typedef struct SensorReading {
    float latitude;
    float longitude;
    float magnitude;
    float depth;
    dateTime datetime;
} sensorReading;

typedef struct MachineDetails {
    char ip_addr[IP_ADDR_LEN];
    char hostname[HOST_LEN];
} machineDetails;

// structure for a single alert
typedef struct Alert {
    int x_coord;
    int y_coord;
    int rank;
    machineDetails machine_dets;
    sensorReading reading;
    unsigned long long report_time;
} alert;

// typedef struct Alert {
//     int x_coords[NNEIGHBORS+1]; // max 4 neighbors + reporting node
//     int y_coords[NNEIGHBORS+1]; // max 4 neighbors + reporting node
//     int neighbor_ranks[NNEIGHBORS];
//     char IP_addresses[NNEIGHBORS+1][IP_ADDR_LEN]; // +1 for terminating char
//     char host[HOST_LEN];
//     sensorReading readings[NNEIGHBORS+1]; // max 4 neighbors + reporting node

// } alert;


// typedef struct LogStruct {
//     int year;
//     int month;
//     int date;
//     int hour;
//     int minute;
//     int second;
//     int reporting_node;
//     float reporting_x_coord;
//     float reporting_y_coord;
//     float reporting_latitude;
//     float reporting_longitude;
//     float reporting_magnitude;
//     float reporting_depth;
//     int neighbors_x_coord[NNEIGHBORS];
//     int neighbors_y_coord[NNEIGHBORS];
//     float neighbors_latitude[NNEIGHBORS];
//     float neighbors_longitude[NNEIGHBORS];
//     float neighbors_magnitude[NNEIGHBORS];
//     float neighbors_depth[NNEIGHBORS];
//     int neighbors_rank[NNEIGHBORS];
//     int matches;
//     char IP_addr[20];
//     char host[256];

// } logstruct;

void sensor_datetime_mpi_dtype(MPI_Datatype* new_datatype);
void sensor_reading_mpi_dtype(MPI_Datatype* new_datatype, MPI_Datatype dt_type_in);
MPI_Datatype create_machine_details_mpi_dtype();
MPI_Datatype create_alert_mpi_dtype(MPI_Datatype r_type,
                                    MPI_Datatype m_dets_type);
void create_all_mpi_types(MPI_Datatype* sensor_dt_type_ptr, MPI_Datatype* reading_type_ptr,
                          MPI_Datatype* machine_dets_type_ptr, MPI_Datatype* alert_type_ptr);

#endif