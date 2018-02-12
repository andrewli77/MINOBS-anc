#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <map>

using namespace std;

int main() {
	int n = 8;
	map<string, int> mapping;

	vector<int> graph[n];


	ifstream file("asia.net");

	string throwaway;

	for (int i = 0; i < n; i++) {
		string s;
		getline(file, s);
		s = s.substr(5, s.size() - 5);

		if (s[s.size() - 1] == ' ') {
			s = s.substr(0, s.size() - 1);
		}

		mapping[s] = i;

		cout << "Mapping: " << s << " to: " << i << endl;

		getline(file, throwaway);
		getline(file, throwaway);
		getline(file, throwaway);
	}


	for (int i = 0; i < n; i++) {
		string s;
		getline(file, s);

		s =  s.substr()
	}
}