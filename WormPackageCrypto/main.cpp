

/**
 	wHook->registerHook("Transmitter Receiving First Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Package First Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Receiving Data Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Package Data Package", REGISTER_EMPTY_CLEAN);
 */
/*
#define BUFSIZE 1*1024*1024 //1M
#define TIMES 100

int main() {
	uint8_t *data = new uint8_t[BUFSIZE];
	uint8_t key[32] = { 1, 2, 3, 4, 5, 6 };
	// Really does not matter what this is, except that it is only used once.
	uint8_t nonce[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	Chacha20 chacha(key, nonce);

	std::cout << "run! " << std::endl;

	timeval start, end;
	gettimeofday(&start, nullptr);

	for (int i = 0; i < TIMES; ++i)
		chacha.crypt(data, BUFSIZE);

	gettimeofday(&end, nullptr);

	double time = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / (double) 1000000);

	std::cout << ((double) BUFSIZE * TIMES) / (1 * 1024 * 1024) << "MB in " << time << "s ";
	std::cout << "[" << ((double) BUFSIZE * TIMES) / (1 * 1024 * 1024) / time << " MB/s]" << std::endl;
	return 0;
}
*/
