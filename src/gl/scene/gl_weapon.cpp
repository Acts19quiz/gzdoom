/*
** gl_weapon.cpp
** Weapon sprite drawing
**
**---------------------------------------------------------------------------
** Copyright 2002-2005 Christoph Oelckers
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
** 4. When not used as part of GZDoom or a GZDoom derivative, this code will be
**    covered by the terms of the GNU Lesser General Public License as published
**    by the Free Software Foundation; either version 2.1 of the License, or (at
**    your option) any later version.
** 5. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
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
#include "gl/system/gl_system.h"
#include "sbar.h"
#include "r_utility.h"
#include "v_video.h"
#include "doomstat.h"
#include "d_player.h"
#include "g_level.h"

#include "gl/system/gl_interface.h"
#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/data/gl_data.h"
#include "gl/dynlights/gl_glow.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/models/gl_models.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"

EXTERN_CVAR (Bool, r_drawplayersprites)
EXTERN_CVAR(Float, transsouls)
EXTERN_CVAR (Bool, st_scale)
EXTERN_CVAR(Int, gl_fuzztype)
EXTERN_CVAR (Bool, r_deathcamera)


//==========================================================================
//
// R_DrawPSprite
//
//==========================================================================

void FGLRenderer::DrawPSprite (player_t * player, DPSprite *psp, float sx, float sy, int cm_index, bool hudModelStep, int OverrideShader)
{
	float			fU1,fV1;
	float			fU2,fV2;
	float			tx;
	float			x1,y1,x2,y2;
	float			scale;
	float			scalex;
	float			ftexturemid;
	                      // 4:3  16:9   16:10  17:10    5:4  17:10    21:9
	static float xratio[] = {1.f, 3.f/4, 5.f/6, 40.f/51, 1.f, 40.f/51, 4.f/7};
	
	// [BB] In the HUD model step we just render the model and break out. 
	if ( hudModelStep )
	{
		gl_RenderHUDModel( psp, sx, sy, cm_index );
		return;
	}

	// decide which patch to use
	bool mirror;
	FTextureID lump = gl_GetSpriteFrame(psp->GetSprite(), psp->GetFrame(), 0, 0, &mirror);
	if (!lump.isValid()) return;

	FMaterial * tex = FMaterial::ValidateTexture(lump, false);
	if (!tex) return;

	tex->BindPatch(cm_index, 0, OverrideShader);

	float vw = (float)viewwidth;
	float vh = (float)viewheight;

	// calculate edges of the shape
	scalex = xratio[WidescreenRatio] * vw / 320;

	tx = sx - (160 + tex->GetScaledLeftOffset(GLUSE_PATCH));
	x1 = tx * scalex + vw/2;
	if (x1 > vw)	return; // off the right side
	x1 += viewwindowx;

	tx += tex->TextureWidth(GLUSE_PATCH);
	x2 = tx * scalex + vw / 2;
	if (x2 < 0) return; // off the left side
	x2 += viewwindowx;


	// killough 12/98: fix psprite positioning problem
	ftexturemid = 100.f - (sy - tex->GetScaledTopOffset(GLUSE_PATCH));

	AWeapon * wi=player->ReadyWeapon;
	if (wi && wi->YAdjust != 0)
	{
		float fYAd = wi->YAdjust;
		if (screenblocks >= 11)
		{
			ftexturemid -= fYAd;
		}
		else if (!st_scale)
		{
			ftexturemid -= StatusBar->GetDisplacement () * fYAd;
		}
	}

	scale = (SCREENHEIGHT*vw) / (SCREENWIDTH * 200.0f);
	y1 = viewwindowy + vh / 2 - (ftexturemid * scale);
	y2 = y1 + (tex->TextureHeight(GLUSE_PATCH) * scale) + 1;

	if (!mirror)
	{
		fU1=tex->GetUL();
		fV1=tex->GetVT();
		fU2=tex->GetUR();
		fV2=tex->GetVB();
	}
	else
	{
		fU2=tex->GetUL();
		fV1=tex->GetVT();
		fU1=tex->GetUR();
		fV2=tex->GetVB();
	}

	if (tex->GetTransparent() || OverrideShader != 0)
	{
		gl_RenderState.EnableAlphaTest(false);
	}
	gl_RenderState.Apply();
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(fU1, fV1); glVertex2f(x1,y1);
	glTexCoord2f(fU1, fV2); glVertex2f(x1,y2);
	glTexCoord2f(fU2, fV1); glVertex2f(x2,y1);
	glTexCoord2f(fU2, fV2); glVertex2f(x2,y2);
	glEnd();
	if (tex->GetTransparent() || OverrideShader != 0)
	{
		gl_RenderState.EnableAlphaTest(true);
	}
}

//==========================================================================
//
//
//
//==========================================================================

static bool isBright(DPSprite *psp)
{
	if (psp != NULL && psp->GetState() != NULL)
	{
		bool disablefullbright = false;
		FTextureID lump = gl_GetSpriteFrame(psp->GetSprite(), psp->GetFrame(), 0, 0, NULL);
		if (lump.isValid())
		{
			FMaterial * tex = FMaterial::ValidateTexture(lump, false);
			if (tex)
				disablefullbright = tex->tex->gl_info.bDisableFullbright;
		}
		return psp->GetState()->GetFullbright() && !disablefullbright;
	}
	return false;
}

//==========================================================================
//
// R_DrawPlayerSprites
//
//==========================================================================

EXTERN_CVAR(Bool, gl_brightfog)

void FGLRenderer::DrawPlayerSprites(sector_t * viewsector, bool hudModelStep)
{
	bool brightflash = false;
	unsigned int i;
	int lightlevel=0;
	FColormap cm;
	sector_t * fakesec, fs;
	AActor * playermo=players[consoleplayer].camera;
	player_t * player=playermo->player;
	
	// this is the same as the software renderer
	if (!player ||
		!r_drawplayersprites ||
		!camera->player ||
		(player->cheats & CF_CHASECAM) || 
		(r_deathcamera && camera->health <= 0))
		return;

	float bobx, boby, wx, wy;
	DPSprite *weapon;

	P_BobWeapon(camera->player, &bobx, &boby, r_TicFracF);

	// Interpolate the main weapon layer once so as to be able to add it to other layers.
	if ((weapon = camera->player->FindPSprite(PSP_WEAPON)) != NULL)
	{
		if (weapon->firstTic)
		{
			wx = weapon->x;
			wy = weapon->y;
		}
		else
		{
			wx = weapon->oldx + (weapon->x - weapon->oldx) * r_TicFracF;
			wy = weapon->oldy + (weapon->y - weapon->oldy) * r_TicFracF;
		}
	}
	else
	{
		wx = 0;
		wy = 0;
	}

	if (gl_fixedcolormap) 
	{
		lightlevel=255;
		cm.GetFixedColormap();
		fakesec = viewsector;
	}
	else
	{
		fakesec    = gl_FakeFlat(viewsector, &fs, false);

		// calculate light level for weapon sprites
		lightlevel = gl_ClampLight(fakesec->lightlevel);

		// calculate colormap for weapon sprites
		if (viewsector->e->XFloor.ffloors.Size() && !glset.nocoloredspritelighting)
		{
			TArray<lightlist_t> & lightlist = viewsector->e->XFloor.lightlist;
			for(i=0;i<lightlist.Size();i++)
			{
				double lightbottom;

				if (i<lightlist.Size()-1) 
				{
					lightbottom=lightlist[i+1].plane.ZatPoint(ViewPos);
				}
				else 
				{
					lightbottom=viewsector->floorplane.ZatPoint(ViewPos);
				}

				if (lightbottom<player->viewz) 
				{
					cm = lightlist[i].extra_colormap;
					lightlevel = gl_ClampLight(*lightlist[i].p_lightlevel);
					break;
				}
			}
		}
		else 
		{
			cm=fakesec->ColorMap;
			if (glset.nocoloredspritelighting) cm.ClearColor();
		}

		lightlevel = gl_CalcLightLevel(lightlevel, getExtraLight(), true);

		if (glset.lightmode == 8)
		{
			// Korshun: the way based on max possible light level for sector like in software renderer.
			float min_L = 36.0 / 31.0 - ((lightlevel / 255.0) * (63.0 / 31.0)); // Lightlevel in range 0-63
			if (min_L < 0)
				min_L = 0;
			else if (min_L > 1.0)
				min_L = 1.0;

			lightlevel = (1.0 - min_L) * 255;
		}
		else
		{
			lightlevel = (2 * lightlevel + 255) / 3;
		}
		lightlevel = gl_CheckSpriteGlow(viewsector, lightlevel, playermo->Pos());

	}
	
	// Korshun: fullbright fog in opengl, render weapon sprites fullbright (but don't cancel out the light color!)
	if (glset.brightfog && ((level.flags&LEVEL_HASFADETABLE) || cm.FadeColor != 0))
	{
		lightlevel = 255;
	}

	PalEntry ThingColor = playermo->fillcolor;
	visstyle_t vis;

	vis.RenderStyle=playermo->RenderStyle;
	vis.Alpha=playermo->Alpha;
	vis.colormap = NULL;
	if (playermo->Inventory) 
	{
		playermo->Inventory->AlterWeaponSprite(&vis);
		if (vis.colormap >= SpecialColormaps[0].Colormap && 
			vis.colormap < SpecialColormaps[SpecialColormaps.Size()].Colormap && 
			cm.colormap == CM_DEFAULT)
		{
			ptrdiff_t specialmap = (vis.colormap - SpecialColormaps[0].Colormap) / sizeof(FSpecialColormap);
			cm.colormap = int(CM_FIRSTSPECIALCOLORMAP + specialmap);
		}
	}

	// Set the render parameters

	int OverrideShader = 0;
	float trans = 0.f;
	if (vis.RenderStyle.BlendOp >= STYLEOP_Fuzz && vis.RenderStyle.BlendOp <= STYLEOP_FuzzOrRevSub)
	{
		vis.RenderStyle.CheckFuzz();
		if (vis.RenderStyle.BlendOp == STYLEOP_Fuzz)
		{
			if (gl.shadermodel >= 4 && gl_fuzztype != 0)
			{
				// Todo: implement shader selection here
				vis.RenderStyle = LegacyRenderStyles[STYLE_Translucent];
				OverrideShader = gl_fuzztype + 4;
				trans = 0.99f;	// trans may not be 1 here
			}
			else
			{
				vis.RenderStyle.BlendOp = STYLEOP_Shadow;
			}
		}
	}

	gl_SetRenderStyle(vis.RenderStyle, false, false);

	if (vis.RenderStyle.Flags & STYLEF_TransSoulsAlpha)
	{
		trans = transsouls;
	}
	else if (vis.RenderStyle.Flags & STYLEF_Alpha1)
	{
		trans = 1.f;
	}
	else if (trans == 0.f)
	{
		trans = vis.Alpha;
	}

	// now draw the different layers of the weapon
	gl_RenderState.EnableBrightmap(true);

	// hack alert! Rather than changing everything in the underlying lighting code let's just temporarily change
	// light mode here to draw the weapon sprite.
	int oldlightmode = glset.lightmode;
	if (glset.lightmode == 8) glset.lightmode = 2;

	for(DPSprite *psp = player->psprites; psp != NULL && psp->GetID() < PSP_TARGETCENTER; psp = psp->GetNext())
	{
		if (psp->GetState() != NULL) 
		{
			FColormap cmc = cm;
			int ll = lightlevel;
			if (isBright(psp)) 
			{
				if (fakesec == viewsector || in_area != area_below)	
					// under water areas keep most of their color for fullbright objects
				{
					cmc.LightColor.r=
					cmc.LightColor.g=
					cmc.LightColor.b=0xff;
				}
				else
				{
					cmc.LightColor.r = (3*cmc.LightColor.r + 0xff)/4;
					cmc.LightColor.g = (3*cmc.LightColor.g + 0xff)/4;
					cmc.LightColor.b = (3*cmc.LightColor.b + 0xff)/4;
				}
				ll = 255;
			}
			// set the lighting parameters (only calls glColor and glAlphaFunc)
			gl_SetSpriteLighting(vis.RenderStyle, playermo, ll, 
				0, &cmc, 0xffffff, trans, isBright(psp), true);
			if (psp->firstTic)
			{ // Can't interpolate the first tic.
				psp->firstTic = false;
				psp->oldx = psp->x;
				psp->oldy = psp->y;
			}

			float sx = psp->oldx + (psp->x - psp->oldx) * r_TicFracF;
			float sy = psp->oldy + (psp->y - psp->oldy) * r_TicFracF;

			if (psp->Flags & PSPF_ADDBOB)
			{
				sx += bobx;
				sy += boby;
			}

			if (psp->Flags & PSPF_ADDWEAPON && psp->GetID() != PSP_WEAPON)
			{
				sx += wx;
				sy += wy;
			}

			DrawPSprite(player, psp, sx, sy, cm.colormap, hudModelStep, OverrideShader);
		}
	}
	gl_RenderState.EnableBrightmap(false);
	glset.lightmode = oldlightmode;
}

//==========================================================================
//
// R_DrawPlayerSprites
//
//==========================================================================

void FGLRenderer::DrawTargeterSprites()
{
	AActor * playermo=players[consoleplayer].camera;
	player_t * player=playermo->player;
	
	if(!player || playermo->renderflags&RF_INVISIBLE || !r_drawplayersprites ||
		mViewActor!=playermo) return;

	gl_RenderState.EnableBrightmap(false);
	gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_RenderState.AlphaFunc(GL_GEQUAL,gl_mask_sprite_threshold);
	gl_RenderState.BlendEquation(GL_FUNC_ADD);
	glColor3f(1.0f,1.0f,1.0f);
	gl_RenderState.SetTextureMode(TM_MODULATE);

	// The Targeter's sprites are always drawn normally.
	for (DPSprite *psp = player->FindPSprite(PSP_TARGETCENTER); psp !=NULL; psp = psp->GetNext())
	{
		if (psp->GetState() != NULL) DrawPSprite(player, psp, psp->x, psp->y, CM_DEFAULT, false, 0);
	}
}