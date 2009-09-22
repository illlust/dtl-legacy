
#include "../src/dtl.hpp"
#include "common.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cassert>

using namespace std;

using dtl::Diff;
using dtl::Ses;

int main(int argc, char *argv[]){
  
  if (isFewArgs(argc)) {
    cerr << "few arguments" << endl;
    return -1;
  }

  string A(argv[1]);
  string B(argv[2]);
  bool fileExist = true;

  if (!isFileExist(A)) {
    cerr << "file A does not exist" << endl;
    fileExist = false;
  }

  if (!isFileExist(B)) {
    cerr << "file B does not exist" << endl;
    fileExist = false;
  }

  if (!fileExist) {
    return -1;
  }

  typedef string elem;
  typedef vector<elem> sequence;
  ifstream Aifs(A.c_str());
  ifstream Bifs(B.c_str());
  elem buf;
  sequence ALines, BLines;
  ostringstream ossLine, ossInfo;

  while(getline(Aifs, buf)){
    ALines.push_back(buf);
  }
  while(getline(Bifs, buf)){
    BLines.push_back(buf);
  }
  
  Diff<elem, sequence > d(ALines, BLines);
  d.compose();

  Ses<elem> ses = d.getSes();
  
  sequence s1 = ALines;
  sequence s2 = d.patch(s1);

  // fpatch 
  assert(BLines == s2);
  cout << "fpatch successed" << endl;

  d.composeUnifiedHunks();
  sequence s3 = d.uniPatch(s1);

  // unipatch 
  assert(BLines == s3);
  cout << "unipatch successed" << endl;
  
  return 0;
}
