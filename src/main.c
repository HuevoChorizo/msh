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

#include <stddef.h> /* NULL */
#include <stdio.h>  /* setbuf, printf */
#include <stdlib.h>
#include <string.h>/*strcmp, strlen,...*/
#include <sys/stat.h>
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

int set();

/*TODO: método switch (o switch dentro del main) para seleccionar los otros
 * métodos*/

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
    fprintf(stderr, ansigreen "☭ > " ansireset);

    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      break; /* EOF */
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)
      continue; /* Empty line */
#if 1

    /*TODO: el cd*/

    /*
     * LAS LINEAS QUE A CONTINUACION SE PRESENTAN SON SOLO
     * PARA DAR UNA IDEA DE COMO UTILIZAR LAS ESTRUCTURAS
     * argvv Y filev. ESTAS LINEAS DEBERAN SER ELIMINADAS.
     */
    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      if (strcmp(argv[0], "cd") == 0) {
        int a = cd(argv[1]);
        if (a == -1)
          printf("Directorio inválido");
      } else if (strcmp(argv[0], "umask") == 0) {
        int a = mascara(argv[1]);
        printf("%o\n", a);
      }
      for (argc = 0; argv[argc]; argc++) {
        // printf("%s ", argv[argc]);
      }
      printf("\n");
    }
  }
  if (filev[1])
    printf("> %s\n", filev[1]); /* OUT */
  if (filev[2])
    printf(">& %s\n", filev[2]); /* ERR */
  if (bg)
    printf("&\n");
/*
 * FIN DE LA PARTE A ELIMINAR
 */
#endif
  exit(0);
  return 0;
}
