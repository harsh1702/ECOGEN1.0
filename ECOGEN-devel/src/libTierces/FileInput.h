#ifndef FILEINPUT_H
#define FILEINPUT_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>


void readFile(std::string file, std::vector<double> &Time, std::vector<double> &Quantity);
void fillVectorData(std::vector<double> &Quantity1, std::vector<double> &Quantity2, std::vector<double> &Time1, std::vector<double> &Time2);

#endif
