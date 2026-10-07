#pragma once

#include <bits/stdc++.h>

using namespace std;

class CRC32 {
public:
    static uint32_t calculate(const vector<uint8_t>& data);    
};