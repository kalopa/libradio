#include <avr/io.h>

unsigned char t1 = 0;
volatile unsigned int t2 = 0;

void func1();
void func2();

void
foo()
{
	t2 = OCR1A;
	if (++t1 >= 100) {
		func1();
		t1 = 0;
	}
	if (++t2 >= 62) {
		func2();
		t2 = 0;
	}
}
