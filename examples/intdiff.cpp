
#include <iostream>
#include <vector>
#include "../src/dtl.hpp"

using namespace std;

int main(int, char**){
  
  int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int b[10] = {3, 5, 1, 4, 5, 1, 7, 9, 6, 10};

  for (int i=0;i<10;++i) {
    cout << a[i] << " ";
  }
  cout << endl;
  for (int i=0;i<10;++i) {
    cout << b[i] << " ";
  }
  cout << endl;
  vector<int> A(&a[0], &a[10]);
  vector<int> B(&b[0], &b[10]);
  dtl::Diff<int, vector<int> > d(A, B);
  d.compose();
  // editDistance
  cout << "editDistance:" << d.getEditDistance() << endl;
  // Longest Common Subsequence
  dtl::Lcs<int> lcs = d.getLcs();
  vector<int> lcs_v = lcs.getSequence();
  cout << "LCS: ";
  for (vector<int>::iterator vit=lcs_v.begin();vit!=lcs_v.end();++vit) {
    cout << *vit << " ";
  }
  cout << endl;

  // Shortest Edit Script
  cout << "SES" << endl;
  d.printSES();

  return 0;
}
