#include <stdio.h>
#include <string.h>
#include "DistributionNetwork.h"
#include "ParseInfo.h"

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Input Error");
		return 0;
	}
	if (strcmp(argv[1], "client") == 0) {
		if (StartSideNode() != DERROR_OK) {
			fprintf(stderr, "==============================================================");
			fprintf(stderr, "\n\t\t\tMAIN NODE ERROR\n");
			fprintf(stderr, "==============================================================\n");
			return -1;
		}
	} else {
		if (argc != 4) {
			fprintf(stderr, "Input Error\n");
			return 0;
		}
		double res = 0;
		struct Integral_t integral = {.begin = 0,
									  .end = 100000,
									  .func = func};

		if (StartMainNode(Read_Number_from_Text(argv[3]), Read_Number_from_Text(argv[2]), integral, &res) != DERROR_OK) {
			fprintf(stderr, "==============================================================");
			fprintf(stderr, "\n\t\t\tCLIENT NODE ERROR\n");
			fprintf(stderr, "==============================================================\n");
			return -1;
		}
		printf("\n[  %f  ]\n", res);
	}
}
