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
 * -> Metacaracteres
 * -> Expansión de Variable
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
  char cwd[1024];
  int resultado;
  if (direccion) {
    resultado = chdir(direccion);
  } else {
    resultado = chdir(home);
  }
  if (resultado == 0) {
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
    } else {
      perror("getcwd");
      return -1;
    }
  } else {
    perror("cd");
  }
  return resultado;
}

int mascara(char *entrada) {

  mode_t mascAnt = umask(0);
  if (!entrada) {
    umask(mascAnt);
    return mascAnt;
  }
  int octal;
  char *comprobacion;
  octal = strtol(entrada, &comprobacion, 8);
  if (entrada == comprobacion) {
    printf("Numero fuera de rango \n");
    umask(mascAnt);
    return mascAnt;
  }
  mascAnt = umask(octal);
  return mascAnt;
}

int limites() {
  struct rlimit *limite = malloc(2 * sizeof(rlim_t));
  getrlimit(RLIMIT_CPU, limite);
  fprintf(stdout, "cpu\t%ld\n", limite->rlim_cur);
  getrlimit(RLIMIT_FSIZE, limite);
  fprintf(stdout, "fsize\t%ld\n", limite->rlim_cur);
  getrlimit(RLIMIT_DATA, limite);
  fprintf(stdout, "data\t%ld\n", limite->rlim_cur);
  getrlimit(RLIMIT_STACK, limite);
  fprintf(stdout, "stack\t%ld\n", limite->rlim_cur);
  getrlimit(RLIMIT_CORE, limite);
  fprintf(stdout, "core\t%ld\n", limite->rlim_cur);
  getrlimit(RLIMIT_NOFILE, limite);
  fprintf(stdout, "nofile\t%ld\n", limite->rlim_cur);
  free(limite);
  return 0;
}

int limit(int recurso, char *argv, char *entrada) {
  int resultado = 0;
  struct rlimit limite;

  if (entrada == NULL) {
    if (getrlimit(recurso, &limite) == -1) {
      perror("getrlimit");
      return -1;
    }
    fprintf(stdout, "%s\t%ld\n", argv, (long)limite.rlim_cur);
  } else {
    rlim_t lmt;
    if (strcmp(entrada, "-1") == 0) {
      lmt = RLIM_INFINITY;
    } else {
      lmt = atoi(entrada);
    }
    if (getrlimit(recurso, &limite) == -1) {
      perror("getrlimit");
      return -1;
    }
    limite.rlim_cur = lmt;

    if (setrlimit(recurso, &limite) == -1) {
      perror("setrlimit");
      return -1;
    }
    resultado = 0;
  }
  return resultado;
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
    return setenv(variable, valor, 1);
  }
  return 0;
}

