/*
 * ext_math.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __WIRBELSCAN_EXT_MATH_H_
#define __WIRBELSCAN_EXT_MATH_H_

#include <stdint.h>
#define EXT_M_DEBUG 0
#define SPLINE_RATIO 0.0
#define nextpair(x,y) *(vals + row * 2 + col++) = x; \
                      *(vals + row * 2 + col++) = y; \
                      row++; \
                      col=0

/******************************************************************************
 * bsp* functions here.
 * 
 *****************************************************************************/
int    bspline          (double * svars,  uint16_t size, double * results);
int    bsp_maxima       (uint16_t size, double * coeffs, double * results);
int    bsp_minima       (uint16_t size, double * coeffs, double * results);
int    bsp_reqbufsize   (int size);
int    bsp_round        (double val, int rnd);


/******************************************************************************
 * debugging only.
 * 
 *****************************************************************************/
void   printmatrix  (double * coeffs, uint16_t cols, uint16_t rows, int wait, FILE * afile = NULL);
void   printcoeffs  (double * svars,  uint16_t size, double * coeffs, FILE * afile = NULL);



/******************************************************************************
 * csp* functions here.
 * 
 *****************************************************************************/ 
int    cspline          (double * svars,  uint16_t size, double * results);
int    csp_zeros        (double * svars,  uint16_t size, double * coeffs, double * results);
int    csp_maxima       (double * svars,  uint16_t size, double * coeffs, double * results);
int    csp_minima       (double * svars,  uint16_t size, double * coeffs, double * results);
double csp_value        (double * svars,  uint16_t size, double * coeffs, double indep);
int    csp_reqbufsize   (int size);
int    linearsolve  (double * coeffs, uint16_t EqCount, double * results);

#endif
