#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstring>
#include <wait.h>
#include <sys/mman.h>

#define READ 0
#define WRITE 1


void handler_singint_father(int signum) {
  if (signum == SIGPIPE || signum == SIGINT) {
    kill(0, SIGUSR1);
    exit(1);
  }
}


void handler_sigusr1_child(int sigint) {
  if (sigint == SIGUSR1) {
    exit(1);
  }
}


void *create_shared_memory(size_t size) {
  // fonction prélevée du TP5 pour l'allocation de mémoire partagée
  const int protection = PROT_READ | PROT_WRITE;
  const int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(nullptr, size, protection, visibility, -1, 0);
}


void execution_enfant(char* image_to_find, int pere_vers_fils[2], int* distance_tab, char* path_tab) {

  // Masquage de SIGINT
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
    perror("signal() ; Erreur lors du masquage de SIGINT");
  }

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal() ; Erreur lors du masquage de SIGPIPE");
  }

  // définition du handler pour SIGUSR1
  if (signal(SIGUSR1, handler_sigusr1_child) == SIG_ERR) {
    perror("signal() ; Erreur lors du traitement du code de l'action (SIGUSR1)");
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
    if (kill(getppid(), SIGCONT) == -1) {
      perror("kill(); couldn't send SIGCONT signal");
    }

    // Bloquage du signal SIGUSR1 pendant la durée des calculs
    if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
      perror("sigprocmask() ; Erreur lors du bloquage de SIGUSR1");
    }

    // Création d'un fils pour execlp
    pid_t pid = fork();
    if (pid == 0) {
      // Processus fils
      if (execlp("img-dist", "img-dist", image_to_find, buffer, nullptr) < 0) {
        ret_value = -2; // img-dist n'a pu s'exécuter
      }
    } else if (pid > 0) {
      // Processus père récupère le status de terminaison du fils et son return
      waitpid(pid, &return_status, 0);
      if (WIFEXITED(return_status)) {
        ret_value = WEXITSTATUS(return_status);
      } else {
        ret_value = -3;} // Processus s'est mal terminé
    } else {
      perror("fork(); Couldn't create img-dist process");
    }

    if (ret_value > 65 or ret_value < 0) {
      // Différentes erreurs possibles
      if (ret_value == -1) {perror("fork() ; img_dist could not be called.");}
      else if (ret_value == -2) {perror("execlp() : img_dist could not be called.");}
      else if (ret_value == -3) {perror("img-dist ; process failed to finish correctly");}
      else {perror("img_dist ; Coudn't get distance between the two images.");} // Code > 65
    } else {

      if (ret_value < *distance_tab || *distance_tab == -10) {
        *distance_tab = ret_value;
        strncpy(path_tab, buffer, 999);
      }
      // comparer la meilleure distance obtenue
    }
    // SIGURS1 est débloqué
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
  }
}


void execution_parent(int pere_vers_f1[2], int pere_vers_f2[2], int* distance_tab, char *path_tab1, char* path_tab2) {

  // Définition du handler pour SIGINT
  if (signal(SIGINT, handler_singint_father) == SIG_ERR ||
      signal(SIGPIPE, handler_singint_father) == SIG_ERR ||
      signal(SIGCONT, handler_singint_father) == SIG_ERR) 
  {
    perror("signal(); could't create a signal handler");
    exit(1);
  }

  char buffer[999];  // Buffer de taille 999 chars.
  int image_count = 0;

  while (fgets(buffer, sizeof(buffer), stdin)) {
    int buffer_size = 0;
    while (buffer[buffer_size] != '\n') {
      buffer_size++;
    }
    buffer[buffer_size] = '\0';


    if (image_count % 2 == 0) {
      // Images avec ID pair sont traitées par Fils1
      write(pere_vers_f1[WRITE], buffer, buffer_size + 1);
    } else {
      // Images avec ID impair sont traitées par Fils2
      write(pere_vers_f2[WRITE], buffer, buffer_size + 1);
    }
    if (image_count > 0) {
      pause();
    }
    image_count++;
  }

  // Fermeture des extremités des deux pipes
  if (close(pere_vers_f1[WRITE]) == -1 ||
      close(pere_vers_f2[WRITE]) == -1)
  {
    perror("close(); couldn't close one of the pipes");
  }
  // Attente de la terminaison des processus fils, permet de les effacer du PCB
  wait(NULL);
  wait(NULL);


  if (distance_tab[0] == -10 && distance_tab[1] == -10) {
    std::cout << "No similar image found (no comparison could be performed successfully)." << std::endl;
  } else {

    int closest_distance;
    char* closest_path;

    // Si un des élément est égal a -10
    if (distance_tab[0] == -10) {
      closest_distance = distance_tab[1];
      closest_path = path_tab2;
    } else if (distance_tab[1] == -10) {
      closest_distance = distance_tab[0];
      closest_path = path_tab1;
    }

    // Dans le cas où les deux registes contiennent des distances calculées
    else if (distance_tab[0] < distance_tab[1] || distance_tab[0] == distance_tab[1]) {
      closest_distance = distance_tab[0];
      closest_path = path_tab1;
    }
    else if (distance_tab[0] > distance_tab[1]) {
      closest_distance = distance_tab[1]; 
      closest_path = path_tab2;
    }
    std::cout << "Most similar image found: '" << closest_path << "' with a distance of " << closest_distance << "." << std::endl;
  }
  // On libère l'espace de mémoire partagée
  munmap(distance_tab, sizeof(int)*2);
  munmap(path_tab1, sizeof(char)*999+1);
  munmap(path_tab2, sizeof(char)*999+1);
}


