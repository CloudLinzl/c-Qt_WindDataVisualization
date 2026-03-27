#include "../inc/loader.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <limits>

using namespace std;

static inline string trim_copy(const string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static inline double safe_stod(const string &s)
{
    string t = trim_copy(s);
    if (t.empty())
        return numeric_limits<double>::quiet_NaN();
    try
    {
        return stod(t);
    }
    catch (const std::exception &)
    {
        return numeric_limits<double>::quiet_NaN();
    }
}

// 标准化时间格式：将 "20251118 13:01:15" 或 "2025-11-18 13:01:22" 统一转换为 "2025-11-18 13:01:15"
static inline string normalize_time(const string &time_str)
{
    string t = trim_copy(time_str);
    if (t.empty())
        return t;

    // 如果已经是标准格式 (包含'-')，直接返回
    if (t.find('-') != string::npos)
    {
        return t;
    }

    // 如果是紧凑格式 "20251118 13:01:15"，需要转换为 "2025-11-18 13:01:15"
    if (t.length() >= 8 && isdigit(t[0]))
    {
        // 检查前8位是否都是数字
        bool is_compact = true;
        for (int i = 0; i < 8 && i < t.length(); ++i)
        {
            if (!isdigit(t[i]))
            {
                is_compact = false;
                break;
            }
        }

        if (is_compact && t.length() >= 8)
        {
            // 格式: YYYYMMDD ...
            string year = t.substr(0, 4);
            string month = t.substr(4, 2);
            string day = t.substr(6, 2);
            string rest = (t.length() > 8) ? t.substr(8) : "";
            return year + "-" + month + "-" + day + rest;
        }
    }

    return t;
}

vector<string> split(string str, char spliter = ',')
{
    vector<string> line;
    string s;
    istringstream iss(str);
    if (str.find(spliter) == string::npos)
    {
        cout << "csv find no spliter" << endl;
        throw "find no spliter";
    }
    while (getline(iss, s, spliter))
    {
        line.push_back(s);
    }
    return line;
}

WIND_SPEED::WIND_SPEED(string data_path)
{
    ifstream csv(data_path);
    string line;
    if (!csv.is_open())
    {
        cout << "csv file not open" << endl;
        throw "cannot csv file open";
    }
    if (getline(csv, line))
    {
        header = split(line);
    }
    // 填充距离门（按成对列：风速、SNR）
    for (size_t i = 1; i < header.size(); i += 2)
    {
        const string &h = header[i];
        size_t mpos = h.find('m');
        if (mpos != string::npos)
        {
            // 取到'm'之前的数字
            size_t start = h.find_last_of(" ,");
            string num = h.substr(0, mpos);
            // 去掉可能的非数字
            string digits;
            for (char c : num)
            {
                if (isdigit(static_cast<unsigned char>(c)))
                    digits.push_back(c);
            }
            if (!digits.empty())
            {
                try
                {
                    distance.push_back(stoi(digits));
                }
                catch (...)
                { /* ignore */
                }
            }
        }
    }
    while (getline(csv, line))
    {
        vector<string> str = split(line);
        time.push_back(str[0]);
        vector<double> speed_row;
        vector<double> snr_row;
        for (size_t i = 1; i < str.size(); i += 2)
        {
            speed_row.push_back(safe_stod(str[i]));
        }
        for (size_t i = 2; i < str.size(); i += 2)
        {
            snr_row.push_back(safe_stod(str[i]));
        }
        speed.push_back(speed_row);
        snr.push_back(snr_row);
    }
}

GIMBAL::GIMBAL(string data_path)
{
    ifstream csv(data_path);
    string line;
    if (!csv.is_open())
    {
        cout << "csv file not open" << endl;
        throw "cannot csv file open";
    }
    if (getline(csv, line))
    {
        // skip header
    }
    while (getline(csv, line))
    {
        vector<string> str = split(line);
        time.push_back(str[0]);
        if (str.size() > 1)
            azi.push_back(safe_stod(str[1]));
        if (str.size() > 2)
            ele.push_back(safe_stod(str[2]));
    }
}

void xlsx_to_csv(string data_path)
{
    string command = "python3 xlsx_to_csv.py \"" + data_path + "\"";
    int ret = system(command.c_str());
    if (ret != 0)
    {
        cout << "xlsx_to_csv.py run failed" << endl;
        throw "xlsx_to_csv.py run failed";
    }
    cout << "xlsx_to_csv.py run success" << endl;
}

DATA_TABLE::DATA_TABLE(string wind_speed_path, string gimbal_path) : size(0)
{
    int i = 0, j = 0;
    WIND_SPEED wind_speed(wind_speed_path);
    GIMBAL gimbal(gimbal_path);
    // 使用风速解析得到的距离门
    distance = wind_speed.distance;

    // 调试信息
    cout << "风速数据时间点数: " << wind_speed.time.size() << endl;
    cout << "云台数据时间点数: " << gimbal.time.size() << endl;
    if (wind_speed.time.size() > 0) {
        cout << "风速首条时间: " << wind_speed.time[0] << " -> " << normalize_time(wind_speed.time[0]) << endl;
    }
    if (gimbal.time.size() > 0) {
        cout << "云台首条时间: " << gimbal.time[0] << " -> " << normalize_time(gimbal.time[0]) << endl;
    }

    while (i < wind_speed.time.size() && j < gimbal.time.size())
    {
        string normalized_wind_time = normalize_time(wind_speed.time[i]);
        string normalized_gimbal_time = normalize_time(gimbal.time[j]);

        if (normalized_wind_time == normalized_gimbal_time)
        {
            time.push_back(normalized_wind_time);
            speed.push_back(wind_speed.speed[i]);
            snr.push_back(wind_speed.snr[i]);
            azi.push_back(gimbal.azi[j]);
            ele.push_back(gimbal.ele[j]);
            i++;
            j++;
            size++;
        }
        else if (normalized_wind_time < normalized_gimbal_time)
        {
            i++;
        }
        else
        {
            j++;
        }
    }

    cout << "匹配到的数据点数: " << size << endl;
}

void DATA_TABLE::pre_process(int slid_window_size, int threshold, double mask)
{
    find_and_repair_outlier(slid_window_size, threshold);
    SNR_mask(mask);
}

void DATA_TABLE::find_and_repair_outlier(int slid_window_size, int threshold)
{
    for (int j = 0; j < speed.size(); j++)
    {
        vector<double> data = speed[j];
        if (data.size() < 3)
        {
            speed[j] = data;
            continue;
        }
        for (size_t i = 1; i + 1 < data.size(); ++i)
        {
            double local = (data[i - 1] + data[i + 1]) / 2.0;
            if (std::abs(data[i] - local) > threshold)
            {
                data[i] = local;
            }
        }
        speed[j] = data;
    }
}

void DATA_TABLE::SNR_mask(double mask)
{
    vector<int> mask_pre;
    for (int j = 0; j < snr.size(); j++)
    {
        int pos = -1;
        for (int i = 0; i < snr[j].size(); i++)
        {
            if (snr[j][i] < mask)
            {
                pos = i;
                break;
            }
        }
        mask_pre.push_back(pos);
    }
    mask_table = mask_pre;
}

void DATA_TABLE::to_csv(const string &output_path) const
{
    ofstream out(output_path);
    if (!out.is_open())
    {
        throw "cannot open output csv";
    }
    // Add BOM for UTF-8
    out << "\xEF\xBB\xBF";
    out << "time,azi,ele";
    for (size_t i = 0; i < distance.size(); ++i)
    {
        out << "," << distance[i] << "m speed";
        out << "," << distance[i] << "m SNR";
    }
    out << "\n";
    size_t rows = time.size();
    for (size_t r = 0; r < rows; ++r)
    {
        out << time[r];
        if (r < azi.size())
            out << "," << azi[r];
        else
            out << ",";
        if (r < ele.size())
            out << "," << ele[r];
        else
            out << ",";
        for (size_t c = 0; c < distance.size(); ++c)
        {
            double v1 = (r < speed.size() && c < speed[r].size()) ? speed[r][c] : numeric_limits<double>::quiet_NaN();
            double v2 = (r < snr.size() && c < snr[r].size()) ? snr[r][c] : numeric_limits<double>::quiet_NaN();
            out << "," << v1 << "," << v2;
        }
        out << "\n";
    }
}
