#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "sensors.h"
#include "constants.h"
#include "helpers.h"
#include "base.h"
#include "structures.h"

// global variables
int cart_rank, alive=1, neighbors[NNEIGHBORS];
sensorReading *reading_ptr = NULL, neighbor_readings[NNEIGHBORS];
machineDetails machine_dets[NNEIGHBORS+1]; // 0 will be current node's machine details
bool request_from_neighbors=false, check_neighbor_values=false;
MPI_Comm cart_comm;
MPI_Datatype sensor_dt_mpi_type_s, reading_mpi_type, alert_mpi_type, machine_dets_mpi_type;
pthread_mutex_t reading_mutex;


int sensor_main(MPI_Comm comm, int *dims) {
    time_t cur_time;
    struct tm timeinfo;

    MPI_Request req;
    sensorReading reading;
    alert *sensor_alerts;
    pthread_t tid;
    int num_matches=0, coords[NDIMS];
    dateTime reading_dt;

    // make cart communicator globally accessible
    cart_comm=comm;

    MPI_Comm_rank(cart_comm, &cart_rank);
    MPI_Cart_coords(cart_comm, cart_rank, NDIMS, coords);
    unsigned int seed = time(NULL) * cart_rank;

    // get left and right neighbors
    MPI_Cart_shift(cart_comm,ROWDIR, DISPLACEMENT, &neighbors[0], &neighbors[1]);
    // get top and bottom neighbors
    MPI_Cart_shift(cart_comm, COLDIR, DISPLACEMENT, &neighbors[2], &neighbors[3]);

    // count number of actual neighbors
    // corner and edge nodes have < 4 neighbors
    int num_neighbors = 0;
    for (int n_idx = 0; n_idx < NNEIGHBORS; n_idx++) {
        if(neighbors[n_idx] >= 0) {
            num_neighbors++;
        }
    }

    create_all_mpi_types(&sensor_dt_mpi_type_s, &reading_mpi_type, &machine_dets_mpi_type, &alert_mpi_type);

    // alerts is an array of size == number of actual neighbors + this node
    // each alert structure contains the alert information (coordinates, ip address, hostname, sensor readings, report time) 
    // for a single node to be sent to base
    sensor_alerts = (alert*)(malloc(sizeof(alert) * (num_neighbors+1))); 
    pthread_mutex_init(&reading_mutex, NULL);
    // spawn thread for communicating with neighbor before entering while loop
    pthread_create(&tid, NULL, communicateWithNeighbors, NULL);


    while(alive) {
        // waits for thread to finish collecting from neighbors if, 
        // in the last iteration, the EQ threshold was exceeded
        // otherwise it will be able to obtain the lock immediately
        pthread_mutex_lock(&reading_mutex);

        // if neighbor values have been collected and are ready for checking
        if (check_neighbor_values) {
            // ------- COMPARE WITH COLLECTED NEIGHBOR READINGS -------
            num_matches=0;
            for(int check_idx=0; check_idx < NNEIGHBORS; check_idx++) {
                if(neighbors[check_idx] >= 0) { // neighbor exists
                    // check if the neighbor's readings match with its readings based on pre-defined thresholds
                    // if yes add to the number of matches
                    num_matches += is_match(reading, neighbor_readings[check_idx]);
                }
            }

            // if >=2 nodes agree, prepare to send an alert
            if (num_matches >= ALERT_THRESHOLD) {
                // compiles alert information for the reporting node and its neighbors
                create_all_alerts(sensor_alerts, num_neighbors);
                // get the report time
                struct timeval tv;
                gettimeofday(&tv, NULL);
                unsigned long long millisecondsSinceEpoch = 
                    (unsigned long long)(tv.tv_sec) * 1000 +
                    (unsigned long long)(tv.tv_usec) / 1000;
                sensor_alerts[0].report_time=millisecondsSinceEpoch;
                MPI_Isend(sensor_alerts, num_neighbors+1, alert_mpi_type, BASE, ALERT_TAG, MPI_COMM_WORLD, &req);
            }
            check_neighbor_values=0;
        }

        // ------- GENERATE SENSOR READINGS -------

        // get the current time
        time(&cur_time);
        timeinfo = *localtime(&cur_time);
        reading_dt = (dateTime){
            .year=timeinfo.tm_year + YEARS_OFFSET,
            .month=timeinfo.tm_mon + MONTHS_OFFSET,
            .date=timeinfo.tm_mday,
            .hour=timeinfo.tm_hour,
            .minute=timeinfo.tm_min,
            .second=timeinfo.tm_sec
        };
        reading.latitude = randFloat(&seed, LAT_MIN, LAT_MAX);
        reading.longitude = randFloat(&seed, LONG_MIN, LONG_MAX);
        randFloatNormal(&seed, 0, MAGNITUDE_MAX, &reading.magnitude, NULL);
        randFloatNormal(&seed, 0, DEPTH_MAX, &reading.depth, NULL);
        reading.datetime = reading_dt;

        reading_ptr = &reading;

        // Finished writing to reading variable (condition variable), release lock
        pthread_mutex_unlock(&reading_mutex);


        // earthquake detected
        if (reading.magnitude > EQ_THRESH) {
            // toggle global flag to let the neighbor communciator thread know to send request signal
            request_from_neighbors=true;
        } 

        sleep(SENSOR_CYCLE);
        // check that the sensor is still alive
        check_if_alive();
    }

    // ensure that the spawned thread has finished before exiting
    pthread_join(tid, NULL);
    free(sensor_alerts);
    pthread_mutex_destroy(&reading_mutex);
    // printf("============CART RANK %d IS DONE WITH OPERATIONS=================\n", cart_rank);

    return 0;
}

