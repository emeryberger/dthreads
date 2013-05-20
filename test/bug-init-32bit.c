#include <iostream>
#include <stdlib.h>

using namespace std;

int main() {

  //int* i = new int;
  int* i = (int*) malloc(sizeof(int));
  *i = 10;
  cout << i << endl;

  return 0;
}
