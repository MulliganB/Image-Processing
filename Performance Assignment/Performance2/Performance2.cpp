// Performance2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Performance2.h"
#include <vector>
#include <future>
#include <thread>
#include <gdiplusheaders.h>
#include <gdipluscolor.h>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//Make number hexadecimal
#define getByte(value, n) (value >> (n*8) & 0xFF)
// Timer - used to established precise timings for code.
class TIMER
{
	LARGE_INTEGER t_;

	__int64 current_time_;

public:
	TIMER()	// Default constructor. Initialises this timer with the current value of the hi-res CPU timer.
	{
		QueryPerformanceCounter(&t_);
		current_time_ = t_.QuadPart;
	}

	TIMER(const TIMER &ct)	// Copy constructor.
	{
		current_time_ = ct.current_time_;
	}

	TIMER& operator=(const TIMER &ct)	// Copy assignment.
	{
		current_time_ = ct.current_time_;
		return *this;
	}

	TIMER& operator=(const __int64 &n)	// Overloaded copy assignment.
	{
		current_time_ = n;
		return *this;
	}

	~TIMER() {}		// Destructor.

	static __int64 get_frequency()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart;
	}

	__int64 get_time() const
	{
		return current_time_;
	}

	void get_current_time()
	{
		QueryPerformanceCounter(&t_);
		current_time_ = t_.QuadPart;
	}

	inline bool operator==(const TIMER &ct) const
	{
		return current_time_ == ct.current_time_;
	}

	inline bool operator!=(const TIMER &ct) const
	{
		return current_time_ != ct.current_time_;
	}

	__int64 operator-(const TIMER &ct) const		// Subtract a TIMER from this one - return the result.
	{
		return current_time_ - ct.current_time_;
	}

	inline bool operator>(const TIMER &ct) const
	{
		return current_time_ > ct.current_time_;
	}

	inline bool operator<(const TIMER &ct) const
	{
		return current_time_ < ct.current_time_;
	}

	inline bool operator<=(const TIMER &ct) const
	{
		return current_time_ <= ct.current_time_;
	}

	inline bool operator>=(const TIMER &ct) const
	{
		return current_time_ >= ct.current_time_;
	}
};

CWinApp theApp;  // The one and only application object

