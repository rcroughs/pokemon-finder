#include <string>
#include <tuple>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <wait.h>
#include <sys/mman.h>
#define READ 0
#define WRITE 1


void handler_singint_father(int n) {
    kill(0, SIGUSR1);
    exit(1);
}

void handler_sigusr1_child(int n) {
    exit(1);
}

void handler_sigusr2_child(int n) {

    exit(1);
}

void *create_shared_memory(size_t size) {
    // fonction prélevée du TP5 pour l'allocation de mémoire partagée
    const int protection = PROT_READ | PROT_WRITE;
    const int visibility = MAP_SHARED | MAP_ANONYMOUS;
    return mmap(nullptr, size, protection, visibility, -1, 0);
}


void execution_enfant(std::string &image_to_find, int pere_vers_fils[2], short int son_number, int* distance_tab, char* path_tab, pid_t father_pid) {

  close(pere_vers_fils[WRITE]);

  // Masquage de SIGINT
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
      perror("signal() ; Erreur lors du masquage de SIGINT");
  }

  // définition du handler pour SIGUSR1
  if (signal(SIGUSR1, handler_sigusr1_child) == SIG_ERR) {
      perror("signal() ; Erreur lors du traitement du code de l'action (SUGINT)");
   }

  // Ajout de SIGUSR1 au set de masquage
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGUSR1) == -1) {
      perror("sigaddset() ; Erreur lors de l'ajout de SIGUSR1 au masque des signaux");
  }

  char buffer[999];

  int return_status; // Récupère le return de execlp --> WIFEXITED vérifiera la terminaison du fils
  int ret_value = -1; // Contiendra la valeur déchiffrée par WEXITSTATUS --> par défaut -1 (erreur fork())

  // On attend que le père envoie une image à comparer dans le pipe. ヽ(ー_ー )ノ
  while (read(pere_vers_fils[READ], buffer, sizeof(buffer))) {

      kill(father_pid, SIGCONT);
      std::string current_img_path(buffer);

      // Bloquage du signal SIGUSR1 pendant la durée des calculs
      if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
          perror("sigprocmask() ; Erreur lors du bloquage de SIGUSR1");
      }

      // Conversion en pointeurs vers const char --> arguments de execlp
      const char *arg1 = image_to_find.c_str();
      const char *arg2 = current_img_path.c_str();

      // Création d'un fils pour execlp
      pid_t pid = fork();
      if (pid == 0) {
          // Processus fils
          if (execlp("img-dist", "img-dist", arg1, arg2, nullptr) < 0) {
              ret_value = -2; // img-dist n'a pu s'exécuter
          }
      } else if (pid > 0) {
          // Processus père récupère le status de terminaison du fils et son return
          waitpid(pid, &return_status, 0);
          if (WIFEXITED(return_status)) {
              ret_value = WEXITSTATUS(return_status);
              } else {
              ret_value = -3;} // Processus s'est mal terminé
      }

      if (ret_value > 65 or ret_value < 0) {
          // Différentes erreurs possibles
          if (ret_value == -1) {perror("fork() ; img_dist could not be called.");}
          else if (ret_value == -2) {perror("execlp() : img_dist could not be called.");}
          else if (ret_value == -3) {perror("img-dist ; process failed to finish correctly");}
          else {perror("img_dist ; Coudn't get distance between the two images.");} // Code > 65
      } else {

          if (ret_value < distance_tab[son_number] || distance_tab[son_number] == -10) {
              distance_tab[son_number] = ret_value;
              strncpy(path_tab + son_number * 1000, arg2, 999);
          }
          // comparer la meilleure distance obtenue
      }
      // SIGURS1 est débloqué
      sigprocmask(SIG_UNBLOCK, &set, nullptr);
  }
}

void execution_parent(int pere_vers_f1[2], int pere_vers_f2[2], int* distance_tab, char *path_tab) {

  // Définition du handler pour SIGINT
  signal(SIGINT, handler_singint_father);

  close(pere_vers_f1[READ]);
  close(pere_vers_f2[READ]);

  char buffer[999];  // Buffer de taille 999 chars.
  int image_count = 0;

  while (fgets(buffer, sizeof(buffer), stdin)) {

    std::string new_path(buffer); // new_path contient le chemin vers la prochaine image à comparer.

    size_t found = new_path.find('\n'); // on trouve l'indice où se situe '\n' dans new_path
    new_path[found] = '\0'; // on le remplace par un caractère nul

    if (image_count % 2 == 0) {
      // Images avec ID pair sont traitées par Fils1
      write(pere_vers_f1[WRITE], new_path.c_str(), new_path.size() + 1);
      kill(getpid(), SIGSTOP);
    } else {
      // Images avec ID impair sont traitées par Fils2
      write(pere_vers_f2[WRITE], new_path.c_str(), new_path.size() + 1);
      kill(getpid(), SIGSTOP);
    }
    image_count++;
  }

  // Fermeture des extremités des deux pipes
  close(pere_vers_f1[WRITE]);
  close(pere_vers_f2[WRITE]);
  // Attente de la terminaison des processus fils, permet de les effacer du PCB
  waitpid(distance_tab[2], nullptr, 0);
  waitpid(distance_tab[3], nullptr, 0);

  if (distance_tab[0] == -10 && distance_tab[1] == -10) {
    std::cout << "No similar image found (no comparison could be performed successfully)." << std::endl;
  } else {

    int closest_distance;
    int decalage; // Permet le déplacement dans le tableau de char

    if (distance_tab[0] < distance_tab[1] || distance_tab[0] == distance_tab[1]) {closest_distance = distance_tab[0]; decalage = 0;}
    else if (distance_tab[0] > distance_tab[1]) {closest_distance = distance_tab[1]; decalage = 1;}
    std::cout << "Most similar image found: '" << path_tab + (decalage * 1000) << "' with a distance of " << closest_distance << "." << std::endl;
  }
  // On libère l'espace de mémoire partagée
  munmap(distance_tab, sizeof(int)*4);
  munmap(path_tab, sizeof(char)*999*2+2);
}



