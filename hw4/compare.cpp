#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <sstream>

using namespace std;

unordered_map<string, int> Map;

int main()
{
	int seg_size, n;
	cout << "segsize: ";
	cin >> seg_size;
	cout << "size of elements: ";
	cin >> n;

	int thread_num = n / seg_size;
	if(n % seg_size != 0) thread_num++;
	// printf("thread_num = %d\n", thread_num);
	fstream in1, in2;
	char s1[1024], s2[1024];
	printf("input file1: ");
	scanf("%s", s1);
	printf("input file2: ");
	scanf("%s", s2);

	in1.open(s1, ios::in);
	in2.open(s2, ios::in);
	if(!in1 || !in2){
		cout << "fail to open files\n";
		return 0;
	}

	cout << "Compare sorting part...\n";
	for(int i = 0; i < thread_num; i++){
		string str1, str2, str3;
		getline(in1, str1);
		getline(in1, str2);
		getline(in1, str3);
		string temp(str1);
		temp += str2;
		temp += str3;
		// cout << temp << endl;
		if(Map.find(temp) == Map.end())
			Map[temp] = 1;
		else Map[temp]++;
	}

	// for(auto& x: Map)
	// 	cout << x.first << " " << x.second << endl;
	for(int i = 0; i < thread_num; i++){
		string str1, str2, str3;
		getline(in2, str1);
		getline(in2, str2);
		getline(in2, str3);
		string temp(str1);
		temp += str2;
		temp += str3;
		if(Map.find(temp) == Map.end()){
			cout << "can't find " << temp << " in file1\n";
			return 0;
		}
		Map[temp]--;
		// cout << "Map[" << temp << "] = " << Map[temp] << endl;
		if(Map[temp] == 0) Map.erase(temp);
	}
	// printf("size = %d\n", Map.size());
	if(Map.size() != 0){
		cout << "error!\n";
		return 0;
	}
	cout << "sorting part is the same!\n";
	
	Map.clear();

	cout << "compare merging part...\n";
	while(thread_num / 2 != 0){
		int odd = 0;
		if(thread_num % 2 != 0) odd = 1;
		thread_num /= 2;
	
		for(int i = 0; i < thread_num; i++){
			string str1, str2, str3;
			getline(in1, str1);
			getline(in1, str2);
			getline(in1, str3);
			string temp(str1);
			temp += str2;
			temp += str3;
			
			if(Map.find(temp) == Map.end())
				Map[temp] = 1;
			else Map[temp]++;	
		}

		for(int i = 0; i < thread_num; i++){
			string str1, str2, str3;
			getline(in2, str1);
			getline(in2, str2);
			getline(in2, str3);
			string temp(str1);
			temp += str2;
			temp += str3;
			
			if(Map.find(temp) == Map.end()){
				cout << "can't find " << temp << " in file1\n";
				return 0; 
			}

			Map[temp]--;
			if(Map[temp] == 0) Map.erase(temp);	
		}
		
		if(Map.size() != 0){
			cout << "error\n";
			return 0;
		}

		Map.clear();		

		thread_num += odd;
	}
	
	cout << "merging part is the same!\n";	
	
	cout << "compare the result...\n";
	string str1, str2;
	getline(in1, str1);
	getline(in2, str2);
	stringstream ss1(str1), ss2(str2);
	// cout << "str1 = " << str1 << endl;
	// cout << "str2 = " << str2 << endl;
	for(int i = 0; i < n; i++){
		int a, b;
		ss1 >> a; ss2 >> b;
		if(a != b){
			cout << "the result is not the same!\n";
			return 0;
		}
	}
	
	cout << "the result is the same!\n";

	return 0;
}
