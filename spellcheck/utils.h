#pragma once

#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <QColor>

using namespace std;

int editDistance(string, string);
vector<string>& sortByRelevance(vector<string>&, string);
vector<string>& removeDuplicates(vector<string>&);
string maintainCase(string, string);
string normalize(string);
string getFileName(string, bool = true);
vector<string> split(string, char);
bool fileExists(string);
int keepBetween(int, int, int);
double keepBetween(double, double, double);
QColor interpolateColors(QColor, QColor, double);