int main(int argc, char *argv[]) {

  if (argc != 2) {
    // Si l'exécution de img-search à un nombre de paramètres incorrect.
    std::cerr << "Format Error : launcher [-i|--interactive|-a|--automatic] image [database_path]" << std::endl;
    return 1;
  }

  std::string image_to_find = argv[1];  // PATH vers l'image de référence.

  size_t found = image_to_find.find('\n'); // on trouve l'indice où se situe '\n' dans image_to_find
  image_to_find[found] = '\0'; // on le remplace par un caractère nul

  // Initialisation des pipes de communication du père vers ses fils. 
  // Pipe unidirectionnels. 〜(￣▽￣〜)
  int pere_vers_fils1[2], pere_vers_fils2[2];
  if (pipe(pere_vers_fils1) < 0 || pipe(pere_vers_fils2) < 0) {
    perror("pipe() ; Couldn't create one of / all of the pipe(s).");
    return 1;
  }

  // Initialisation de la mémoire partagée (path --> tableau de 999*2 char* (sans les \0) --> contient les deux chemins)
  int* shared_memory_distance =  static_cast<int*>(create_shared_memory(sizeof(int) * 4));
  char *shared_memory_path = static_cast<char *>(create_shared_memory(2 * sizeof(char) * 999 + 2));
  // Initialisation des caractères séparateurs
  shared_memory_path[999] = '\0';
  shared_memory_path[2 * 999 + 1] = '\0';

  if (shared_memory_distance == MAP_FAILED || shared_memory_path == MAP_FAILED) {
      perror("mmap() ; couldn't create shared memory");
      return 1;
  }
  shared_memory_distance[0] = -10; shared_memory_distance[1] = -10; // initialisation des valeurs par défaut

  pid_t father_pid = getpid();

  pid_t child_pid = fork();  // On crée la main fork.

  if (child_pid == 0) {

    // Processus enfant (destiné a se dédoubler en 2 sous enfants).
    pid_t sub_child_pid = fork();
    
    if (sub_child_pid == 0) {
      // sub-Processus Fils n°2
      close(pere_vers_fils2[WRITE]); // fermeture de l'écriture
      close(pere_vers_fils1[WRITE]);
      shared_memory_distance[2] = getpid(); // Pour pouvoir accéder au pid du fils
      execution_enfant(image_to_find, pere_vers_fils2, 1, shared_memory_distance, shared_memory_path, father_pid);
      // On lance l'exécution enfant avec les pipes respectifs au sub-Processus Fils n°2.

    } else if (sub_child_pid > 0) {
      // sub-Processus Fils n°1 ; (sub_child_pid renvoie le PID du processus Fils n°2).
      close(pere_vers_fils1[WRITE]); // fermeture de l'écriture
      close(pere_vers_fils2[WRITE]);
      shared_memory_distance[3] = getpid();
      execution_enfant(image_to_find, pere_vers_fils1, 0, shared_memory_distance, shared_memory_path, father_pid);
      // On lance l'exécution enfant avec les pipes respectifs au sub-Processus Fils n°1.

    } else {
      perror("fork() error ; couldn't create sub-fork().");
      return 1;
    }

  } else if (child_pid > 0) {
    // Processus Parent ; (child_pid renvoie le PID de son processus fils).
    // fermeture des extrémités non utilisées des pipes --> le père accède en lecture
    close(pere_vers_fils1[READ]); close(pere_vers_fils2[READ]);
    execution_parent(pere_vers_fils1, pere_vers_fils2, shared_memory_distance, shared_memory_path);
    // On lance l'exécution parent avec tout les pipes.

  } else {
    // Fork error (￣^￣)/
    perror("fork() error ; couldn't create main fork().");
    return 1;
  }
  return 0;
}


// Pour le moment, img-search ne s'exécute pas de la meilleure des manières. J'ai rectifié ce qui tourne autour
// des consignes pour la mémoire partagée + j'ai supprimé les pipes inutiles. Il y a quelques changements d'implémentation
// à droite à gauche, mais sinon le code est essentiellement le même (si besoin d'explications, demandez moi :))
//
// J'ai rajouté la fonction du TP5 permettant de retourner un pointeur vers une mémoire partagée configurée
//
// Le fait de ne pas faire de sleep(1) après chaque write du processus père mène à des comportements innatendus
// de la part de img-search. Je ne connais pas encore la raison étant donné que write() est un appel bloquant, mais il
// va falloir comprendre pourquoi cela arrive.
//
// Pour ce qui est des erreurs dans les exécutions fils, voici les différents codes (vous êtes libre de changer ça) :
//      -1 : le fork() a échoué
//      -2 : execlp() chargé d'appeller img-dist a échoué
//      -3 : erreur dans execlp()
//
// Dernier point : je n'ai pas encore géré SIGPIPE car je ne comprends pas la différence de son handler avec celui
// de SIGINT, si quelqu'un a compris, merci de m'éclairer :)
