<<<<<<< HEAD
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>

void exection_enfant(const char &image_to_find) {
  // Execution des enfants (ils fonctionnent de la meme manière)
}

void execution_parent() {
  // Execution du processus parent
  char buffer[999];
  while (fgets(buffer, 999, stdin)) {
    std::string new_path(buffer);
    // Communication aux enfants des adresses

  };
  // Envoie des signaux pour les enfants

}

int main(int argc, char* argv[]) {
  const char image_to_find = *argv[1];
  // Creation des deux processus, de la mémoire partagée 
  // et des pipes de communication


  return 0;
}
=======
#include <string>
#include <tuple>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>


int main() {
  return 0;
}
>>>>>>> master
