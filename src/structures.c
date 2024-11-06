
#include "structures.h"
#include "constants.h"

// creates the MPI_Datatype for DateTime structure
void sensor_datetime_mpi_dtype(MPI_Datatype* new_datatype) {
    int blocklengths[DATETIME_NVALS];
    MPI_Datatype types[DATETIME_NVALS];
    MPI_Aint offsets[DATETIME_NVALS] = {
        offsetof(dateTime,year), 
        offsetof(dateTime, month),
        offsetof(dateTime, date),
        offsetof(dateTime, hour),
        offsetof(dateTime, minute),
        offsetof(dateTime, second)

        };

    for (int i = 0; i < DATETIME_NVALS; i++) {
        blocklengths[i] = 1;
        types[i] = MPI_INT;
    }
    MPI_Type_create_struct(DATETIME_NVALS, blocklengths, offsets, types, new_datatype);
    MPI_Type_commit(new_datatype);
}

// creates the MPI_Datatype for SensorReading structure
void sensor_reading_mpi_dtype(MPI_Datatype* new_datatype, MPI_Datatype sensor_dt_mpi_type_st) {
    int blocklengths[READING_NVALS];
    MPI_Datatype types[READING_NVALS];
    MPI_Aint offsets[READING_NVALS] = {
        offsetof(sensorReading,latitude), 
        offsetof(sensorReading, longitude),
        offsetof(sensorReading, magnitude),
        offsetof(sensorReading, depth),
        offsetof(sensorReading, datetime)
        };

    for (int i = 0; i < READING_NVALS-1; i++) {
        blocklengths[i] = 1;
        types[i] = MPI_FLOAT;
    }
    blocklengths[READING_NVALS-1] = 1;
    types[READING_NVALS-1] = sensor_dt_mpi_type_st;
    MPI_Type_create_struct(READING_NVALS, blocklengths, offsets, types, new_datatype);
    MPI_Type_commit(new_datatype);
}

// creates the MPI_Datatype for Alert structure
MPI_Datatype create_alert_mpi_dtype(MPI_Datatype reading_mpi_type_st,
                                    MPI_Datatype machine_dets_type) {
    MPI_Datatype alert_dtype;

    int blocklengths[ALERT_NVALS] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype types[ALERT_NVALS] = {MPI_INT, MPI_INT, MPI_INT, machine_dets_type, reading_mpi_type_st, MPI_UNSIGNED_LONG_LONG};
    MPI_Aint offsets[ALERT_NVALS] = {
        offsetof(alert, x_coord), 
        offsetof(alert, y_coord),
        offsetof(alert, rank),
        offsetof(alert, machine_dets),
        offsetof(alert, reading),
        offsetof(alert, report_time)
        };

    MPI_Type_create_struct(ALERT_NVALS, blocklengths, offsets, types, &alert_dtype);
    MPI_Type_commit(&alert_dtype);
    return alert_dtype;

}

// creates the MPI_Datatype for MachineDetails structure
MPI_Datatype create_machine_details_mpi_dtype() {
    MPI_Datatype machine_details_type;
    int blocklengths[MACHINE_DETS_NVALS] = {IP_ADDR_LEN, HOST_LEN};
    MPI_Datatype types[MACHINE_DETS_NVALS] = {MPI_CHAR, MPI_CHAR};
    MPI_Aint offsets[MACHINE_DETS_NVALS] = {
        offsetof(machineDetails,ip_addr), 
        offsetof(machineDetails, hostname)
        };

    MPI_Type_create_struct(MACHINE_DETS_NVALS, blocklengths, offsets, types, &machine_details_type);
    MPI_Type_commit(&machine_details_type);
    return machine_details_type;
}

// helper function which creates all 4 types above
void create_all_mpi_types(MPI_Datatype* sensor_dt_type_ptr, MPI_Datatype* reading_type_ptr,
                          MPI_Datatype* machine_dets_type_ptr, MPI_Datatype* alert_type_ptr) {
    sensor_datetime_mpi_dtype(sensor_dt_type_ptr);
    sensor_reading_mpi_dtype(reading_type_ptr, *sensor_dt_type_ptr);
    *machine_dets_type_ptr= create_machine_details_mpi_dtype();
    *alert_type_ptr = create_alert_mpi_dtype(*reading_type_ptr, *machine_dets_type_ptr);
}