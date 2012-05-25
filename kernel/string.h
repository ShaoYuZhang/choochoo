#ifndef STRING_H_
#define STRING_H_

inline int strlen(char* name) {
  char* c = name;
  while (*c) {
    c++;
  }
  return c - name;
}

int equal(char* a, char* b, int l){
  for(int i = 0; i < l ; i++){
    if (a[i] != b[i] ){
      return 0;
    }
  }
  return 1;
}



#endif //STRING_H_