void communicate_machine_details() {
    get_machine_details(machine_dets[0].ip_addr, machine_dets[0].hostname);
    MPI_Request send_requests[NNEIGHBORS], recv_requests[NNEIGHBORS];
    int complete=0;
    MPI_Status statuses[NNEIGHBORS];
    for (int j = 0; j<NNEIGHBORS; j++) {
        // non-blocking send and receive of machine details to every neighbor (if they exist)
        if (neighbors[j] >= 0) {
            MPI_Isend(&machine_dets[0], 1, machine_dets_mpi_type, neighbors[j], MACHINE_DETS_TAG, cart_comm, &send_requests[j]);
            MPI_Irecv(&machine_dets[j+1], 1, machine_dets_mpi_type, neighbors[j], MACHINE_DETS_TAG, cart_comm, &recv_requests[j]);
        } else {
            send_requests[j] = MPI_REQUEST_NULL;
            recv_requests[j] = MPI_REQUEST_NULL;
        }
    }

    // non-blocking check performed in a loop to see if the node has obtained machine details from all its neighbors
    while (alive && !complete) {
        // need to check if the node is alive so that it is not stuck in this loop if it is killed
        check_if_alive();
        MPI_Testall(NNEIGHBORS, recv_requests, &complete, statuses);
    }
}

void* communicateWithNeighbors(void* pArg) {
    MPI_Request send_requests[NNEIGHBORS], recv_requests[NNEIGHBORS];
    MPI_Status recv_statuses[NNEIGHBORS];
    if (alive) {
        // before entering while loop, send the IP address and host name of this node to all neighbors
        // and obtain the IP addresses and host names of its neighbors (both are non-blocking)
        communicate_machine_details();
    }

    // main loop for communication
    while (alive){
        if(request_from_neighbors) { // main sensor has indicated that it wants to collect neighbor readings
            // acquire lock to make sure the main sensor's readings are not overwritten 
            // while this thread is collecting neighbor readings
            pthread_mutex_lock(&reading_mutex);

            for (int i=0; i< NNEIGHBORS; i++) {
                if (neighbors[i] >= 0) { // neighbors exist
                    // non-blocking send to all neighbors using the tag to identify it as a reading request
                    MPI_Isend(&cart_rank, 1, MPI_INT, neighbors[i], TAG_READING_REQUEST, cart_comm, &send_requests[i]);
                    // set up non-blocking receive to obtain the neighbor's readings
                    MPI_Irecv(&neighbor_readings[i], 1, reading_mpi_type, neighbors[i], 0, cart_comm, &recv_requests[i]);
                } else {
                    recv_requests[i] = MPI_REQUEST_NULL;
                }
            }

            int complete = 0;

            // non-blocking check performed in a loop to see if the node has obtained sensor readings from all its neighbors
            while(!complete && alive) {
                // need to check if it is alive so it can escape this loop if it has been killed off by the base station
                check_if_alive();
                // check and fulfill incoming requests for readings so that a deadlock is not reached 
                // where all nodes are waiting for each other (expanded upon in the report)
                check_requests(cart_comm, reading_ptr, reading_mpi_type, cart_rank);
                // non-blocking check of status of completion for the receives
                MPI_Testall(NNEIGHBORS, recv_requests, &complete, recv_statuses);
            }

            // reset flag back to 0 once it has completed collecting from neighbors
            request_from_neighbors = false;

            // indicate to the main node that it has finished collecting the neighbor's values and to check it for matches
            check_neighbor_values=alive;
            pthread_mutex_unlock(&reading_mutex);
        } else if (reading_ptr != NULL) {
            // otherwise, no need to obtain values from neighbors, just check for any incoming requests and fulfill them
            // ensures that at least one reading has been generated, otherwise sensor readings will be garbage values
            check_requests(cart_comm, reading_ptr, reading_mpi_type, cart_rank);

        }
    }

    // thread has been killed off, return from function
    return NULL;
}


