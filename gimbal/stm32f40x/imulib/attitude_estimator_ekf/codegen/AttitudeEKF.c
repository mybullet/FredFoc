/*
 * File: AttitudeEKF.c
 *
 * MATLAB Coder version            : 2.6
 * C/C++ source code generated on  : 25-Apr-2016 13:53:22
 */

/* Include files */
#include "AttitudeEKF.h"

/* Variable Definitions */
static float Ji[9];
static boolean_T Ji_not_empty;
static float x_apo[12];
static float P_apo[144];
static float Q[144];
static boolean_T Q_not_empty;

/* Function Declarations */
static void AttitudeEKF_init(void);
static void b_mrdivide(float A[72], const float B[36]);
static void cross(const float a[3], const float b[3], float c[3]);
static void inv(const float x[9], float y[9]);
static void mrdivide(float A[108], const float B[81]);
static float norm(const float x[3]);

/* Function Definitions */

/*
 * Arguments    : void
 * Return Type  : void
 */
static void AttitudeEKF_init(void)
{
  int i;
  static const float fv6[12] = { 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
    -9.81F, 1.0F, 0.0F, 0.0F };

  for (i = 0; i < 12; i++) {
    x_apo[i] = fv6[i];
  }

  for (i = 0; i < 144; i++) {
    P_apo[i] = 200.0F;
  }
}

/*
 * Arguments    : float A[72]
 *                const float B[36]
 * Return Type  : void
 */
static void b_mrdivide(float A[72], const float B[36])
{
  float b_A[36];
  signed char ipiv[6];
  int k;
  int j;
  int c;
  int jAcol;
  int ix;
  float temp;
  float s;
  int i;
  int jBcol;
  int kBcol;
  memcpy(&b_A[0], &B[0], 36U * sizeof(float));
  for (k = 0; k < 6; k++) {
    ipiv[k] = (signed char)(1 + k);
  }

  for (j = 0; j < 5; j++) {
    c = j * 7;
    jAcol = 0;
    ix = c;
    temp = (real32_T)fabs(b_A[c]);
    for (k = 2; k <= 6 - j; k++) {
      ix++;
      s = (real32_T)fabs(b_A[ix]);
      if (s > temp) {
        jAcol = k - 1;
        temp = s;
      }
    }

    if (b_A[c + jAcol] != 0.0F) {
      if (jAcol != 0) {
        ipiv[j] = (signed char)((j + jAcol) + 1);
        ix = j;
        jAcol += j;
        for (k = 0; k < 6; k++) {
          temp = b_A[ix];
          b_A[ix] = b_A[jAcol];
          b_A[jAcol] = temp;
          ix += 6;
          jAcol += 6;
        }
      }

      k = (c - j) + 6;
      for (i = c + 1; i + 1 <= k; i++) {
        b_A[i] /= b_A[c];
      }
    }

    jBcol = c;
    jAcol = c + 6;
    for (kBcol = 1; kBcol <= 5 - j; kBcol++) {
      temp = b_A[jAcol];
      if (b_A[jAcol] != 0.0F) {
        ix = c + 1;
        k = (jBcol - j) + 12;
        for (i = 7 + jBcol; i + 1 <= k; i++) {
          b_A[i] += b_A[ix] * -temp;
          ix++;
        }
      }

      jAcol += 6;
      jBcol += 6;
    }
  }

  for (j = 0; j < 6; j++) {
    jBcol = 12 * j;
    jAcol = 6 * j;
    for (k = 1; k <= j; k++) {
      kBcol = 12 * (k - 1);
      if (b_A[(k + jAcol) - 1] != 0.0F) {
        for (i = 0; i < 12; i++) {
          A[i + jBcol] -= b_A[(k + jAcol) - 1] * A[i + kBcol];
        }
      }
    }

    temp = 1.0F / b_A[j + jAcol];
    for (i = 0; i < 12; i++) {
      A[i + jBcol] *= temp;
    }
  }

  for (j = 5; j > -1; j += -1) {
    jBcol = 12 * j;
    jAcol = 6 * j - 1;
    for (k = j + 2; k < 7; k++) {
      kBcol = 12 * (k - 1);
      if (b_A[k + jAcol] != 0.0F) {
        for (i = 0; i < 12; i++) {
          A[i + jBcol] -= b_A[k + jAcol] * A[i + kBcol];
        }
      }
    }
  }

  for (jAcol = 4; jAcol > -1; jAcol += -1) {
    if (ipiv[jAcol] != jAcol + 1) {
      for (jBcol = 0; jBcol < 12; jBcol++) {
        temp = A[jBcol + 12 * jAcol];
        A[jBcol + 12 * jAcol] = A[jBcol + 12 * (ipiv[jAcol] - 1)];
        A[jBcol + 12 * (ipiv[jAcol] - 1)] = temp;
      }
    }
  }
}

/*
 * Arguments    : const float a[3]
 *                const float b[3]
 *                float c[3]
 * Return Type  : void
 */
static void cross(const float a[3], const float b[3], float c[3])
{
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
}

/*
 * Arguments    : const float x[9]
 *                float y[9]
 * Return Type  : void
 */
static void inv(const float x[9], float y[9])
{
  float b_x[9];
  int p1;
  int p2;
  int p3;
  float absx11;
  float absx21;
  float absx31;
  int itmp;
  float b_y;
  for (p1 = 0; p1 < 9; p1++) {
    b_x[p1] = x[p1];
  }

  p1 = 0;
  p2 = 3;
  p3 = 6;
  absx11 = (real32_T)fabs(x[0]);
  absx21 = (real32_T)fabs(x[1]);
  absx31 = (real32_T)fabs(x[2]);
  if ((absx21 > absx11) && (absx21 > absx31)) {
    p1 = 3;
    p2 = 0;
    b_x[0] = x[1];
    b_x[1] = x[0];
    b_x[3] = x[4];
    b_x[4] = x[3];
    b_x[6] = x[7];
    b_x[7] = x[6];
  } else {
    if (absx31 > absx11) {
      p1 = 6;
      p3 = 0;
      b_x[0] = x[2];
      b_x[2] = x[0];
      b_x[3] = x[5];
      b_x[5] = x[3];
      b_x[6] = x[8];
      b_x[8] = x[6];
    }
  }

  absx21 = b_x[1] / b_x[0];
  b_x[1] /= b_x[0];
  absx11 = b_x[2] / b_x[0];
  b_x[2] /= b_x[0];
  b_x[4] -= absx21 * b_x[3];
  b_x[5] -= absx11 * b_x[3];
  b_x[7] -= absx21 * b_x[6];
  b_x[8] -= absx11 * b_x[6];
  if ((real32_T)fabs(b_x[5]) > (real32_T)fabs(b_x[4])) {
    itmp = p2;
    p2 = p3;
    p3 = itmp;
    b_x[1] = absx11;
    b_x[2] = absx21;
    absx11 = b_x[4];
    b_x[4] = b_x[5];
    b_x[5] = absx11;
    absx11 = b_x[7];
    b_x[7] = b_x[8];
    b_x[8] = absx11;
  }

  absx31 = b_x[5];
  b_y = b_x[4];
  absx21 = b_x[5] / b_x[4];
  b_x[8] -= absx21 * b_x[7];
  absx11 = (absx21 * b_x[1] - b_x[2]) / b_x[8];
  absx21 = -(b_x[1] + b_x[7] * absx11) / b_x[4];
  y[p1] = ((1.0F - b_x[3] * absx21) - b_x[6] * absx11) / b_x[0];
  y[p1 + 1] = absx21;
  y[p1 + 2] = absx11;
  absx11 = -(absx31 / b_y) / b_x[8];
  absx21 = (1.0F - b_x[7] * absx11) / b_x[4];
  y[p2] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
  y[p2 + 1] = absx21;
  y[p2 + 2] = absx11;
  absx11 = 1.0F / b_x[8];
  absx21 = -b_x[7] * absx11 / b_x[4];
  y[p3] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
  y[p3 + 1] = absx21;
  y[p3 + 2] = absx11;
}

/*
 * Arguments    : float A[108]
 *                const float B[81]
 * Return Type  : void
 */
