#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "constants.h"
#include "helpers.h"


// generates random scale from 0 to 1
double randScale(unsigned int* seed) {
    return (double)rand_r(seed) / (double)RAND_MAX;
}

// generates random float between upper and lower bound
float randFloat(unsigned int* seed, float lbound, float ubound){
    double rand_scale = randScale(seed);
    float range = ubound-lbound;
    return (float)(rand_scale * range + lbound);
}


/**
 * @brief Generates floating point values in a given range from the normal distribution with mean (ubound-lbound)/2.
 * @param seed unsigned int* which is the seed for the random number generation
 * @param lbound float lower bound
 * @param ubound float upper bound
 * @param returnTwoVals bool whether to return two values or only one
 * @param float1 float* pointer to store the first generated float
 * @param float2 float* pointer to store the second generated float. Set to NULL if only one float is required
 * 
 * reference: https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
 */
void randFloatNormal(unsigned int* seed, float lbound, float ubound, float* float1, float* float2) {
    double rand_scale_1 = randScale(seed);
    double rand_scale_2 = randScale(seed);

    double bm_r = sqrt(-2.0 * log(rand_scale_1));
    double bm_theta = 2.0 * PI * rand_scale_2;

    double rand_scale_norm_1 = (float)(sigmoid(bm_r * cos(bm_theta)));

    float range = ubound-lbound;
    *float1 = (float)(rand_scale_norm_1 * range + lbound);

    if (float2) {
        double rand_scale_norm_2 = (float)(sigmoid(bm_r * sin(bm_theta) + 1.0));
        *float2 = (float)(rand_scale_norm_2 * range + lbound);
    }
}
\
double sigmoid(double x) {
    return 1/(1 + exp(-x));
}

/**
 * @brief Calculates the distance between two (lat, long) coordinates (on a perfect sphere)
 * @param lat1 float latitude of first coordinate
 * @param long1 float longitude of first coordinate
 * @param lat2 float latitude of second coordinate
 * @param long2 float longitude of second coordinate
 * 
 * reference: https://en.wikipedia.org/wiki/Haversine_formula
 */
double haversine_distance(float lat1, float long1, float lat2, float long2) {
    double dLat = deg2rad(lat2-lat1);  // deg2rad below
    double dLon = deg2rad(long2-long1); 
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * sin(dLon/2) * sin(dLon/2);
    double c = asin(sqrt(a));
    double d = 2 * R * c;
    return d;
}

double deg2rad(float deg) {
  return deg * (PI/180.0);
}

// gets the machine's IP address and the hostname
// reference: https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
void get_machine_details(char ip_addr[IP_ADDR_LEN], char host_buff[HOST_LEN]) {
    char* ip_buffer;
    int success;
    struct hostent *host_entry;
    success= gethostname(host_buff, HOST_LEN);
    if (success < 0) {
        printf("ERROR: CANNOT GET HOST NAME\n");
        exit(success);
    }
    host_entry = gethostbyname(host_buff);
    ip_buffer = inet_ntoa(*((struct in_addr*)
						host_entry->h_addr_list[0]));
    strcpy(ip_addr, ip_buffer);

}

//https://stackoverflow.com/a/19288271
int int_mod(int a, int b) {
    int r = a%b;
    return r < 0 ? r + b : r;
}