int selector(char **argv) {
  if (strcmp(argv[0], "cd") == 0) {
    return cd(argv[1]);
  } else if (strcmp(argv[0], "umask") == 0) {
    if (argv[1] == NULL || argv[2] == NULL) {
      int a = mascara(argv[1]);
      printf("%o\n", a);
      return 0;
    } else {
      printf("Mas de un argumento en la máscara\n");
      return -1;
    }
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
  mypid = getpid();
  char pidmsh[16];
  sprintf(pidmsh, "%d", mypid);
  setenv("mypid", pidmsh, 1);

  sigset_t mask;
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  int entrada_est = dup(0);
  int salida_est = dup(1);
  int salida_err = dup(2);
  int rediren = 0;
  int redirsal = 0;
  int redirerr = 0;
  int error_redir;

  char ***argvv = NULL;
  int argvc;
  char **argv = NULL;
  int argc;
  char *filev[3] = {NULL, NULL, NULL};
  int bg;
  int ret;
  int status = 0;
  int statussal;
  home = getenv("HOME");
  set("prompt", "msh> ");

  setbuf(stdout, NULL); /* Unbuffered */
  setbuf(stdin, NULL);

  while (1) {
    error_redir = 0;
    char statusc[16];
    sprintf(statusc, "%d", status);
    setenv("status", statusc, 1);
    char bgpidc[16];
    sprintf(bgpidc, "%d", bgpid);
    setenv("bgpid", bgpidc, 1);
    /*TODO: Cambiar el modo de funcionar del promt.*/
    // No sé si está terminado o no, tendría que revisar la documentación, pero
    // es una aproximación «funcional»

    prompt = getenv("prompt");
    fprintf(stderr, "%s", prompt);

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

    int pipa[n][2];
    for (int i = 0; i < n - 1; i++) {
      pipe(pipa[i]);
    }

    /*TODO: Gestionar fallo a la hora de redireccionar, ignorando el resto del
     * mensaje y dandole información al usuario. */
    if (filev[0] != NULL) {
      int fd = open(filev[0], O_RDONLY);
      if (fd > 0) {
        close(0);
        dup(fd);
        close(fd);
        rediren = 1;
      } else {
        perror("Error en la redireccion de la entrada.\n");
        error_redir = 1;
      }
    }
    if (filev[1] != NULL) {
      int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd > 0) {
        close(1);
        dup(fd);
        close(fd);
        redirsal = 1;
      } else {
        perror("Error en la redireccion de la salida estandar.\n");
        error_redir = 1;
      }
    }
    if (filev[2] != NULL) {
      int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd > 0) {
        close(2);
        dup(fd);
        close(fd);
        redirerr = 1;
      } else {
        perror("Error en la redireccion de la salida de error\n");
        error_redir = 1;
      }
    }
    if (error_redir == 0) {
      if (bg) {
        for (argvc = 0; (argv = argvv[argvc]); argvc++) {
          pid_t pid1, pid2;
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

              if (selector(argvv[argvc]) == 1) {
                if (execvp(argv[0], argv) == -1) {
                  fprintf(stderr, "ERROR execvp");
                  exit(-1);
                }
              }

              exit(0);

            } else {
              bgpid = pid2;
              printf("[%d]\n", bgpid);
              exit(0);
            }
          } else {
            /*TODO: Yo sé que esto es erróneo probablemente, pero si te soy
             * sincero, de momento funciona.*/
            bgpid = pid1 + 1;
            wait(&pid1);
          }
        }

      } else {
        pid_t pids[n];
        for (argvc = 0; (argv = argvv[argvc]); argvc++) {
          /*TODO: Metacaracteres*/
          if (n > 1) {
            pid_t pid1 = fork();
            if (pid1 == 0) {
              sigset_t mascproc;
              sigemptyset(&mascproc);
              sigprocmask(SIG_SETMASK, &mascproc, NULL);

              int pipaGest = gestionPipes(argvc, n);
              switch (pipaGest) {
              case 1: {
                close(1);
                dup(pipa[0][1]);
                for (int i = 0; i < n - 1; i++) {
                  close(pipa[i][0]);
                  close(pipa[i][1]);
                }
                break;
              }

              case 2: {
                close(0);
                close(1);
                dup(pipa[argvc - 1][0]);
                dup(pipa[argvc][1]);
                for (int i = 0; i < n - 1; i++) {
                  close(pipa[i][0]);
                  close(pipa[i][1]);
                }
                break;
              }

              case 3: {
                close(0);
                dup(pipa[n - 2][0]);
                for (int i = 0; i < n - 1; i++) {
                  close(pipa[i][0]);
                  close(pipa[i][1]);
                }
                break;
              }
              } // termina el switch

              int seleccion = selector(argvv[argvc]);
              status = seleccion;
              if (seleccion == 1) {
                if (execvp(argv[0], argv) == -1) {
                  fprintf(stderr, "ERROR execvp\n");
                  exit(-1);
                }
                exit(0);
              } else {
                exit(status);
              }
            } else {
              pids[argvc] = pid1;
            }
          } else {
            int seleccion = selector(argvv[argvc]);
            status = seleccion;
            if (seleccion == 1) {
              pid_t pid1 = fork();
              if (pid1 == 0) {
                sigset_t mascproc;
                sigemptyset(&mascproc);
                sigprocmask(SIG_SETMASK, &mascproc, NULL);
                if (execvp(argv[0], argv) == -1) {
                  fprintf(stderr, "ERROR execvp\n");
                  exit(-1);
                }
                exit(0);
              } else {
                pids[argvc] = pid1;
              }
            } else {
              pids[argvc] = -1;
            }
          }
        }
        for (int i = 0; i < n - 1; i++) {
          close(pipa[i][0]);
          close(pipa[i][1]);
        }
        if (n > 0 && pids[n - 1] != -1) {
          waitpid(pids[n - 1], &statussal, 0);
          status = WEXITSTATUS(statussal);
        }
      }
    }
    if (rediren) {
      close(0);
      dup(entrada_est);
      rediren = 0;
    }
    if (redirsal) {
      close(1);
      dup(salida_est);
      redirsal = 0;
    }
    if (redirerr) {
      close(2);
      dup(salida_err);
      redirerr = 0;
    }
  }

  exit(0);
  return 0;
}
