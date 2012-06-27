#ifndef Poly_h
#define Poly_h

typedef struct Poly {
	int order;
	float a[6];
} Poly;

void velocity_poly_init(Poly* p,
    float a0, float a1, float a2, float a3, float a4);

void distance_poly_init(Poly* p,
    float a0, float a1, float a2, float a3, float a4, float a5);

float poly_eval(const Poly const* p, float x);
#endif // Poly.h
