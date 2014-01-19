/*
 * ext_math.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ext_math.h"


/******************************************************************************
 * internal macros.
 * 
 *****************************************************************************/

#define nextquad(a,b,c,d) *(coeff + row * (coeff_rows + 1) + col++) = a; \
                          *(coeff + row * (coeff_rows + 1) + col++) = b; \
                          *(coeff + row * (coeff_rows + 1) + col++) = c; \
                          *(coeff + row * (coeff_rows + 1) + col++) = d
#define nextres(y)        *(coeff + row * (coeff_rows + 1) + coeff_rows) = y
#define BSP_POLY_ORD 3          // ATM there is no need for other polynom orders
#define BSP_NUM_OUTP 5 * size  // resolution output array

#if EXT_M_DEBUG
#define dprint(s...)  printf(s);
#else
#define dprint(s...)
#endif


static int bsp_compute_intervals(int * u, int num_cp) {
  int i;
  int n = num_cp - 1;
  int t = BSP_POLY_ORD + 1;

  for (i = 0; i <= num_cp + BSP_POLY_ORD; i++) {
    if (i < t)
      u[i] = 0;
    else if ((t <= i) && (i <= n))
      u[i] = i-t + 1;
    else if (i > n) {
      u[i] = n-t + 2;
      // if n-t=-2 then we're screwed, everything goes to 0
      if (u[i] == 0)
        return 0;
      }
    }
  return 1;
}

// calculate the bspline blending value
static double bsp_blend(int k, int t, int * u, double intv) {
  double retval;

  if (t == 1) {      // base case for the recursion
    if ((u[k] <= intv) && (intv < u[k+1]))
      retval = 1.0;
    else
      retval = 0.0;
    }
  else {
    if ((u[k+t-1]==u[k]) && (u[k+t]==u[k+1]))  // check for divide by zero
      retval = 0.0;
    else if (u[k+t-1]==u[k]) // if a term's denominator is zero,use just the other
      retval = (u[k+t] - intv) / (u[k+t] - u[k+1]) * bsp_blend(k+1, t-1, u, intv);
    else if (u[k+t]==u[k+1])
      retval = (intv - u[k]) / (u[k+t-1] - u[k]) * bsp_blend(k, t-1, u, intv);
    else
      retval = (intv - u[k])   / (u[k+t-1] - u[k]) * bsp_blend(k,   t-1, u, intv) +
               (u[k+t] - intv) / (u[k+t] - u[k+1]) * bsp_blend(k+1, t-1, u, intv);
    }
  return retval;
}

static void bsp_compute_point(int * u, int size, double intv, double * svars, double * results) {
  int i;
  double tmp;

  *(results + 0) = 0;
  *(results + 1) = 0;

  for (i = 0; i < size; i++) {
    tmp = bsp_blend(i,BSP_POLY_ORD + 1,u,intv);
    *(results + 0) += (*(svars + i*2 + 0)) * tmp;
    *(results + 1) += (*(svars + i*2 + 1)) * tmp;
    }
}

/******************************************************************************
 * int bspline (double * svars, uint16_t size, double * results);
 * svars        input buffer
 * size         size of input buffer (number of rows)
 * results      output buffer of appropriate size
 *
 *  returns 1 on SUCCESS, 0 otherwise.
 *****************************************************************************/

int bspline(double * svars, uint16_t size, double * results) {
  double increment = (double) (size - BSP_POLY_ORD)/(BSP_NUM_OUTP - 1);
  double * lastp   = (double *) malloc(2 * sizeof(double));  
  double interval  = 0.0;
  int i;
  int * u = (int *) malloc((size + BSP_POLY_ORD + 1) * sizeof(int));

  if (! bsp_compute_intervals(u, size)) {
    free(lastp);
    return 0;
    }

  for (i = 0; i < (BSP_NUM_OUTP) - 1; i++) {
    bsp_compute_point(u, size, interval, svars, lastp);
    *(results + i*2 + 0) = *(lastp + 0);
    *(results + i*2 + 1) = *(lastp + 1);
    interval += increment;
    }
   // put in the last point
  *(results + (BSP_NUM_OUTP - 1)*2 + 0) = *(svars + (size - 1) * 2 + 0);
  *(results + (BSP_NUM_OUTP - 1)*2 + 1) = *(svars + (size - 1) * 2 + 1);
  free(u);
  free(lastp);
  return 1;
}