void check_requests(MPI_Comm cart_comm, sensorReading* reading, MPI_Datatype reading_mpi_type, int cart_rank) {
    int send=0, sender;
    MPI_Status status;
    MPI_Request send_request;

    // checks for any incoming messages with reading request tag
    MPI_Iprobe(MPI_ANY_SOURCE, TAG_READING_REQUEST, cart_comm, &send, &status);
    while(send) { // keep looping while more messages are found
        // if it has entered the loop, we *know* that there is an incoming message
        // so using blocking receive will not cause a lot of delay
        MPI_Recv(&sender, 1, MPI_INT, status.MPI_SOURCE, TAG_READING_REQUEST, cart_comm, MPI_STATUS_IGNORE);
        // once the request has been received, send back the node's sensor readings using non-blocking send
        MPI_Isend(reading, 1, reading_mpi_type, sender, 0, cart_comm, &send_request);
        send=0;
        // check for any more pending incoming messages
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_READING_REQUEST, cart_comm, &send, &status);
    }
}

int is_match(sensorReading reading1, sensorReading reading2) {
    float lat1, long1, mag1, depth1, lat2, long2, mag2, depth2;

    lat1 = reading1.latitude;
    long1 = reading1.longitude;
    mag1 = reading1.magnitude;
    depth1 = reading1.depth;

    lat2 = reading2.latitude;
    long2 = reading2.longitude;
    mag2 = reading2.magnitude;
    depth2 = reading2.depth;

    // distance (in km) between two coordinates assuming a perfect sphere
    double distance = haversine_distance(lat1, long1, lat2, long2);
    // absolute difference in magnitude
    float diff_mag = fabs(mag1-mag2);
    // absolute difference in depth
    float diff_depth = fabs(depth1 - depth2);

    return (distance < DISTANCE_MATCH_THRESH && diff_mag < MAGNITUDE_MATCH_THRESH && diff_depth < DEPTH_MATCH_THRESH);
}

void check_if_alive() {
    int msg_pending = 0;
    // checks if any incoming messages from base with the sentinel tag
    MPI_Iprobe(BASE, SENTINEL_TAG, MPI_COMM_WORLD, &msg_pending, MPI_STATUS_IGNORE);
    if (msg_pending) {
        // if yes, then receive the message and kill off the node
        MPI_Recv(&alive, 1, MPI_INT, BASE, SENTINEL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

}

// creates alert for reporting node and all neighbors
void create_all_alerts(alert* alerts, int num_neighbors) {
    alert sensor_alert;
    alerts[0] = create_alert(cart_rank, 0);
    int alert_idx = 1;
    for(int i=0; i < NNEIGHBORS; i++) {
        if(neighbors[i] >= 0) {
            sensor_alert = create_alert(neighbors[i], i+1);
            
            alerts[alert_idx]=sensor_alert;
            alert_idx++;
        }

    }
}

// creates alert for a single node
alert create_alert(int rank, int idx) {
    int coords[2];
    MPI_Cart_coords(cart_comm, rank, NDIMS, coords);
    alert sensor_alert;
    // printf("creating alert (x: %d, y: %d, rank:%d, ip: %d\n", coords[0], coords[1], neighbors[rank], )
    sensor_alert = (alert){
        .x_coord=coords[0],
        .y_coord=coords[1],
        .rank=rank, 
    };
    if(idx == 0) {
        sensor_alert.reading = *reading_ptr;
        sensor_alert.machine_dets = machine_dets[0];
    } else {
        sensor_alert.reading=neighbor_readings[idx-1];
        sensor_alert.machine_dets=machine_dets[idx];
    }
    return sensor_alert;
}

