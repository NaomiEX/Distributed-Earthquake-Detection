#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#include <sys/time.h>
#include "constants.h"
#include "helpers.h"
#include "sensors.h"
#include "structures.h"
#include "base.h"

//Create a global array to store balloon seismic sensor readings
sensorReading *global_arr = NULL;


MPI_Datatype sensor_dt_mpi_type, reading_mpi_type_b, machine_det_type, alert_type;

//Mutex lock for POSIX thread
pthread_mutex_t gMutex; 

bool termination_signal = false;

int size; // Number of MPI processes
int true_alert = 0;
int false_alert = 0;
double total_comm_time = 0;
int message_num = 0;
int idx=0, first=0, valid_idx_up_to=0;



int max_iterations = 0;

int base_station_main(MPI_Comm world_comm, MPI_Comm comm, int *dims, int size, int max_iterations, int sentinel, int caas){
    size = size; // total number of nodes
    max_iterations = max_iterations;

    pthread_t tid[NUM_THREAD];

    
    int global_arr_size = dims[0] * dims[1];
    
    global_arr = (sensorReading *)malloc(global_arr_size * sizeof(sensorReading));

    create_all_mpi_types(&sensor_dt_mpi_type, &reading_mpi_type_b, &machine_det_type, &alert_type);

    // create the file if it does not exist
    FILE* fPtr = fopen("rec_sentinel.txt", "w");
    fclose(fPtr);

    // Initialise the mutex lock for the global array
    pthread_mutex_init(&gMutex, NULL);

    // Create the balloon seismic sensor
    pthread_create(&tid[0], 0, simulate_balloon_func, &global_arr_size);

    int terminate = 0;  //terminate flag

    char filename[256];
    sprintf(filename,"logs(%d,%d).txt",dims[0],dims[1]);
    FILE *log_file = fopen(filename, "w"); // Create log file

    // time_t time_init = time(NULL) ;
    printf("Start logging\n");
    int iterations = base_aux(world_comm, comm, log_file, global_arr_size, max_iterations, sentinel, caas);

    // if reached here, the user has entered the sentinel value/reached max. iterations

    // Send terminating message to all sensor nodes
    int rank;
    MPI_Request *request = (MPI_Request*)(malloc(sizeof(MPI_Request) * size-1));
    MPI_Status *status = (MPI_Status*)(malloc(sizeof(MPI_Status) *size-1));
    for (rank = 1; rank < size; rank++)
    {
        MPI_Isend(&terminate, 1, MPI_INT, rank, SENTINEL_TAG, world_comm, &request[rank-1]);
    }

    // ensure all kill signals have been sent before proceeding
    MPI_Waitall(size-1, request, status);

    termination_signal = true; //terminate balloon seismic sensor

    // wait until the thread terminates before proceeding
    pthread_join(tid[0], NULL); 

    // Print summary to log file
    summary(log_file, iterations, true_alert, false_alert, total_comm_time, message_num);


    // Close file
    fclose(log_file);
    printf("Terminate base station.\n");
    free(request);
    free(status);
    return 0;
    
}