using namespace std;
//Pixel processing as described in the rosetta code Bilinear scaling algorithm
float process(float s, float e, float t){ return s + (e - s)*t; }
float pixelProcess(float c00, float c10, float c01, float c11, float tx, float ty){
	return process(process(c00, c10, tx), process(c01, c11, tx), ty);
}
CImage* Copy(CImage *source)
{
	//Create new Image pointer
	CImage *dest = new CImage;
	//Create new image with the same height and width as the old image
	dest->Create(source->GetWidth(), source->GetHeight(), source->GetBPP());
	//Populate new image with the contnet of the old image
	source->Draw(dest->GetDC(), 0, 0, dest->GetWidth(), dest->GetHeight(), 0, 0, source->GetWidth(), source->GetHeight());
	//Finalise the changes to the new image 
	dest->ReleaseDC();
	//return copy of original image
	return dest;
}
CImage* Brighten(CImage *i)
{
	//Copy image for function
	CImage *dest = Copy(i);

	//Get width and height to save time later
	int width = dest->GetWidth();
	int height = dest->GetHeight();

	//Iterate over pixels using countdown forloop 
	for (int y = height; --y;)
	{
		for (int x = width; --x;)
		{
			//Get colour reference
			COLORREF pixel = dest->GetPixel(x, y);
			//Split colour reference into its components
			BYTE r = GetRValue(pixel);
			BYTE g = GetGValue(pixel);
			BYTE b = GetBValue(pixel);
			//Brighten variables if needed
			if ((r + 10) > 255) r = 255; else r += 10;
			if ((g + 10) > 255) g = 255; else g += 10;
			if ((b + 10) > 255) b = 255; else b += 10;
			//Store components as a whole colour reference again
			pixel = RGB(r, g, b);
			//Set pixel with new colour
			dest->SetPixel(x, y, pixel);
		}
	}
	//Return brightened image
	return dest;
}
CImage* Rotate(CImage *i)
{
	//Copy image for function
	CImage *b1 = Copy(i);

	//Get bitmap of image using Gdi+ and the built in bitmap format of the CImage
	Gdiplus::Bitmap* gdiPlusBitmap = Gdiplus::Bitmap::FromHBITMAP(b1->Detach(), NULL);
	//rotate image by 90 degrees
	gdiPlusBitmap->RotateFlip(Gdiplus::Rotate90FlipNone);
	
	if (gdiPlusBitmap)
	{
		//get bitmap reference
		HBITMAP *hbmp = new HBITMAP;
		//Store bitmap in reference
		gdiPlusBitmap->GetHBITMAP(Gdiplus::Color::White, hbmp);
		//Store referenced bitmap back into image using new format
		b1->Attach(*(hbmp));
		//Delete all unwanted variabels and pointers
		delete gdiPlusBitmap, hbmp;
	}
	//Return rotated image
	return b1;
}
CImage* GrayScale(CImage *i)
{
	//Copy image for function
	CImage *b1 = Copy(i);

	//Get height and width to save time later
	int height = b1->GetHeight();
	int width = b1->GetWidth();

	//Nested for loop to iterate over all pixels
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			//get colour reference
			COLORREF ref = b1->GetPixel(x, y);
			//Split reference into individual components
			byte red = GetRValue(ref);
			byte green = GetGValue(ref);
			byte blue = GetBValue(ref);
			//Make grey using luminace equation
			ref = ((red * 61) + (green * 174) + (blue * 21)) / 256;
			//Combine result and store as a complete colour reference 
			COLORREF pixel = RGB(ref, ref, ref);
			//Change pixels colour
			b1->SetPixel(x, y, pixel);
		}
	}
	//Return greyscaled image
	return b1;
}
std::vector<std::vector<int>> BilinearScaling(CImage *i)
{
	//Copy image for the function
	CImage *b1 = Copy(i);
	//Get width and height to save time continuously getting them in the for loops
	float width = b1->GetWidth();
	float height = b1->GetHeight();
	//Calculate the new dimensions
	float newWidth = width / 2;
	float newHeight = height / 2;
	//Initilaise the storage for the pixels' colour references
	float vecLen = newWidth * newHeight;
	std::vector<std::vector<int>> v(newHeight, std::vector<int>(newWidth));
	
	for (int y = 0; y < ((int)newHeight); ++y)
	{
		//Calculate the distance between pixels choosen based on the scale of the resize for the y axis
		float gy = y / (float)(newHeight)* (height - 1);
		int gyi = (int)gy;
		for (int x = 0; x < ((int)newWidth); ++x)
		{
			//Calculate the distance between pixels choosen based on the scale of the resize for the x axis
			float gx = x / (float)(newWidth)* (width - 1);
			int gxi = (int)gx;
			//Get colour references from around the current pixel
			uint32_t c00 = b1->GetPixel(gxi, gyi);
			uint32_t c10 = b1->GetPixel(gxi + 1, gyi);
			uint32_t c01 = b1->GetPixel(gxi, gyi + 1);
			uint32_t c11 = b1->GetPixel(gxi + 1, gyi + 1);
			uint32_t result = 0;
			for (uint32_t k = 0; k < 3; ++k)
			{
				//use surrounding colour references to calculate the average colour at that point in the new image
				result |= (uint8_t)pixelProcess(getByte(c00, k), getByte(c10, k), getByte(c01, k), getByte(c11, k), gx - gxi, gy - gyi) << (8 * k);
			}
			//Store the new colour reference for that point in the new image
			v[y][x] = result;
		}
	}
	//return vector of vectors containing the colour references of the pixels in their corresponding locations
	return v;
}
CImage* resize(CImage *i)
{
	//Make copy of image
	CImage *b1 = Copy(i);

	//Calculate the new width and height of image after resize
	float newWidth = b1->GetWidth() / 2;
	float newHeight = b1->GetHeight() / 2;
	//create a vector for the pixels colour referneces to be stored in
	float vecLen = newHeight * newWidth;
	std::vector<std::vector<int>> v(newHeight, std::vector<int>(newWidth));
	//get and stored the colour references for the new image
	v = BilinearScaling(b1);

	//create new image based on the old one passed in
	CImage *dest = new CImage;
	dest->Create(newWidth, newHeight, b1->GetBPP());
	b1->Draw(dest->GetDC(), 0, 0, dest->GetWidth(), dest->GetHeight(), 0, 0, b1->GetWidth(), b1->GetHeight());
	dest->ReleaseDC();
	//Make the image look like the original using the colour references in the vector
	for (int y = 0; y < newHeight; ++y)
	{
		for (int x = 0; x < newWidth; ++x)
		{
			uint32_t result = v[y][x];
			dest->SetPixel(x, y, result);
		}
	}
	//return new, resized image
	return dest;
}
void section1()
{
	//Section 1 - images 1 and 2
	CImage b1, *dest1;
	b1.Load(L"IMG_1.JPG");
	CImage b2, *dest2;
	b2.Load(L"IMG_2.JPG");
	std::cout << "loaded 1\n";

	//Rotate images
	dest1 = Rotate(&b1);
	dest2 = Rotate(&b2);
	std::cout << "Rotated 1\n";

	//Resize images
	dest1 = resize(dest1);
	dest2 = resize(dest2);
	std::cout << "resize 1\n";

	//Brighten images
	dest1 = Brighten(dest1);
	dest2 = Brighten(dest2);
	std::cout << "Brighten 1\n";

	//Greyscale images
	dest1 = GrayScale(dest1);
	dest2 = GrayScale(dest2);
	std::cout << "GreyScale 1\n";

	//Save images and delete pointers
	dest1->Save(L"IMG_1.PNG");
	delete dest1, b1;
	dest2->Save(L"IMG_2.PNG");
	delete dest2, b2;
	std::cout << "end 1\n";
}
void section2()
{
	//Section 2 - images 3 and 4
	CImage b3, *dest3;
	b3.Load(L"IMG_3.JPG");
	CImage b4, *dest4;
	b4.Load(L"IMG_4.JPG");
	std::cout << "loaded 2\n";

	//Rotate images
	dest3 = Rotate(&b3);
	dest4 = Rotate(&b4);
	std::cout << "Rotated 2\n";

	//Resize images
	dest3 = resize(dest3);
	dest4 = resize(dest4);
	std::cout << "resize 2\n";

	//Brighten images
	dest3 = Brighten(dest3);
	dest4 = Brighten(dest4);
	std::cout << "Brighten 2\n";

	//Greyscale images
	dest3 = GrayScale(dest3);
	dest4 = GrayScale(dest4);
	std::cout << "GreyScale 2\n";

	//Save images and delete pointers
	dest3->Save(L"IMG_3.PNG");
	delete dest3, b3;
	dest4->Save(L"IMG_4.PNG");
	delete dest4, b4;
	std::cout << "end 2\n";
}
void section3()
{
	//Section 3 - images 5 and 6
	CImage b5, *dest5;
	b5.Load(L"IMG_5.JPG");
	CImage b6, *dest6;
	b6.Load(L"IMG_6.JPG");
	std::cout << "loaded 3\n";

	//Rotate images
	dest5 = Rotate(&b5);
	dest6 = Rotate(&b6);
	std::cout << "Rotated 3\n";

	//Resize images
	dest5 = resize(dest5);
	dest6 = resize(dest6);
	std::cout << "resize 3\n";

	//Brighten Images
	dest5 = Brighten(dest5);
	dest6 = Brighten(dest6);
	std::cout << "Brighten 3\n";

	//Applied greyscale to images
	dest5 = GrayScale(dest5);
	dest6 = GrayScale(dest6);
	std::cout << "GreyScale 3\n";

	//Save images and delete pointers
	dest5->Save(L"IMG_5.PNG");
	delete dest5, b5;
	dest6->Save(L"IMG_6.PNG");
	delete dest6, b6;
	std::cout << "end 3\n";
}
void section4()
{
	//section 4 - images 7 and 8
	CImage b7, *dest7;
	b7.Load(L"IMG_7.JPG");
	CImage b8, *dest8;
	b8.Load(L"IMG_8.JPG");
	std::cout << "loaded 4\n";

	//Rotate images
	dest7 = Rotate(&b7);
	dest8 = Rotate(&b8);
	std::cout << "Rotated 4\n";

	//Resize Images
	dest7 = resize(dest7);
	dest8 = resize(dest8);
	std::cout << "resize 4\n";

	//Brighten images
	dest7 = Brighten(dest7);
	dest8 = Brighten(dest8);
	std::cout << "Brighten 4\n";

	//Apply greyscale effect to images
	dest7 = GrayScale(dest7);
	dest8 = GrayScale(dest8);
	std::cout << "GreyScale 4\n";

	//Save images and delete pointers
	dest7->Save(L"IMG_7.PNG");
	delete dest7, b7;
	dest8->Save(L"IMG_8.PNG");
	delete dest8, b8;
	std::cout << "end 4\n";
}
void section5()
{
	//Section 5 - images 9 and 10
	CImage b9, *dest9;
	b9.Load(L"IMG_9.JPG");
	CImage b10, *dest10;
	b10.Load(L"IMG_10.JPG");
	std::cout << "loaded 5\n";

	//Rotate images
	dest9 = Rotate(&b9);
	dest10 = Rotate(&b10);
	std::cout << "Rotated 5\n";

	//Resize images
	dest9 = resize(dest9);
	dest10 = resize(dest10);
	std::cout << "resize 5\n";

	//Brighten images
	dest9 = Brighten(dest9);
	dest10 = Brighten(dest10);
	std::cout << "Brighten 5\n";

	//Apply greyscale effect to images
	dest9 = GrayScale(dest9);
	dest10 = GrayScale(dest10);
	std::cout << "GreyScale 5\n";

	//save images and remove pointers
	dest9->Save(L"IMG_9.PNG");
	delete dest9, b9;
	dest10->Save(L"IMG_10.PNG");
	delete dest10, b10;
	std::cout << "end 5\n";
}
void section6()
{
	//section 6 - images 11 and 12
	CImage b11, *dest11;
	b11.Load(L"IMG_11.JPG");
	CImage b12, *dest12;
	b12.Load(L"IMG_12.JPG");
	std::cout << "loaded 6\n";

	//Rotating images
	dest11 = Rotate(&b11);
	dest12 = Rotate(&b12);
	std::cout << "Rotated 6\n";

	//Resize images
	dest11 = resize(dest11);
	dest12 = resize(dest12);
	std::cout << "resize 6\n";

	//Brighten Images
	dest11 = Brighten(dest11);
	dest12 = Brighten(dest12);
	std::cout << "Brighten 6\n";

	//Apply greyscale effect to images
	dest11 = GrayScale(dest11);
	dest12 = GrayScale(dest12);
	std::cout << "GreyScale 6\n";

	//Save images and remove pointers
	dest11->Save(L"IMG_11.PNG");
	delete dest11, b11;
	dest12->Save(L"IMG_12.PNG");
	delete dest12, b12;
	std::cout << "end 6\n";
}
void FirstHalf()
{
	//First half of images - future threads
	std::future<void> f1 = std::async(std::launch::async, []{section1(); });
	std::future<void> f2 = std::async(std::launch::async, []{section2(); });
	std::future<void> f3 = std::async(std::launch::async, []{section3(); });
	//ending all threads
	f1.get();
	f2.get();
	f3.get();
}
void SecondHalf()
{
	//Second half of images future threads
	std::future<void> f4 = std::async(std::launch::async, []{section4(); });
	std::future<void> f5 = std::async(std::launch::async, []{section5(); });
	std::future<void> f6 = std::async(std::launch::async, []{section6(); });
	//Ending all threads
	f4.get();
	f5.get();
	f6.get();
}
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize Microsoft Foundation Classes, and print an error if failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// Application starts here...

		// Time the application's execution time.
		TIMER start;	// DO NOT CHANGE THIS LINE

		//--------------------------------------------------------------------------------------
		// Process the images...   // Put your code here...
		::cout << "sizeof (DIBSECTION)" << sizeof(DIBSECTION) << std::endl;
		::cout << "sizeof (BITMAP)" << sizeof(BITMAP) << std::endl;
		////This is the function that contains the future threads to the second half of images
		SecondHalf();
		//This is the function that contains the future threads to the first half of images
		FirstHalf(); 

		std::cout << "Done\n";

		//-------------------------------------------------------------------------------------------------------

		// How long did it take?...   DO NOT CHANGE FROM HERE...

		TIMER end;

		TIMER elapsed;

		elapsed = end - start;

		__int64 ticks_per_second = start.get_frequency();

		// Display the resulting time...

		double elapsed_seconds = (double)elapsed.get_time() / (double)ticks_per_second;

		cout << "Elapsed time (seconds): " << elapsed_seconds;
		cout << endl;
		cout << "Press a key to continue" << endl;

		char c;
		cin >> c;
	}

	return nRetCode;
}
