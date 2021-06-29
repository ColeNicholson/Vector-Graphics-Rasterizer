#include "Svg2Bmp.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
using namespace std;

void SetLimits ();

int main (int argc, char * argv [])
{
	if (argc < 2)
	{
		cerr << "Usage: " << argv[0] << " <filename>\n";
		exit (1);
	}
	SetLimits ();
	string inSvg = argv[1];
	string inBmp;
	if (argc > 2)
		inBmp = argv[2];
	Svg2Bmp svg2bmp (inSvg, inBmp);
	return 0;
}

void SetLimits ()
{
	// Beginning of resource limit code
	int getrlimit(int resource, struct rlimit *rlim);
	int getrusage(int who, struct rusage *usage);
	int setrlimit(int resource, const struct rlimit *rlim);
	struct rlimit __trl1;
	const int __trl1r = getrlimit(RLIMIT_CPU, & __trl1);
	struct rlimit __trl2 = {10, __trl1.rlim_max};
	const int __trl2r = setrlimit(RLIMIT_CPU, & __trl2);
	struct rlimit __trl3;
	const int __trl3r = getrlimit(RLIMIT_FSIZE, & __trl3);
	struct rlimit __trl4 = {1000000000, __trl3.rlim_max};
	const int __trl4r = setrlimit(RLIMIT_FSIZE, & __trl4);
	// End of time limit code
}
