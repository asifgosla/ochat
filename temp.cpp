#include <cstdlib>
#include <iostream>
using namespace std;

int main() {
  cout << "Tossing a coin 10 times!" << std::endl;
  srand(time(0));
  for (int i = 0; i < 10; ++i) {
    if (int y = rand(); y >= RAND_MAX / 2) {
      cout << "heads: " << y << endl;
    } else
      cout << "tails" << endl;
  }

  if (int s = 1, t = 0; s + t > 0) {
    cout << "s+t =" << s + t << endl;
  }
  return 0;
}
