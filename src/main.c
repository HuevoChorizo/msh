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
/*TODO: GENERAL
 * -> PIPES, ARREGLAR LO DE LAS MÚLTIPLES
 * -> REDIRECCIÓN Y TAL
 * -> SELECTOR
 * -> METER LAS MIERDAS VIEJAS
 */

#include <fcntl.h>
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

/*TODO: Variables de entorno*/
extern char **environ;
char *home;
char *prompt;
int mypid;
int bgpid;
int status;

extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */

int gestionPipes(int k, int n) {
  if (n == 1)
    return 0;
  if (k == 0)
    return 1;
  if (k > 0 && k != n - 1)
    return 2;
  if (k == n - 1)
    return 3;

  return 0;
}

int cd(char *direccion) {
  if (direccion)
    return chdir(direccion);
  return chdir(home);
}

int mascara(char *entrada) {
  if (!entrada) {
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
    int lmt = strtol(entrada, NULL, 10);
    /*TODO: Comprobar que si limit = -1, se establece el máximo.*/
    getrlimit(recurso, limite);
    limite->rlim_max = lmt;
    setrlimit(recurso, limite);
  }
  free(limite);
  return 0;
}

int sets() {
  int i = 0;
  while (environ[i]) {
    printf("%s\n", environ[i++]);
  }
  return 0;
}

int set(char *variable, char *valor) {
  if (valor == NULL) {
    char *aux = getenv(variable);
    fprintf(stdout, "%s=%s\n", variable, aux);
  } else {
    setenv(variable, valor, 1);
  }
  return 0;
}

/*TODO: Cambiar los returns si funcionan o no, dependiendo de tal devuelve lo
 * que sea.*/
int selector(char **argv) {
  if (strcmp(argv[0], "cd") == 0) {
    return cd(argv[1]);
  } else if (strcmp(argv[0], "umask") == 0) {
    int a = mascara(argv[1]);
    printf("%o\n", a);
    return 0;
  } else if (strcmp(argv[0], "limit") == 0) {
    if (argv[1] == NULL) {
      return limites();
    } else if (strcmp(argv[1], "cpu") == 0) {
      return limit(RLIMIT_CPU, argv[1], argv[2]);
    } else if (strcmp(argv[1], "fsize") == 0) {
      return limit(RLIMIT_FSIZE, argv[1], argv[2]);
    } else if (strcmp(argv[1], "data") == 0) {
      return limit(RLIMIT_DATA, argv[1], argv[2]);
    } else if (strcmp(argv[1], "stack") == 0) {
      return limit(RLIMIT_STACK, argv[1], argv[2]);
    } else if (strcmp(argv[1], "core") == 0) {
      return limit(RLIMIT_CORE, argv[1], argv[2]);
    } else if (strcmp(argv[1], "nofile") == 0) {
      return limit(RLIMIT_NOFILE, argv[1], argv[2]);
    } else {
      fprintf(stderr,
              "El recurso: «%s» no existe o no está implementada su "
              "gestión\n",
              argv[1]);
      return 0;
    }
  } else if (strcmp(argv[0], "set") == 0) {
    if (argv[1] == NULL) {
      return sets();
    } else if (argv[2] == NULL) {
      return set(argv[1], argv[2]);
    } else {
      char valor[1024] = "\0";
      for (int i = 2; argv[i] != NULL; i++)
        strcat(valor, argv[i]);
      set(argv[1], valor);
      return 0;
    }
  } else {
    return 1;
  }
}

int main(void) {
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
    setenv("promt", strcat(cwd, "\nmsh> "), 1);
    char *promt = getenv("promt");
    fprintf(stderr, "%s", promt);
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      break; /* EOF */
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)
      continue; /* Empty line */

    int n = 0;
    while (argvv[n] != NULL) {
      n++;
    }
    /*TODO: Hacer múltiples pipes para las payadas estas*/
    int pipa[n][2];
    for (int i = 0; i < n - 1; i++) {
      pipe(pipa[i]);
    }

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      /*TODO: Guardar entrada y salida estándar*/
      pid_t pid1, pid2;
      if (bg) {
        pid1 = fork();
        if (pid1 == -1)
          fprintf(stderr, "ERROR fork");
        if (pid1 == 0) {
          pid2 = fork();
          if (pid2 == -1)
            fprintf(stderr, "ERROR fork");
          if (pid2 == 0) {
            int pipaGest = gestionPipes(argvc, n);
            switch (pipaGest) {
            case 1: {
              close(1);
              dup(pipa[0][1]);
              close(pipa[0][0]);
              close(pipa[0][1]);
              break;
            }

            case 2: {
              close(0);
              close(1);
              dup(pipa[argvc - 1][0]);
              dup(pipa[argvc][1]);
              close(pipa[argvc - 1][0]);
              close(pipa[argvc][1]);
              break;
            }

            case 3: {
              close(0);
              dup(pipa[n - 2][0]);
              close(pipa[n - 2][0]);
              close(pipa[n - 2][1]);
              break;
            }
            }

            /*TODO: EN LUGAR DE HACER UN SELECTOR, USAR EL FOR INICIAL LOL*/
            if (selector(argvv[argvc]) == 1) {
              if (execvp(argv[0], argv) == -1) {
                fprintf(stderr, "ERROR execvp");
                exit(0);
              }
            }

            exit(0);

          } else {
            exit(0);
          }
        } else {
          wait(&pid1);
        }
      }
      /*Sin Background:*/
      else {
        int seleccion = selector(argvv[argvc]);
        if (seleccion == 1) {
          pid1 = fork();
          if (pid1 == 0) {
            if (n > 1) {
              pid2 = fork();
              if (pid2 == 0) {
                int pipaGest = gestionPipes(argvc, n);
                switch (pipaGest) {
                case 1: {
                  close(1);
                  dup(pipa[0][1]);
                  close(pipa[0][0]);
                  close(pipa[0][1]);
                  break;
                }

                case 2: {
                  close(0);
                  close(1);
                  dup(pipa[argvc - 1][0]);
                  dup(pipa[argvc][1]);
                  close(pipa[argvc - 1][0]);
                  close(pipa[argvc][1]);
                  break;
                }

                case 3: {
                  close(0);
                  dup(pipa[n - 2][0]);
                  close(pipa[n - 2][0]);
                  close(pipa[n - 2][1]);
                  break;
                }
                }
                if (execvp(argv[0], argv) == -1) {
                  fprintf(stderr, "ERROR execvp");
                  exit(1);
                }
                exit(0);
              } else {
                exit(0);
              }
            } else {
              if (execvp(argv[0], argv) == -1) {
                fprintf(stderr, "ERROR execvp");
                exit(1);
              }
              exit(0);
            }
          } else {
            wait(&pid1);
          }
        }
      }
    }
  }

  exit(0);
  return 0;
}
