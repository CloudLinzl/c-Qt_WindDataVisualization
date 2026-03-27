#ifndef LOADER_HPP
#define LOADER_HPP
#include <vector>
#include <string>
using namespace std;

class WIND_SPEED{
public:
    vector<string> header;
    vector<string> time;
    vector<int> distance; 
    vector<vector<double>> speed;
    vector<vector<double>> snr;

    WIND_SPEED(string data_path);
};

class GIMBAL{
public:
    vector<string> time;
    vector<double> azi;
    vector<double> ele;

    GIMBAL(string data_path);
};


class DATA_TABLE{
public:
    int size;
    vector<string> time;
    vector<int> distance;
    vector<vector<double>> speed;
    vector<vector<double>> snr;
    vector<double> azi;
    vector<double> ele;
    vector<int> mask_table;

    DATA_TABLE(string wind_speed_path, string gimbal_path);
    void pre_process(int slid_window_size, int threshold, double mask);
    void find_and_repair_outlier(int slid_window_size, int threshold);
    void SNR_mask(double mask);
    void to_csv(const string& output_path) const;

};

DATA_TABLE load_data(string wind_speed_path, string gimbal_path);

#endif
