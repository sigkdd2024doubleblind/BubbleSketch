#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <set>
#include <map>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include "BOBHASH32.h"
#include "params.h"
#include "BaseSketch.h"
#include "ssummary.h"
#include "heavykeeper.h"
#include "spacesaving.h"
#include "CuckooCounter.h"
#include "BubbleSketch.h"
#include "CMSketch.h"
#include "Uss.h"
#include "WavingSketch.h"
#include "DASketch.h"

using namespace std;
map <string ,int> B,C;
struct node {string x;int y;} p[32000005];
//ifstream fin("../../real_data/1.dat",ios::in|ios::binary);
string s[32000005];
char tmp[105];
int cmp(node i,node j) {return i.y>j.y;}

std::vector<std::string> func_names;

std::map<std::string, int> AAE;
std::map<std::string, double> ARE;
std::map<std::string, int> _sum;
std::map<std::string, double> _throughput;
std::map<std::string, double> insert_throughput;
std::map<std::string, double> query_throughput;
std::vector<sketch::BaseSketch*> func;

void writeResultToCSV(const string ruleName, const std::string &fileName, int mem, int K) {
    ofstream in(fileName, ios::app);
    if (in.is_open()) {
        for (int i = 0; i < func_names.size(); i++) {
            in << 
                mem << "," << 
                func_names[i] << "," << 
                K << "," <<
                AAE[func_names[i]] / (K + 0.0) << "," << 
                ARE[func_names[i]] / (K + 0.0) << "," << 
                _sum[func_names[i]] << "/" << K << "," <<
               // _throughput[func_names[i]] << "," << endl;
                insert_throughput[func_names[i]] << "," <<
                query_throughput[func_names[i]] << "," << endl;
       }
        in.close();
        

    } else {
        cout << "Unable to open file" << endl;
    }
}

int main(int argc, char** argv)
{
    int MEM=1000,K=1000;
    int c;
    char dataset[40]={'\0'};
    while((c=getopt(argc, argv, "d:m:k:"))!=-1) {
        switch(c) {
            case 'd':
                strcpy(dataset,optarg);
                break;
            case 'm':
                MEM=atoi(optarg);
                break;
            case 'k':
                K=atoi(optarg);
                break;
        }
    }
    cout<<"MEM="<<MEM<<"KB"<<endl<<"Find top "<<K<<endl<<endl;
    cout<<"preparing all algorithms"<<endl;
    int m=MAX_INSERT;  // the number of flows

    // preparing Double-Anonymous Sketch
    int DA_M;
    for (DA_M=1; 32*DA_M*TOP_d+32*DA_M*CMM_d<=MEM*1024*8; DA_M++);if (DA_M%2==0) DA_M--;
    func.push_back(new dasketch(DA_M, K));

    // preparing cuckoocounter
    int cc_M;
    for (cc_M = 1; 64 * cc_M*CC_d + 432 * K <= MEM * 1000 * 8; cc_M++); if (cc_M % 2 == 0) cc_M--;
    std::cout << cc_M << std::endl;
    func.push_back(new cuckoocounter(cc_M, K, 3, 0.01));

    // preparing BubbleSketch
    func.push_back(new sketch::bubblesketch::BubbleSketch(48, K, MEM));

    // // preparing heavykeeper
    int hk_M;
    for (hk_M=1; 32*hk_M*HK_d+432*K<=MEM*1024*8; hk_M++); if (hk_M%2==0) hk_M--;
    func.push_back(new heavykeeper(hk_M, K));

    // preparing spacesaving
    int ss_M;
    for (ss_M=1; 432*ss_M<=MEM*1024*8; ss_M++);
    func.push_back(new spacesaving(ss_M, K));

    // prepare clear
    for (auto &iter : func) {
        func_names.push_back(iter->get_name());
        iter->clear();
    }

    // Inserting
	timespec time1, time2;
	long long resns;
	char default_dataset[40]="./1.dat";
	if(dataset[0]=='\0') strcpy(dataset, default_dataset);
	cout<<"dataset: "<<dataset<<endl<<endl;
	ifstream fin(dataset, ios::in|ios::binary);
	if(!fin) {printf("Dataset not exists!\n");return -1;}
    int packet_num = 0;
	for (int i = 1; i <= m; i++)
	{
        if (fin.eof()) {
            break;
        }
        packet_num++;
		fin.read(tmp, KEY_LEN);
		tmp[KEY_LEN]='\0';
		s[i] = string(tmp, KEY_LEN);
		B[s[i]]++;
	}
    m = packet_num;
    std::cout << m << std::endl;
    printf("flow num = %d\n", packet_num);
    printf("flow type = %d\n", B.size());


	printf("*************throughput（insert）************\n");
    
    std::cout << m << std::endl;

    for (auto &sketch_func : func) {
        clock_gettime(CLOCK_MONOTONIC, &time1);
        for (int i = 1; i <= m; i++) {
            sketch_func->Insert(s[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &time2);
	        resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
        double throughput = (double)1000.0 * m / resns;
        printf("throughput of %s (insert): %.6lf Mips\n", sketch_func->get_name().c_str(), throughput);
        insert_throughput[sketch_func->get_name()] = throughput;
    }
        printf("*************throughput(query)************\n");
        std::cout << m << std::endl;

    for (auto &sketch_func : func) {
        clock_gettime(CLOCK_MONOTONIC, &time1);
        std::cout << sketch_func->get_name() << " work" << std::endl;;
        sketch_func->work();
        clock_gettime(CLOCK_MONOTONIC, &time2);
	        resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
        double throughput = (double)1000.0 * m / resns;
        printf("throughput of %s (query): %.6lf Mips\n", sketch_func->get_name().c_str(), throughput);
        query_throughput[sketch_func->get_name()] = throughput;
    }

    printf("\npreparing true flow\n");
	// preparing true flow
	int cnt=0;
    for (map <string,int>::iterator sit=B.begin(); sit!=B.end(); sit++)
    {
        p[++cnt].x=sit->first;
        p[cnt].y=sit->second;
    }
    sort(p+1,p+cnt+1,cmp);
    for (int i=1; i<=K+10; i++) {
        C[p[i].x] = p[i].y;
    } 


    // Calculating PRE, ARE, AAE
    cout << "Calculating\n" << endl;
    for (auto &sketch_func : func) {
        std::string str;
        int num;
        std::set<std::string> keys;
        for (int i = 0; i < K; i++) {
            auto [str, num] = sketch_func->Query(i);
            if (keys.count(str)) {
                continue;
            }
            keys.insert(str);
            if (C[str]) {
                _sum[sketch_func->get_name()]++;
                AAE[sketch_func->get_name()] += abs(B[str] - num);
                ARE[sketch_func->get_name()] += abs(B[str] - num) / (B[str] + 0.0);
            }
        }

        printf("%s:\n", sketch_func->get_name().c_str());
        printf("\tAccepted: %d/%d %.10f\n", _sum[sketch_func->get_name()], K, (_sum[sketch_func->get_name()] / (K + 0.0)));
        printf("\tARE: %.10f\n", ARE[sketch_func->get_name()] / K);
        printf("\tAAE: %.10f\n\n", AAE[sketch_func->get_name()] / (K + 0.0));
    }
    string resultFile = "result.csv";
    writeResultToCSV(string(dataset), resultFile, MEM, K);

	return 0;
}
