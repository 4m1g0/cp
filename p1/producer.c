
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int num_producers = 2;
int num_consumers = 2;
int buffer_size = 1;
int iterations = 100;
int max_time = 100000;

static struct option long_options[] = {
	{ .name = "producers",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "consumers",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "buffer_size",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "iterations",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{ .name = "max_time",
	  .has_arg = required_argument,
	  .flag = NULL,
	  .val = 0},
	{0, 0, 0, 0}
};

static void usage(int i)
{
	printf(
		"Usage:  producers [OPTION] [DIR]\n"
		"Launch producers and consumers\n"
		"Opciones:\n"
		"  -p n, --producers=<n>: number of producers\n"
		"  -c n, --consumers=<n>: number of consumers\n"
		"  -b n, --buffer_size=<n>: number of elements in buffer\n"
		"  -i n, --iterations=<n>: total number of iterations\n"
		"  -m n, --max_time=<n>: max time a thread can wait\n"
		"  -h, --help: muestra esta ayuda\n\n"
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

	if (!strcmp(option.name, "producers")) {
		if (!get_int(arg, &num_producers)
		    || num_producers <= 0) {
			printf("'%s': no es un entero válido\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "consumers")) {
		if (!get_int(arg, &num_consumers)
		    || num_consumers <= 0) {
			printf("'%s': no es un entero válido\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "buffer_size")) {
		if (!get_int(arg, &buffer_size)
		    || buffer_size <= 0) {
			printf("'%s': no es un entero válido\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "iterations")) {
		if (!get_int(arg, &iterations)
		    || iterations <= 0) {
			printf("'%s': no es un entero válido\n", arg);
			usage(-3);
		}
	}
	if (!strcmp(option.name, "max_time")) {
		if (!get_int(arg, &max_time)
		    || iterations <= 0) {
			printf("'%s': no es un entero válido\n", arg);
			usage(-3);
		}
	}
}

static int handle_options(int argc, char **argv)
{
	while (1) {
		int c;
		int option_index = 0;

		c = getopt_long (argc, argv, "hp:c:b:i:m:",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			handle_long_options(long_options[option_index],
				optarg);
			break;

		case 'p':
			if (!get_int(optarg, &num_producers)
			    || num_producers <= 0) {
				printf("'%s': no es un entero válido\n",
				       optarg);
				usage(-3);
			}
			break;


		case 'c':
			if (!get_int(optarg, &num_consumers)
			    || num_consumers <= 0) {
				printf("'%s': no es un entero válido\n",
				       optarg);
				usage(-3);
			}
			break;

		case 'b':
			if (!get_int(optarg, &buffer_size)
			    || buffer_size <= 0) {
				printf("'%s': no es un entero válido\n",
				       optarg);
				usage(-3);
			}
			break;

		case 'i':
			if (!get_int(optarg, &iterations)
			    || iterations <= 0) {
				printf("'%s': no es un entero válido\n",
				       optarg);
				usage(-3);
			}
			break;
			
	    case 'm':
			if (!get_int(optarg, &max_time)
			    || max_time <= 0) {
				printf("'%s': no es un entero válido\n",
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

struct element {
	int producer;
	int value;
	int time;
};

struct element **elements = NULL;
int count = 0;

void insert_element(struct element *element)
{
	if (count == buffer_size) {
		printf("buffer is full\n");
		exit(-1);
	}
	elements[count] = element;
	count++;
}

struct element *remove_element(void)
{
	if (count == 0) {
		printf("buffer is empty\n");
		exit(-1);
	}
	count--;
	return elements[count];
}


pthread_cond_t buffer_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_empty = PTHREAD_COND_INITIALIZER;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_full_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_empty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t consumptions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t productions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t suicide_mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_info {    /* Used as argument to thread_start() */
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       thread_num;       /* Application-defined thread # */
};

int productions = 0;
int consumptions = 0;
int num_full = 0, num_empty = 0;
int suicide;

void *producer_function(void *ptr)
{
	struct thread_info *t =  ptr;
	unsigned int seed = t->thread_num;

	printf("producer thread %d\n", t->thread_num);
	while(1) {
	    pthread_mutex_lock(&productions_mutex);
        
        // comprobamos si ya se han producido todos los elementos
	    if (productions == iterations)
	    {
	        pthread_mutex_unlock(&productions_mutex);
	        printf("%d: producer sale\n", t->thread_num);
	        break;
	    }
	    productions++;
	    pthread_mutex_unlock(&productions_mutex);
	    
		struct element *e = malloc(sizeof(*e));
		if (e == NULL) {
			printf("Out of memory");
			exit(-1);
		}
		e->producer = t->thread_num;
		
		e->value = rand_r(&seed) % 1000;
		e->time = rand_r(&seed) % max_time;
		usleep(e->time);
		printf("%d: produces %d in %d microseconds\n", t->thread_num, e->value, e->time);
		
		pthread_mutex_lock(&buffer_mutex);
		
		while(count == buffer_size) {
			pthread_cond_wait(&buffer_full, &buffer_mutex);
		}

		insert_element(e);
		if(count==1)
			pthread_cond_broadcast(&buffer_empty);
			
		pthread_mutex_unlock(&buffer_mutex);
	}
	return NULL;
}

void *consumer_function(void *ptr)
{
	struct thread_info *t =  ptr;
	printf("consumer thread %d\n", t->thread_num);
	
	while(1) {
	    // comprobamos si debe suicidarse
	    pthread_mutex_lock(&suicide_mutex);
	    if (suicide)
	    {
	        pthread_mutex_unlock(&suicide_mutex);
	        printf("%d: consumer se suicida\n", t->thread_num);
	        suicide = 0;
	        break;
	    }
	    pthread_mutex_unlock(&suicide_mutex);
	    
	    // comprobamos si ya se han consumido todos los elementos
	    pthread_mutex_lock(&consumptions_mutex);
	    if (consumptions == iterations)
	    {
	        pthread_mutex_unlock(&consumptions_mutex);
	        printf("%d: consumer se sale\n", t->thread_num);
	        break;
	    }
	    consumptions++;
	    pthread_mutex_unlock(&consumptions_mutex);
	
		struct element *e;
		
		pthread_mutex_lock(&buffer_mutex);
		if (count == 0)
		{
		    pthread_mutex_lock(&num_empty_mutex);
		    num_empty++;
		    pthread_mutex_unlock(&num_empty_mutex);
		}
		while(count==0)
			pthread_cond_wait(&buffer_empty, &buffer_mutex);

		e = remove_element();
		
		if(count == buffer_size -1)
		{
			pthread_cond_broadcast(&buffer_full);
			pthread_mutex_lock(&num_full_mutex);
		    num_full++;
		    pthread_mutex_unlock(&num_full_mutex);
	    }
		pthread_mutex_unlock(&buffer_mutex);
        
        usleep(e->time);
        
		printf("%d: consumes %d\n", t->thread_num, e->value);
		free(e);
	}
	return NULL;
}

void *timer_function(void *n)
{
    int create_consumer = 0;
    while(1)
    {
        sleep(1); // esperamos 1 segundo
        create_consumer = 0;
        
        // Este mutex no es necesario, si coincidiese que alguien lo esta actualizando la unica consecuencia es que haria una iteracion más y se saldria.
        // pthread_mutex_lock(&consumptions_mutex);
        if (consumptions == iterations) // ya se han consumido todos
        {
            // pthread_mutex_unlock(&consumptions_mutex);
            printf("Timer se sale\n");
            break;
        }
        // pthread_mutex_unlock(&consumptions_mutex);
        
        
        printf("Timer comprueba estado de saturación %d\n", num_full);
        pthread_mutex_lock(&num_full_mutex);
        if (num_full > 10) // si se ha llenado mas de 10 veces creamos un productor nuevo
        {
            create_consumer = 1;
            num_consumers++; // esta variable solo la usa un thread, por lo tanto no va a haber accesos concurrentes
        }

        num_full = 0;
        pthread_mutex_unlock(&num_full_mutex);
        
        // comprobamos si debemos matar un consumidor, teniendo en cuenta dejar siempre al menos 1
        pthread_mutex_lock(&num_empty_mutex);
        if (num_empty > 10 && num_consumers > 1)
        {
            pthread_mutex_lock(&suicide_mutex);
            suicide = 1;
            pthread_mutex_unlock(&suicide_mutex);
            num_consumers--; // esta variable solo la usa un thread, por lo tanto no va a haber accesos concurrentes
        }
        num_empty = 0;
        pthread_mutex_unlock(&num_empty_mutex);
        
        
        if (create_consumer)
        {
            struct thread_info *consumer_info = malloc(sizeof(struct thread_info));
            
            if (consumer_info == NULL) {
		        printf("Not enough memory\n");
		        exit(1);
	        }
	        
	        consumer_info->thread_num = num_consumers++;
	        if ( 0 != pthread_create(&consumer_info->thread_id, NULL, consumer_function, consumer_info)) {
			    printf("Failing creating consumer thread %d", consumer_info->thread_num);
			    exit(1);
		    }
		    
	        printf("creating consumer n %d\n", consumer_info->thread_num);
	    }
    }
    return NULL;
}

void producers_consumers(num_producers, num_consumers)
{
	int i;
	struct thread_info *producer_infos;
	struct thread_info *consumer_infos;

	printf("creating buffer with %d elements\n", buffer_size);
	elements = malloc(sizeof(struct element) * buffer_size);

	if (elements == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	printf("creating %d producers\n", num_producers);
	producer_infos = malloc(sizeof(struct thread_info) * num_producers);

	if (producer_infos == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	/* Create independent threads each of which will execute function */
	for (i = 0; i < num_producers; i++) {
		producer_infos[i].thread_num = i;
		if ( 0 != pthread_create(&producer_infos[i].thread_id, NULL,
					 producer_function, &producer_infos[i])) {
			printf("Failing creating consumer thread %d", i);
			exit(1);
		}
	}

	printf("creating %d consumers\n", num_consumers);
	consumer_infos = malloc(sizeof(struct thread_info) * num_consumers);

	if (consumer_infos == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	for (i = 0; i < num_consumers; i++) {
		consumer_infos[i].thread_num = i;
		if ( 0 != pthread_create(&consumer_infos[i].thread_id, NULL,
					 consumer_function, &consumer_infos[i])) {
			printf("Failing creating consumer thread %d", i);
			exit(1);
		}
	}
	printf("creating timer\n");
	struct thread_info timer_info;
	if ( 0 != pthread_create(&timer_info.thread_id, NULL,
					 timer_function, NULL)) {
			printf("Failing creating timer thread");
			exit(1);
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

	producers_consumers(num_producers, num_consumers);

	exit (0);
}
