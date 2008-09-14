#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

void DumpUIntToUSB (unsigned int data);
void DumpStringToUSB (const char *text);
void DumpHexToUSB (unsigned int v, char bytes);

#endif /* DEBUG_PRINT_H */

