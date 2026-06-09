#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#   define M_PI 3.1415926535897932384626433832
#endif

constexpr double power(double base, int exp) {
    double res = 1.0;
    for (int i = 0; i < exp; ++i) {
        res *= base;
    }
    return res;
}

constexpr long long factorial(int n) {
    long long res = 1;
    for (int i = 2; i <= n; ++i) {
        res *= i;
    }
    return res;
}

constexpr double constexpr_sin(double x) {
    if (x > M_PI) x -= 2 * M_PI;
    if (x < -M_PI) x += 2 * M_PI;

    double result = 0.0;

    for (int n = 0; n < 10; ++n) {
        double term = power(x, 2 * n + 1) / factorial(2 * n + 1);
        if (n % 2 == 1) {
            result -= term;
        }
        else {
            result += term;
        }
    }
    return result;
}

constexpr double constexpr_cos(double x) {
    if (x > M_PI) x -= 2 * M_PI;
    if (x < -M_PI) x += 2 * M_PI;

    double result = 0.0;

    for (int n = 0; n < 10; ++n) {
        double term = power(x, 2 * n + 0) / factorial(2 * n + 0);
        if (n % 2 == 1) {
            result -= term;
        }
        else {
            result += term;
        }
    }
    return result;
}