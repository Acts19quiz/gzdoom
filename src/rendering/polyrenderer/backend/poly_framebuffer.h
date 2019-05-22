#pragma once

#include "gl_sysfb.h"
#include "rendering/swrenderer/r_memory.h"
#include "rendering/swrenderer/drawers/r_thread.h"
#include "rendering/polyrenderer/drawers/poly_triangle.h"

struct FRenderViewpoint;
class PolyDataBuffer;
class PolyRenderState;
class SWSceneDrawer;

class PolyFrameBuffer : public SystemBaseFrameBuffer
{
	typedef SystemBaseFrameBuffer Super;

public:
	RenderMemory *GetFrameMemory() { return &mFrameMemory; }
	PolyRenderState *GetRenderState() { return mRenderState.get(); }
	DCanvas *GetCanvas() { return mCanvas.get(); }
	const DrawerCommandQueuePtr &GetDrawCommands();
	void FlushDrawCommands();

	unsigned int GetLightBufferBlockSize() const;

	std::unique_ptr<SWSceneDrawer> swdrawer;

	PolyFrameBuffer(void *hMonitor, bool fullscreen);
	~PolyFrameBuffer();

	void Update();

	void InitializeState() override;

	void CleanForRestart() override;
	void PrecacheMaterial(FMaterial *mat, int translation) override;
	void UpdatePalette() override;
	uint32_t GetCaps() override;
	void WriteSavePic(player_t *player, FileWriter *file, int width, int height) override;
	sector_t *RenderView(player_t *player) override;
	void SetTextureFilterMode() override;
	void TextureFilterChanged() override;
	void StartPrecaching() override;
	void BeginFrame() override;
	void BlurScene(float amount) override;
	void PostProcessScene(int fixedcm, const std::function<void()> &afterBloomDrawEndScene2D) override;

	IHardwareTexture *CreateHardwareTexture() override;
	FModelRenderer *CreateModelRenderer(int mli) override;
	IVertexBuffer *CreateVertexBuffer() override;
	IIndexBuffer *CreateIndexBuffer() override;
	IDataBuffer *CreateDataBuffer(int bindingpoint, bool ssbo, bool needsresize) override;

	FTexture *WipeStartScreen() override;
	FTexture *WipeEndScreen() override;

	TArray<uint8_t> GetScreenshotBuffer(int &pitch, ESSType &color_type, float &gamma) override;

	void SetVSync(bool vsync) override;
	void Draw2D() override;

private:
	sector_t *RenderViewpoint(FRenderViewpoint &mainvp, AActor * camera, IntRect * bounds, float fov, float ratio, float fovratio, bool mainview, bool toscreen);
	void RenderTextureView(FCanvasTexture *tex, AActor *Viewpoint, double FOV);
	void DrawScene(HWDrawInfo *di, int drawmode);
	void UpdateShadowMap();

	void CheckCanvas();

	std::unique_ptr<PolyRenderState> mRenderState;
	std::unique_ptr<DCanvas> mCanvas;
	std::shared_ptr<DrawerCommandQueue> mDrawCommands;
	RenderMemory mFrameMemory;
};

inline PolyFrameBuffer *GetPolyFrameBuffer() { return static_cast<PolyFrameBuffer*>(screen); }
