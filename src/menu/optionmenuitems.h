/*
** optionmenuitems.h
** Control items for option menus
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/


void M_DrawConText (int color, int x, int y, const char *str);
void M_DrawSlider (int x, int y, double min, double max, double cur,int fracdigits);
void M_SetVideoMode();



//=============================================================================
//
// opens a submenu, action is a submenu name
//
//=============================================================================

class FOptionMenuItemSubmenu : public FOptionMenuItem
{
	int mParam;
public:
	FOptionMenuItemSubmenu(const char *label, const char *menu, int param = 0)
		: FOptionMenuItem(label, menu)
	{
		mParam = param;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColorMore);
		return indent;
	}

	bool Activate()
	{
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
		M_SetMenu(mAction, mParam);
		return true;
	}
};


//=============================================================================
//
// Executes a CCMD, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemCommand : public FOptionMenuItemSubmenu
{
public:
	FOptionMenuItemCommand(const char *label, const char *menu)
		: FOptionMenuItemSubmenu(label, menu)
	{
	}

	bool Activate()
	{
		C_DoCommand(mAction);
		return true;
	}

};

//=============================================================================
//
// Executes a CCMD after confirmation, action is a CCMD name
//
//=============================================================================

class FOptionMenuItemSafeCommand : public FOptionMenuItemCommand
{
	// action is a CCMD
public:
	FOptionMenuItemSafeCommand(const char *label, const char *menu)
		: FOptionMenuItemCommand(label, menu)
	{
	}

	bool MenuEvent (int mkey, bool fromcontroller)
	{
		if (mkey == MKEY_MBYes)
		{
			C_DoCommand(mAction);
			return true;
		}
		return FOptionMenuItemCommand::MenuEvent(mkey, fromcontroller);
	}

	bool Activate()
	{
		M_StartMessage("Do you really want to do this?", 0);
		return true;
	}
};

//=============================================================================
//
// Base class for option lists
//
//=============================================================================

class FOptionMenuItemOptionBase : public FOptionMenuItem
{
protected:
	// action is a CVAR
	FOptionValues *mValues;
	FBoolCVar *mGrayCheck;
	int mCenter;
public:

	enum
	{
		OP_VALUES = 0x11001
	};

	FOptionMenuItemOptionBase(const char *label, const char *menu, const char *values, const char *graycheck, int center)
		: FOptionMenuItem(label, menu)
	{
		FOptionValues **opt = OptionValues.CheckKey(values);
		if (opt != NULL) 
		{
			mValues = *opt;
		}
		else
		{
			mValues = NULL;
		}
		mGrayCheck = (FBoolCVar*)FindCVar(graycheck, NULL);
		if (mGrayCheck != NULL && mGrayCheck->GetRealType() != CVAR_Bool) mGrayCheck = NULL;
		mCenter = center;
	}

	bool SetString(int i, const char *newtext)
	{
		if (i == OP_VALUES) 
		{
			FOptionValues **opt = OptionValues.CheckKey(newtext);
			if (opt != NULL) 
			{
				mValues = *opt;
				int s = GetSelection();
				if (s >= (int)mValues->mValues.Size()) s = 0;
				SetSelection(s);	// readjust the CVAR if its value is outside the range now
				return true;
			}
		}
		return false;
	}



	//=============================================================================
	virtual int GetSelection() = 0;
	virtual void SetSelection(int Selection) = 0;

	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		bool grayed = mGrayCheck != NULL && !(**mGrayCheck);

		if (mCenter)
		{
			indent = (screen->GetWidth() / 2);
		}
		drawLabel(indent, y, OptionSettings.mFontColor, grayed);

		int overlay = grayed? MAKEARGB(96,48,0,0) : 0;
		const char *text;
		int Selection = GetSelection();
		if (Selection < 0)
		{
			text = "Unknown";
		}
		else
		{
			text = mValues->mValues[Selection].Text;
		}
		screen->DrawText (SmallFont, OptionSettings.mFontColorValue, indent + CURSORSPACE, y, 
			text, DTA_CleanNoMove_1, true, DTA_ColorOverlay, overlay, TAG_DONE);
		return indent;
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller)
	{
		if (mValues->mValues.Size() > 0)
		{
			int Selection = GetSelection();
			if (mkey == MKEY_Left)
			{
				if (Selection == -1) Selection = 0;
				else if (--Selection < 0) Selection = mValues->mValues.Size()-1;
			}
			else if (mkey == MKEY_Right || mkey == MKEY_Enter)
			{
				if (++Selection >= (int)mValues->mValues.Size()) Selection = 0;
			}
			else
			{
				return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
			}
			SetSelection(Selection);
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
		}
		return true;
	}

	bool Selectable()
	{
		return !(mGrayCheck != NULL && !(**mGrayCheck));
	}
};

//=============================================================================
//
// Change a CVAR, action is the CVAR name
//
//=============================================================================

class FOptionMenuItemOption : public FOptionMenuItemOptionBase
{
	// action is a CVAR
	FBaseCVar *mCVar;
public:

	FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck, int center)
		: FOptionMenuItemOptionBase(label, menu, values, graycheck, center)
	{
		mCVar = FindCVar(mAction, NULL);
	}

	//=============================================================================
	int GetSelection()
	{
		int Selection = -1;
		if (mValues != NULL && mCVar != NULL && mValues->mValues.Size() > 0)
		{
			if (mValues->mValues[0].TextValue.IsEmpty())
			{
				UCVarValue cv = mCVar->GetGenericRep(CVAR_Float);
				for(unsigned i=0;i<mValues->mValues.Size(); i++)
				{
					if (fabs(cv.Float - mValues->mValues[i].Value) < FLT_EPSILON)
					{
						Selection = i;
						break;
					}
				}
			}
			else
			{
				UCVarValue cv = mCVar->GetGenericRep(CVAR_String);
				for(unsigned i=0;i<mValues->mValues.Size(); i++)
				{
					if (mValues->mValues[i].TextValue.CompareNoCase(cv.String) == 0)
					{
						Selection = i;
						break;
					}
				}
			}
		}
		return Selection;
	}

	void SetSelection(int Selection)
	{
		UCVarValue value;
		if (mValues != NULL && mCVar != NULL && mValues->mValues.Size() > 0)
		{
			if (mValues->mValues[0].TextValue.IsEmpty())
			{
				value.Float = (float)mValues->mValues[Selection].Value;
				mCVar->SetGenericRep (value, CVAR_Float);
			}
			else
			{
				value.String = mValues->mValues[Selection].TextValue.LockBuffer();
				mCVar->SetGenericRep (value, CVAR_String);
				mValues->mValues[Selection].TextValue.UnlockBuffer();
			}
		}
	}
};

//=============================================================================
//
// This class is used to capture the key to be used as the new key binding
// for a control item
//
//=============================================================================

class DEnterKey : public DMenu
{
	DECLARE_CLASS(DEnterKey, DMenu)

	int *pKey;

public:
	DEnterKey(DMenu *parent, int *keyptr)
	: DMenu(parent)
	{
		pKey = keyptr;
		SetMenuMessage(1);
		menuactive = MENU_WaitKey;	// There should be a better way to disable GUI capture...
	}

	bool TranslateKeyboardEvents()
	{
		return false; 
	}

	void SetMenuMessage(int which)
	{
		if (mParentMenu->IsKindOf(RUNTIME_CLASS(DListMenu)))
		{
			DListMenu *m = barrier_cast<DListMenu*>(mParentMenu);
			FListMenuItem *it = m->GetItem(NAME_Controlmessage);
			if (it != NULL)
			{
				it->SetValue(0, which);
			}
		}
	}

	bool Responder(event_t *ev)
	{
		if (ev->type == EV_KeyDown)
		{
			*pKey = ev->data1;
			menuactive = MENU_On;
			SetMenuMessage(0);
			Close();
			mParentMenu->MenuEvent((ev->data1 == KEY_ESCAPE)? MKEY_Abort : MKEY_Input, 0);
			return true;
		}
		return false;
	}

	void Drawer()
	{
		mParentMenu->Drawer();
	}
};

#ifndef NO_IMP
IMPLEMENT_ABSTRACT_CLASS(DEnterKey)
#endif

//=============================================================================
//
// // Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

class FOptionMenuItemControl : public FOptionMenuItem
{
	FKeyBindings *mBindings;
	int mInput;
	bool mWaiting;
public:

	FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings)
		: FOptionMenuItem(label, menu)
	{
		mBindings = bindings;
		mWaiting = false;
	}


	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, mWaiting? OptionSettings.mFontColorHighlight: OptionSettings.mFontColor);

		char description[64];
		int Key1, Key2;

		mBindings->GetKeysForCommand(mAction, &Key1, &Key2);
		C_NameKeys (description, Key1, Key2);
		if (description[0])
		{
			M_DrawConText(CR_WHITE, indent + CURSORSPACE, y-1+OptionSettings.mLabelOffset, description);
		}
		else
		{
			screen->DrawText(SmallFont, CR_BLACK, indent + CURSORSPACE, y + OptionSettings.mLabelOffset, "---",
				DTA_CleanNoMove_1, true, TAG_DONE);
		}
		return indent;
	}

	//=============================================================================
	bool MenuEvent(int mkey, bool fromcontroller)
	{
		if (mkey == MKEY_Input)
		{
			mWaiting = false;
			mBindings->SetBind(mInput, mAction);
			return true;
		}
		else if (mkey == MKEY_Clear)
		{
			mBindings->UnbindACommand(mAction);
			return true;
		}
		else if (mkey == MKEY_Abort)
		{
			mWaiting = false;
			return true;
		}
		return false;
	}

	bool Activate()
	{
		mWaiting = true;
		DMenu *input = new DEnterKey(DMenu::CurrentMenu, &mInput);
		M_ActivateMenu(input);
		return true;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticText : public FOptionMenuItem
{
	EColorRange mColor;
public:
	FOptionMenuItemStaticText(const char *label, bool header)
		: FOptionMenuItem(label, NAME_None, true)
	{
		mColor = header? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, mColor);
		return -1;
	}

	bool Selectable()
	{
		return false;
	}

};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuItemStaticTextSwitchable : public FOptionMenuItem
{
	EColorRange mColor;
	FString mAltText;
	int mCurrent;

public:
	FOptionMenuItemStaticTextSwitchable(const char *label, const char *label2, FName action, bool header)
		: FOptionMenuItem(label, action, true)
	{
		mColor = header? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;
		mAltText = label2;
		mCurrent = 0;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		const char *txt = mCurrent? (const char*)mAltText : mLabel;
		int w = SmallFont->StringWidth(txt) * CleanXfac_1;
		int x = (screen->GetWidth() - w) / 2;
		screen->DrawText (SmallFont, mColor, x, y, txt, DTA_CleanNoMove_1, true, TAG_DONE);
		return -1;
	}

	bool SetValue(int i, int val)
	{
		if (i == 0) 
		{
			mCurrent = val;
			return true;
		}
		return false;
	}

	bool SetString(int i, const char *newtext)
	{
		if (i == 0) 
		{
			mAltText = newtext;
			return true;
		}
		return false;
	}

	bool Selectable()
	{
		return false;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderBase : public FOptionMenuItem
{
	// action is a CVAR
	double mMin, mMax, mStep;
	int mShowValue;
public:
	FOptionMenuSliderBase(const char *label, double min, double max, double step, int showval)
		: FOptionMenuItem(label, NAME_None)
	{
		mMin = min;
		mMax = max;
		mStep = step;
		mShowValue = showval;
	}

	virtual double GetValue() = 0;
	virtual void SetValue(double val) = 0;

	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColor);
		M_DrawSlider (indent + CURSORSPACE, y + OptionSettings.mLabelOffset, mMin, mMax, GetValue(), mShowValue);
		return indent;
	}

	//=============================================================================
	bool MenuEvent (int mkey, bool fromcontroller)
	{
		double value = GetValue();

		if (mkey == MKEY_Left)
		{
			value -= mStep;
		}
		else if (mkey == MKEY_Right)
		{
			value += mStep;
		}
		else
		{
			return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
		}
		SetValue(clamp(value, mMin, mMax));
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
		return true;
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderCVar : public FOptionMenuSliderBase
{
	FBaseCVar *mCVar;
public:
	FOptionMenuSliderCVar(const char *label, const char *menu, double min, double max, double step, int showval)
		: FOptionMenuSliderBase(label, min, max, step, showval)
	{
		mCVar = FindCVar(menu, NULL);
	}

	double GetValue()
	{
		if (mCVar != NULL)
		{
			return mCVar->GetGenericRep(CVAR_Float).Float;
		}
		else
		{
			return 0;
		}
	}

	void SetValue(double val)
	{
		if (mCVar != NULL)
		{
			UCVarValue value;
			value.Float = (float)val;
			mCVar->SetGenericRep(value, CVAR_Float);
		}
	}
};

//=============================================================================
//
//
//
//=============================================================================

class FOptionMenuSliderVar : public FOptionMenuSliderBase
{
	float *mPVal;
public:

	FOptionMenuSliderVar(const char *label, float *pVal, double min, double max, double step, int showval)
		: FOptionMenuSliderBase(label, min, max, step, showval)
	{
		mPVal = pVal;
	}

	double GetValue()
	{
		return *mPVal;
	}

	void SetValue(double val)
	{
		*mPVal = (float)val;
	}
};

//=============================================================================
//
// // Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

class FOptionMenuItemColorPicker : public FOptionMenuItem
{
	FColorCVar *mCVar;
public:

	enum
	{
		CPF_RESET = 0x20001,
	};

	FOptionMenuItemColorPicker(const char *label, const char *menu)
		: FOptionMenuItem(label, menu)
	{
		FBaseCVar *cv = FindCVar(menu, NULL);
		if (cv->GetRealType() == CVAR_Color)
		{
			mCVar = (FColorCVar*)cv;
		}
		else mCVar = NULL;
	}

	//=============================================================================
	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		drawLabel(indent, y, OptionSettings.mFontColor);

		if (mCVar != NULL)
		{
			int box_x = indent + CURSORSPACE;
			int box_y = y + OptionSettings.mLabelOffset * CleanYfac_1 / 2;
			screen->Clear (box_x, box_y, box_x + 32*CleanXfac_1, box_y + (SmallFont->GetHeight() - 1) * CleanYfac_1,
				-1, (uint32)*mCVar | 0xff000000);
		}
		return indent;
	}

	bool SetValue(int i, int v)
	{
		if (i == CPF_RESET && mCVar != NULL)
		{
			mCVar->ResetToDefault();
			return true;
		}
		return false;
	}

	bool Activate()
	{
		if (mCVar != NULL)
		{
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
			DMenu *picker = StartPickerMenu(DMenu::CurrentMenu, mLabel, mCVar);
			if (picker != NULL)
			{
				M_ActivateMenu(picker);
				return true;
			}
		}
		return false;
	}
};

class FOptionMenuScreenResolutionLine : public FOptionMenuItem
{
	FString mResTexts[3];
	int mSelection;
	int mHighlight;
	int mMaxValid;
public:

	enum
	{
		SRL_INDEX = 0x30000,
		SRL_SELECTION = 0x30003,
		SRL_HIGHLIGHT = 0x30004,
	};

	FOptionMenuScreenResolutionLine(const char *action)
		: FOptionMenuItem("", action)
	{
		mSelection = 0;
		mHighlight = -1;
	}

	bool SetValue(int i, int v)
	{
		if (i == SRL_SELECTION)
		{
			mSelection = v;
			return true;
		}
		else if (i == SRL_HIGHLIGHT)
		{
			mHighlight = v;
			return true;
		}
		return false;
	}

	bool GetValue(int i, int *v)
	{
		if (i == SRL_SELECTION)
		{
			*v = mSelection;
			return true;
		}
		return false;
	}

	bool SetString(int i, const char *newtext)
	{
		if (i >= SRL_INDEX && i <= SRL_INDEX+2) 
		{
			mResTexts[i-SRL_INDEX] = newtext;
			if (mResTexts[0].IsEmpty()) mMaxValid = -1;
			else if (mResTexts[1].IsEmpty()) mMaxValid = 0;
			else if (mResTexts[2].IsEmpty()) mMaxValid = 1;
			else mMaxValid = 2;
			return true;
		}
		return false;
	}

	bool GetString(int i, char *s, int len)
	{
		if (i >= SRL_INDEX && i <= SRL_INDEX+2) 
		{
			strncpy(s, mResTexts[i-SRL_INDEX], len-1);
			s[len-1] = 0;
			return true;
		}
		return false;
	}

	bool MenuEvent (int mkey, bool fromcontroller)
	{
		if (mkey == MKEY_Left)
		{
			if (--mSelection < 0) mSelection = mMaxValid;
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
			return true;
		}
		else if (mkey == MKEY_Right)
		{
			if (++mSelection > mMaxValid) mSelection = 0;
			S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
			return true;
		}
		else 
		{
			return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
		}
		return false;
	}

	bool Activate()
	{
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
		M_SetVideoMode();
		return true;
	}

	int Draw(FOptionMenuDescriptor *desc, int y, int indent)
	{
		int colwidth = screen->GetWidth() / 3;
		EColorRange color;

		for (int x = 0; x < 3; x++)
		{
			if (x == mHighlight)
				color = CR_GOLD;	//ValueColor;
			else
				color = CR_BRICK;	//LabelColor;

			screen->DrawText (SmallFont, color, colwidth * x + 20 * CleanXfac_1, y, mResTexts[x], DTA_CleanNoMove_1, true, TAG_DONE);
		}
		return colwidth * mSelection + 20 * CleanXfac_1 - CURSORSPACE;
	}

	bool Selectable()
	{
		return mMaxValid >= 0;
	}
};

#ifndef NO_IMP
CCMD(am_restorecolors)
{
	if (DMenu::CurrentMenu != NULL && DMenu::CurrentMenu->IsKindOf(RUNTIME_CLASS(DOptionMenu)))
	{
		DOptionMenu *m = (DOptionMenu*)DMenu::CurrentMenu;
		const FOptionMenuDescriptor *desc = m->GetDescriptor();
		// Find the color cvars by scanning the MapColors menu.
		for (unsigned i = 0; i < desc->mItems.Size(); ++i)
		{
			desc->mItems[i]->SetValue(FOptionMenuItemColorPicker::CPF_RESET, 0);
		}
	}
}
#endif
