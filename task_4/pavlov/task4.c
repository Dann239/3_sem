#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>

double getrand(struct drand48_data* buff) { //gives a random evenly distributed value
    double res;
    drand48_r(buff, &res);
    return res;
}

double function(double x) { //the function itself: sin(sqrt(x)) + 1
    double rt = sqrt(x);
    if(rt == 0)
        return 0;
    else
        return sin(rt) + 1;
}

double antiderivative(double x) { //the antiderivative for checking
    return x + 2 * sin(sqrt(x)) - 2 * sqrt(x) * cos(sqrt(x));
}

struct monte_arg {
    double x, dx, y;
    long long n;
};

double result;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *monte_integral(void* _arg) { //the monte-carlo function itself
    struct monte_arg* arg = _arg;
    struct drand48_data buff;
    srand48_r(rand(), &buff);
    long long hits = 0;
    for(long long i = 0; i < arg->n; i++)
        if(function(arg->x + arg->dx * getrand(&buff)) > getrand(&buff) * arg->y)
            hits++;
    double res = arg->dx * arg->y * hits / arg->n;
    pthread_mutex_lock(&mutex);
    result += res;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

double analytic_integral(double x, double dx) { //analytic integral for checking
    return antiderivative(x+dx) - antiderivative(x);
}

long long get_time() { //get current system time in milliseconds
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
}

int fd_out = 0;
double x0 = 0, dx = 16, ymax = 2;
void measure(long long thread_num, long long nsteps) { //perform a measurement for a given amount of threads and steps
    long long t = get_time();
    result = 0;

    pthread_t *threads = malloc(thread_num * sizeof(pthread_t));
    struct monte_arg *args = malloc(thread_num * sizeof(struct monte_arg));
    for(long long i = 0; i < thread_num; i++) {
        args[i].y = ymax;
        args[i].x = x0 + dx * i / thread_num;
        args[i].dx = dx / thread_num;
        args[i].n = nsteps / thread_num;
    }

    for(long long i = 0; i < thread_num; i++)
        pthread_create(&threads[i], NULL, monte_integral, &args[i]);
    for(long long i = 0; i < thread_num; i++)
        pthread_join(threads[i], NULL);

    free(threads);
    free(args);
    long long dt = get_time() - t;
    printf("threads: %lld; time: %lld ms; nsteps: %lld; analytic: %lf; monte: %lf\n", thread_num, dt, nsteps, analytic_integral(x0, dx), result);
    dprintf(fd_out, "%lld\t%lld\t%lld\n", thread_num, dt, nsteps);

}

long long min(long long a, long long b) { //min(a,b)
    return a < b ? a : b;
}

int main() {
    long long nsteps[11] = {294053760, 61261200, 10810800, 7207200, 3603600, 1441440, 720720, 554400, 277200, 110880, 55440}; //anti-primes with a large amount of divisors
    char* names[11] = {"table3e8.csv", "table6e7.csv", "table1e7.csv", "table7e6.csv", "table4e6.csv", "table1e6.csv", "table7e5.csv", "table6e5.csv", "table3e5.csv", "table1e5.csv", "table6e4.csv"};
    for(int n = 0; n < 11; n++) {
        fd_out = open(names[n], O_WRONLY | O_CREAT, 0777);
        for(long long i = 1; i < 32000; i++)
            if(nsteps[n] % i == 0)
                measure(i, nsteps[n]);
    }
}