static void mrdivide(float A[108], const float B[81])
{
  float b_A[81];
  signed char ipiv[9];
  int k;
  int j;
  int c;
  int jAcol;
  int ix;
  float temp;
  float s;
  int i;
  int jBcol;
  int kBcol;
  memcpy(&b_A[0], &B[0], 81U * sizeof(float));
  for (k = 0; k < 9; k++) {
    ipiv[k] = (signed char)(1 + k);
  }

  for (j = 0; j < 8; j++) {
    c = j * 10;
    jAcol = 0;
    ix = c;
    temp = (real32_T)fabs(b_A[c]);
    for (k = 2; k <= 9 - j; k++) {
      ix++;
      s = (real32_T)fabs(b_A[ix]);
      if (s > temp) {
        jAcol = k - 1;
        temp = s;
      }
    }

    if (b_A[c + jAcol] != 0.0F) {
      if (jAcol != 0) {
        ipiv[j] = (signed char)((j + jAcol) + 1);
        ix = j;
        jAcol += j;
        for (k = 0; k < 9; k++) {
          temp = b_A[ix];
          b_A[ix] = b_A[jAcol];
          b_A[jAcol] = temp;
          ix += 9;
          jAcol += 9;
        }
      }

      k = (c - j) + 9;
      for (i = c + 1; i + 1 <= k; i++) {
        b_A[i] /= b_A[c];
      }
    }

    jBcol = c;
    jAcol = c + 9;
    for (kBcol = 1; kBcol <= 8 - j; kBcol++) {
      temp = b_A[jAcol];
      if (b_A[jAcol] != 0.0F) {
        ix = c + 1;
        k = (jBcol - j) + 18;
        for (i = 10 + jBcol; i + 1 <= k; i++) {
          b_A[i] += b_A[ix] * -temp;
          ix++;
        }
      }

      jAcol += 9;
      jBcol += 9;
    }
  }

  for (j = 0; j < 9; j++) {
    jBcol = 12 * j;
    jAcol = 9 * j;
    for (k = 1; k <= j; k++) {
      kBcol = 12 * (k - 1);
      if (b_A[(k + jAcol) - 1] != 0.0F) {
        for (i = 0; i < 12; i++) {
          A[i + jBcol] -= b_A[(k + jAcol) - 1] * A[i + kBcol];
        }
      }
    }

    temp = 1.0F / b_A[j + jAcol];
    for (i = 0; i < 12; i++) {
      A[i + jBcol] *= temp;
    }
  }

  for (j = 8; j > -1; j += -1) {
    jBcol = 12 * j;
    jAcol = 9 * j - 1;
    for (k = j + 2; k < 10; k++) {
      kBcol = 12 * (k - 1);
      if (b_A[k + jAcol] != 0.0F) {
        for (i = 0; i < 12; i++) {
          A[i + jBcol] -= b_A[k + jAcol] * A[i + kBcol];
        }
      }
    }
  }

  for (jAcol = 7; jAcol > -1; jAcol += -1) {
    if (ipiv[jAcol] != jAcol + 1) {
      for (jBcol = 0; jBcol < 12; jBcol++) {
        temp = A[jBcol + 12 * jAcol];
        A[jBcol + 12 * jAcol] = A[jBcol + 12 * (ipiv[jAcol] - 1)];
        A[jBcol + 12 * (ipiv[jAcol] - 1)] = temp;
      }
    }
  }
}

/*
 * Arguments    : const float x[3]
 * Return Type  : float
 */
static float norm(const float x[3])
{
  float y;
  float scale;
  int k;
  float absxk;
  float t;
  y = 0.0F;
  scale = 1.17549435E-38F;
  for (k = 0; k < 3; k++) {
    absxk = (real32_T)fabs(x[k]);
    if (absxk > scale) {
      t = scale / absxk;
      y = 1.0F + y * t * t;
      scale = absxk;
    } else {
      t = absxk / scale;
      y += t * t;
    }
  }

  return scale * (real32_T)sqrt(y);
}

/*
 * LQG Postion Estimator and Controller
 *  Observer:
 *         x[n|n]   = x[n|n-1] + M(y[n] - Cx[n|n-1] - Du[n])
 *         x[n+1|n] = Ax[n|n] + Bu[n]
 *
 *  $Author: Tobias Naegeli $    $Date: 2014 $    $Revision: 3 $
 *
 *
 *  Arguments:
 *  approx_prediction: if 1 then the exponential map is approximated with a
 *  first order taylor approximation. has at the moment not a big influence
 *  (just 1st or 2nd order approximation) we should change it to rodriquez
 *  approximation.
 *  use_inertia_matrix: set to true if you have the inertia matrix J for your
 *  quadrotor
 *  xa_apo_k: old state vectotr
 *  zFlag: if sensor measurement is available [gyro, acc, mag]
 *  dt: dt in s
 *  z: measurements [gyro, acc, mag]
 *  q_rotSpeed: process noise gyro
 *  q_rotAcc: process noise gyro acceleration
 *  q_acc: process noise acceleration
 *  q_mag: process noise magnetometer
 *  r_gyro: measurement noise gyro
 *  r_accel: measurement noise accel
 *  r_mag: measurement noise mag
 *  J: moment of inertia matrix
 * Arguments    : unsigned char approx_prediction
 *                unsigned char use_inertia_matrix
 *                const unsigned char zFlag[3]
 *                float dt
 *                const float z[9]
 *                float q_rotSpeed
 *                float q_rotAcc
 *                float q_acc
 *                float q_mag
 *                float r_gyro
 *                float r_accel
 *                float r_mag
 *                const float J[9]
 *                float xa_apo[12]
 *                float Pa_apo[144]
 *                float Rot_matrix[9]
 *                float eulerAngles[3]
 *                float debugOutput[4]
 * Return Type  : void
 */
