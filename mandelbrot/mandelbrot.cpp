// Mandelbrot set example
// Adam Sampson <a.sampson@abertay.ac.uk>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <complex>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include<algorithm>
#include <thread>

// Import things we need from the standard library
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::complex;
using std::cout;
using std::endl;
using std::ofstream;

ofstream times("mandlebrotTimes.csv");

// Define the alias "the_clock" for the clock type we're going to use.
typedef std::chrono::steady_clock the_clock;

// The size of the image to generate.
const int WIDTH = 1920;
const int HEIGHT = 1024;

// The number of times to iterate before we assume that a point isn't in the
// Mandelbrot set.
// (You may need to turn this up if you zoom further into the set.)
const int MAX_ITERATIONS = 1000;

// The image data.
// Each pixel is represented as 0xRRGGBB.
uint32_t image[HEIGHT][WIDTH];


// Write the image to a TGA file with the given name.
// Format specification: http://www.gamers.org/dEngine/quake3/TGA.txt
void write_tga(const char *filename)
{
	ofstream outfile(filename, ofstream::binary);

	uint8_t header[18] = {
		0, // no image ID
		0, // no colour map
		2, // uncompressed 24-bit image
		0, 0, 0, 0, 0, // empty colour map specification
		0, 0, // X origin
		0, 0, // Y origin
		WIDTH & 0xFF, (WIDTH >> 8) & 0xFF, // width
		HEIGHT & 0xFF, (HEIGHT >> 8) & 0xFF, // height
		24, // bits per pixel
		0, // image descriptor
	};
	outfile.write((const char *)header, 18);

	for (int y = 0; y < HEIGHT; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			uint8_t pixel[3] = {
				image[y][x] & 0xFF, // blue channel
				(image[y][x] >> 8) & 0xFF, // green channel
				(image[y][x] >> 16) & 0xFF, // red channel
			};
			outfile.write((const char *)pixel, 3);
		}
	}

	outfile.close();
	if (!outfile)
	{
		// An error has occurred at some point since we opened the file.
		cout << "Error writing to " << filename << endl;
		exit(1);
	}
}


// Render the Mandelbrot set into the image array.
// The parameters specify the region on the complex plane to plot.
void compute_mandelbrot(double left, double right, double top, double bottom, int yPosSt, int yPosEnd)
{
	for (int y = yPosSt; y < yPosEnd; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			// Work out the point in the complex plane that
			// corresponds to this pixel in the output image.
			complex<double> c(left + (x * (right - left) / WIDTH), top + (y * (bottom - top) / HEIGHT));

			// Start off z at (0, 0).
			complex<double> z(0.0, 0.0);

			// Iterate z = z^2 + c until z moves more than 2 units
			// away from (0, 0), or we've iterated too many times.
			int iterations = 0;

			// % cost before optimization - ~84.41%
			
			// Original @ ~84.41%
			//while (abs(z) < 2.0 && iterations < MAX_ITERATIONS) // abs(z) = sqrt(z^2) = (abs(z))^2 = z^2
			//{
			//	z = (z * z) + c;

			//	++iterations;
			//}

			// % cost after optimization - ~71.07%

			// Optimized @ ~
			while (std::norm(z) < 4.0 && iterations < MAX_ITERATIONS) // abs(z) = sqrt(z^2) = (abs(z))^2 = z^2		std::norm - rtns the magnitude squared of a complex number.
			{
				z = (z * z) + c;

				++iterations;
			}

			if (iterations == MAX_ITERATIONS)
			{
				// z didn't escape from the circle.
				// This point is in the Mandelbrot set.
				image[y][x] = 0x000000; // black
			}
			else
			{
				// z escaped within less than MAX_ITERATIONS
				// iterations. This point isn't in the set.

				int red = 255;
				int green = 100;
				int blue = 100;
				int col = (red << 16) | (green << 8) | (blue);

				image[y][x] = (col*iterations) / MAX_ITERATIONS;
			}
		}
	}
}

long long computeMedian(std::list<long long> times)
{
	long long median = 0;
	auto iter = times.begin();

	for (int i = 0; i < times.size() / 2; ++i)
	{
		++iter;
	}

	// Check for even num of elements
	if (times.size() % 2 == 0)
	{
		// Return the value at current iteration + the value at previous iteration divided by 2;
		return ((*iter + (*--iter)) / 2);
	}
	else
	{
		// Otherwise we have an odd amount of elements so just return the middle value.
		return *iter;
	}
}

