
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "queue.h"

int num_chairs = 5;
int num_barbers = 5;
int num_customers = 100;
int max_waiting_time = 1000000;
int choosy_percent = 30;

static struct option long_options[] = {
	{ .name = "chairs",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "barbers",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "customers",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "max_waiting_time",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "choosy_percent",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = NULL,
	  .has_arg = 0,
	  .flag = NULL,
	  .val = 0}
};

static void usage(int i)
{
	printf(
		"Usage:  barber [OPTION] [DIR]\n"
		"Launch barbers and customers\n"
		"Opciones:\n"
		"  -b n, --barbers=<n>: number of barbers\n"
		"  -c n, --chairs=<n>: number of chairs\n"
		"  -n n, --clients=<n>: number of customers\n"
		"  -t n, --max_waiting_time=<n>: maximum time a customer will wait\n"
		"  -p n, --choosy-percent=<n>: percentage of customers that want a specific barber\n"
		"  -h, --help: show this help\n\n"
	);
	exit(i);
}

static int get_int(char *arg, int *value)
{
	char *end;
	*value = strtol(arg, &end, 10);

	return (end != NULL);
}

static void handle_long_options(struct option option, char *arg)
{
	if (!strcmp(option.name, "help"))
		usage(0);

	if (!strcmp(option.name, "barbers")) {
		if (!get_int(arg, &num_barbers)
		    || num_barbers <= 0) {
			printf("'%s': not a valid integer\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "chairs")) {
		if (!get_int(arg, &num_chairs)
		    || num_chairs <= 0) {
			printf("'%s': not a valid integer\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "customers")) {
		if (!get_int(arg, &num_customers)
		    || num_customers <= 0) {
			printf("'%s': not a valid integer\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "max_waiting_time")) {
		if (!get_int(arg, &max_waiting_time)
		    || max_waiting_time <= 0) {
			printf("'%s': not a valid integer\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "choosy_percent")) {
		if (!get_int(arg, &choosy_percent)
		    || choosy_percent <= 0) {
			printf("'%s': not a valid integer\n", arg);
			usage(-3);
		}
	}
}

static int handle_options(int argc, char **argv)
{
	while (1) {
		int c;
		int option_index = 0;

		c = getopt_long (argc, argv, "hb:c:n:t:p:",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			handle_long_options(long_options[option_index],
				optarg);
			break;

		case 'b':
			if (!get_int(optarg, &num_barbers)
			    || num_barbers <= 0) {
				printf("'%s': not a valid integer\n",
				       optarg);
				usage(-3);
			}
			break;

		case 'c':
			if (!get_int(optarg, &num_chairs)
			    || num_chairs <= 0) {
				printf("'%s': not a valid integer\n",
				       optarg);
				usage(-3);
			}
			break;

		case 'n':
			if (!get_int(optarg, &num_customers)
			    || num_customers <= 0) {
				printf("'%s': not a valid integer\n",
				       optarg);
				usage(-3);
			}
			break;

		case 't':
			if (!get_int(optarg, &max_waiting_time)
			    || max_waiting_time <= 0) {
				printf("'%s': not a valid integer\n",
				       optarg);
				usage(-3);
			}
			break;
		case 'p':
			if (!get_int(optarg, &choosy_percent)
			    || choosy_percent <= 0) {
				printf("'%s': not a valid integer\n",
				       optarg);
				usage(-3);
			}
			break;
		case '?':
		case 'h':
			usage(0);
			break;

		default:
			printf ("?? getopt returned character code 0%o ??\n", c);
			usage(-1);
		}
	}
	return 0;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct barber_info {    /* Used as argument to thread_start() */
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       barber_num;
};

struct customer_info {    /* Used as argument to thread_start() */
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       customer_num;
};

int served_customers = 0;
int refused_customers = 0;
int salir = 0;
Queue customers_queue;
Queue barbers_queue;

unsigned int seed = 12345678;

void cut_hair(int barber, int customer)
{
	unsigned int time = rand_r(&seed) % 500000;
	// time that takes to do the haircut
	usleep(time);

	printf("barber <%d> cut hair of customer <%d> took %d time\n", barber, customer, time);
}

void *barber_function(void *ptr)
{
	struct barber_info *t =  ptr;

	printf("barber thread %d\n", t->barber_num);

	while(1) {
		pthread_mutex_lock(&mutex);
		if (salir)
		{
		    pthread_mutex_unlock(&mutex);
		    printf("Barber %d se va a su casa.\n", t->barber_num);
		    return NULL;
		}
		if (IsEmpty(customers_queue)) {
		    if (served_customers + refused_customers == num_customers) // si ya estan todos atendidos se sale
		    {
		        salir = 1;
		        printf("Clientes que se fueron: %d, clientes atendidos: %d\n", refused_customers, served_customers);
		        while (!IsEmpty(barbers_queue)) // despertamos a todos los barberos para cerrar la barberia
		            Dequeue(barbers_queue);
		        
		        pthread_mutex_unlock(&mutex);
		        return NULL;
		    }
			printf("barber %d goes to sleep\n", t->barber_num);
			Enqueue(t->barber_num, barbers_queue, &mutex, -1, t->barber_num); // nos metemos en la cola a dormir
			if (salir)
		    {
		        pthread_mutex_unlock(&mutex);
		        printf("Barber %d goes home\n", t->barber_num);
		        return NULL;
		    }
		}
		int customer = FrontAndDequeue(customers_queue, t->barber_num); // despertamos al primero de la cola
		printf("barber %d despierta al customer %d\n", t->barber_num, customer);
		
		if (customer == -1) // caso especial, hay gente esperando pero nadie quiere que le cortes el pelo tu, nos vamos a dormir y cuando llege un cliente nuevo volvemos a comprobar desde el principio.
		{
		    printf("barber %d goes to sleep because nobody wants to get his hair cut with him :(\n", t->barber_num);
			Enqueue(t->barber_num, barbers_queue, &mutex, -1, t->barber_num); // nos metemos en la cola a dormir
			printf("barber %d despierta\n", t->barber_num);
			pthread_mutex_unlock(&mutex);
			continue;
		}
		
		served_customers++;
		pthread_mutex_unlock(&mutex);
		cut_hair(t->barber_num, customer);
	}
}

void get_hair_cut(int i)
{
	printf("customer %d is getting a hair cut\n", i);
}

void *customer_function(void *ptr)
{
	struct customer_info *t =  ptr;
	unsigned int time = rand_r(&seed) % 100000 + t->customer_num * 100000;
	int favourite = -1;
	int wait_time = rand_r(&seed) % max_waiting_time;
	
	if (rand_r(&seed) % 100 < choosy_percent)
	{
	    favourite = rand_r(&seed) % num_barbers;
	}

	// time that takes until it arrives to the barber shop
	usleep(time);

	printf("customer thread %d arrives in %d and wants to get his hair cut from barber %d in %d\n", t->customer_num, time, favourite, wait_time);

	pthread_mutex_lock(&mutex);
	if(IsFull(customers_queue))  {
		printf("waiting room full for customer %d\n", t->customer_num);
		refused_customers++;
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	if(!IsEmpty(barbers_queue))
	{
		int barbero = FrontAndDequeue(barbers_queue, favourite);
		if (barbero != -1)
		    favourite = barbero; // si despertamos a alguien queremos que sea ese el que nos corte el pelo (que no se meta otro por el medio)
		
		printf("customer %d despierta al barbero %d para que le corte el pelo y lo marca como favorito: %d\n", t->customer_num, barbero, favourite);
    }
    else
    {
        printf("customer %d se encuentra con todos los barberos ocupados\n", t->customer_num);
    }
	
    // nos metemos en la cola a esperar a que el barbero nos atienda
    if (Enqueue(t->customer_num, customers_queue, &mutex, wait_time, favourite))
    {
        refused_customers++;
        printf("vamos a borrar a %d\n", t->customer_num);
        Delete(t->customer_num, customers_queue);
        pthread_mutex_unlock(&mutex);
        printf("customer %d is furious and leave\n", t->customer_num);
        return NULL;
    }
    
	pthread_mutex_unlock(&mutex);
	get_hair_cut(t->customer_num);

	return NULL;
}

void create_threads(void)
{
	int i;
	struct barber_info *barber_info;
	struct customer_info *customer_info;
	
	// creamos la cola de clientes
	customers_queue = CreateQueue(num_chairs);
	barbers_queue = CreateQueue(num_barbers);

	printf("creating %d barbers\n", num_barbers);
	barber_info = malloc(sizeof(struct barber_info) * num_barbers);

	if (barber_info == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	/* Create independent threads each of which will execute function */
	for (i = 0; i < num_barbers; i++) {
		barber_info[i].barber_num = i;
		if ( 0 != pthread_create(&barber_info[i].thread_id, NULL,
					 barber_function, &barber_info[i])) {
			printf("Failing creating barber thread %d", i);
			exit(1);
		}
	}

	printf("creating %d consumers\n", num_customers);
	customer_info = malloc(sizeof(struct customer_info) * num_customers);

	if (customer_info == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	for (i = 0; i < num_customers; i++) {
		customer_info[i].customer_num = i;
		if ( 0 != pthread_create(&customer_info[i].thread_id, NULL,
					 customer_function, &customer_info[i])) {
			printf("Failing creating customer thread %d", i);
			exit(1);
		}
	}
	pthread_exit(0);
}

int main (int argc, char **argv)
{
	int result = handle_options(argc, argv);

	if (result != 0)
		exit(result);

	if (argc - optind != 0) {
		printf ("Extra arguments\n\n");
		while (optind < argc)
			printf ("'%s' ", argv[optind++]);
		printf ("\n");
		usage(-2);
	}

	create_threads();

	exit (0);
}
