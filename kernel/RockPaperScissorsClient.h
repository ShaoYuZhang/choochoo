#ifndef RPS_CLIENT_H_
#define RPS_CLIENT_H_

void client1();

void startClientsRPS();

char SignUp(signed char server_tid);

char Play(signed char server_tid, char index, char move);

char Quit(signed char server_tid, char index);


#endif // RPS_CLIENT_H_
