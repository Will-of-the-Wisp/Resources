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
#include "gui/buttons/guiButtonCtrl.h"

#include "gui/core/guiDefaultControlRender.h"


//-----------------------------------------------------------------------------
// The GuiFadingButtonCtrl class handles rendering of fading button gui controls.
//-----------------------------------------------------------------------------	
/*
	Alpha values are applied to the control's profile colors. Optional mFill 
	variable enables/disables filling of background/border. 
*/

class GuiFadingButtonCtrl : public GuiButtonCtrl {
	typedef GuiButtonCtrl Parent;

public:
	DECLARE_CONOBJECT(GuiFadingButtonCtrl);

	bool mFill;

	// Time
	U32 wakeTime;
	U32 fadeinTime;
	U32 fadeoutTime;

	// Fading
	U32 mAlpha;
	S32 mMode;

	// Options
	bool fadeIn_onWake;

	GuiFadingButtonCtrl() {
		mFill = true;
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

	bool onWake() {
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

	static void initPersistFields() {
		addField("fill", TypeBool, Offset(mFill, GuiFadingButtonCtrl));
		addField("fadeinTime", TypeS32, Offset(fadeinTime, GuiFadingButtonCtrl));
		addField("fadeoutTime", TypeS32, Offset(fadeoutTime, GuiFadingButtonCtrl));

		addField("alpha", TypeS32, Offset(mAlpha, GuiFadingButtonCtrl));

		addField("fadeInOnWake", TypeBool, Offset(fadeIn_onWake, GuiFadingButtonCtrl));

		Parent::initPersistFields();
	}
	
	void onRender(Point2I offset, const RectI& updateRect)
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
	
		bool highlight = mMouseOver;
		bool depressed = mDepressed;

		ColorI fontColor   = mActive ? ( highlight ? mProfile->mFontColorHL : mProfile->mFontColor ) : mProfile->mFontColorNA;
		ColorI fillColor   = mActive ? ( highlight ? mProfile->mFillColorHL : mProfile->mFillColor ) : mProfile->mFillColorNA;
		ColorI borderColor = mActive ? ( highlight ? mProfile->mBorderColorHL : mProfile->mBorderColor ) : mProfile->mBorderColorNA;

		// Apply alpha.
		fontColor.alpha = mAlpha;
		fillColor.alpha = mAlpha;
		borderColor.alpha = mAlpha;

		RectI boundsRect(offset, getExtent());
		
		if (mFill)
		{
			if (!mHasTheme)
			{
				if (mProfile->mBorder != 0)
					renderFilledBorder(boundsRect, borderColor, fillColor, mProfile->mBorderThickness);
				else
					GFX->getDrawUtil()->drawRectFill(boundsRect, fillColor);
			}
			else
			{
				S32 indexMultiplier = 1;

				if (!mActive)
					indexMultiplier = 4;
				else if (mDepressed || mStateOn)
					indexMultiplier = 2;
				else if (mMouseOver)
					indexMultiplier = 3;

				renderSizableBitmapBordersFilled(boundsRect, indexMultiplier, mProfile);
			}
		}
		
		Point2I textPos = offset;
		if( depressed )
			textPos += Point2I( 1, 1 );

		GFX->getDrawUtil()->setBitmapModulation( fontColor );
		renderJustifiedText( textPos, getExtent(), mButtonText );

		//render the children
		renderChildControls( offset, updateRect);
	}
};


IMPLEMENT_CONOBJECT(GuiFadingButtonCtrl);

ConsoleDocClass(GuiFadingButtonCtrl, "@brief Gui button control that will fade in and out. Only for buttons with no image, using profiles to 'fill' the color and borders.\n\n" );

DefineConsoleMethod(GuiFadingButtonCtrl, fadeIn, void, (), , "") {
	object->fadeIn();
}

DefineConsoleMethod(GuiFadingButtonCtrl, fadeOut, void, (), , "") {
	object->fadeOut();
}
