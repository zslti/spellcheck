﻿#include "utils.h"
#include <chrono>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>
#include <QDebug>

using namespace std;

typedef chrono::time_point<chrono::high_resolution_clock> TimePoint;
typedef chrono::duration<double> Duration;

map<string, char> diacritics = {
	{"ü", 'u'}, {"ó", 'o'}, {"ő", 'o'}, {"ú", 'u'}, {"ű", 'u'}, {"é", 'e'}, {"á", 'a'}, 
    {"ă", 'a'}, {"î", 'i'}, {"â", 'a'}, {"ș", 's'}, {"ț", 't'},
};

string normalize(string str) {
    for(int i = 0; i < str.length(); i++) {
        if(diacritics.find(str.substr(i, 2)) != diacritics.end()) {
        	str[i] = diacritics[str.substr(i, 2)];
        	str.erase(i + 1, 1);
        }
    }
	return str.c_str();
}

vector<string>& removeDuplicates(vector<string> &arr) {
	sort(arr.begin(), arr.end());
	arr.erase(unique(arr.begin(), arr.end()), arr.end());
	return arr;
}

int editDistance(string str1, string str2) {
    static map<pair<string, string>, int> memo;
    if(memo.find({str1, str2}) != memo.end()) return memo[{str1, str2}];

    string normalizedStr1 = normalize(str1).c_str();
    string normalizedStr2 = normalize(str2).c_str();

	int n = normalizedStr1.length();
	int m = normalizedStr2.length();

	if(n == 0 || m == 0) return max(n, m);
    vector<vector<int>> dp(n + 1, vector<int>(m + 1));
    for(int i = 0; i <= n; i++) {
    	for(int j = 0; j <= m; j++) {
            if(i == 0) dp[i][j] = j;
            else if(j == 0) dp[i][j] = i;
            else if(normalizedStr1[i - 1] == normalizedStr2[j - 1]) dp[i][j] = dp[i - 1][j - 1];
            else dp[i][j] = 1 + min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
        }
    }
    memo[{str1, str2}] = dp[n][m];

    //qDebug() << "edit distance of: " << normalizedStr1 << " and " << normalizedStr2 << " is " << dp[n][m];
    return dp[n][m];
}

vector<string>& sortByRelevance(vector<string> &arr, string key) {
    TimePoint start = chrono::high_resolution_clock::now();

	sort(arr.begin(), arr.end(), [key](string a, string b) {
        bool aStartsWithKey = a.find(key) == 0;
        bool bStartsWithKey = b.find(key) == 0;

        // ha mindkettő a kulccsal kezdődik, akkor edit distance alapján rendezünk
        if(aStartsWithKey && bStartsWithKey) return editDistance(a, key) < editDistance(b, key);

        // ha az egyik a kulccsal kezdődik, a másik nem, akkor az elsőt rakjuk előrébb
        if(aStartsWithKey && !bStartsWithKey) return true;
        if(!aStartsWithKey && bStartsWithKey) return false;

        // ha az edit distance megegyezik, akkor a hosszabb szót rakjuk előrébb 
        // (pl. kulcsszó: elégg, akkor az eléggé legyen hamarabb mint elég)
        if(editDistance(a, key) == editDistance(b, key)) return a.length() > b.length();

        // különben edit distance alapján rendezünk
        return editDistance(a, key) < editDistance(b, key);
    });

    Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
	//qDebug() << "sort by relevance took" << elapsedSeconds.count() << " seconds (" << arr.size() << "elements )";
    return arr;
}

string maintainCase(string str, string caseToMaintain) {
    while(caseToMaintain.length() < str.length()) {
     	caseToMaintain += caseToMaintain[caseToMaintain.length()];
    }
    if(caseToMaintain.length() > str.length()) {
    	caseToMaintain = caseToMaintain.substr(0, str.length());
    }

	for(int i = 0; i < caseToMaintain.length(); i++) {
    	str[i] = isupper(caseToMaintain[i]) ? toupper(str[i]) : tolower(str[i]);
    }
	return str;
}