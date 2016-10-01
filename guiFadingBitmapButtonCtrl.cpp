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
#include "gui/buttons/guiBitmapButtonCtrl.h"

#include "gfx/gfxTextureManager.h"
#include "gui/core/guiDefaultControlRender.h"


//-----------------------------------------------------------------------------
// The GuiFadingBitmapButtonCtrl class handles rendering of fading button 
// controls that include bitmaps.
//-----------------------------------------------------------------------------	
/*
	Alpha values are used to set the bitmap modulation. 
*/

class GuiFadingBitmapButtonCtrl : public GuiBitmapButtonCtrl {
	typedef GuiBitmapButtonCtrl Parent;

public:
	DECLARE_CONOBJECT(GuiFadingBitmapButtonCtrl);

	// Time
	U32 wakeTime;
	U32 fadeinTime;
	U32 fadeoutTime;

	// Fading
	U32 mAlpha;
	S32 mMode;

	// Options
	bool fadeIn_onWake;

	GuiFadingBitmapButtonCtrl() {
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

	static void initPersistFields() {
		addField("fadeinTime", TypeS32, Offset(fadeinTime, GuiFadingBitmapButtonCtrl));
		addField("fadeoutTime", TypeS32, Offset(fadeoutTime, GuiFadingBitmapButtonCtrl));

		addField("alpha", TypeS32, Offset(mAlpha, GuiFadingBitmapButtonCtrl));

		addField("fadeInOnWake", TypeBool, Offset(fadeIn_onWake, GuiFadingBitmapButtonCtrl));

		Parent::initPersistFields();
	}
	
	//------------------------------------------------------------------------------
	// Rendering
	//------------------------------------------------------------------------------
	void GuiFadingBitmapButtonCtrl::onRender(Point2I offset, const RectI& updateRect)
	{
		GFXTexHandle& texture = getTextureForCurrentState();
		if( texture ) 
		{
			renderButton( texture, offset, updateRect );
			renderChildControls( offset, updateRect );
		}
		else
			Parent::onRender(offset, updateRect);
	}

	void GuiFadingBitmapButtonCtrl::renderButton(GFXTexHandle &texture, const Point2I &offset, const RectI& updateRect)
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
		GFX->getDrawUtil()->setBitmapModulation(color);

		switch (mBitmapMode) 
		{
			case BitmapStretched: {
				RectI rect(offset, getExtent());
				GFX->getDrawUtil()->drawBitmapStretch(texture, rect);
				break;
			}

			case BitmapCentered: {
				Point2I p = offset;

				p.x += getExtent().x / 2 - texture.getWidth() / 2;
				p.y += getExtent().y / 2 - texture.getHeight() / 2;

				GFX->getDrawUtil()->drawBitmap(texture, p);
				break;
			}
		}
	}
};


IMPLEMENT_CONOBJECT(GuiFadingBitmapButtonCtrl);

ConsoleDocClass(GuiFadingBitmapButtonCtrl, "@brief Gui button control with an image that will fade in and out.\n\n" );

DefineConsoleMethod(GuiFadingBitmapButtonCtrl, fadeIn, void, (), , "") {
	object->fadeIn();
}

DefineConsoleMethod(GuiFadingBitmapButtonCtrl, fadeOut, void, (), , "") {
	object->fadeOut();
}
