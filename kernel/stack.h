#ifndef STACK_H_
#define STACK_H_

#include <util.h>
#include <memory.h>

typedef struct stack {
  void** top;
  void** min;
  void** max;
  void* arr[];
} stack;

static inline stack* stack_new(unsigned int numStack) {
  stack* s = (stack*) kmalloc(sizeof(stack) + sizeof(void*) * numStack);
  s->top = s->arr;
  s->min = s->top;
  s->max = s->top + numStack;
  return s;
}

static inline void stack_push(stack *s, void *item) {
  ASSERT(s->top != s->max, "pushing a full stack");
  *s->top = item;
  s->top++;
}

static inline void* stack_pop(stack *s) {
  ASSERT(s->top != s->min, "popping an empty stack");
  s->top--;
  return *s->top;
}
#endif // STACK_H_
