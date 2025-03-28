#include "akua_config.h"

void printAsGoArray(const unsigned char* data, size_t length)
{
	printf("[");
	for (size_t i = 0; i < length; i++)
	{
		if (i > 0)
		{
			printf(", ");
		}
		printf("0x%02X", data[i]);
	}
	printf("]\n");
}