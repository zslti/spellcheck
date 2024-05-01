#pragma once

#include <string>
#include <vector>

using namespace std;

extern int editDistance(string, string);
extern vector<string>& sortByEditDistance(vector<string>&, string);
extern vector<string>& removeDuplicates(vector<string>&);
extern string maintainCase(string, string);