/******************************************************************************
 * int bsp_reqbufsize (int size);
 * size   size of input buffer (number of rows)
 *
 *  returns required buffer size for bspline result buffer.
 *****************************************************************************/

int bsp_reqbufsize (int size) {
  return (BSP_NUM_OUTP) * 2;
}

static int bsp_discuss (uint16_t size, double * coeffs, double * results, int maxmin) {
  int i;
  int retval = 0;

  for (i = 1; i < size - 1; i++) {
    if (maxmin == 1) {
      if ((*(coeffs + i*2 + 1) >= *(coeffs + (i-1)*2 + 1)) && (*(coeffs + i*2 + 1) > *(coeffs + (i+1)*2 + 1))) {
         *(results + retval*2 + 0) = *(coeffs + i*2 + 0);
         *(results + retval*2 + 1) = *(coeffs + i*2 + 1);
         retval++;
         }  
      }
    else {
      if ((*(coeffs + i*2 + 1) <= *(coeffs + (i-1)*2 + 1)) && (*(coeffs + i*2 + 1) < *(coeffs + (i+1)*2 + 1))) {
         *(results + retval*2 + 0) = *(coeffs + i*2 + 0);
         *(results + retval*2 + 1) = *(coeffs + i*2 + 1);
         retval++;
         } 
      }
    }
  return retval;
}

/******************************************************************************
 * int bsp_maxima  (uint16_t size, double * coeffs, double * results);
 * int bsp_minima  (uint16_t size, double * coeffs, double * results);
 *
 * size     size of input buffer (number of rows)
 * coeffs   coeffs/samples returned by bspline.
 * results  buffer of appropriate size to store results,
 *
 *  return value = number of extrema found.
 *****************************************************************************/

int bsp_maxima (uint16_t size, double * coeffs, double * results) {
  return bsp_discuss (size, coeffs, results, 1);
}

int bsp_minima (uint16_t size, double * coeffs, double * results) {
  return bsp_discuss (size, coeffs, results, -1);
}

int bsp_round (double val, int rnd) {
  int retval;
  retval = rnd * ((int) val/rnd);

  if ((val - ((double) retval)) >= ((double) rnd/2.0))
    return retval + rnd;
  else
    return retval;
}


#if 1
/******************************************************************************
 * int csp_reqbufsize (int size);
 * size   size of input buffer (number of rows)
 *
 *  returns required buffer size for cspline result buffer.
 *****************************************************************************/

int csp_reqbufsize (int size) {
  return (size - 1) * 4;
}

/******************************************************************************
 * int cspline (double * svars, uint16_t size, double * results);
 * svars        input buffer
 * size         size of input buffer (number of rows)
 * results      output buffer of appropriate size
 *
 *  returns 1 on SUCCESS, 0 otherwise.
 *****************************************************************************/

