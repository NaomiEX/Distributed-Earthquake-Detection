#ifndef HELPERS_H
#define HELPERS_H

double randScale(unsigned int* seed);
float randFloat(unsigned int* seed, float lbound, float ubound);
void randFloatNormal(unsigned int* seed, float lbound, float ubound, float* float1, float* float2);
double sigmoid(double x);
double deg2rad(float deg);
double haversine_distance(float lat1, float long1, float lat2, float long2);
void get_machine_details(char ip_addr[IP_ADDR_LEN], char host_buff[HOST_LEN]);
int int_mod(int a, int b);

#endif