void AttitudeEKF(unsigned char approx_prediction, unsigned char
                 use_inertia_matrix, const unsigned char zFlag[3], float dt,
                 const float z[9], float q_rotSpeed, float q_rotAcc, float q_acc,
                 float q_mag, float r_gyro, float r_accel, float r_mag, const
                 float J[9], float xa_apo[12], float Pa_apo[144], float
                 Rot_matrix[9], float eulerAngles[3], float debugOutput[4])
{
  int i;
  float fv0[3];
  float wak[3];
  float zek[3];
  int r2;
  float b_zek[3];
  float fv1[3];
  float O[9];
  float b_O[9];
  static const signed char iv0[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

  float fv2[3];
  float maxval;
  int r1;
  float fv3[9];
  float fv4[3];
  float muk[3];
  float x_apr[12];
  signed char I[144];
  static float A_lin[144];
  static const signed char iv1[36] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  static float b_A_lin[144];
  float v[12];
  float P_apr[144];
  float a[108];
  static const signed char b_a[108] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  float S_k[81];
  static const signed char b[108] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  float b_r_gyro[9];
  float K_k[108];
  float b_S_k[36];
  static const signed char c_a[36] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  static const signed char b_b[36] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  float c_r_gyro[3];
  float y[36];
  int r3;
  float a21;
  float d_a[72];
  static const signed char e_a[72] = { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0 };

  static const signed char c_b[72] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0 };

  float d_r_gyro[6];
  float c_S_k[6];
  float b_K_k[72];
  static const signed char f_a[72] = { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 1 };

  static const signed char d_b[72] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1 };

  float b_z[6];
  float fv5[3];

  /*  function [xa_apo,Pa_apo,Rot_matrix,eulerAngles,debugOutput]... */
  /*      = AttitudeEKF(approx_prediction,use_inertia_matrix,zFlag,dt,z,q_rotSpeed,q_rotAcc,q_acc,q_mag,r_gyro,r_accel,r_mag,J) */
  /*   */
  /*   */
  /*  %LQG Position Estimator and Controller */
  /*  % Observer: */
  /*  %        x[n|n]   = x[n|n-1] + M(y[n] - Cx[n|n-1] - Du[n]) */
  /*  %        x[n+1|n] = Ax[n|n] + Bu[n] */
  /*  % */
  /*  % $Author: Tobias Naegeli $    $Date: 2014 $    $Revision: 3 $ */
  /*  % */
  /*  % */
  /*  % Arguments: */
  /*  % approx_prediction: if 1 then the exponential map is approximated with a */
  /*  % first order taylor approximation. has at the moment not a big influence */
  /*  % (just 1st or 2nd order approximation) we should change it to rodriquez */
  /*  % approximation. */
  /*  % use_inertia_matrix: set to true if you have the inertia matrix J for your */
  /*  % quadrotor */
  /*  % xa_apo_k: old state vectotr */
  /*  % zFlag: if sensor measurement is available [gyro, acc, mag] */
  /*  % dt: dt in s */
  /*  % z: measurements [gyro, acc, mag] */
  /*  % q_rotSpeed: process noise gyro */
  /*  % q_rotAcc: process noise gyro acceleration */
  /*  % q_acc: process noise acceleration */
  /*  % q_mag: process noise magnetometer */
  /*  % r_gyro: measurement noise gyro */
  /*  % r_accel: measurement noise accel */
  /*  % r_mag: measurement noise mag */
  /*  % J: moment of inertia matrix */
  /*   */
  /*   */
  /*  % Output: */
  /*  % xa_apo: updated state vectotr */
  /*  % Pa_apo: updated state covariance matrix */
  /*  % Rot_matrix: rotation matrix */
  /*  % eulerAngles: euler angles */
  /*  % debugOutput: not used */
  /*   */
  /*   */
  /*  %% model specific parameters */
  /*   */
  /*   */
  /*   */
  /*  % compute once the inverse of the Inertia */
  /*  persistent Ji; */
  /*  if isempty(Ji) */
  /*      Ji=single(inv(J)); */
  /*  end */
  /*   */
  /*  %% init */
  /*  persistent x_apo */
  /*  if(isempty(x_apo)) */
  /*      gyro_init=single([0;0;0]); */
  /*      gyro_acc_init=single([0;0;0]); */
  /*      acc_init=single([0;0;-9.81]); */
  /*  %     mag_init=single([1;0;0]); */
  /*  %     x_apo=single([gyro_init;gyro_acc_init;acc_init;mag_init]); */
  /*      x_apo=single([gyro_init;gyro_acc_init;acc_init]); */
  /*       */
  /*  end */
  /*   */
  /*  persistent P_apo */
  /*  if(isempty(P_apo)) */
  /*      %     P_apo = single(eye(NSTATES) * 1000); */
  /*  %     P_apo = single(200*ones(12)); */
  /*      P_apo = single(100*eye(9)); */
  /*  end */
  /*   */
  /*  debugOutput = single(zeros(4,1)); */
  /*   */
  /*  %% copy the states */
  /*  wx=  x_apo(1);   % x  body angular rate */
  /*  wy=  x_apo(2);   % y  body angular rate */
  /*  wz=  x_apo(3);   % z  body angular rate */
  /*   */
  /*  wax=  x_apo(4);  % x  body angular acceleration */
  /*  way=  x_apo(5);  % y  body angular acceleration */
  /*  waz=  x_apo(6);  % z  body angular acceleration */
  /*   */
  /*  zex=  x_apo(7);  % x  component gravity vector */
  /*  zey=  x_apo(8);  % y  component gravity vector */
  /*  zez=  x_apo(9);  % z  component gravity vector */
  /*   */
  /*  % mux=  x_apo(10); % x  component magnetic field vector */
  /*  % muy=  x_apo(11); % y  component magnetic field vector */
  /*  % muz=  x_apo(12); % z  component magnetic field vector */
  /*   */
  /*   */
  /*   */
  /*   */
  /*  %% prediction section */
  /*  % compute the apriori state estimate from the previous aposteriori estimate */
  /*  %body angular accelerations */
  /*  % if (use_inertia_matrix==1) */
  /*  %     wak =[wax;way;waz]+Ji*(-cross([wax;way;waz],J*[wax;way;waz]))*dt; */
  /*  % else */
  /*      wak =[wax;way;waz]; */
  /*  % end */
  /*   */
  /*  %body angular rates */
  /*  wk =[wx;  wy; wz] + dt*wak; */
  /*   */
  /*  %derivative of the prediction rotation matrix */
  /*  O=[0,-wz,wy;wz,0,-wx;-wy,wx,0]'; */
  /*   */
  /*  %prediction of the earth z vector */
  /*  % if (approx_prediction==1) */
  /*  %     %e^(Odt)=I+dt*O+dt^2/2!O^2 */
  /*  %     % so we do a first order approximation of the exponential map */
  /*  %     zek =(O*dt+single(eye(3)))*[zex;zey;zez]; */
  /*  %      */
  /*  % else */
  /*      zek =(single(eye(3))+O*dt+dt^2/2*O^2)*[zex;zey;zez]; */
  /*      %zek =expm2(O*dt)*[zex;zey;zez]; not working because use double */
  /*      %precision */
  /*  % end */
  /*   */
  /*   */
  /*   */
  /*  % %prediction of the magnetic vector */
  /*  % if (approx_prediction==1) */
  /*  %     %e^(Odt)=I+dt*O+dt^2/2!O^2 */
  /*  %     % so we do a first order approximation of the exponential map */
  /*  %     muk =(O*dt+single(eye(3)))*[mux;muy;muz]; */
  /*  % else */
  /*  %      muk =(single(eye(3))+O*dt+dt^2/2*O^2)*[mux;muy;muz]; */
  /*  %     %muk =expm2(O*dt)*[mux;muy;muz]; not working because use double */
  /*  %     %precision */
  /*  % end */
  /*   */
  /*  % x_apr=[wk;wak;zek;muk]; */
  /*  x_apr=[wk;wak;zek]; */
  /*   */
  /*  % compute the apriori error covariance estimate from the previous */
  /*  %aposteriori estimate */
  /*   */
  /*  EZ=[0,zez,-zey; */
  /*      -zez,0,zex; */
  /*      zey,-zex,0]'; */
  /*  % MA=[0,muz,-muy; */
  /*  %     -muz,0,mux; */
  /*  %     muy,-mux,0]'; */
  /*   */
  /*  E=single(eye(3)); */
  /*  Z=single(zeros(3)); */
  /*   */
  /*  % A_lin=[ Z,  E,  Z,  Z */
  /*  %     Z,  Z,  Z,  Z */
  /*  %     EZ, Z,  O,  Z */
  /*  %     MA, Z,  Z,  O]; */
  /*   */
  /*  A_lin=[ Z,  E,  Z */
  /*      Z,  Z,  Z */
  /*      EZ, Z,  O]; */
  /*  %     MA, Z,  Z,  O */
  /*   */
  /*  A_lin=eye(9)+A_lin*dt; */
  /*   */
  /*  %process covariance matrix */
  /*   */
  /*  persistent Q */
  /*  if (isempty(Q)) */
  /*  %     Q=diag([ q_rotSpeed,q_rotSpeed,q_rotSpeed,... */
  /*  %         q_rotAcc,q_rotAcc,q_rotAcc,... */
  /*  %         q_acc,q_acc,q_acc,... */
  /*  %         q_mag,q_mag,q_mag]); */
  /*       */
  /*      Q=diag([ q_rotSpeed,q_rotSpeed,q_rotSpeed,... */
  /*          q_rotAcc,q_rotAcc,q_rotAcc,... */
  /*          q_acc,q_acc,q_acc]); */
  /*  end */
  /*   */
  /*  P_apr=A_lin*P_apo*A_lin'+Q; */
  /*   */
  /*   */
  /*  %% update */
  /*  % if zFlag(1)==1&&zFlag(2)==1&&zFlag(3)==1 */
  /*  %      */
  /*  % %     R=[r_gyro,0,0,0,0,0,0,0,0; */
  /*  % %         0,r_gyro,0,0,0,0,0,0,0; */
  /*  % %         0,0,r_gyro,0,0,0,0,0,0; */
  /*  % %         0,0,0,r_accel,0,0,0,0,0; */
  /*  % %         0,0,0,0,r_accel,0,0,0,0; */
  /*  % %         0,0,0,0,0,r_accel,0,0,0; */
  /*  % %         0,0,0,0,0,0,r_mag,0,0; */
  /*  % %         0,0,0,0,0,0,0,r_mag,0; */
  /*  % %         0,0,0,0,0,0,0,0,r_mag]; */
  /*  %      R_v=[r_gyro,r_gyro,r_gyro,r_accel,r_accel,r_accel,r_mag,r_mag,r_mag]; */
  /*  %     %observation matrix */
  /*  %     %[zw;ze;zmk]; */
  /*  %     H_k=[  E,     Z,      Z,    Z; */
  /*  %         Z,     Z,      E,    Z; */
  /*  %         Z,     Z,      Z,    E]; */
  /*  %      */
  /*  %     y_k=z(1:9)-H_k*x_apr; */
  /*  %      */
  /*  %      */
  /*  %     %S_k=H_k*P_apr*H_k'+R; */
  /*  %      S_k=H_k*P_apr*H_k'; */
  /*  %      S_k(1:9+1:end) = S_k(1:9+1:end) + R_v; */
  /*  %     K_k=(P_apr*H_k'/(S_k)); */
  /*  %      */
  /*  %      */
  /*  %     x_apo=x_apr+K_k*y_k; */
  /*  %     P_apo=(eye(12)-K_k*H_k)*P_apr; */
  /*  % else */
  /*  %     if zFlag(1)==1&&zFlag(2)==0&&zFlag(3)==0 */
  /*  %          */
  /*  %         R=[r_gyro,0,0; */
  /*  %             0,r_gyro,0; */
  /*  %             0,0,r_gyro]; */
  /*  %         R_v=[r_gyro,r_gyro,r_gyro]; */
  /*  %         %observation matrix */
  /*  %          */
  /*  %         H_k=[  E,     Z,      Z,    Z]; */
  /*  %          */
  /*  %         y_k=z(1:3)-H_k(1:3,1:12)*x_apr; */
  /*  %          */
  /*  %        % S_k=H_k(1:3,1:12)*P_apr*H_k(1:3,1:12)'+R(1:3,1:3); */
  /*  %         S_k=H_k(1:3,1:12)*P_apr*H_k(1:3,1:12)'; */
  /*  %         S_k(1:3+1:end) = S_k(1:3+1:end) + R_v; */
  /*  %         K_k=(P_apr*H_k(1:3,1:12)'/(S_k)); */
  /*  %          */
  /*  %          */
  /*  %         x_apo=x_apr+K_k*y_k; */
  /*  %         P_apo=(eye(12)-K_k*H_k(1:3,1:12))*P_apr; */
  /*  %     else */
  /*  %         if  zFlag(1)==1&&zFlag(2)==1&&zFlag(3)==0 */
  /*               */
  /*  %             R=[r_gyro,0,0,0,0,0; */
  /*  %                 0,r_gyro,0,0,0,0; */
  /*  %                 0,0,r_gyro,0,0,0; */
  /*  %                 0,0,0,r_accel,0,0; */
  /*  %                 0,0,0,0,r_accel,0; */
  /*  %                 0,0,0,0,0,r_accel]; */
  /*               */
  /*              R_v=[r_gyro,r_gyro,r_gyro,r_accel,r_accel,r_accel]; */
  /*              %observation matrix */
  /*               */
  /*              H_k=[  E,     Z,      Z,    Z; */
  /*                  Z,     Z,      E,    Z]; */
  /*               */
  /*              y_k=z(1:6)-H_k(1:6,1:9)*x_apr; */
  /*               */
  /*             % S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'+R(1:6,1:6); */
  /*              S_k=H_k(1:6,1:9)*P_apr*H_k(1:6,1:9)'; */
  /*              S_k(1:6+1:end) = S_k(1:6+1:end) + R_v; */
  /*              K_k=(P_apr*H_k(1:6,1:9)'/(S_k)); */
  /*               */
  /*               */
  /*              x_apo=x_apr+K_k*y_k; */
  /*              P_apo=(eye(9)-K_k*H_k(1:6,1:9))*P_apr; */
  /*  %         else */
  /*  %             if  zFlag(1)==1&&zFlag(2)==0&&zFlag(3)==1 */
  /*  % %                 R=[r_gyro,0,0,0,0,0; */
  /*  % %                     0,r_gyro,0,0,0,0; */
  /*  % %                     0,0,r_gyro,0,0,0; */
  /*  % %                     0,0,0,r_mag,0,0; */
  /*  % %                     0,0,0,0,r_mag,0; */
  /*  % %                     0,0,0,0,0,r_mag]; */
  /*  %                   R_v=[r_gyro,r_gyro,r_gyro,r_mag,r_mag,r_mag]; */
  /*  %                 %observation matrix */
  /*  %                  */
  /*  %                 H_k=[  E,     Z,      Z,    Z; */
  /*  %                     Z,     Z,      Z,    E]; */
  /*  %                  */
  /*  %                 y_k=[z(1:3);z(7:9)]-H_k(1:6,1:12)*x_apr; */
  /*  %                  */
  /*  %                 %S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'+R(1:6,1:6); */
  /*  %                 S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'; */
  /*  %                 S_k(1:6+1:end) = S_k(1:6+1:end) + R_v; */
  /*  %                 K_k=(P_apr*H_k(1:6,1:12)'/(S_k)); */
  /*  %                  */
  /*  %                  */
  /*  %                 x_apo=x_apr+K_k*y_k; */
  /*  %                 P_apo=(eye(12)-K_k*H_k(1:6,1:12))*P_apr; */
  /*  %             else */
  /*  %                 x_apo=x_apr; */
  /*  %                 P_apo=P_apr; */
  /*  %             end */
  /*  %         end */
  /*  %     end */
  /*  % end */
  /*   */
  /*   */
  /*   */
  /*  %% euler anglels extraction */
  /*  z_n_b = -x_apo(7:9)./norm(x_apo(7:9)); */
  /*  % m_n_b = x_apo(10:12)./norm(x_apo(10:12)); */
  /*  m_n_b = [0.2;0.2;0.2]; */
  /*   */
  /*  y_n_b=cross(z_n_b,m_n_b); */
  /*  y_n_b=y_n_b./norm(y_n_b); */
  /*   */
  /*  x_n_b=(cross(y_n_b,z_n_b)); */
  /*  x_n_b=x_n_b./norm(x_n_b); */
  /*   */
  /*   */
  /*  xa_apo=x_apo; */
  /*  Pa_apo=P_apo; */
  /*  % rotation matrix from earth to body system */
  /*  Rot_matrix=[x_n_b,y_n_b,z_n_b]; */
  /*   */
  /*   */
  /*  phi=atan2(Rot_matrix(2,3),Rot_matrix(3,3)); */
  /*  theta=-asin(Rot_matrix(1,3)); */
  /*  psi=atan2(Rot_matrix(1,2),Rot_matrix(1,1)); */
  /*  eulerAngles=[phi;theta;psi]*57.29578; */
  /*  Output: */
  /*  xa_apo: updated state vectotr */
  /*  Pa_apo: updated state covariance matrix */
  /*  Rot_matrix: rotation matrix */
  /*  eulerAngles: euler angles */
  /*  debugOutput: not used */
  /* % model specific parameters */
  /*  compute once the inverse of the Inertia */
  /* 'AttitudeEKF:360' if isempty(Ji) */
  if (!Ji_not_empty) {
    /* 'AttitudeEKF:361' Ji=single(inv(J)); */
    inv(J, Ji);
    Ji_not_empty = true;
  }

  /* % init */
  /* 'AttitudeEKF:366' if(isempty(x_apo)) */
  /* 'AttitudeEKF:376' if(isempty(P_apo)) */
  /* 'AttitudeEKF:381' debugOutput = single(zeros(4,1)); */
  for (i = 0; i < 4; i++) {
    debugOutput[i] = 0.0F;
  }

  /* % copy the states */
  /* 'AttitudeEKF:384' wx=  x_apo(1); */
  /*  x  body angular rate */
  /* 'AttitudeEKF:385' wy=  x_apo(2); */
  /*  y  body angular rate */
  /* 'AttitudeEKF:386' wz=  x_apo(3); */
  /*  z  body angular rate */
  /* 'AttitudeEKF:388' wax=  x_apo(4); */
  /*  x  body angular acceleration */
  /* 'AttitudeEKF:389' way=  x_apo(5); */
  /*  y  body angular acceleration */
  /* 'AttitudeEKF:390' waz=  x_apo(6); */
  /*  z  body angular acceleration */
  /* 'AttitudeEKF:392' zex=  x_apo(7); */
  /*  x  component gravity vector */
  /* 'AttitudeEKF:393' zey=  x_apo(8); */
  /*  y  component gravity vector */
  /* 'AttitudeEKF:394' zez=  x_apo(9); */
  /*  z  component gravity vector */
  /* 'AttitudeEKF:396' mux=  x_apo(10); */
  /*  x  component magnetic field vector */
  /* 'AttitudeEKF:397' muy=  x_apo(11); */
  /*  y  component magnetic field vector */
  /* 'AttitudeEKF:398' muz=  x_apo(12); */
  /*  z  component magnetic field vector */
  /* % prediction section */
  /*  compute the apriori state estimate from the previous aposteriori estimate */
  /* body angular accelerations */
  /* 'AttitudeEKF:406' if (use_inertia_matrix==1) */
  if (use_inertia_matrix == 1) {
    /* 'AttitudeEKF:407' wak =[wax;way;waz]+Ji*(-cross([wax;way;waz],J*[wax;way;waz]))*dt; */
    fv0[0] = x_apo[3];
    fv0[1] = x_apo[4];
    fv0[2] = x_apo[5];
    zek[0] = x_apo[3];
    zek[1] = x_apo[4];
    zek[2] = x_apo[5];
    for (r2 = 0; r2 < 3; r2++) {
      wak[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        wak[r2] += J[r2 + 3 * i] * zek[i];
      }
    }

    cross(fv0, wak, b_zek);
    for (r2 = 0; r2 < 3; r2++) {
      zek[r2] = -b_zek[r2];
    }

    fv1[0] = x_apo[3];
    fv1[1] = x_apo[4];
    fv1[2] = x_apo[5];
    for (r2 = 0; r2 < 3; r2++) {
      fv0[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        fv0[r2] += Ji[r2 + 3 * i] * zek[i];
      }

      wak[r2] = fv1[r2] + fv0[r2] * dt;
    }
  } else {
    /* 'AttitudeEKF:408' else */
    /* 'AttitudeEKF:409' wak =[wax;way;waz]; */
    wak[0] = x_apo[3];
    wak[1] = x_apo[4];
    wak[2] = x_apo[5];
  }

  /* body angular rates */
  /* 'AttitudeEKF:413' wk =[wx;  wy; wz] + dt*wak; */
  /* derivative of the prediction rotation matrix */
  /* 'AttitudeEKF:416' O=[0,-wz,wy;wz,0,-wx;-wy,wx,0]'; */
  O[0] = 0.0F;
  O[1] = -x_apo[2];
  O[2] = x_apo[1];
  O[3] = x_apo[2];
  O[4] = 0.0F;
  O[5] = -x_apo[0];
  O[6] = -x_apo[1];
  O[7] = x_apo[0];
  O[8] = 0.0F;

  /* prediction of the earth z vector */
  /* 'AttitudeEKF:419' if (approx_prediction==1) */
  if (approx_prediction == 1) {
    /* e^(Odt)=I+dt*O+dt^2/2!O^2 */
    /*  so we do a first order approximation of the exponential map */
    /* 'AttitudeEKF:422' zek =(O*dt+single(eye(3)))*[zex;zey;zez]; */
    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        b_O[i + 3 * r2] = O[i + 3 * r2] * dt + (float)iv0[i + 3 * r2];
      }
    }

    fv2[0] = x_apo[6];
    fv2[1] = x_apo[7];
    fv2[2] = x_apo[8];
    for (r2 = 0; r2 < 3; r2++) {
      b_zek[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        b_zek[r2] += b_O[r2 + 3 * i] * fv2[i];
      }
    }
  } else {
    /* 'AttitudeEKF:424' else */
    /* 'AttitudeEKF:425' zek =(single(eye(3))+O*dt+dt^2/2*O^2)*[zex;zey;zez]; */
    maxval = dt * dt / 2.0F;
    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        b_O[r2 + 3 * i] = 0.0F;
        for (r1 = 0; r1 < 3; r1++) {
          b_O[r2 + 3 * i] += O[r2 + 3 * r1] * O[r1 + 3 * i];
        }
      }
    }

    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        fv3[i + 3 * r2] = ((float)iv0[i + 3 * r2] + O[i + 3 * r2] * dt) + maxval
          * b_O[i + 3 * r2];
      }
    }

    fv2[0] = x_apo[6];
    fv2[1] = x_apo[7];
    fv2[2] = x_apo[8];
    for (r2 = 0; r2 < 3; r2++) {
      b_zek[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        b_zek[r2] += fv3[r2 + 3 * i] * fv2[i];
      }
    }

    /* zek =expm2(O*dt)*[zex;zey;zez]; not working because use double */
    /* precision */
  }

  /* prediction of the magnetic vector */
  /* 'AttitudeEKF:433' if (approx_prediction==1) */
  if (approx_prediction == 1) {
    /* e^(Odt)=I+dt*O+dt^2/2!O^2 */
    /*  so we do a first order approximation of the exponential map */
    /* 'AttitudeEKF:436' muk =(O*dt+single(eye(3)))*[mux;muy;muz]; */
    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        b_O[i + 3 * r2] = O[i + 3 * r2] * dt + (float)iv0[i + 3 * r2];
      }
    }

    fv4[0] = x_apo[9];
    fv4[1] = x_apo[10];
    fv4[2] = x_apo[11];
    for (r2 = 0; r2 < 3; r2++) {
      muk[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        muk[r2] += b_O[r2 + 3 * i] * fv4[i];
      }
    }
  } else {
    /* 'AttitudeEKF:437' else */
    /* 'AttitudeEKF:438' muk =(single(eye(3))+O*dt+dt^2/2*O^2)*[mux;muy;muz]; */
    maxval = dt * dt / 2.0F;
    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        b_O[r2 + 3 * i] = 0.0F;
        for (r1 = 0; r1 < 3; r1++) {
          b_O[r2 + 3 * i] += O[r2 + 3 * r1] * O[r1 + 3 * i];
        }
      }
    }

    for (r2 = 0; r2 < 3; r2++) {
      for (i = 0; i < 3; i++) {
        fv3[i + 3 * r2] = ((float)iv0[i + 3 * r2] + O[i + 3 * r2] * dt) + maxval
          * b_O[i + 3 * r2];
      }
    }

    fv4[0] = x_apo[9];
    fv4[1] = x_apo[10];
    fv4[2] = x_apo[11];
    for (r2 = 0; r2 < 3; r2++) {
      muk[r2] = 0.0F;
      for (i = 0; i < 3; i++) {
        muk[r2] += fv3[r2 + 3 * i] * fv4[i];
      }
    }

    /* muk =expm2(O*dt)*[mux;muy;muz]; not working because use double */
    /* precision */
  }

  /* 'AttitudeEKF:443' x_apr=[wk;wak;zek;muk]; */
  x_apr[0] = x_apo[0] + dt * wak[0];
  x_apr[1] = x_apo[1] + dt * wak[1];
  x_apr[2] = x_apo[2] + dt * wak[2];
  for (i = 0; i < 3; i++) {
    x_apr[i + 3] = wak[i];
  }

  for (i = 0; i < 3; i++) {
    x_apr[i + 6] = b_zek[i];
  }

  for (i = 0; i < 3; i++) {
    x_apr[i + 9] = muk[i];
  }

  /*  compute the apriori error covariance estimate from the previous */
  /* aposteriori estimate */
  /* 'AttitudeEKF:448' EZ=[0,zez,-zey; */
  /* 'AttitudeEKF:449'     -zez,0,zex; */
  /* 'AttitudeEKF:450'     zey,-zex,0]'; */
  /* 'AttitudeEKF:451' MA=[0,muz,-muy; */
  /* 'AttitudeEKF:452'     -muz,0,mux; */
  /* 'AttitudeEKF:453'     muy,-mux,0]'; */
  /* 'AttitudeEKF:455' E=single(eye(3)); */
  /* 'AttitudeEKF:456' Z=single(zeros(3)); */
  /* 'AttitudeEKF:458' A_lin=[ Z,  E,  Z,  Z */
  /* 'AttitudeEKF:459'     Z,  Z,  Z,  Z */
  /* 'AttitudeEKF:460'     EZ, Z,  O,  Z */
  /* 'AttitudeEKF:461'     MA, Z,  Z,  O]; */
  /* 'AttitudeEKF:463' A_lin=eye(12)+A_lin*dt; */
  memset(&I[0], 0, 144U * sizeof(signed char));
  for (i = 0; i < 12; i++) {
    I[i + 12 * i] = 1;
    for (r2 = 0; r2 < 3; r2++) {
      A_lin[r2 + 12 * i] = iv1[r2 + 3 * i];
    }

    for (r2 = 0; r2 < 3; r2++) {
      A_lin[(r2 + 12 * i) + 3] = 0.0F;
    }
  }

  A_lin[6] = 0.0F;
  A_lin[7] = x_apo[8];
  A_lin[8] = -x_apo[7];
  A_lin[18] = -x_apo[8];
  A_lin[19] = 0.0F;
  A_lin[20] = x_apo[6];
  A_lin[30] = x_apo[7];
  A_lin[31] = -x_apo[6];
  A_lin[32] = 0.0F;
  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 3)) + 6] = 0.0F;
    }
  }

  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 6)) + 6] = O[i + 3 * r2];
    }
  }

  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 9)) + 6] = 0.0F;
    }
  }

  A_lin[9] = 0.0F;
  A_lin[10] = x_apo[11];
  A_lin[11] = -x_apo[10];
  A_lin[21] = -x_apo[11];
  A_lin[22] = 0.0F;
  A_lin[23] = x_apo[9];
  A_lin[33] = x_apo[10];
  A_lin[34] = -x_apo[9];
  A_lin[35] = 0.0F;
  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 3)) + 9] = 0.0F;
    }
  }

  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 6)) + 9] = 0.0F;
    }
  }

  for (r2 = 0; r2 < 3; r2++) {
    for (i = 0; i < 3; i++) {
      A_lin[(i + 12 * (r2 + 9)) + 9] = O[i + 3 * r2];
    }
  }

  for (r2 = 0; r2 < 12; r2++) {
    for (i = 0; i < 12; i++) {
      b_A_lin[i + 12 * r2] = (float)I[i + 12 * r2] + A_lin[i + 12 * r2] * dt;
    }
  }

  /* process covariance matrix */
  /* 'AttitudeEKF:468' if (isempty(Q)) */
  if (!Q_not_empty) {
    /* 'AttitudeEKF:469' Q=diag([ q_rotSpeed,q_rotSpeed,q_rotSpeed,... */
    /* 'AttitudeEKF:470'         q_rotAcc,q_rotAcc,q_rotAcc,... */
    /* 'AttitudeEKF:471'         q_acc,q_acc,q_acc,... */
    /* 'AttitudeEKF:472'         q_mag,q_mag,q_mag]); */
    v[0] = q_rotSpeed;
    v[1] = q_rotSpeed;
    v[2] = q_rotSpeed;
    v[3] = q_rotAcc;
    v[4] = q_rotAcc;
    v[5] = q_rotAcc;
    v[6] = q_acc;
    v[7] = q_acc;
    v[8] = q_acc;
    v[9] = q_mag;
    v[10] = q_mag;
    v[11] = q_mag;
    memset(&Q[0], 0, 144U * sizeof(float));
    for (i = 0; i < 12; i++) {
      Q[i + 12 * i] = v[i];
    }

    Q_not_empty = true;
  }

  /* 'AttitudeEKF:475' P_apr=A_lin*P_apo*A_lin'+Q; */
  for (r2 = 0; r2 < 12; r2++) {
    for (i = 0; i < 12; i++) {
      A_lin[r2 + 12 * i] = 0.0F;
      for (r1 = 0; r1 < 12; r1++) {
        A_lin[r2 + 12 * i] += b_A_lin[r2 + 12 * r1] * P_apo[r1 + 12 * i];
      }
    }
  }

  for (r2 = 0; r2 < 12; r2++) {
    for (i = 0; i < 12; i++) {
      maxval = 0.0F;
      for (r1 = 0; r1 < 12; r1++) {
        maxval += A_lin[r2 + 12 * r1] * b_A_lin[i + 12 * r1];
      }

      P_apr[r2 + 12 * i] = maxval + Q[r2 + 12 * i];
    }
  }

  /* % update */
  /* 'AttitudeEKF:479' if zFlag(1)==1&&zFlag(2)==1&&zFlag(3)==1 */
  if ((zFlag[0] == 1) && (zFlag[1] == 1) && (zFlag[2] == 1)) {
    /*      R=[r_gyro,0,0,0,0,0,0,0,0; */
    /*          0,r_gyro,0,0,0,0,0,0,0; */
    /*          0,0,r_gyro,0,0,0,0,0,0; */
    /*          0,0,0,r_accel,0,0,0,0,0; */
    /*          0,0,0,0,r_accel,0,0,0,0; */
    /*          0,0,0,0,0,r_accel,0,0,0; */
    /*          0,0,0,0,0,0,r_mag,0,0; */
    /*          0,0,0,0,0,0,0,r_mag,0; */
    /*          0,0,0,0,0,0,0,0,r_mag]; */
    /* 'AttitudeEKF:490' R_v=[r_gyro,r_gyro,r_gyro,r_accel,r_accel,r_accel,r_mag,r_mag,r_mag]; */
    /* observation matrix */
    /* [zw;ze;zmk]; */
    /* 'AttitudeEKF:493' H_k=[  E,     Z,      Z,    Z; */
    /* 'AttitudeEKF:494'         Z,     Z,      E,    Z; */
    /* 'AttitudeEKF:495'         Z,     Z,      Z,    E]; */
    /* 'AttitudeEKF:497' y_k=z(1:9)-H_k*x_apr; */
    /* S_k=H_k*P_apr*H_k'+R; */
    /* 'AttitudeEKF:501' S_k=H_k*P_apr*H_k'; */
    for (r2 = 0; r2 < 9; r2++) {
      for (i = 0; i < 12; i++) {
        a[r2 + 9 * i] = 0.0F;
        for (r1 = 0; r1 < 12; r1++) {
          a[r2 + 9 * i] += (float)b_a[r2 + 9 * r1] * P_apr[r1 + 12 * i];
        }
      }

      for (i = 0; i < 9; i++) {
        S_k[r2 + 9 * i] = 0.0F;
        for (r1 = 0; r1 < 12; r1++) {
          S_k[r2 + 9 * i] += a[r2 + 9 * r1] * (float)b[r1 + 12 * i];
        }
      }
    }

    /* 'AttitudeEKF:502' S_k(1:9+1:end) = S_k(1:9+1:end) + R_v; */
    b_r_gyro[0] = r_gyro;
    b_r_gyro[1] = r_gyro;
    b_r_gyro[2] = r_gyro;
    b_r_gyro[3] = r_accel;
    b_r_gyro[4] = r_accel;
    b_r_gyro[5] = r_accel;
    b_r_gyro[6] = r_mag;
    b_r_gyro[7] = r_mag;
    b_r_gyro[8] = r_mag;
    for (r2 = 0; r2 < 9; r2++) {
      O[r2] = S_k[10 * r2] + b_r_gyro[r2];
    }

    for (r2 = 0; r2 < 9; r2++) {
      S_k[10 * r2] = O[r2];
    }

    /* 'AttitudeEKF:503' K_k=(P_apr*H_k'/(S_k)); */
    for (r2 = 0; r2 < 12; r2++) {
      for (i = 0; i < 9; i++) {
        K_k[r2 + 12 * i] = 0.0F;
        for (r1 = 0; r1 < 12; r1++) {
          K_k[r2 + 12 * i] += P_apr[r2 + 12 * r1] * (float)b[r1 + 12 * i];
        }
      }
    }

    mrdivide(K_k, S_k);

    /* 'AttitudeEKF:506' x_apo=x_apr+K_k*y_k; */
    for (r2 = 0; r2 < 9; r2++) {
      maxval = 0.0F;
      for (i = 0; i < 12; i++) {
        maxval += (float)b_a[r2 + 9 * i] * x_apr[i];
      }

      O[r2] = z[r2] - maxval;
    }

    for (r2 = 0; r2 < 12; r2++) {
      maxval = 0.0F;
      for (i = 0; i < 9; i++) {
        maxval += K_k[r2 + 12 * i] * O[i];
      }

      x_apo[r2] = x_apr[r2] + maxval;
    }

    /* 'AttitudeEKF:507' P_apo=(eye(12)-K_k*H_k)*P_apr; */
    memset(&I[0], 0, 144U * sizeof(signed char));
    for (i = 0; i < 12; i++) {
      I[i + 12 * i] = 1;
    }

    for (r2 = 0; r2 < 12; r2++) {
      for (i = 0; i < 12; i++) {
        maxval = 0.0F;
        for (r1 = 0; r1 < 9; r1++) {
          maxval += K_k[r2 + 12 * r1] * (float)b_a[r1 + 9 * i];
        }

        A_lin[r2 + 12 * i] = (float)I[r2 + 12 * i] - maxval;
      }
    }

    for (r2 = 0; r2 < 12; r2++) {
      for (i = 0; i < 12; i++) {
        P_apo[r2 + 12 * i] = 0.0F;
        for (r1 = 0; r1 < 12; r1++) {
          P_apo[r2 + 12 * i] += A_lin[r2 + 12 * r1] * P_apr[r1 + 12 * i];
        }
      }
    }
  } else {
    /* 'AttitudeEKF:508' else */
    /* 'AttitudeEKF:509' if zFlag(1)==1&&zFlag(2)==0&&zFlag(3)==0 */
    if ((zFlag[0] == 1) && (zFlag[1] == 0) && (zFlag[2] == 0)) {
      /* 'AttitudeEKF:511' R=[r_gyro,0,0; */
      /* 'AttitudeEKF:512'             0,r_gyro,0; */
      /* 'AttitudeEKF:513'             0,0,r_gyro]; */
      /* 'AttitudeEKF:514' R_v=[r_gyro,r_gyro,r_gyro]; */
      /* observation matrix */
      /* 'AttitudeEKF:517' H_k=[  E,     Z,      Z,    Z]; */
      /* 'AttitudeEKF:519' y_k=z(1:3)-H_k(1:3,1:12)*x_apr; */
      /*  S_k=H_k(1:3,1:12)*P_apr*H_k(1:3,1:12)'+R(1:3,1:3); */
      /* 'AttitudeEKF:522' S_k=H_k(1:3,1:12)*P_apr*H_k(1:3,1:12)'; */
      for (r2 = 0; r2 < 3; r2++) {
        for (i = 0; i < 12; i++) {
          b_S_k[r2 + 3 * i] = 0.0F;
          for (r1 = 0; r1 < 12; r1++) {
            b_S_k[r2 + 3 * i] += (float)c_a[r2 + 3 * r1] * P_apr[r1 + 12 * i];
          }
        }

        for (i = 0; i < 3; i++) {
          O[r2 + 3 * i] = 0.0F;
          for (r1 = 0; r1 < 12; r1++) {
            O[r2 + 3 * i] += b_S_k[r2 + 3 * r1] * (float)b_b[r1 + 12 * i];
          }
        }
      }

      /* 'AttitudeEKF:523' S_k(1:3+1:end) = S_k(1:3+1:end) + R_v; */
      c_r_gyro[0] = r_gyro;
      c_r_gyro[1] = r_gyro;
      c_r_gyro[2] = r_gyro;
      for (r2 = 0; r2 < 3; r2++) {
        zek[r2] = O[r2 << 2] + c_r_gyro[r2];
      }

      for (r2 = 0; r2 < 3; r2++) {
        O[r2 << 2] = zek[r2];
      }

      /* 'AttitudeEKF:524' K_k=(P_apr*H_k(1:3,1:12)'/(S_k)); */
      for (r2 = 0; r2 < 12; r2++) {
        for (i = 0; i < 3; i++) {
          y[r2 + 12 * i] = 0.0F;
          for (r1 = 0; r1 < 12; r1++) {
            y[r2 + 12 * i] += P_apr[r2 + 12 * r1] * (float)b_b[r1 + 12 * i];
          }
        }
      }

      r1 = 0;
      r2 = 1;
      r3 = 2;
      maxval = (real32_T)fabs(O[0]);
      a21 = (real32_T)fabs(O[1]);
      if (a21 > maxval) {
        maxval = a21;
        r1 = 1;
        r2 = 0;
      }

      if ((real32_T)fabs(O[2]) > maxval) {
        r1 = 2;
        r2 = 1;
        r3 = 0;
      }

      O[r2] /= O[r1];
      O[r3] /= O[r1];
      O[3 + r2] -= O[r2] * O[3 + r1];
      O[3 + r3] -= O[r3] * O[3 + r1];
      O[6 + r2] -= O[r2] * O[6 + r1];
      O[6 + r3] -= O[r3] * O[6 + r1];
      if ((real32_T)fabs(O[3 + r3]) > (real32_T)fabs(O[3 + r2])) {
        i = r2;
        r2 = r3;
        r3 = i;
      }

      O[3 + r3] /= O[3 + r2];
      O[6 + r3] -= O[3 + r3] * O[6 + r2];
      for (i = 0; i < 12; i++) {
        b_S_k[i + 12 * r1] = y[i] / O[r1];
        b_S_k[i + 12 * r2] = y[12 + i] - b_S_k[i + 12 * r1] * O[3 + r1];
        b_S_k[i + 12 * r3] = y[24 + i] - b_S_k[i + 12 * r1] * O[6 + r1];
        b_S_k[i + 12 * r2] /= O[3 + r2];
        b_S_k[i + 12 * r3] -= b_S_k[i + 12 * r2] * O[6 + r2];
        b_S_k[i + 12 * r3] /= O[6 + r3];
        b_S_k[i + 12 * r2] -= b_S_k[i + 12 * r3] * O[3 + r3];
        b_S_k[i + 12 * r1] -= b_S_k[i + 12 * r3] * O[r3];
        b_S_k[i + 12 * r1] -= b_S_k[i + 12 * r2] * O[r2];
      }

      /* 'AttitudeEKF:527' x_apo=x_apr+K_k*y_k; */
      for (r2 = 0; r2 < 3; r2++) {
        maxval = 0.0F;
        for (i = 0; i < 12; i++) {
          maxval += (float)c_a[r2 + 3 * i] * x_apr[i];
        }

        zek[r2] = z[r2] - maxval;
      }

      for (r2 = 0; r2 < 12; r2++) {
        maxval = 0.0F;
        for (i = 0; i < 3; i++) {
          maxval += b_S_k[r2 + 12 * i] * zek[i];
        }

        x_apo[r2] = x_apr[r2] + maxval;
      }

      /* 'AttitudeEKF:528' P_apo=(eye(12)-K_k*H_k(1:3,1:12))*P_apr; */
      memset(&I[0], 0, 144U * sizeof(signed char));
      for (i = 0; i < 12; i++) {
        I[i + 12 * i] = 1;
      }

      for (r2 = 0; r2 < 12; r2++) {
        for (i = 0; i < 12; i++) {
          maxval = 0.0F;
          for (r1 = 0; r1 < 3; r1++) {
            maxval += b_S_k[r2 + 12 * r1] * (float)c_a[r1 + 3 * i];
          }

          A_lin[r2 + 12 * i] = (float)I[r2 + 12 * i] - maxval;
        }
      }

      for (r2 = 0; r2 < 12; r2++) {
        for (i = 0; i < 12; i++) {
          P_apo[r2 + 12 * i] = 0.0F;
          for (r1 = 0; r1 < 12; r1++) {
            P_apo[r2 + 12 * i] += A_lin[r2 + 12 * r1] * P_apr[r1 + 12 * i];
          }
        }
      }
    } else {
      /* 'AttitudeEKF:529' else */
      /* 'AttitudeEKF:530' if  zFlag(1)==1&&zFlag(2)==1&&zFlag(3)==0 */
      if ((zFlag[0] == 1) && (zFlag[1] == 1) && (zFlag[2] == 0)) {
        /*              R=[r_gyro,0,0,0,0,0; */
        /*                  0,r_gyro,0,0,0,0; */
        /*                  0,0,r_gyro,0,0,0; */
        /*                  0,0,0,r_accel,0,0; */
        /*                  0,0,0,0,r_accel,0; */
        /*                  0,0,0,0,0,r_accel]; */
        /* 'AttitudeEKF:539' R_v=[r_gyro,r_gyro,r_gyro,r_accel,r_accel,r_accel]; */
        /* observation matrix */
        /* 'AttitudeEKF:542' H_k=[  E,     Z,      Z,    Z; */
        /* 'AttitudeEKF:543'                 Z,     Z,      E,    Z]; */
        /* 'AttitudeEKF:545' y_k=z(1:6)-H_k(1:6,1:12)*x_apr; */
        /*  S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'+R(1:6,1:6); */
        /* 'AttitudeEKF:548' S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'; */
        for (r2 = 0; r2 < 6; r2++) {
          for (i = 0; i < 12; i++) {
            d_a[r2 + 6 * i] = 0.0F;
            for (r1 = 0; r1 < 12; r1++) {
              d_a[r2 + 6 * i] += (float)e_a[r2 + 6 * r1] * P_apr[r1 + 12 * i];
            }
          }

          for (i = 0; i < 6; i++) {
            b_S_k[r2 + 6 * i] = 0.0F;
            for (r1 = 0; r1 < 12; r1++) {
              b_S_k[r2 + 6 * i] += d_a[r2 + 6 * r1] * (float)c_b[r1 + 12 * i];
            }
          }
        }

        /* 'AttitudeEKF:549' S_k(1:6+1:end) = S_k(1:6+1:end) + R_v; */
        d_r_gyro[0] = r_gyro;
        d_r_gyro[1] = r_gyro;
        d_r_gyro[2] = r_gyro;
        d_r_gyro[3] = r_accel;
        d_r_gyro[4] = r_accel;
        d_r_gyro[5] = r_accel;
        for (r2 = 0; r2 < 6; r2++) {
          c_S_k[r2] = b_S_k[7 * r2] + d_r_gyro[r2];
        }

        for (r2 = 0; r2 < 6; r2++) {
          b_S_k[7 * r2] = c_S_k[r2];
        }

        /* 'AttitudeEKF:550' K_k=(P_apr*H_k(1:6,1:12)'/(S_k)); */
        for (r2 = 0; r2 < 12; r2++) {
          for (i = 0; i < 6; i++) {
            b_K_k[r2 + 12 * i] = 0.0F;
            for (r1 = 0; r1 < 12; r1++) {
              b_K_k[r2 + 12 * i] += P_apr[r2 + 12 * r1] * (float)c_b[r1 + 12 * i];
            }
          }
        }

        b_mrdivide(b_K_k, b_S_k);

        /* 'AttitudeEKF:553' x_apo=x_apr+K_k*y_k; */
        for (r2 = 0; r2 < 6; r2++) {
          maxval = 0.0F;
          for (i = 0; i < 12; i++) {
            maxval += (float)e_a[r2 + 6 * i] * x_apr[i];
          }

          d_r_gyro[r2] = z[r2] - maxval;
        }

        for (r2 = 0; r2 < 12; r2++) {
          maxval = 0.0F;
          for (i = 0; i < 6; i++) {
            maxval += b_K_k[r2 + 12 * i] * d_r_gyro[i];
          }

          x_apo[r2] = x_apr[r2] + maxval;
        }

        /* 'AttitudeEKF:554' P_apo=(eye(12)-K_k*H_k(1:6,1:12))*P_apr; */
        memset(&I[0], 0, 144U * sizeof(signed char));
        for (i = 0; i < 12; i++) {
          I[i + 12 * i] = 1;
        }

        for (r2 = 0; r2 < 12; r2++) {
          for (i = 0; i < 12; i++) {
            maxval = 0.0F;
            for (r1 = 0; r1 < 6; r1++) {
              maxval += b_K_k[r2 + 12 * r1] * (float)e_a[r1 + 6 * i];
            }

            A_lin[r2 + 12 * i] = (float)I[r2 + 12 * i] - maxval;
          }
        }

        for (r2 = 0; r2 < 12; r2++) {
          for (i = 0; i < 12; i++) {
            P_apo[r2 + 12 * i] = 0.0F;
            for (r1 = 0; r1 < 12; r1++) {
              P_apo[r2 + 12 * i] += A_lin[r2 + 12 * r1] * P_apr[r1 + 12 * i];
            }
          }
        }
      } else {
        /* 'AttitudeEKF:555' else */
        /* 'AttitudeEKF:556' if  zFlag(1)==1&&zFlag(2)==0&&zFlag(3)==1 */
        if ((zFlag[0] == 1) && (zFlag[1] == 0) && (zFlag[2] == 1)) {
          /*                  R=[r_gyro,0,0,0,0,0; */
          /*                      0,r_gyro,0,0,0,0; */
          /*                      0,0,r_gyro,0,0,0; */
          /*                      0,0,0,r_mag,0,0; */
          /*                      0,0,0,0,r_mag,0; */
          /*                      0,0,0,0,0,r_mag]; */
          /* 'AttitudeEKF:563' R_v=[r_gyro,r_gyro,r_gyro,r_mag,r_mag,r_mag]; */
          /* observation matrix */
          /* 'AttitudeEKF:566' H_k=[  E,     Z,      Z,    Z; */
          /* 'AttitudeEKF:567'                     Z,     Z,      Z,    E]; */
          /* 'AttitudeEKF:569' y_k=[z(1:3);z(7:9)]-H_k(1:6,1:12)*x_apr; */
          /* S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'+R(1:6,1:6); */
          /* 'AttitudeEKF:572' S_k=H_k(1:6,1:12)*P_apr*H_k(1:6,1:12)'; */
          for (r2 = 0; r2 < 6; r2++) {
            for (i = 0; i < 12; i++) {
              d_a[r2 + 6 * i] = 0.0F;
              for (r1 = 0; r1 < 12; r1++) {
                d_a[r2 + 6 * i] += (float)f_a[r2 + 6 * r1] * P_apr[r1 + 12 * i];
              }
            }

            for (i = 0; i < 6; i++) {
              b_S_k[r2 + 6 * i] = 0.0F;
              for (r1 = 0; r1 < 12; r1++) {
                b_S_k[r2 + 6 * i] += d_a[r2 + 6 * r1] * (float)d_b[r1 + 12 * i];
              }
            }
          }

          /* 'AttitudeEKF:573' S_k(1:6+1:end) = S_k(1:6+1:end) + R_v; */
          d_r_gyro[0] = r_gyro;
          d_r_gyro[1] = r_gyro;
          d_r_gyro[2] = r_gyro;
          d_r_gyro[3] = r_mag;
          d_r_gyro[4] = r_mag;
          d_r_gyro[5] = r_mag;
          for (r2 = 0; r2 < 6; r2++) {
            c_S_k[r2] = b_S_k[7 * r2] + d_r_gyro[r2];
          }

          for (r2 = 0; r2 < 6; r2++) {
            b_S_k[7 * r2] = c_S_k[r2];
          }

          /* 'AttitudeEKF:574' K_k=(P_apr*H_k(1:6,1:12)'/(S_k)); */
          for (r2 = 0; r2 < 12; r2++) {
            for (i = 0; i < 6; i++) {
              b_K_k[r2 + 12 * i] = 0.0F;
              for (r1 = 0; r1 < 12; r1++) {
                b_K_k[r2 + 12 * i] += P_apr[r2 + 12 * r1] * (float)d_b[r1 + 12 *
                  i];
              }
            }
          }

          b_mrdivide(b_K_k, b_S_k);

          /* 'AttitudeEKF:577' x_apo=x_apr+K_k*y_k; */
          for (r2 = 0; r2 < 3; r2++) {
            d_r_gyro[r2] = z[r2];
          }

          for (r2 = 0; r2 < 3; r2++) {
            d_r_gyro[r2 + 3] = z[6 + r2];
          }

          for (r2 = 0; r2 < 6; r2++) {
            c_S_k[r2] = 0.0F;
            for (i = 0; i < 12; i++) {
              c_S_k[r2] += (float)f_a[r2 + 6 * i] * x_apr[i];
            }

            b_z[r2] = d_r_gyro[r2] - c_S_k[r2];
          }

          for (r2 = 0; r2 < 12; r2++) {
            maxval = 0.0F;
            for (i = 0; i < 6; i++) {
              maxval += b_K_k[r2 + 12 * i] * b_z[i];
            }

            x_apo[r2] = x_apr[r2] + maxval;
          }

          /* 'AttitudeEKF:578' P_apo=(eye(12)-K_k*H_k(1:6,1:12))*P_apr; */
          memset(&I[0], 0, 144U * sizeof(signed char));
          for (i = 0; i < 12; i++) {
            I[i + 12 * i] = 1;
          }

          for (r2 = 0; r2 < 12; r2++) {
            for (i = 0; i < 12; i++) {
              maxval = 0.0F;
              for (r1 = 0; r1 < 6; r1++) {
                maxval += b_K_k[r2 + 12 * r1] * (float)f_a[r1 + 6 * i];
              }

              A_lin[r2 + 12 * i] = (float)I[r2 + 12 * i] - maxval;
            }
          }

          for (r2 = 0; r2 < 12; r2++) {
            for (i = 0; i < 12; i++) {
              P_apo[r2 + 12 * i] = 0.0F;
              for (r1 = 0; r1 < 12; r1++) {
                P_apo[r2 + 12 * i] += A_lin[r2 + 12 * r1] * P_apr[r1 + 12 * i];
              }
            }
          }
        } else {
          /* 'AttitudeEKF:579' else */
          /* 'AttitudeEKF:580' x_apo=x_apr; */
          for (i = 0; i < 12; i++) {
            x_apo[i] = x_apr[i];
          }

          /* 'AttitudeEKF:581' P_apo=P_apr; */
          memcpy(&P_apo[0], &P_apr[0], 144U * sizeof(float));
        }
      }
    }
  }

  /* % euler anglels extraction */
  /* 'AttitudeEKF:590' z_n_b = -x_apo(7:9)./norm(x_apo(7:9)); */
  maxval = norm(*(float (*)[3])&x_apo[6]);
  a21 = norm(*(float (*)[3])&x_apo[9]);
  for (i = 0; i < 3; i++) {
    /* 'AttitudeEKF:591' m_n_b = x_apo(10:12)./norm(x_apo(10:12)); */
    muk[i] = -x_apo[i + 6] / maxval;
    b_zek[i] = x_apo[i + 9] / a21;
  }

  /* 'AttitudeEKF:593' y_n_b=cross(z_n_b,m_n_b); */
  wak[0] = muk[1] * b_zek[2] - muk[2] * b_zek[1];
  wak[1] = muk[2] * b_zek[0] - muk[0] * b_zek[2];
  wak[2] = muk[0] * b_zek[1] - muk[1] * b_zek[0];

  /* 'AttitudeEKF:594' y_n_b=y_n_b./norm(y_n_b); */
  maxval = norm(wak);
  for (r2 = 0; r2 < 3; r2++) {
    wak[r2] /= maxval;
  }

  /* 'AttitudeEKF:596' x_n_b=(cross(y_n_b,z_n_b)); */
  b_zek[0] = wak[1] * muk[2] - wak[2] * muk[1];
  b_zek[1] = wak[2] * muk[0] - wak[0] * muk[2];
  b_zek[2] = wak[0] * muk[1] - wak[1] * muk[0];

  /* 'AttitudeEKF:597' x_n_b=x_n_b./norm(x_n_b); */
  maxval = norm(b_zek);
  for (r2 = 0; r2 < 3; r2++) {
    b_zek[r2] /= maxval;
  }

  /* 'AttitudeEKF:600' xa_apo=x_apo; */
  for (i = 0; i < 12; i++) {
    xa_apo[i] = x_apo[i];
  }

  /* 'AttitudeEKF:601' Pa_apo=P_apo; */
  memcpy(&Pa_apo[0], &P_apo[0], 144U * sizeof(float));

  /*  rotation matrix from earth to body system */
  /* 'AttitudeEKF:603' Rot_matrix=[x_n_b,y_n_b,z_n_b]; */
  for (r2 = 0; r2 < 3; r2++) {
    Rot_matrix[r2] = b_zek[r2];
    Rot_matrix[3 + r2] = wak[r2];
    Rot_matrix[6 + r2] = muk[r2];
  }

  /* 'AttitudeEKF:606' phi=atan2(Rot_matrix(2,3),Rot_matrix(3,3)); */
  /* 'AttitudeEKF:607' theta=-asin(Rot_matrix(1,3)); */
  /* 'AttitudeEKF:608' psi=atan2(Rot_matrix(1,2),Rot_matrix(1,1)); */
  /* 'AttitudeEKF:609' eulerAngles=[phi;theta;psi]*57.29578; */
  fv5[0] = (real32_T)atan2(Rot_matrix[7], Rot_matrix[8]);
  fv5[1] = -(real32_T)asin(Rot_matrix[6]);
  fv5[2] = (real32_T)atan2(Rot_matrix[3], Rot_matrix[0]);
  for (r2 = 0; r2 < 3; r2++) {
    eulerAngles[r2] = fv5[r2] * 57.2957802F;
  }
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void AttitudeEKF_initialize(void)
{
  Q_not_empty = false;
  Ji_not_empty = false;
  AttitudeEKF_init();
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void AttitudeEKF_terminate(void)
{
  /* (no terminate code required) */
}

/*
 * File trailer for AttitudeEKF.c
 *
 * [EOF]
 */
