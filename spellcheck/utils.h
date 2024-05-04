#pragma once

#include <string>
#include <vector>

using namespace std;

extern int editDistance(string, string);
extern vector<string>& sortByRelevance(vector<string>&, string);
extern vector<string>& removeDuplicates(vector<string>&);
extern string maintainCase(string, string);
extern string normalize(string);