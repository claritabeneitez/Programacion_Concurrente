#include <pthread.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdbool.h>

sem_t sem_limite_lectores; // Semáforo de lectores simultáneos
sem_t sem_limite_escritores; // Semáforo de escritores simultáneos
sem_t sem_leer[15]; // Semáforo de cada lector para leer
sem_t sem_fin_leer[15]; // Semáforo de cada lector para finalizar de leer
sem_t sem_escribir[15]; // Semáforo de cada escritor para escribir
sem_t sem_fin_escribir[15]; // Semáforo de cada escritor para finalizar de escribir
sem_t sem_leyendo;
sem_t sem_escribiendo;
int leyendo, primero, il, escribiendo, ie = 0;
bool papel = false;

void *lectores(int *arg);

void *escritores(int *arg);

//lectores-escritores
int main (int argc,char *argv[]){
  int i;
  int N1 = atoi(argv[1]); // número de lectores total
  int N2 = atoi(argv[2]); // número máximo de lectores simultáneamente
  int N3 = atoi(argv[3]); // número de escritores total
  if (N1 < N2){
    fprintf(stdout, "N2 < N1 \n");
    exit(0);
  }
  
  pthread_t thread_id[argc];
  sem_init(&sem_limite_lectores, 0, N2); 
  // int sem_init(sem_t *sem, int pshared, unsigned int value);
  // Inicializa un semáforo
  // El primer parámetro es un puntero al semáforo que se va a inicializar
  // El segundo, es un entero que indica si se va a utilizar entre procesos (distinto de cero) o entre hilos (cero)
  // El tercero, es el valor inicial del semáforo
  sem_init(&sem_limite_escritores, 0, 1);
  sem_init(&sem_leyendo, 0, 1);
  sem_init(&sem_escribiendo, 0, 1);
  for(i = 0; i < N1; i++){
    sem_init(&sem_leer[i], 0, 0);
    sem_init(&sem_fin_leer[i],0, 0);
    pthread_create(&(thread_id[i]), NULL, (void *)lectores, (void *)(i + 1));
    // int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
    // Crear un nuevo hilo dentro de un proceso
    // El primer parámetro es un puntero al identificador del nuevo hilo
    // El segundo, es un puntero a una estructura que especifica los atributos del nuevo hilo
    // El tercero, es un puntero a la función que se ejecutará en el nuevo hilo
    // El cuarto, es un puntero a los argumentos que se pasaran al nuevo hilo
  }

  for(i = 0; i < N3; i++){
    sem_init(&sem_escribir[i], 0, 0);
    sem_init(&sem_fin_escribir[i],0, 0);
    pthread_create(&(thread_id[i]), NULL, (void *)escritores, (void *)(i + 1));
  }

  while(1){
    int opcion = 5;
    int seleccion = 0;
    fprintf(stdout, "1. Intentar leer\n");
    fprintf(stdout, "2. Finalizar leer\n");
    fprintf(stdout, "3. Intentar escribir\n");
    fprintf(stdout, "4. Finalizar escribir\n");
    fprintf(stdout, "5. Salir\n");
    scanf("%d", &opcion);
    switch(opcion){
    case 1:
      fprintf(stdout, "Introduzca el número del lector (de 1 a %d): ", N1);
      scanf("%d", &seleccion);
      sem_post(&sem_leer[seleccion]);
      // int sem_post(sem_t *sem);
      // Desbloquea un semáforo. Incrementa el valor del semáforo y despierta uno de los hilos que estén esperando por el semáforo
      break;

    case 2:
      fprintf(stdout, "Introduzca el número del lector (de 1 a %d): ", N1);
      scanf("%d", &seleccion);
      sem_post(&sem_fin_leer[seleccion]);
      break;

    case 3:
      fprintf(stdout, "Introduzca el número del escritor (de 1 a %d): ", N3);
      scanf("%d", &seleccion);
      sem_post(&sem_escribir[seleccion]);
      break;

    case 4:
      fprintf(stdout, "Introduzca el número del escritor (de 1 a %d): ", N3);
      scanf("%d", &seleccion);
      sem_post(&sem_fin_escribir[seleccion]);
      //sem_post(&sem_escribiendo);
      break;

    case 5:
      sem_destroy(&sem_limite_lectores);
      // int sem_destroy(sem_t *sem);
      // Destruye un semáforo y libera los recursos 
      sem_destroy(&sem_limite_escritores);
      exit(0);
      break;
    }
  }
  return 0;
}

