#include "Poly.h"

// make a new polynomial p(x) = sum_{n=0}^{4} a_n * x^n
void velocity_poly_init(Poly* p, float a0, float a1, float a2, float a3, float a4) {
	p->a[0] = a0;
	p->a[1] = a1;
	p->a[2] = a2;
	p->a[3] = a3;
	p->a[4] = a4;
  p->order = 4;
}

void distance_poly_init(Poly* p, float a0, float a1, float a2, float a3, float a4, float a5) {
  p->a[0] = a0;
	p->a[1] = a1;
	p->a[2] = a2;
	p->a[3] = a3;
	p->a[4] = a4;
	p->a[5] = a5;
  p->order = 5;
}

// evaluate polynomial at x
float poly_eval(const Poly const* p, float x) {

	float rv = 0;
	float x2n = 1; // at^n

	for (int n = 0; n <= p->order; n++) {
		rv += p->a[n] * x2n;
		x2n *= x;
	}

	return rv;
}
