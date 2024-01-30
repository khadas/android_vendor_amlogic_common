/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "GammaOperation"

#include <math.h>

#include "GammaOperation.h"

GammaOperation::GammaOperation()
{
}

GammaOperation::~GammaOperation()
{
}

//============================gamma Power Convert Start=======================================
int GammaOperation::GammaOperation_BaseGammaConvert(unsigned short *gamma_value, double basePower , double targetPower)
{
    int ret = -1;
    std::vector<double> xData, yData;

    if (basePower == targetPower) {
        return 0;
    }

    if (gamma_value == NULL) {
        return -1;
    }

    for (int j = 0; j < 256; ++j)
    {
        xData.push_back(((double)j)/255.0);
        yData.push_back((double)gamma_value[j]);
    }

    tk::spline continuousLine;
    continuousLine.set_points(xData, yData);
    std::vector<int> returnCurve;

    for (int j = 0; j < 256; ++j)
    {
        returnCurve.push_back(continuousLine(pow(((double)j) / 255.0 , targetPower / basePower  )));
        gamma_value[j] = returnCurve[j];
    }

    ret = 0;

    return ret;

}

//============================gamma Power Convert End=======================================

//=========================whiteBalan ceconvert start=======================================
interpolation_info_t* GammaOperation::nat_cubic_spline(int num_points, interpolation_info_t* output_fun) {
    float x_delta[num_points-1]; /* x differences */
    float y_delta[num_points-1]; /* y differences */
    float A[num_points][NUMBER_POINTS]; /* matrix for solving for c */
    float h[num_points]; /* vector for solving for c */
    /* Goal: create interpolating functions of the form:
       S_j(x) = a_j(x) + b_j(x - x_j) + c_j(x - x_j)^2 + d_j(x - x_j)^3
       where 0 < j < num_points
    */

    /* First, want to solve: Ac = h where A is a matrix and c and h are
       vectors
    */
    int i; /* loop index */
    /* If there aren't enough points to interpolate, bail */
    if (num_points < 3) {
        return NULL;
    }

    /* Assign parameters to our S struct */
    output_fun->num_points = num_points;

    /* Build x diff and y diff */
    for (i = 1; i < num_points; i++) {
        x_delta[i-1] = output_fun->x[i] - output_fun->x[i-1];
        y_delta[i-1] = output_fun->y[i] - output_fun->y[i-1];
    }

    /* Build "a" vector (just y) */
    for (i = 0; i < num_points; i++) {
        output_fun->a[i] = output_fun->y[i];
    }

    /* Build A matrix */
    build_A_matrix(x_delta, num_points, A);
    /* Build h vector */
    build_h_vector(h, x_delta, output_fun->a, num_points);
    /* Solve matrix equation for c vector (Ac = h) */
    solve_matrix(output_fun->c, h, num_points, A);
    /* Build b vector */
    build_b_vector(output_fun->b, x_delta, y_delta, output_fun->c, num_points);
    /* Build d vector */
    build_d_vector(output_fun->d, x_delta, output_fun->c, num_points);
    /* Return S struct containing all the coeffs and init vals */
    return output_fun;
}

void GammaOperation::build_A_matrix(float *x_delta, int num_points,
        float A[][NUMBER_POINTS]) {
    int i;

    /* Set top and bottom corners */
    A[0][0] = 1;
    A[num_points-1][num_points-1] = 1;

    /* Fill in the matrix by natural cubic spline algorithm */
    for (i = 1; i < num_points-1; i++) {
        A[i][i-1] = x_delta[i-1];
        A[i][i]   = 2*(x_delta[i-1]+x_delta[i]);
        A[i][i+1] = x_delta[i];
    }
}

float* GammaOperation::build_h_vector(float *h, float *x_delta, float *a, int num_points) {
    int i;

    /* Set top and bottom */
    h[0] = 0.;
    h[num_points-1] = 0.;

    /* Fill in the vector by natural cubic spline algorithm */
    for (i = 1; i < num_points-1; i++) {
        h[i] = 3.*((a[i+1]-a[i]) / x_delta[i] - (a[i]-a[i-1]) / x_delta[i-1]);
    }

    return h;
}

