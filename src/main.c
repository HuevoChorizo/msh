/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */

#include <signal.h>
#include <stddef.h> /* NULL */
#include <stdio.h>  /* setbuf, printf */
#include <stdlib.h>
#include <string.h>/*strcmp, strlen,...*/
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define ansiblue "\033[1;36m"
#define ansigreen "\x1b[1;32m"
#define ansireset "\x1b[0m"

enum comandos {
  CD = 1,
  UMASK,
  LIMIT,
  SET,
};
char *home;

extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */
int cd(char *direccion) {
  if (direccion != NULL)
    return chdir(direccion);
  return chdir(home);
}

int mascara(char *entrada) {
  if (entrada == NULL) {
    mode_t mascAnt = umask(0);
    umask(mascAnt);
    return mascAnt;
  }

  int octal;
  octal = strtol(entrada, NULL, 8);
  mode_t mascAnt = umask(octal);
  return mascAnt;
}
int limites() {
  struct rlimit *limite = malloc(2 * sizeof(rlim_t));
  getrlimit(RLIMIT_CPU, limite);
  fprintf(stdout, "cpu\t%ld\n", limite->rlim_max);
  getrlimit(RLIMIT_FSIZE, limite);
  fprintf(stdout, "fsize\t%ld\n", limite->rlim_max);
  getrlimit(RLIMIT_DATA, limite);
  fprintf(stdout, "data\t%ld\n", limite->rlim_max);
  getrlimit(RLIMIT_STACK, limite);
  fprintf(stdout, "stack\t%ld\n", limite->rlim_max);
  getrlimit(RLIMIT_CORE, limite);
  fprintf(stdout, "core\t%ld\n", limite->rlim_max);
  getrlimit(RLIMIT_NOFILE, limite);
  fprintf(stdout, "nofile\t%ld\n", limite->rlim_max);
  free(limite);
  return 0;
}

int limit(int recurso, char *argv, char *entrada) {
  struct rlimit *limite = malloc(2 * sizeof(rlim_t));
  if (entrada == NULL) {
    getrlimit(recurso, limite);
    fprintf(stdout, "%s\t%ld\n", argv, limite->rlim_max);
  } else {
    int lmt = strtol(entrada, NULL, 8);
    /*TODO: Comprobar que si limit = -1, se establece el máximo.*/
    getrlimit(recurso, limite);
    limite->rlim_max = lmt;
    setrlimit(recurso, limite);
  }
  return 0;
}

int set(char *variable, );

int main(void) {
  fprintf(stderr, "\n");
  char ***argvv = NULL;
  int argvc;
  char **argv = NULL;
  int argc;
  char *filev[3] = {NULL, NULL, NULL};
  int bg;
  int ret;
  home = getenv("HOME");

  setbuf(stdout, NULL); /* Unbuffered */
  setbuf(stdin, NULL);

  while (1) {
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    fprintf(stderr, ansiblue "%s\n", cwd);
    fprintf(stderr, ansigreen "msh ☭ > " ansireset);

    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      break; /* EOF */
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)
      continue; /* Empty line */
#if 1

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      if (strcmp(argv[0], "cd") == 0) {
        int a = cd(argv[1]);
        if (a == -1)
          printf("Directorio inválido");
      } else if (strcmp(argv[0], "umask") == 0) {
        int a = mascara(argv[1]);
        printf("%o\n", a);
      } else if (strcmp(argv[0], "set") == 0) {
        for (argc = 0; argv[argc]; argc++) {
          // printf("%s ", argv[argc]);
        }
      } else if (strcmp(argv[0], "limit") == 0) {
        if (argv[1] == NULL)
          limites();
        else if (strcmp(argv[1], "cpu") == 0) {
          limit(RLIMIT_CPU, argv[1], argv[2]);
        } else if (strcmp(argv[1], "fsize") == 0) {
          limit(RLIMIT_FSIZE, argv[1], argv[2]);
        } else if (strcmp(argv[1], "data") == 0) {
          limit(RLIMIT_DATA, argv[1], argv[2]);
        } else if (strcmp(argv[1], "stack") == 0) {
          limit(RLIMIT_STACK, argv[1], argv[2]);
        } else if (strcmp(argv[1], "core") == 0) {
          limit(RLIMIT_CORE, argv[1], argv[2]);
        } else if (strcmp(argv[1], "nofile") == 0) {
          limit(RLIMIT_NOFILE, argv[1], argv[2]);
        } else {
          fprintf(
              stderr,
              "El recurso: «%s» no existe o no está implementada su gestión\n",
              argv[1]);
        }

      } else if (strcmp(argv[0], "set") == 0) {
        set();
      } else {
        pid_t pid = fork();

        if (pid == -1) {
          fprintf(stderr, "ERROR fork");
        } else if (pid == 0) {
          if (execvp(argv[0], argv) == -1) {
            fprintf(stderr, "ERROR execvp");
          }
        } else {
          int valor;
          wait(&valor);
        }
      }

      printf("\n");
    }
  }
#endif
  exit(0);
  return 0;
}
