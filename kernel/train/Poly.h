#ifndef Poly_h
#define Poly_h

typedef struct Poly {
  int t0;
  int t1;
  int v0;
  int v1;
} Poly;

void poly_init(Poly* p, int t0, int t1, int v0, int v1);

int eval_dist(const Poly const* p, int t);
int eval_velo(const Poly const* p, int t);

#endif // Poly.h