float* GammaOperation::solve_matrix(float *x, float *h, int num_points,
        float A[][NUMBER_POINTS]) {
    /* Solves tridiagonal matrix equation Ax = h
       for tridiagonal matrix A using Thomas' algorithm.
       This requires the matrix to be diagonally dominant or symmetric
       positive definite. This should always be the case for our spline.
    */

    float a[num_points]; /* values to the left of diagonal of A */
    float b[num_points]; /* values on the diagonal of A */
    float c[num_points]; /* values to the right of diagonal of A */
    int i; /* loop index */
    float w[num_points]; /* used as a temp variable */

    /* Set end points */
    b[0] = 1.;
    c[0] = 0.;

    a[num_points-1] = 0.;
    b[num_points-1] = 1.;

    /* Build a, b, c */
    for (i = 1; i < num_points-1; i++) {
        a[i] = A[i][i-1];
        b[i] = A[i][i];
        c[i] = A[i][i+1];
    }

   /* Apply Thomas' algorithm */
   for (i = 1; i < num_points; i++) {
       w[i] = a[i] / b[i-1];
       b[i] = b[i] - w[i]*c[i-1];
       h[i] = h[i] - w[i]*h[i-1];

   }

   /* Back substitute x */
   x[num_points-1] = h[num_points-1] / b[num_points-1];

   for (i = num_points-2; i >= 0; i--) {
       x[i] = (h[i] - c[i] * x[i+1]) / b[i];
   }

   return x;
}

float* GammaOperation::build_b_vector(float *b, float *x_delta, float *y_delta, float *c,
                                        int num_points) {
    int i; /* loop index */

    /* Build b by natural cubic spline */
    for (i = 0; i < num_points-1; i++) {
        b[i] = y_delta[i] / x_delta[i] - x_delta[i] / 3 * (2 * c[i] + c[i+1]);
    }

    return b;
}

float* GammaOperation::build_d_vector(float *d, float *x_delta, float *c, int num_points) {

    int i; /* loop index */

    /* Build d by natural cubic spline */
    for (i = 0; i < num_points-1; i++) {
        d[i] = (c[i+1] - c[i]) / (3 * x_delta[i]);
    }

    return d;
}

int GammaOperation::evaluate(interpolation_info_t *function, float val, float *result) {
    /* Use the interpolation to evaluate a val, answer stored in
       result
    */

    int i; /* loop index */

    /* if the val is less than the smallest value, outside of range,
       bail.
    */
    if (val < function->x[0]) {
        /* -1 represents too small of a val ... not sure how errno is used
           in arduino so I'm using return values to set error types.
        */
        return -1;
    }

    /* If the val is greater than the largest value, outside of range,
       bail.
    */
    else if (val > function->x[function->num_points-1]) {
        /* Set error val */
        return -2;
    }

    for (i = 0; i < function->num_points-1; i++) {
        /* If val equals an element in my x array, just return the
           corresponding y val
        */
        if (almost_equals(val, function->x[i])) {
            *result = function->y[i];
            /* return val of 0 means no errors */
            return 0;
        }

        /* Check the next element also, since I use the range next */
        else if (almost_equals(val, function->x[i+1])) {
            *result = function->y[i+1];
            return 0;
        }
        else if (val > function->x[i] && val < function->x[i+1]) {
            /* If the val falls between 2 initial x values, find the value the
               interpolation gives.
            */
            *result = spline_func(function, val, i);
            return 0;
        }
    }

    /* Something has gone horribly wrong */
    return -3;
}

float GammaOperation::spline_func(interpolation_info_t *function, float val, int i) {
    /* This function selects the appropriate spline function and evaluates
       that function at val.
    */

    /* Each p is a different term in the polynomial (number after p is the
       power)
    */
    float p0;
    float p1;
    float p2;
    float p3;

    /* It's useful to copy and paste the functional form of our spline from
       above:

       S_i(x) = a_i(x) + b_i(x - x_i) + c_i(x - x_i)^2 + d_i(x - x_i)^3
       where 0 < i < num_points.

       We take the ith element from the a, b, c, d, and x arrays for the
       {letter}_i terms. The x term is val in this function. S_i(x) is the
       returned value from the interpolation.
    */

    p0 = function->a[i];
    p1 = function->b[i] * (val - function->x[i]);
    p2 = function->c[i] * (val - function->x[i])*(val - function->x[i]);
    p3 = function->d[i] * (val -
            function->x[i]) * (val - function->x[i]) * (val - function->x[i]);
    return p0 + p1 + p2 + p3;
}

bool GammaOperation::almost_equals(float a, float b) {
    /* Comparing floats for exact equality is shady, they won't ever be
       exactly the same, so use this function to test if they're close enough.
    */

    float c = a - b; /* c is the difference */

    /* Make sure c is positive */
    if (c < 0) {
        c = -1*c;
    }

    /* If the difference is sufficiently small, return true */
    if (c < 0.00000001) {
        return true;
    }
    /* Otherwise, return false */
    else {
        return false;
    }
}
//===========================whiteBalan ceconvert end===================================

GammaOperation *GammaOperation::mInstance = NULL;
GammaOperation *GammaOperation::GetInstance()
{
    if (NULL == mInstance) {
        mInstance = new GammaOperation();
    }
    return mInstance;
}
