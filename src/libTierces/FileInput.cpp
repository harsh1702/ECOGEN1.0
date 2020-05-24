#include "FileInput.h"

using namespace std;



void readFile(string file, vector<double> &Time, vector<double> &Quantity)
{
  //as of now first column is for time while the second column is for quantity to be varied
  //can add interpolation schemes later

    ifstream strmIn(file.c_str());
    string line;
    double dat1, dat2;
    if (strmIn) {
        while (getline(strmIn,line)) {
            strmIn >> dat1 >> dat2;
            Time.push_back(dat1);
            Quantity.push_back(dat2);
        }
    }
    else {
        cout << "Error : reading data file " << file << "\n";
        exit(0);
    }
}

void fillVectorData(vector<double> &Quantity1, vector<double> &Quantity2, vector<double> &Time1, vector<double> &Time2)
{
  if (Quantity1.size() == Quantity2.size())
  {
    return ;
  }

  if (Quantity1.size() > Quantity2.size())
  {
    while (Quantity1.size() > Quantity2.size())
    {
      //pushing the value of the last element in the smaller vector
      Quantity2.push_back(Quantity1[Quantity2.size()]);
      Time2.push_back(Time1[Time2.size()]);
    }
  }

  if (Quantity1.size() < Quantity2.size())
  {
    while (Quantity1.size() < Quantity2.size())
    {
      //pushing the value of the last element in the smaller vector
      Quantity1.push_back(Quantity2[Quantity1.size()]);
      Time1.push_back(Time2[Time1.size()]);
    }
  }

}
