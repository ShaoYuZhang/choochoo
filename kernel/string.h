#ifndef STRING_H_
#define STRING_H_

inline int strlen(char* name) {
  char* c = name;
  while (*c) {
    c++;
  }
  return c - name;
}

#endif //STRING_H_
