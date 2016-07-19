#ifndef DEFINES_H_
#define DEFINES_H_

#define SAFELY_DELETE(p) do {if (p) delete p; p = NULL;} while(0);
#define MAX_URL_SIZE 8192

#endif // DEFINES_H_