std::list<long long> calculateSlices()
{
	std::list<long long> times;
	int sliceCounter = 1;

	for (int i = 0; i < HEIGHT; i += 64)
	{
		// Start timing
		the_clock::time_point start = the_clock::now();

		// This shows the whole set.
		//compute_mandelbrot(-2.0, 1.0, 1.125, -1.125, i, i + 64);

		// This zooms in on an interesting bit of detail.
		compute_mandelbrot(-0.751085, -0.734975, 0.118378, 0.134488, i, i + 64);

		// Stop timing
		the_clock::time_point end = the_clock::now();

		// Compute the difference between the two times in milliseconds
		auto time_taken = duration_cast<milliseconds>(end - start).count();
		cout << "Computing the Mandelbrot slice number " << sliceCounter << " took: " << time_taken << " ms." << endl;

		++sliceCounter;

		times.push_back(time_taken);
	}

	return times;
}

std::list<long long> runMultipleTimings()
{
	std::list<long long> times;
	int counter = 0;

	while (counter < 7)
	{
		// This shows the whole set.
		compute_mandelbrot(-2.0, 1.0, 1.125, -1.125, 16, 498);

		// Start timing
		the_clock::time_point start = the_clock::now();

		compute_mandelbrot(-2.0, 1.0, 1.125, -1.125, 0, HEIGHT);

		// Stop timing
		the_clock::time_point end = the_clock::now();

		// Compute the difference between the two times in milliseconds
		auto time_taken = duration_cast<milliseconds>(end - start).count();
		cout << "Computing the Mandelbrot set took: " << time_taken << " ms." << endl;

		times.push_back(time_taken);

		// This zooms in on an interesting bit of detail.
		//compute_mandelbrot(-0.751085, -0.734975, 0.118378, 0.134488);

		++counter;
	}

	return times;
}

void standardMandlebrot()
{
	// This shows the whole set.
	//compute_mandelbrot(-2.0, 1.0, 1.125, -1.125, 16, 498);

	// Zoomed in.
	//compute_mandelbrot(-0.751085, -0.734975, 0.118378, 0.134488, 0, HEIGHT);

	// Start timing
	the_clock::time_point start = the_clock::now();

	compute_mandelbrot(-2.0, 1.0, 1.125, -1.125, 0, HEIGHT);

	// Stop timing
	the_clock::time_point end = the_clock::now();

	// Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	cout << "Computing the Mandelbrot set took: " << time_taken << " ms." << endl;

	// This zooms in on an interesting bit of detail.
	//compute_mandelbrot(-0.751085, -0.734975, 0.118378, 0.134488);
}

void standardMandlebrot_Th(int increment)
{
	std::vector<std::thread> mbThreads;

	// Start timing
	the_clock::time_point start = the_clock::now();

	// 1020 is the terminating condition as this accounts for rounding(floor) of division by 8.
	for (int i = 0; i < 1020; i += increment)
	{
		// Zoomed in.
		//mbThreads.push_back(std::thread(compute_mandelbrot, -0.751085, -0.734975, 0.118378, 0.134488, i, i + increment));

		// Whole set.
		mbThreads.push_back(std::thread(compute_mandelbrot, -2.0, 1.0, 1.125, -1.125, i, i + increment));
	}

	for (int i = 0; i < mbThreads.size(); ++i)
	{
		if (mbThreads[i].joinable())
		{
			mbThreads[i].join();
		}
	}

	// Stop timing
	the_clock::time_point end = the_clock::now();

	// Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();

	cout << "Computing the Mandelbrot set with " << mbThreads.size() << " threads took: " << time_taken << " ms." << endl;

	times << time_taken << ",\n";
}

void runMultiMbThreadTimings()
{
	int counter = 1.0f;
	int increment = HEIGHT / counter;

	while (counter < 9)
	{
		standardMandlebrot_Th(increment);
		++counter;
		increment = HEIGHT / counter;
	}
}

int main(int argc, char *argv[])
{
	cout << "Please wait..." << endl;

	//standardMandlebrot();
	//std::list<long long> times = runMultipleTimings();
	//std::list<long long> times = calculateSlices();

	/*times.sort();

	for (auto iter = times.begin(); iter != times.end(); ++iter)
	{
		std::cout << *iter << '\n';
	}

	long long median = computeMedian(times);

	std::cout << "The median of all times: " << median << '\n';*/

	//standardMandlebrot_Th();
	runMultiMbThreadTimings();
	
	write_tga("output.tga");

	return 0;
}