int cspline (double * svars, uint16_t size, double * results) {
  double * coeff;
  int coeff_rows = csp_reqbufsize(size);     
  int srow = 0;
  int row = 0, col = 0;
  int start, stop;
  int retval = 0;

  // allocate memory
  if (! (coeff = (double *) malloc(coeff_rows * (coeff_rows + 1) * sizeof(double)))) {
    printf("%s: could not allocate memory.", __FUNCTION__);
    return -1;
    }

  memset(coeff,   0, coeff_rows * (coeff_rows + 1) * sizeof(double));
  memset(results, 0, coeff_rows * sizeof(double));

  // now start assigning values

  // (I)
  row = 0; col = 0; srow = 0;
  nextquad(6 * *(svars + srow * 2), 2, 0, 0);

  // (II)
  row = 1; col = coeff_rows - 4; srow = size - 1;
  nextquad(6 * *(svars + srow * 2), 2, 0, 0);

  // (III) .. (V)
  start = 2; stop = start + (size - 2); col = 0; srow = 1;
  for (row = start; row < stop; row++) {
    nextquad( 6 * *(svars + srow * 2),  2, 0, 0);
    nextquad(-6 * *(svars + srow * 2), -2, 0, 0);
    col -= 4;
    srow++;
    }

  // (VI) .. (VIII)
  start = row; stop = start + (size - 2); col = 0; srow = 1;
  for (row = start; row < stop; row++) {
    nextquad( 3 * pow(*(svars + srow * 2),2),  2 * *(svars + srow * 2),  1, 0);
    nextquad(-3 * pow(*(svars + srow * 2),2), -2 * *(svars + srow * 2), -1, 0);
    col -= 4;
    srow++;
    }

  // (IX)
  col = 0; srow = 0;
  nextquad(pow(*(svars + srow * 2),3), pow(*(svars + srow * 2),2), *(svars + srow * 2), 1);
  nextres(*(svars + srow * 2 + 1));
  row++;

  // (X)
  col = coeff_rows - 4; srow = size - 1;
  nextquad(pow(*(svars + srow * 2),3), pow(*(svars + srow * 2),2), *(svars + srow * 2), 1);
  nextres(*(svars + srow * 2 + 1));
  row++;

  // (XI) .. (XVI)
  start = row; stop = start + 2*(size - 2); col = 0; srow = 1;
  for (row = start; row < stop; row++) {
    nextquad(pow(*(svars + srow * 2),3), pow(*(svars + srow * 2),2), *(svars + srow * 2), 1);
    nextres(*(svars + srow * 2 + 1));
    row++;
    nextquad(pow(*(svars + srow * 2),3), pow(*(svars + srow * 2),2), *(svars + srow * 2), 1);
    nextres(*(svars + srow * 2 + 1));
    col -= 4;
    srow++;
    }

  dprint("==== coeff ====\n");  
  #if EXT_M_DEBUG
  printmatrix (coeff, coeff_rows + 1, coeff_rows, 0);
  #endif

  if (! linearsolve (coeff, coeff_rows, results)) {
     printf("%s: could not solve matrix.", __FUNCTION__);     
     retval = -1;
     }

  free(coeff);
  return retval;
}

#define EPSILON 0.00001
static bool fequal(double a, double b, double epsilon = EPSILON) {
  return (fabs(a-b) < epsilon);
}

static void ddx (double * in, double * out) {
  memset(out, 0, 4 * sizeof(double));
  if (!fequal(*(in + 0), 0.0)) *(out + 1)= 3 * *(in + 0);
  if (!fequal(*(in + 1), 0.0)) *(out + 2)= 2 * *(in + 1);
  if (!fequal(*(in + 2), 0.0)) *(out + 3)= 1 * *(in + 2);
}

static void d2dx (double * in, double * out) {
  double * tmp;
  tmp = (double *) malloc(4 * sizeof(double));
  ddx(in, tmp);
  ddx(tmp, out);
  free(tmp);
}

enum mres {
  EXT_UNK = 0,
  EXT_MAX = -1,
  EXT_MIN = 1
};

static int maxmin(double * in, double val) {
  dprint("% 10.3f % 10.3f% 10.3f% 10.3f\n",   *(in+0), *(in+1), *(in+2), *(in+3));
  if ((val * *(in + 2) + *(in + 3)) > 0) return EXT_MIN;
  if ((val * *(in + 2) + *(in + 3)) < 0) return EXT_MAX;
  //unknown; TODO: implement further detection.
  return EXT_UNK; 
}

static double nrt(double x, int exponent) {
  int isodd = ((int) exponent & 1);
  if (isodd && (x < 0))
    return -exp(log(x * -1.0)/exponent);
  return exp(log(x)/exponent);
} 

static double pi(void) {
  return (double) 3.1415926535897932384626433832795029L;
}

double csp_value (double * svars, uint16_t size, double * coeffs, double indep) {
int i;

for (i = 0; i < size - 1; i++) {
  if ((*(svars + i*2 +0) <= indep) && (indep < *(svars + (i+1)*2 +0))) {
    return *(coeffs + i*4 + 0)*indep*indep*indep +
           *(coeffs + i*4 + 1)*indep*indep +
           *(coeffs + i*4 + 2)*indep +
           *(coeffs + i*4 + 3);
    }
  }
//should never happen.
return 0;
}