//simulate balloon function
void* simulate_balloon_func(void *pArg){
    int* p = (int*)pArg;
    int arr_size = *p; //Derefences the pointer

    time_t cur_time;
    struct tm timeinfo;
    sensorReading new_reading; //Create a reading
    dateTime new_reading_dt;
    unsigned int seed = time(NULL);

    do{
        sleep(BASE_CYCLE);
		pthread_mutex_lock(&gMutex); //Lock global array
        
        // generate seismic reading detection date/time
        time(&cur_time);
        timeinfo = *localtime(&cur_time);
        new_reading_dt = (dateTime){
            .year=timeinfo.tm_year + YEARS_OFFSET,
            .month=timeinfo.tm_mon + MONTHS_OFFSET,
            .date=timeinfo.tm_mday,
            .hour=timeinfo.tm_hour,
            .minute=timeinfo.tm_min,
            .second=timeinfo.tm_sec
        };

        // generate seismic readings
        new_reading.latitude = randFloat(&seed, LAT_MIN, LAT_MAX);
        new_reading.longitude = randFloat(&seed, LONG_MIN, LONG_MAX);
        randFloatNormal(&seed, 0, MAGNITUDE_MAX, &new_reading.magnitude, NULL);
        randFloatNormal(&seed, 0, DEPTH_MAX, &new_reading.depth, NULL);
        new_reading.datetime = new_reading_dt;

        //Update the global array
        global_arr[idx] = new_reading;
        idx = (idx+1)%arr_size; // circularly loops
        first=1; // flag to see whether there is at least one value in the array
        // keep the index up to which is stored valid readings (beyond it may be garbage values)
        valid_idx_up_to = valid_idx_up_to + 1 > arr_size ? arr_size : valid_idx_up_to + 1;

		pthread_mutex_unlock(&gMutex);
	}while(!termination_signal);
	
	return 0;
}

