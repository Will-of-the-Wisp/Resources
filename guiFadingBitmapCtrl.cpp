//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "gui/fading/guiFadingControls.h"
#include "gui/controls/guiBitmapCtrl.h"


//-----------------------------------------------------------------------------
// The GuiFadingBitmapCtrl class handles rendering of fading bitmap gui controls.
//-----------------------------------------------------------------------------	

class GuiFadingBitmapCtrl : public GuiBitmapCtrl {
	typedef GuiBitmapCtrl Parent;

public:
	DECLARE_CONOBJECT(GuiFadingBitmapCtrl);

	// Time
	U32 wakeTime;
	U32 fadeinTime;
	U32 fadeoutTime;

	// Fading
	U32 mAlpha;
	S32 mMode;

	// Options
	bool fadeIn_onWake;

	GuiFadingBitmapCtrl() {
		wakeTime = 0;
		fadeinTime = 1000;
		fadeoutTime = 1000;

		mAlpha = 255;
		mMode = idle;

		fadeIn_onWake = false;
	}

	void onPreRender() {
		Parent::onPreRender();
		setUpdate();
	}

	void onMouseDown(const GuiEvent &) {
		Con::executef(this, "click");
	}

	bool onKeyDown(const GuiEvent &) {
		Con::executef(this, "click");
		return true;
	}

	bool onWake() 
	{
		if (!Parent::onWake())
			return false;

		if (fadeIn_onWake)
			fadeIn();
		else
			wakeTime = Platform::getRealMilliseconds();

		return true;
	}

	void setMode(S32 mode)
	{
		wakeTime = Platform::getRealMilliseconds();
		mMode = mode;
	}

	void fadeIn()
	{
		if (mMode == idle)
			setMode(fadingIn);
	}

	void fadeOut()
	{
		if (mMode == idle)
			setMode(fadingOut);
	}

	void onRender(Point2I offset, const RectI &updateRect) 
	{
		U32 elapsed = Platform::getRealMilliseconds() - wakeTime;

		if (mMode == fadingIn)
		{
			if (elapsed < fadeinTime)
				mAlpha = 255 * F32(elapsed) / F32(fadeinTime);
			else
			{
				mAlpha = 255;
				setMode(idle);
			}				
		}
		else if (mMode == fadingOut)
		{
			if (elapsed < fadeoutTime)
				mAlpha = 255 - (255 * (F32(elapsed) / F32(fadeoutTime)));
			else
			{
				mAlpha = 0;
				setMode(idle);
			}				
		}

		ColorI color(255, 255, 255, mAlpha);
		if (mTextureObject) 
		{
			GFX->getDrawUtil()->setBitmapModulation(color);

			if (mWrap) 
			{
				GFXTextureObject* texture = mTextureObject;
				RectI srcRegion;
				RectI dstRegion;
				F32 xdone = ((F32)getExtent().x / (F32)texture->mBitmapSize.x) + 1;
				F32 ydone = ((F32)getExtent().y / (F32)texture->mBitmapSize.y) + 1;

				S32 xshift = mStartPoint.x%texture->mBitmapSize.x;
				S32 yshift = mStartPoint.y%texture->mBitmapSize.y;
				for (S32 y = 0; y < ydone; ++y) 
				{
					for (S32 x = 0; x < xdone; ++x) 
					{
						srcRegion.set(0, 0, texture->mBitmapSize.x, texture->mBitmapSize.y);
						dstRegion.set(((texture->mBitmapSize.x*x) + offset.x) - xshift,
							((texture->mBitmapSize.y*y) + offset.y) - yshift,
							texture->mBitmapSize.x,
							texture->mBitmapSize.y);
						GFX->getDrawUtil()->drawBitmapStretchSR(texture, dstRegion, srcRegion);
					}
				}
			}
			else 
			{
				RectI rect(offset, getExtent());
				GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect);
			}
		}

		if (mProfile->mBorder || !mTextureObject) 
		{
			RectI rect(offset.x, offset.y, getExtent().x, getExtent().y);
			ColorI borderCol(mProfile->mBorderColor);
			borderCol.alpha = mAlpha;
			GFX->getDrawUtil()->drawRect(rect, borderCol);
		}

		renderChildControls(offset, updateRect);
	}

	static void initPersistFields() 
	{
		addField("fadeinTime", TypeS32, Offset(fadeinTime, GuiFadingBitmapCtrl));
		addField("fadeoutTime", TypeS32, Offset(fadeoutTime, GuiFadingBitmapCtrl));

		addField("alpha", TypeS32, Offset(mAlpha, GuiFadingBitmapCtrl));

		addField("fadeInOnWake", TypeBool, Offset(fadeIn_onWake, GuiFadingBitmapCtrl));

		Parent::initPersistFields();
	}
};

IMPLEMENT_CONOBJECT(GuiFadingBitmapCtrl);

ConsoleDocClass(GuiFadingBitmapCtrl, "@brief Gui control with an image that will fade in and out. Includes child controls.\n\n" );

DefineConsoleMethod(GuiFadingBitmapCtrl, fadeIn, void, (), , "") {
	object->fadeIn();
}

DefineConsoleMethod(GuiFadingBitmapCtrl, fadeOut, void, (), , "") {
	object->fadeOut();
}