#define acc 0.0001

int csp_zeros (double * svars, uint16_t size, double * coeffs, double * results) {
 int spline, extrema = 0;
 for (spline = 0; spline < size; spline++) {
    double rangelow = *(svars + spline * 2);
    double rangehigh= *(svars + (spline + 1) * 2);
    double r = *(coeffs + spline * 4 + 1) / *(coeffs + spline * 4 + 0);
    double s = *(coeffs + spline * 4 + 2) / *(coeffs + spline * 4 + 0);
    double t = *(coeffs + spline * 4 + 3) / *(coeffs + spline * 4 + 0);
    double y1, y2, y3;
    double p = s - pow(r,2)/3;
    double q = (2 * pow(r,3)) / 27 - (r * s)/3 + t;
    double D = pow(q/2,2) + pow(p/3,3);
   
    dprint("p = % 10.4f, q = % 10.4f => D = % 10.4f %s\n",
      p, q, D, D>0?"D > 0":D<0?"D < 0":"D = 0");    
    if (D > 0) {
      double T = sqrt(D);
      double u = nrt(-q/2 + T,3);
      double v = nrt(-q/2 - T,3);
      dprint("T = % 10.4f, u = % 10.4f, v = % 10.4f\n",
        T, u, v);
      y1 = u + v -r/3;
      // two more complex solutions; skipping them for now.
      // y2 = (u + v) / 2 - j (((u -v) * sqrt(3))/2) - r/3;
      // y3 = (u + v) / 2 + j (((u -v) * sqrt(3))/2) - r/3;
      dprint("y1 = % 10.4f\n",y1);
      if (((y1 + acc) >= rangelow) && (y1 < rangehigh))
         *(results + extrema++) = y1;    
      }
    else if (fequal(D, 0.0)) {
      if (!fequal(p,0.0) && !fequal(q,0.0)) {
        y1 = -r/3;
        dprint("y1 = % 10.4f\n", y1);
        if (((y1 + acc) >= rangelow) && (y1 < rangehigh))
           *(results + extrema++) = 0;
        }
      else {
        double u = nrt(-q/2 + sqrt(D),3);
        double v = nrt(-q/2 - sqrt(D),3);
        y1 = u + v -r/3;
        y2 = -(u + v)/2 -r/3;
        dprint("y1 = % 10.4f, y2 = % 10.4f\n",y1,y2);
        if (((y1 + acc) >= rangelow) && (y1 < rangehigh))
           *(results + extrema++) = y1;
        if (((y2 + acc) >= rangelow) && (y2 < rangehigh))
           *(results + extrema++) = y2;
        }    
     }
    else {
      //casus irreducibilis
      double u = sqrt(pow(-p/3,3));
      double w = acos(-q/(2*u));
      dprint("u = % 10.4f, w = % 10.4f\n", u, w);
      y1 = 2 * nrt(u,3) * cos(w/3);
      y2 = 2 * nrt(u,3) * cos(w/3 + (2 * pi()/3));
      y3 = 2 * nrt(u,3) * cos(w/3 + (4 * pi()/3));
      dprint("y1 = % 10.4f, y2 = % 10.4f, y3 = % 10.4f\n", y1, y2, y3);
      if (((y1 + acc) >= rangelow) && (y1 < rangehigh))
         *(results + extrema++) = y1;
      if (((y2 + acc) >= rangelow) && (y2 < rangehigh))
         *(results + extrema++) = y2;
      if (((y3 + acc) >= rangelow) && (y3 < rangehigh))
         *(results + extrema++) = y3;
     }
  } // end loop
  return extrema;
}