int main(int argc, char *argv[]) {

  if (argc != 2) {
    // Si l'exécution de img-search à un nombre de paramètres incorrect.
    std::cerr << "Format Error : launcher [-i|--interactive|-a|--automatic] image [database_path]" << std::endl;
    return 1;
  }

  char* image_to_find = argv[1];  // PATH vers l'image de référence.

  // Initialisation des pipes de communication du père vers ses fils. 
  // Pipe unidirectionnels. 〜(￣▽￣〜)
  int pere_vers_fils1[2], pere_vers_fils2[2];
  if (pipe(pere_vers_fils1) < 0 || pipe(pere_vers_fils2) < 0) {
    perror("pipe() ; Couldn't create one of / all of the pipe(s).");
    return 1;
  }

  // Initialisation de la mémoire partagée (path --> tableau de 999*2 char* (sans les \0) --> contient les deux chemins)
  int* shared_memory_distance =  static_cast<int*>(create_shared_memory(sizeof(int) * 2));
  char *shared_memory_path1 = static_cast<char *>(create_shared_memory(sizeof(char) * 999 + 1));
  char *shared_memory_path2 = static_cast<char *>(create_shared_memory(sizeof(char) * 999 + 1));
  // Initialisation des caractères séparateurs
  shared_memory_path1[999] = '\0'; shared_memory_path2[999] = '\0';

  if (shared_memory_distance == MAP_FAILED || shared_memory_path1 == MAP_FAILED || shared_memory_path2 == MAP_FAILED) {
    perror("mmap() ; couldn't create shared memory");
    return 1;
  }
  shared_memory_distance[0] = -10; shared_memory_distance[1] = -10; // initialisation des valeurs par défaut

  pid_t child_pid = fork();  // On crée la main fork.

  if (child_pid == 0) {

    // Processus enfant (destiné a se dédoubler en 2 sous enfants).

    close(pere_vers_fils1[WRITE]); // fermeture de l'écriture
    close(pere_vers_fils2[WRITE]);
    execution_enfant(image_to_find, pere_vers_fils1,&shared_memory_distance[0], shared_memory_path1);


  } else if (child_pid > 0) {
    // Processus Parent ; (child_pid renvoie le PID de son processus fils).
    // fermeture des extrémités non utilisées des pipes --> le père accède en lecture
    pid_t second_child = fork ();

    if (second_child == 0) {
      close(pere_vers_fils1[WRITE]); close(pere_vers_fils2[WRITE]); // Fermeture des pipes d'écriture
      execution_enfant(image_to_find, pere_vers_fils2, &shared_memory_distance[1], shared_memory_path2);
      // On lance l'exécution enfant avec les pipes respectifs au sub-Processus Fils n°2.
    } else if (second_child > 0) {
      close(pere_vers_fils1[READ]); close(pere_vers_fils2[READ]);
      // On lance l'exécution parent avec tout les pipes.
      execution_parent(pere_vers_fils1, pere_vers_fils2, shared_memory_distance, shared_memory_path1, shared_memory_path2);
    } else {
      perror("fork() error; couldn't create the second child");
    }

  } else {
    // Fork error (￣^￣)/
    perror("fork() error ; couldn't create the first child");
    return 1;
  }
  return 0;
}
