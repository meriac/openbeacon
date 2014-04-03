#include <stdio.h>

int main( int argc, char *argv[] ) 
{
  char *data_name = "data";
  int c;
  int i=0;
  
  if( argc == 2 ) {
    data_name = argv[1];
  }

  printf( "uint8_t %s[] = {\n", data_name );

  while( (c = getchar()) >= 0 ) {
    if( i== 0 ) {
      printf( "\t" );
    } else if( i%8 == 0 ) {
      printf( ",\n\t" );
    } else {
      printf( ", " );
    }
    i++;
    printf( "0x%02x", c );
  }

  printf( "\n};\n" );

  return 0;
}