static int discuss (const double * svars, uint16_t size, double * coeffs, double * results, int searchmax) {
  int spline, extrema = 0;

  for (spline = 0; spline < size; spline++) {
     double rangelow = *(svars + spline * 2);
     double rangehigh= *(svars + (spline + 1) * 2);

     double * ddx_f  = (double *) malloc(4 * sizeof(double));
     double * d2dx_f = (double *) malloc(4 * sizeof(double));
     ddx (coeffs + spline * 4, ddx_f);
     d2dx(coeffs + spline * 4, d2dx_f);
     dprint("ddx  % 10.3f % 10.3f% 10.3f% 10.3f\t\t", *(ddx_f+0),  *(ddx_f+1),  *(ddx_f+2),  *(ddx_f+3));
     dprint("d2dx % 10.3f % 10.3f% 10.3f% 10.3f\n",   *(d2dx_f+0), *(d2dx_f+1), *(d2dx_f+2), *(d2dx_f+3));
     if (! fequal(*(ddx_f + 1), 0.0)) {
        double f,p_2,s,x1,x2;
        f = 1 / *(ddx_f + 1);
        *(ddx_f + 1) = 1;
        *(ddx_f + 2) = f * *(ddx_f + 2);
        *(ddx_f + 3) = f * *(ddx_f + 3);
        p_2 = 0.5 * *(ddx_f + 2);
        s   = pow(p_2,2) - *(ddx_f + 3);
        if (s >= 0) {        
           x1 = -p_2 + sqrt(s);
           x2 = -p_2 - sqrt(s);
           if (((x1 + acc) >= rangelow) && (x1 < rangehigh)) {
             int e = maxmin(d2dx_f, x1);
             if (e == EXT_MAX) {
               dprint("x1 = %f => local maximum\n", x1);
               if (searchmax)
                 *(results + extrema++) = x1;
               }
             else if (e == EXT_MIN) {
               dprint("x1 = %f => local minimum\n", x1);
               if (! searchmax)
                 *(results + extrema++) = x1;
               }
             else {
               dprint("d2/dx f(%f)=0  => unknown\n", x1);
               }
             }
           else {
             dprint("x1 = %f => out of range %f..%f\n", x1, rangelow, rangehigh);
             }
           if (!fequal(x1, x2) && ((x2 + acc) >= rangelow) && (x2 < rangehigh)) {
             int e = maxmin(d2dx_f, x2);
             if (e == EXT_MAX) {
               dprint("x2 = %f => local maximum\n", x2);
               if (searchmax)
                 *(results + extrema++) = x2;
               }
             else if (e == EXT_MIN) {
               dprint("x2 = %f => local minimum\n", x2);
               if (! searchmax)
                 *(results + extrema++) = x2;
               }
             else {
               dprint("d2/dx f(%f)=0  => unknown\n", x2);
               }
             }
           else {
             dprint("x2 = %f => out of range %f..%f\n", x2, rangelow, rangehigh);
             }
           }
        else {              
           dprint("no solution, p^2/4 - q < 0\n");
           }
        }
     else {
       if (!fequal(*(ddx_f + 2), 0.0)) {
         double f,x1,x2;
         x2 = *(ddx_f + 2);
         f = 1 / *(ddx_f + 2);
         *(ddx_f + 2) = f * *(ddx_f + 2);
         *(ddx_f + 3) = f * *(ddx_f + 3);
         x1 = - *(ddx_f + 3);
         if (((x1 + acc) >= rangelow) && (x1 < rangehigh)) {
           if (x2 < 0) {
             dprint("x1 = %f => local maximum (linear part only)\n", x1);
             if (searchmax)
               *(results + extrema++) = x1;
             }
           else  {
             dprint("x1 = %f => local minimum (linear part only)\n", x1);
             if (! searchmax)
               *(results + extrema++) = x1;
             }
           }
         }
       else {
         dprint("no solution, no 'x' left over\n");
         }
       }
     free(ddx_f);
     free(d2dx_f);
     }
  return extrema; 
}

int csp_maxima (double * svars,  uint16_t size, double * coeffs, double * results) {
  return discuss (svars, size, coeffs, results, 1);
}

int csp_minima (double * svars,  uint16_t size, double * coeffs, double * results) {
  return discuss (svars, size, coeffs, results, 0);
}

/******************************************************************************
 * linear equotion solving by elimination with row pivoting
 *--------------------------------------------------------------------
 *    double * coeffs       extended coefficient matrix,
 *                          pointer to element[0][0] of
 *                          an array[EqCount][EqCount+1]
 *
 *    int EqCount           number of equotions
 *
 *    double * results      matrix with results,
 *                          pointer to array[EqCount] where
 *                          to store results.
 *  returns 1 on SUCCESS, 0 otherwise.
 *
 * -> may be used somewhere later indep. on csp*, don't remove yet.
 *****************************************************************************/