//Log report structure
void log_report(alert* raised_alert, sensorReading balloon_seismic_reading, int iteration, bool matched, float comm_time, FILE *log_file,  int alert_count)
{   
    //Declare variables   
    int i;
    const char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    // Get day of the week
    time_t cur_time = time(NULL);
    struct tm timeinfo = *localtime(&cur_time);
    int day = timeinfo.tm_wday;
  

    fprintf(log_file, "-----------------------------------------------------------------------------------------\n");
    fprintf(log_file, "Iteration: %d\n", iteration);
    fprintf(log_file, "Logged time:%*s %04d-%02d-%02d %02d:%02d:%02d\n", COLWIDTH_LARGE, days[day], timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    fprintf(log_file, "Alert reported time:  %*s %04d-%02d-%02d %02d:%02d:%02d\n", COLWIDTH_SMALL, days[day], raised_alert[0].reading.datetime.year, raised_alert[0].reading.datetime.month, raised_alert[0].reading.datetime.date, raised_alert[0].reading.datetime.hour, raised_alert[0].reading.datetime.minute, raised_alert[0].reading.datetime.second);

    if (matched)
    {
        fprintf(log_file, "Alert type: Conclusive\n\n");
        true_alert++;
    }
    else
    {
        fprintf(log_file, "Alert type: Inconclusive\n\n");
        false_alert++;
    }

    // header for reporting node
    fprintf(log_file, "%-*s%-*s%*s%*s\n", 
        COLWIDTH_LARGE, "Reporting Node", 
        COLWIDTH_LARGE, "Seismic Coord",
        COLWIDTH_LARGE+COLWIDTH_SMALL, "Magnitude", 
        COLWIDTH_LARGE+COLWIDTH_MEDIUM, "IPv4");

    // logging reporting node's sensor readings
    fprintf(log_file, "%d(%d,%d)%*s%.2f,%.2f)%*.2f%*s(%s)\n", 
                    raised_alert[0].rank, raised_alert[0].x_coord, raised_alert[0].y_coord,
                    COLWIDTH_MEDIUM, "(", raised_alert[0].reading.latitude,raised_alert[0].reading.longitude,
                    COLWIDTH_SMALL+COLWIDTH_LARGE, raised_alert[0].reading.magnitude,
                    COLWIDTH_MEDIUM + COLWIDTH_SMALL + COLWIDTH_LARGE, raised_alert[0].machine_dets.ip_addr, raised_alert[0].machine_dets.hostname);
    
    // header for neighboring nodes
    fprintf(log_file, "%-*s%-*s%-*s%-*s%-*s%-*s\n", 
                COLWIDTH_LARGE, "Adjacent Node", 
                COLWIDTH_LARGE, "Seismic Coord",
                COLWIDTH_LARGE, "Diff(Coord, km)", 
                COLWIDTH_LARGE, "Magnitude",
                COLWIDTH_LARGE, "Diff(Mag)", 
                COLWIDTH_LARGE, "IPv4");


    // logging report for neighboring nodes
    for (i = 1; i < alert_count; i++)
    {
            fprintf(log_file, "%d(%d,%d)%*s%.2f,%.2f)%*lf%*.2f%*lf%*s(%s)\n", 
                    raised_alert[i].rank, raised_alert[i].x_coord, raised_alert[i].y_coord,
                    COLWIDTH_MEDIUM, "(", raised_alert[i].reading.latitude,raised_alert[i].reading.longitude,
                    COLWIDTH_MEDIUM, haversine_distance(raised_alert[0].reading.latitude, raised_alert[0].reading.longitude, raised_alert[i].reading.latitude, raised_alert[i].reading.longitude),
                    COLWIDTH_MEDIUM, raised_alert[i].reading.magnitude,
                    COLWIDTH_MEDIUM + COLWIDTH_SMALL, fabs(raised_alert[0].reading.magnitude - raised_alert[i].reading.magnitude),
                    COLWIDTH_LARGE, raised_alert[i].machine_dets.ip_addr, raised_alert[i].machine_dets.hostname);        
    }

    // ballon seismic sensor reading logs
    fprintf(log_file, "\nBalloon seismic reporting time:\t%s %04d-%02d-%02d %02d:%02d:%02d\n", 
        days[day], balloon_seismic_reading.datetime.year, balloon_seismic_reading.datetime.month, balloon_seismic_reading.datetime.date, balloon_seismic_reading.datetime.hour, balloon_seismic_reading.datetime.minute, balloon_seismic_reading.datetime.second);
    fprintf(log_file, "Balloon seismic reporting Coord: (%.2f,%.2f)\n", balloon_seismic_reading.latitude, balloon_seismic_reading.longitude);
    fprintf(log_file, "Balloon seismic reporting Coord Diff. with Reporting Node (km): %.2f\n", haversine_distance(raised_alert[0].reading.latitude, raised_alert[0].reading.longitude, balloon_seismic_reading.latitude, balloon_seismic_reading.longitude));
    fprintf(log_file, "Balloon seismic reporting Magnitude: %.2f\n", balloon_seismic_reading.magnitude);
    fprintf(log_file, "Balloon seismic reporting Magnitude Diff. with Reporting Node (km): %.2f\n\n", fabs(raised_alert[0].reading.magnitude - balloon_seismic_reading.magnitude));

    // key metrics and thresholds
    fprintf(log_file, "Communication time (seconds): %.3f\n", comm_time);
    total_comm_time += comm_time;
    fprintf(log_file, "Total Messages send between reporting node and base station: 1\n" ); 
    fprintf(log_file, "Number of adjacent matches to reporting node: %d\n", alert_count-1); 
    fprintf(log_file, "Coordinate difference threshold (km):  %d\n", DISTANCE_MATCH_THRESH);
    fprintf(log_file, "Magnitude difference threshold: %d\n", MAGNITUDE_MATCH_THRESH);
    fprintf(log_file, "Earthquake magnitude threshold: %.2f\n", EQ_THRESH);
    fprintf(log_file, "-----------------------------------------------------------------------------------------\n");

    printf("Log reported\n");
}

//Log report summary 
void summary(FILE *log_file, int iterations, int true_alert, int false_alert, float total_comm_time, int message_num){
    fprintf(log_file, "-----------------------------------------------------------------------------------------\n");
    fprintf(log_file, "Number of messages passed throughout the network when an alert is detected: %d\n", true_alert + false_alert); 
    fprintf(log_file, "Total Iterations: %d\n", iterations);
    fprintf(log_file, "True Alerts: %d\n", true_alert);
    fprintf(log_file, "False Alerts: %d\n", false_alert);
    fprintf(log_file, "Total communication time (s): %lf\n", total_comm_time);
}

// Performs the task of receiving reports from sensor nodes and determining the match type in the given time duration
int base_aux(MPI_Comm world_comm, MPI_Comm comm, FILE *log_file, int global_arr_size, int max_iterations, int sentinel, int caas)
{
    MPI_Status status;
    bool true_match = false;
    int iterations = 0, alert_incoming, alert_count;  // Keeps track of the number of iterations
    alert raised_alert[NNEIGHBORS+1];
    bool termination = false;
    int input_sentinel;
    
    
    while (!termination && iterations < max_iterations) // Stop when user has input sentinel value or exceeded the time and max iterations
    {
        alert_incoming=0;

        // check if any incoming messages to base with the alert tag
        MPI_Iprobe(MPI_ANY_SOURCE, ALERT_TAG, MPI_COMM_WORLD, &alert_incoming, &status);

        // If there is an incoming alert from a node, receive it
        if(alert_incoming) {
            
            // gets the size of the message
            // this is required because different nodes have different number of neighbors
            // thus they send different number of node alert information (one for each neighbor+itself)
            MPI_Get_count(&status, alert_type, &alert_count);
            // raised_alert = (alert*)(malloc(sizeof(alert) * alert_count)); // inefficient

            // receive the alert message
            MPI_Recv(raised_alert, alert_count, alert_type, status.MPI_SOURCE, ALERT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            alert_incoming=0;
            
            // reference: https://stackoverflow.com/a/3756954
            unsigned long long report_time_nsecs = raised_alert[0].report_time;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            unsigned long long millisecondsSinceEpoch = 
                (unsigned long long)(tv.tv_sec) * 1000 +
                (unsigned long long)(tv.tv_usec) / 1000;

            double comm_time = ((double)(millisecondsSinceEpoch - report_time_nsecs)) /1000;

            // ensure that at least one balloon reading has been generated
            while (!first) {}
            // Check if there is a match
            // Lock global array so other processes cannot access it
            pthread_mutex_lock(&gMutex);

            int i;
            // to increase probability of a match check every populated value in the global array
            for (i = 0; i < valid_idx_up_to; i++)
            {
                float lat1 = global_arr[i].latitude;
                float long1 = global_arr[i].longitude;
                float mag1 = global_arr[i].magnitude;
                float depth1 = global_arr[i].depth;
                float lat2 = raised_alert[0].reading.latitude;
                float long2 = raised_alert[0].reading.longitude;
                float mag2 = raised_alert[0].reading.magnitude;
                float depth2 = raised_alert[0].reading.depth;

                // match balloon readings with seafloor sensor readings
                int match = matched(lat1, long1, mag1, depth1, lat2, long2, mag2, depth2);
                if (match)
                {
                    log_report(raised_alert, global_arr[i], iterations, true, comm_time, log_file, alert_count);
                    message_num++;
                    true_match = true;
                    break;
                }
            }

            if (!true_match) // If there is no true match then log a mismatch
            {
                int g_i = int_mod(idx-1, global_arr_size);

                log_report(raised_alert, global_arr[g_i], iterations, false, comm_time, log_file, alert_count);
                
            }
            // Release lock of global array
            pthread_mutex_unlock(&gMutex);
        }

        if (caas == 0){
            FILE *rec_sentinel = fopen("rec_sentinel.txt", "r");
            // Check file for sentinel value
            fscanf(rec_sentinel, "%d", &input_sentinel);
            fclose(rec_sentinel);
            
            if (sentinel == input_sentinel)
            {
                printf("Sentinel value specified, terminate the program.\n");
                sleep(SENSOR_CYCLE);
                termination = true;
            }
            
            else{
                sleep(BASE_CYCLE);
            }
       } else { // within caas, no need to check sentinel file
            sleep(SENSOR_CYCLE);
       }
        iterations++;
    }
    
    return iterations;
}


// check if the two readings match
bool matched(float lat1, float long1, float mag1, float depth1, float lat2, float long2, float mag2, float depth2) {

    double distance = haversine_distance(lat1, long1, lat2, long2);
    float diff_mag = fabs(mag1-mag2);
    float diff_depth = fabs(depth1 - depth2);
    return (distance < DISTANCE_MATCH_THRESH && diff_mag < MAGNITUDE_MATCH_THRESH && diff_depth < DEPTH_MATCH_THRESH);
}