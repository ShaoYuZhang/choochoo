#ifndef RPS_CLIENT_H_
#define RPS_CLIENT_H_

void client1();

int SignUp(signed char server_tid);

int Play(signed char server_tid, char index, char move);

int Quit(signed char server_tid);


#endif // RPS_CLIENT_H_