int linearsolve (double * coeffs, uint16_t EqCount, double * results) {

  uint16_t  step = 0;            // eliminination step
  uint16_t  pivotRow;
  uint16_t  columns = EqCount + 1;
   int16_t  row, col;
  uint8_t   err = 0;
  double multiplier;
  double epsilon = 0.0;       // accuracy
  double maximum;                // row pivoting

  dprint("%s: %d columns, %d rows\n", __FUNCTION__, columns, EqCount);

  do {
    dprint("%s: step %2i out of %2i\n", __FUNCTION__, step+1, EqCount-1);
    maximum = fabs(*(coeffs + step * columns + step));     // find largest element
    pivotRow = step;
    for (row = step + 1; row < EqCount; row++)
      if (fabs(*(coeffs + row * columns + step)) > maximum) {
        maximum = fabs(*(coeffs + row * columns + step));
        pivotRow = row;
        }
    if ((err = (maximum < epsilon))) break; // not solvable 

    if (pivotRow != step) {  // exchange rows, if needed
      double h;
      for (col = step; col <= EqCount; col++) {
        h = *(coeffs + step * columns + col);
        *(coeffs + step * columns + col) = *(coeffs + pivotRow * columns + col);
        *(coeffs + pivotRow * columns + col)= h;
        }
      }

    // elimination -> zeros in column step from row "step + 1"
    for (row = step + 1; row < EqCount; row++ ) {
      multiplier = -( *(coeffs + row * columns + step) / *(coeffs + step * columns + step) );
      *(coeffs + row * columns + step) = 0.0;
      for (col = step+1; col <= EqCount ; col++)
        // add rows "row" && "step"
        *(coeffs + row * columns + col) += multiplier * *(coeffs + step * columns + col);
      }

    #if EXT_M_DEBUG
       printmatrix(coeffs, columns, EqCount, 0);
    #endif
    step++;
    }
  while ( step < EqCount-1 );

  if (err) {
    printf("%s: could not solve equotion\n", __FUNCTION__);
    return 0; 
    }
  else {
    /* calculate solutions from triangular matrix */

    // last row
    *(results + (EqCount-1)) =  *(coeffs + (EqCount-1) * columns + EqCount) / *(coeffs + (EqCount-1) * columns + (EqCount-1));       
    // remaining rows
    for (row = EqCount-2; row >= 0; row--) {
      for (col=EqCount-1; col>row; col--)
        *(coeffs + row * columns + EqCount) -= *(results + col) * ( *(coeffs + row * columns + col)); // right side
      *(results + row) = *(coeffs + row * columns + EqCount) / *(coeffs + row * columns + row);
      }
    return 1;  
    }
}
#endif


/******************************************************************************
 * debugging tools.
 *
 *****************************************************************************/
#define PRINTF(a...) if (afile) fprintf(afile, a); else printf(a)

void printmatrix (double * coeffs, uint16_t cols, uint16_t rows, int wait, FILE * afile) {
  uint16_t row, col;

  for (row = 0; row < rows; row++ ) {
    PRINTF("%d\t", row+1);
    for (col = 0; col < cols; col++) 
      PRINTF ("  % 10.4f", *(coeffs + row * cols + col));
    PRINTF("\n");
  }
  if (wait) {
    printf("--------------------------------------------------------\n");
    printf("press <RETURN> to continue..");
    getchar();
    }
}

void printcoeffs (double * svars, uint16_t size, double * coeffs, FILE * afile) {
  uint16_t row, cpos = 0;

  for (row = 0; row < size; row++) {
    PRINTF("%10.4f <= x < %-10.4f\t=>\t",
        *(svars + row * 2),  *(svars + (row + 1) * 2));
    PRINTF("f(x) = % 10.4f*x³ %+10.4f*x² %+10.4f*x %+10.4f\n",
        *(coeffs + cpos), *(coeffs + cpos + 1),
        *(coeffs + cpos + 2), *(coeffs + cpos + 3));
    cpos += 4;
  }
}
