#include <random>
#include "SSDB.h"
#include <time.h>


std::string i2s(int64_t v)
{
	char buf[21];
	sprintf(buf, "%lld", v);
	return buf;
}


template<typename T> T random_t(T min_ = (std::numeric_limits<T>::min)(), T max_ = (std::numeric_limits<T>::max)() )
{
	static std::default_random_engine engine(time(0));
	std::uniform_int_distribution<T> dis(min_, max_);
	return dis(engine);
}

void fo_test()
{
	ssdb::Client* p = ssdb::Client::connect("127.0.0.1", 8883);
	clock_t ct_start = 0;
	size_t count = 0;
	size_t last_count = 0;
	while(p)
	{
		p->zset("busy:1", i2s(random_t<uint64_t>()), random_t<uint64_t>());
		std::vector<std::string> ret;
		int64_t start = random_t<uint64_t>();
		int64_t stop = start + random_t<uint32_t>();
		p->zscan("busy:1", std::string(), &start, &stop, 1, &ret);

		++count;

		if (clock() - ct_start > CLOCKS_PER_SEC)
		{
			size_t speed1 = count - last_count;
			last_count = count;
			printf("%d\n", speed1);
			ct_start = clock();
		}
	}
}

int main()
{
	fo_test();
	return 0;
}
