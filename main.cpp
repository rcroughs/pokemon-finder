#include <cstdio>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
  const char image_to_find = *argv[1];
  std::vector<std::string> images_set;
  char buffer[999];
  while (fgets(buffer, 999, stdin)) {
    std::string new_path(buffer);
    images_set.push_back(new_path); 
  };
  return 0;
}
