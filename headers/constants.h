#ifndef CONSTANTS_H
#define CONSTANTS_H

// datetime constants
#define YEARS_OFFSET 1900 // C records the years from 1900
#define MONTHS_OFFSET 1 // tm records month with range [0, 11]

// distributed constants
#define BASE 0
#define SENSOR_CYCLE 2
#define BASE_CYCLE 1
#define SENTINEL_TAG 999
#define MACHINE_DETS_TAG 4
#define ALERT_TAG 10

#define NUM_THREAD 1

// cartesian topology constants
#define NDIMS 2
#define ROWDIR 0
#define COLDIR 1
#define DISPLACEMENT 1
#define NNEIGHBORS 4
#define TAG_READING_REQUEST 1

// geographical constants
#define LAT_MIN -14
#define LAT_MAX -16
#define LONG_MIN 167
#define LONG_MAX 169

// earthquake constants
#define DEPTH_MAX 700
#define MAGNITUDE_MAX 10
#define EQ_THRESH 2.5
#define DISTANCE_MATCH_THRESH 100 // larger threshold for testing only
#define MAGNITUDE_MATCH_THRESH 2 // larger threshold for testing only
#define DEPTH_MATCH_THRESH 100
#define READING_NVALS 5
#define DATETIME_NVALS 6

// math constants
#define PI 22.0/7.0
#define R 6371 // radius of the earth (in km)

// log constants
#define COLWIDTH_SMALL 10
#define COLWIDTH_MEDIUM 15
#define COLWIDTH_LARGE 20

// alert constants
#define ALERT_NVALS 6
#define ALERT_THRESHOLD 2

// machine details constants
#define MACHINE_DETS_NVALS 2
#define IP_ADDR_LEN 16 // 15 + 1 for terminating char
#define HOST_LEN 256

#endif
