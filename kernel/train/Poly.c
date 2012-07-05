#include "Poly.h"

// time in ms, velocity in mm/10microsecond
void poly_init(Poly* p, int t0, int t1, int v0, int v1) {
  p->t0 = t0;
  p->t1 = t1;
  p->v0 = v0;
  p->v1 = v1;
}

float eval_dist(const Poly const* p, int t) {
  int uNumer = t - p->t0;
  int uDeno =  p->t1 - p->t0;

  int deltaT = p->t1 - p->t0;
  int deltaV = p->v1 - p->v0;

  float u = (float)uNumer / uDeno;
  float part1 = -u * u * u * u * deltaT * deltaV / 100000 / 2;
  float part2 = u * u * u * deltaT * deltaV / 100000;
  float part3 = u * p->v0 * deltaT / 100000;
  return part1 + part2 + part3;
}

int eval_velo(const Poly const* p, int t) {
  int uNumer = t - p->t0;
  int uDeno =  p->t1 -p ->t0;

  int deltaV = p->v1 - p->v0;

  float u = (float)uNumer / uDeno;
  float part1 = - u * u * u * deltaV * 2;
  float part2 = u * u * deltaV * 3;
  return (int)(part1 + part2 + p->v0);
}
