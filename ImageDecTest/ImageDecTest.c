#include <windows.h> // timer stuff
#include <stdio.h> // printf

/*
 Assuming 8 BPP non-indexed image buffer (grayscale).
 Assuming left-to-right then top-to-bottom pixels
*/

#define INPUT_NUM_COL 15360
#define INPUT_NUM_ROW 8640
#define OUTPUT_NUM_COL (INPUT_NUM_COL >> 1)
#define OUTPUT_NUM_ROW (INPUT_NUM_ROW >> 1)

void imageDecimate_BasicForLoops(
	unsigned char* outputImage,
	const unsigned char* inputImage) {

	for (int x = 0, xO = 0; x < INPUT_NUM_COL; x += 2, ++xO) {
		for (int y = 0, yO = 0; y < INPUT_NUM_ROW; y += 2, ++yO) {
			outputImage[OUTPUT_NUM_COL * yO + xO] =
				inputImage[INPUT_NUM_COL * y + x];
		}
	}
}

void imageDecimate_BitPattern(
	unsigned char* outputImage,
	const unsigned char* inputImage) {

	const unsigned char* p = outputImage
		+ OUTPUT_NUM_COL * OUTPUT_NUM_ROW;

	// create bit pattern which represents:
	// 10101010-00000000

	unsigned char pattern[INPUT_NUM_COL * 2 >> 3] = { 0 };
	for (int i = 0; i < INPUT_NUM_COL; ++i) {
		pattern[i >> 3] |= ((i + 1) & 1) << (7 - (i % 8));
	}

	int n = 0;
	const int f = INPUT_NUM_COL * 2;
	while (outputImage != p) {
		for (int r = 0; r < f; ++r) {
			if (pattern[r >> 3] & (1 << (7 - (n % 8)))) {
				*outputImage++ = inputImage[n];
			}
			++n;
		}
	}
}

void imageDecimate_PointerArithmatic(
	unsigned char* outputImage,
	const unsigned char* inputImage) {

	const unsigned char* p = outputImage
		+ OUTPUT_NUM_COL * OUTPUT_NUM_ROW;
	int n = 0;
	while (outputImage != p) {
		*outputImage++ = *inputImage;
		inputImage += !(++n % OUTPUT_NUM_COL) ? INPUT_NUM_COL + 2 : 2;
	}
}

void imageDecimate_Combination(
	unsigned char* outputImage,
	const unsigned char* inputImage) {

	const unsigned char* p = outputImage
		+ OUTPUT_NUM_COL * OUTPUT_NUM_ROW;

	int n = 0;
	while (outputImage != p) {
		for (int r = 0; r < INPUT_NUM_COL >> 1; ++r, n += 2) {
			*outputImage++ = inputImage[n];
		}
		n += INPUT_NUM_COL;
	}
}

// answer from: https://stackoverflow.com/a/40186458/1204153
void imageDecimate_StackOverflowKent(
	unsigned char* outputImage,
	const unsigned char* inputImage) {

	int outputIndex;
	int inputIndex;
	for (int p = 0; p < OUTPUT_NUM_ROW; p++) {
		inputIndex = p * INPUT_NUM_COL * 2;
		outputIndex = p * OUTPUT_NUM_COL;
		for (int q = 0; q < OUTPUT_NUM_COL; q++) {
			outputImage[outputIndex] = inputImage[inputIndex];
			inputIndex += 2;
			outputIndex++;
		}
	}
}

LARGE_INTEGER timeme(void (*f)(unsigned char*, const unsigned char*)) {

	LARGE_INTEGER elapsedMicroseconds = { 0 };

	unsigned char* inputBuf = (unsigned char*)malloc(INPUT_NUM_ROW * INPUT_NUM_COL);
	unsigned char* outputBuf = (unsigned char*)malloc(OUTPUT_NUM_ROW * OUTPUT_NUM_COL);

	if (!inputBuf || !outputBuf) { return elapsedMicroseconds; }

	{ // fill a circle to make it easier to see changes
		const int cX = INPUT_NUM_COL >> 1, cY = INPUT_NUM_ROW >> 1,
			r = (cX < cY ? cX : cY), rS = r * r;
		for (int y = 0; y < INPUT_NUM_ROW; ++y) {
			const int dY = y - cY, dYs = dY * dY;
			for (int x = 0; x < INPUT_NUM_COL; ++x) {
				const int dX = x - cX;
				inputBuf[INPUT_NUM_COL * y + x] =
					dX * dX + dYs < rS ? 0xFF : 0x00;
			}
		}
	}

	{
		LARGE_INTEGER startingTime, endingTime, frequency;
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&startingTime);

		(*f)(outputBuf, inputBuf);

		QueryPerformanceCounter(&endingTime);
		elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
		elapsedMicroseconds.QuadPart *= 1000000;
		elapsedMicroseconds.QuadPart /= frequency.QuadPart;
	}

	free((void*)inputBuf);
	free((void*)outputBuf);

	return elapsedMicroseconds;
}

void main() {
	printf("Two For Loops: %lld microseconds\n", timeme(imageDecimate_BasicForLoops).QuadPart);
	printf("Bit Pattern  : %lld microseconds\n", timeme(imageDecimate_BitPattern).QuadPart);
	printf("Pointer Arith: %lld microseconds\n", timeme(imageDecimate_PointerArithmatic).QuadPart);
	printf("Combination  : %lld microseconds\n", timeme(imageDecimate_Combination).QuadPart);
	printf("Stack Overf  : %lld microseconds\n", timeme(imageDecimate_StackOverflowKent).QuadPart);
}
