#pragma once
#include "mfcc.h"
#include <vector>
#include <string>
#include <map>

class GenderRecognizer {
public:
    void addExample(const std::string& label, const MFCC& mfcc);
    std::string recognize(const MFCC& mfcc, int k);

private:
    std::map<std::string, std::vector<MFCC>> db;
    float distance(const MFCC& a, const MFCC& b);
};
