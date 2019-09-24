#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double getrand() {
    return drand48();
}

double function(double x) {
    double rt = sqrt(x);
    if(rt == 0)
        return 0;
    else
        return sin(rt) + 1;
}

double antiderivative(double x) {
    return x + 2 * sin(sqrt(x)) - 2 * sqrt(x) * cos(sqrt(x));
}

double monte_integral(double x, double dx, double y, int n) {
    int hits = 0;
    for(int i = 0; i < n; i++)
        if(function(x + dx * getrand()) > getrand() * y)
            hits++;
    return dx * y * hits / n;
}

double dumb_integral(double x, double dx, int n) {
    double res = 0;
    for(int i = 0; i < n; i++)
        res += function(x + dx * i / n) * dx / n;
    return res;
}

double analytic_integral(double x, double dx){
    return antiderivative(x+dx) - antiderivative(x);
}

int main() {
    printf("%f\n", analytic_integral(0, 16));
    printf("%f\n", dumb_integral(0, 16, 5e8));
    printf("%f\n", monte_integral(0, 16, 2, 5e8));
}