void *lectores(int *arg){
  int lector = (int)arg; // initialization makes integer from pointer without a cast
  while(1){
    fprintf(stdout, "[Lector %d] -> Esperando a intentar leer...\n", lector);
    sem_wait(&sem_leer[lector]); // esperando a teclado
    // int sem_wait(sem_t *sem);
    // Bloquea un semáforo. Si el valor actual del semáforo es mayor que cero decrementa el valor del semáforo y continua la ejecución del hilo; si el valor actual es cero el hilo se suspende hasta que aumente el valor 
    fprintf(stdout, "[Lector %d] -> Intentando leer...\n", lector);
    sem_wait(&sem_limite_lectores); // N2 lectores leyendo concurrentemente, limitamos concurrencia antes a no superar el limite de lectores que a comprobar si el papel está escribiéndose (sino habria que comprobarlo dos veces)
    if (leyendo == 0 && papel == true){ // papel ocupado por un escritor
      il++; // lectores intentado leer
      sem_wait(&sem_escribiendo);
      if (leyendo < il){
	il--;
	sem_post(&sem_escribiendo); // cuando el papel deja de estar ocupado por un escritor, cada lector permite el paso de su siguiente que estaba esperando excepto el último
      }
    }    
    leyendo++;
    if (leyendo == 1){
      primero = lector;
    }
    fprintf(stdout, "[Lector %d] -> Leyendo...\n", lector);
    papel = true;
    if(leyendo != 0 && lector == primero){ // lector ganador bloquea el papel para los escritores
      sem_wait(&sem_leyendo);
    }
    sem_wait(&sem_fin_leer[lector]); // esperando a teclado
    fprintf(stdout, "[Lector %d] -> Fin lectura\n", lector);
    sem_post(&sem_limite_lectores);
    leyendo--;
    if(leyendo == 0){
      papel = false;
      il=0;
      sem_post(&sem_leyendo); // último lector desbloquea el papel para los escritores
    }
  }
  pthread_exit(NULL);
  // Forma segura de terminar con la ejecución de un hilo. Permite a otros hilos continuar ejecutando sin verse afectados por la terminación del hilo actual.
  return 0;
}

void *escritores(int *arg){
  int escritor = (int)arg;
  while(1){
    fprintf(stdout, "[Escritor %d] -> Esperando a intentar escribir...\n", escritor);
    sem_wait(&sem_escribir[escritor]); // esperando a teclado
    fprintf(stdout, "[Escritor %d] -> Intentando escribir...\n", escritor);
    ie++; // escritores intentando escribir
    sem_wait(&sem_limite_escritores); // 1 escritor escribiendo concurrentemente
    if(papel == true) {
      sem_wait(&sem_leyendo);
    }
    if(escribiendo == 0){ // primer escritor debe bloquear el semáforo para los lectores
      sem_wait(&sem_escribiendo);
    }
    escribiendo++;
    fprintf(stdout, "[Escritor %d] -> Escribiendo...\n", escritor);
    papel = true;
    sem_wait(&sem_fin_escribir[escritor]); // esperando a teclado
    fprintf(stdout, "[Escritor %d] -> Fin escritura\n", escritor);
    papel = false;
    sem_post(&sem_limite_escritores);
    ie--;
    if (ie == 0){ // prioridad escritores en contienda
      escribiendo = 0;
      sem_post(&sem_escribiendo);
    }
  }
  pthread_exit(NULL);
  return 0;
}

