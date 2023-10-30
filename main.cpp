#include <string>
#include <string>
#include <tuple>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>

#define READ 0
#define WRITE 1


void execution_enfant(const std::string &image_to_find, int pere_vers_fils[2], int fils_vers_pere[2]) {

  char buffer[999];
  // On attend que le père envoie une image à comparer dans le pipe. ヽ(ー_ー )ノ
  read(pere_vers_fils[READ], buffer, sizeof(buffer));
  std::string current_img_path(buffer);

  std::string command = "img_dist " + image_to_find + " " + current_img_path;
  int ret_value = system(command.c_str());  // Appel à la fonction img_dist, disponible via le PATH.
  write(fils_vers_pere[WRITE], &ret_value, sizeof(int));
  // On envoie directement le résultat au père pour qu'il puisse comparer. <(￣︶￣)>
  


  // SIDE NOTE : Ici, on va vouloir exécuter ce code en boucle tant que le signal usr1 ou usr2
  // n'est pas arrivé. Je laisse le code relatif aux signaux à rajouter plus tard. (désolé .__.")
  // Un autre point ; l'exécution parent pourrait très bien décider d'envoyer un signal singint ;
  // dans ce cas ci, la procédure est de finir l'exécution de la boucle puis de terminer le programme
  // avec le code d'erreur 1. Je laisse le soin d'implémenter la gestion de ces signaux. Courage ! <3

}

void execution_parent(int pere_vers_f1[2], int pere_vers_f2[2], int f1_vers_pere[2], int f2_vers_pere[2]) {

  char buffer[999];  // Buffer de taille 999 chars.
  int image_count = 0;
  std::tuple<int, std::string> closest(NULL, nullptr);
  // On déclare notre tuple qui contiendra l'image la plus proche.

  while (fgets(buffer, sizeof(buffer), stdin)) {

    std::string new_path(buffer); // new_path contient le chemin vers la prochaine image à comparer.
    int current_dist;

    if (image_count % 2 == 0) {
      // Images avec ID pair sont traitées par Fils1
      write(pere_vers_f1[WRITE], new_path.c_str(), new_path.size() + 1);
      read(f1_vers_pere[READ], &current_dist, sizeof(int));
    } else {
      // Images avec ID impair sont traitées par Fils2
      write(pere_vers_f2[WRITE], new_path.c_str(), new_path.size() + 1);
      read(f2_vers_pere[READ], &current_dist, sizeof(int));
    }
    
    if (current_dist > 65) {
      perror("img_dist ; Coudn't get distance between the two images.");
    } else {
      // On compare la meilleure distance obtenue. (っ ᵔ◡ᵔ)っ
      // On update si on a trouvé une meilleure correspondance.
      if (std::get<0>(closest) == NULL) {
        std::get<0>(closest) = current_dist; 
        std::get<1>(closest) = new_path;
      } else {
        if (std::get<0>(closest) >= current_dist) {
          std::get<0>(closest) = current_dist;
          std::get<1>(closest) = new_path;
        }
      }
    }
    image_count++;
  };

  // Ici on gère les signaux des enfants puis on écrit sur la sortie standard
  // le résultat de la recherche avec img_dist. Si on a pas de dist, on affiche
  // que la recherche n'a donné aucun résultat >__<"

  if (std::get<0>(closest) == NULL) {
    std::cout << "No similar image found (no comparison could be performed successfully)." << std::endl;
  } else {
    std::cout << "Most similar image found: " << std::get<1>(closest) << " with a distance of " << std::get<0>(closest) << "." << std::endl;
  }

}



int main(int argc, char *argv[]) {

  if (argc != 2) {
    // Si l'exécution de img-search à un nombre de paramètres incorrect.
    std::cerr << "Mauvaise utilisation : img_search prend 2 paramètres." << std::endl;
    return 1;
  }

  const std::string image_to_find = argv[1];  // PATH vers l'image de référence.

  // Initialisation des pipes de communication du père vers ses fils. 
  // Pipe unidirectionnels. 〜(￣▽￣〜)
  int pere_vers_fils1[2], pere_vers_fils2[2];
  int fils1_vers_pere[2], fils2_vers_pere[2];
  if (pipe(pere_vers_fils1) < 0 || pipe(pere_vers_fils2) < 0 || pipe(fils1_vers_pere) < 0 || pipe(fils2_vers_pere) < 0) {
    perror("pipe() ; Couldn't create one of / all of the pipe(s).");
    return 1;
  }

  pid_t child_pid = fork();  // On crée la main fork.

  if (child_pid == 0) {
    // Processus enfant (destiné a se dédoubler en 2 sous enfants).
    pid_t sub_child_pid = fork();
    
    if (sub_child_pid == 0) {
      // sub-Processus Fils n°2
      close(pere_vers_fils2[WRITE]); close(fils2_vers_pere[READ]);
      execution_enfant(image_to_find, pere_vers_fils2, fils2_vers_pere);
      // On lance l'exécution enfant avec les pipes respectifs au sub-Processus Fils n°2.

    } else if (sub_child_pid > 0) {
      // sub-Processus Fils n°1 ; (sub_child_pid renvoie le PID du processus Fils n°2).
      close(pere_vers_fils1[WRITE]); close(fils1_vers_pere[READ]);
      execution_enfant(image_to_find, pere_vers_fils1, fils1_vers_pere);
      // On lance l'exécution enfant avec les pipes respectifs au sub-Processus Fils n°1.

    } else {
      perror("fork() error ; couldn't create sub-fork().");
      return 1;
    }

  } else if (child_pid > 0) {
    // Processus Parent ; (child_pid renvoie le PID de son processus fils).

    // On ferme les sens des canaux auquels le processus père n'a pas accès.
    close(pere_vers_fils1[READ]); close(pere_vers_fils2[READ]);
    close(fils1_vers_pere[WRITE]); close(fils2_vers_pere[WRITE]);

    execution_parent(pere_vers_fils1, pere_vers_fils2, fils1_vers_pere, fils2_vers_pere);
    // On lance l'exécution parent avec tout les pipes. Ils ne sont plus anonymes.

  } else {
    // Fork error (￣^￣)/
    perror("fork() error ; couldn't create main fork().");
    return 1;
  }

  return 0;
}


// Dans son état actuel, img-search ne peut pas compiler correctement (et ne devrait pas par ailleurs) :
// les signaux ne sont pas traités, ce qui entraine les exécutions fils à ne s'exéctuer qu'une seule fois.
// Les boucles sont sont à rajouter. Je ne suis également pas certain que la ligne 20 
// (exécution de la commande system avec comme programme list-file.sh) soit fonctionnelle ; laucher.sh devrait
// normalement permettre l'utilisation de la commande list-file puisqu'il la place dans $PATH mais ceci est à revoir.
