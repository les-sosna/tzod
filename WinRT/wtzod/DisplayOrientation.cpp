#include "pch.h"
#include "DisplayOrientation.h"

using namespace Windows::Graphics::Display;

int ComputeDisplayRotation(DisplayOrientations nativeOrientation, DisplayOrientations currentOrientation)
{
	int rotation = 0;

	switch (nativeOrientation)
	{
	default:
		// Note: NativeOrientation can only be Landscape or Portrait even though
		// the DisplayOrientations enum has other values.
		assert(false);

	case DisplayOrientations::Landscape:
		switch (currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = 0;
			break;

		case DisplayOrientations::Portrait:
			rotation = 270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = 180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = 90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = 90;
			break;

		case DisplayOrientations::Portrait:
			rotation = 0;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = 270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = 180;
			break;
		}
		break;
	}
	return rotation;
}
