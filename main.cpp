#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")

#include <windows.h>
#include <tchar.h>
#include <d3d9.h>
#include <directxmath.h>
#include <stdio.h>
using namespace DirectX;

TCHAR gName[100] = _T("改・高速フォント表示サンプルプログラム");
bool bShowTextMetrics = true;

D3DMATRIX conv(XMMATRIX& m) {
    D3DMATRIX d3dm = {
        m.r[0].m128_f32[0], m.r[0].m128_f32[1], m.r[0].m128_f32[2], m.r[0].m128_f32[3],
        m.r[1].m128_f32[0], m.r[1].m128_f32[1], m.r[1].m128_f32[2], m.r[1].m128_f32[3],
        m.r[2].m128_f32[0], m.r[2].m128_f32[1], m.r[2].m128_f32[2], m.r[2].m128_f32[3],
        m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2], m.r[3].m128_f32[3],
    };
    return d3dm;
}

struct Vtx {
    float x, y, z;
    float u, v;
};

struct LineVtx {
    float x, y, z;
    DWORD color;
};

void drawLineW(IDirect3DDevice9* dev, float sx, float sy, float sz, float ex, float ey, float ez, DWORD color) {
    LineVtx p[2] = {
            {sx, sy, sz, color},
            {ex, ey, ez, color}
    };
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_LINELIST, 1, &p, sizeof(LineVtx));
}

void drawRectW(IDirect3DDevice9* dev, float l, float t, float w, float h, DWORD color) {
    drawLineW(dev, l, t, 0.0f, l + w, t, 0.0f, color);
    drawLineW(dev, l + w, t, 0.0f, l + w, t - h, 0.0f, color);
    drawLineW(dev, l + w, t - h, 0.0f, l, t - h, 0.0f, color);
    drawLineW(dev, l, t - h, 0.0f, l, t, 0.0f, color);
}
void drawTextMetrics(IDirect3DDevice9* dev, TEXTMETRIC tm, GLYPHMETRICS gm, int ox, int oy) {
    XMMATRIX idn= XMMatrixIdentity();
    dev->SetTransform(D3DTS_WORLD, &conv(idn));
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Base Line
    D3DVIEWPORT9 vp;
    dev->GetViewport(&vp);
    drawLineW(dev, (float)vp.Width / -2.0f, (float)oy, 0.0f, (float)vp.Width / 2, (float)oy, 0.0f, 0xffff0000);

    // Ascent Line
    drawLineW(dev, (float)vp.Width / -2.0f, (float)(oy + tm.tmAscent), 0.0f, (float)vp.Width / 2, (float)(oy + tm.tmAscent), 0.0f, 0xffff0000);

    // Descent Line
    drawLineW(dev, (float)vp.Width / -2.0f, (float)(oy - tm.tmDescent), 0.0f, (float)vp.Width / 2, (float)(oy - tm.tmDescent), 0.0f, 0xffff0000);

    // Origin
    drawRectW(dev, (float)ox - 2.0f, (float)oy + 2.0f, 4.0f, 4.0f, 0xff00ff00);

    // Next Origin
    drawRectW(dev, (float)(ox + gm.gmCellIncX) - 2.0f, (float)oy + 2.0f, 4.0f, 4.0f, 0xffffff00);

    // BlackBox
    drawRectW(dev, (float)(ox + gm.gmptGlyphOrigin.x), (float)(oy + gm.gmptGlyphOrigin.y), (float)gm.gmBlackBoxX, (float)gm.gmBlackBoxY, 0x00ff0000ff);
};




LRESULT CALLBACK WndProc(HWND hWnd, UINT mes, WPARAM wParam, LPARAM lParam) {
    switch (mes) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if (wParam == 's')
            bShowTextMetrics = !bShowTextMetrics;
        break;
    }
    return DefWindowProc(hWnd, mes, wParam, lParam);
}

struct FONT {
    IDirect3DTexture9* pTex;
    GLYPHMETRICS gm;
};
void createFont(IDirect3DDevice9* dev, const wchar_t* c, int fontSize, FONT* font) {
    // フォントの生成
    int fontWeight = 500;
    //LOGFONT lf = { fontSize, 0, 0, 0, fontWeight, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("ＭＳ Ｐ明朝") };
    LOGFONT lf = { fontSize, 0, 0, 0, fontWeight, 0, 0, 0, 
        SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("BIZ UDMincho Medium") };
    HFONT hFont = CreateFontIndirect(&lf);
    //if (hFont == NULL) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    // デバイスにフォントを持たせないとGetGlyphOutline関数はエラーとなる
    HDC hdc = GetDC(NULL);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    // フォントビットマップ取得
    UINT code = (UINT)c[0];
    const int gradFlag = GGO_GRAY8_BITMAP; // GGO_GRAY2_BITMAP or GGO_GRAY4_BITMAP or GGO_GRAY8_BITMAP
    int grad = 0; // 階調の最大値
    switch (gradFlag) {
    case GGO_GRAY2_BITMAP: grad = 4; break;
    case GGO_GRAY4_BITMAP: grad = 16; break;
    case GGO_GRAY8_BITMAP: grad = 64; break;
    }
    //if (grad == 0) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    GLYPHMETRICS gm;
    CONST MAT2 mat = { {0,1},{0,0},{0,0},{0,1} };
    DWORD size = GetGlyphOutlineW(hdc, code, gradFlag, &gm, 0, NULL, &mat);
    BYTE* pMono = new BYTE[size];
    GetGlyphOutlineW(hdc, code, gradFlag, &gm, size, pMono, &mat);
    // デバイスコンテキストとフォントハンドルはもういらないので解放
    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    // テクスチャ作成
    IDirect3DTexture9* pTex = 0;
    gm.gmBlackBoxX = (gm.gmBlackBoxX + 3) / 4 * 4;
    //int fontHeight = gm.gmBlackBoxY;
    dev->CreateTexture(gm.gmBlackBoxX, gm.gmBlackBoxY, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL);
    // テクスチャにフォントビットマップ情報を書き込み
    D3DLOCKED_RECT lockedRect;
    pTex->LockRect(0, &lockedRect, NULL, 0);  // ロック
    DWORD* pTexBuf = (DWORD*)lockedRect.pBits;   // テクスチャメモリへのポインタ
    for (int y = 0; y < gm.gmBlackBoxY; y++) {
        for (int x = 0; x < gm.gmBlackBoxX; x++) {
            int i = y * gm.gmBlackBoxX + x;
            DWORD alpha = pMono[i] * 255 / grad;
            pTexBuf[i] = (alpha << 24) | 0x00ffffff;
        }
    }
    pTex->UnlockRect(0);  // アンロック
    delete[] pMono;

    font->pTex = pTex;
    font->gm = gm;
}

void drawFont(IDirect3DDevice9* dev, FONT* font, int ox, int oy) {
    // 各種行列
    XMMATRIX localScale = XMMatrixScaling((float)font->gm.gmBlackBoxX, (float)font->gm.gmBlackBoxY, 1.0f);
    XMMATRIX localOffset = XMMatrixTranslation((float)font->gm.gmptGlyphOrigin.x, (float)font->gm.gmptGlyphOrigin.y, 0.0f);
    XMMATRIX localMat = localScale*localOffset;
    XMMATRIX worldOffset = XMMatrixTranslation((float)ox - 0.5f, (float)oy + 0.5f, 0.0f);
    XMMATRIX world = localMat *worldOffset;
    dev->SetTransform(D3DTS_WORLD, &conv(world));
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
    dev->SetTexture(0, font->pTex);
    //g_pD3DDev->SetStreamSource(0, pVertexBuffer, 0, sizeof(Vtx));
    dev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    // アプリケーションの初期化
    MSG msg; HWND hWnd;
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, (TCHAR*)gName, NULL };
    if (!RegisterClassEx(&wcex))
        return 0;
    int screenW = 640, screenH = 480;
    RECT R = { 0, 0, screenW, screenH };
    ::AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
    if (!(hWnd = CreateWindow(gName, gName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, R.right - R.left, R.bottom - R.top, NULL, NULL, hInstance, NULL)))
        return 0;
    // Direct3Dの初期化
    LPDIRECT3D9 g_pD3D;
    LPDIRECT3DDEVICE9 g_pD3DDev;
    if (!(g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return 0;
    D3DPRESENT_PARAMETERS d3dpp = { screenW, screenH, D3DFMT_X8R8G8B8, 0, D3DMULTISAMPLE_NONE, 0, D3DSWAPEFFECT_DISCARD, NULL, TRUE, 0, D3DFMT_UNKNOWN, 0, 0 };
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pD3DDev))) {
        g_pD3D->Release();
        return 0;
    }
    ShowWindow(hWnd, SW_SHOW);

/*
    // フォントの生成
    int fontSize = 240;
    int fontWeight = 0;
    //LOGFONT lf = { fontSize, 0, 0, 0, fontWeight, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("ＭＳ Ｐ明朝") };
    LOGFONT lf = {fontSize, 0, 0, 0, fontWeight, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("ＭＳ Ｐゴシック")};
    HFONT hFont = CreateFontIndirect(&lf);
    if (hFont == NULL) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    // デバイスにフォントを持たせないとGetGlyphOutline関数はエラーとなる
    HDC hdc = GetDC(NULL);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    // フォントビットマップ取得
    const wchar_t c[] = L"7";
    UINT code = (UINT)c[0];
    const int gradFlag = GGO_GRAY2_BITMAP; // GGO_GRAY2_BITMAP or GGO_GRAY4_BITMAP or GGO_GRAY8_BITMAP
    int grad = 0; // 階調の最大値
    switch (gradFlag) {
    case GGO_GRAY2_BITMAP: grad = 4; break;
    case GGO_GRAY4_BITMAP: grad = 16; break;
    case GGO_GRAY8_BITMAP: grad = 64; break;
    }
    if (grad == 0) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    GLYPHMETRICS gm;
    CONST MAT2 mat = { {0,1},{0,0},{0,0},{0,1} };
    DWORD size = GetGlyphOutlineW(hdc, code, gradFlag, &gm, 0, NULL, &mat);
    BYTE* pMono = new BYTE[size];
    GetGlyphOutlineW(hdc, code, gradFlag, &gm, size, pMono, &mat);
    // デバイスコンテキストとフォントハンドルはもういらないので解放
    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    // テクスチャ作成
    IDirect3DTexture9* pTex = 0;
    gm.gmBlackBoxX = (gm.gmBlackBoxX + 3) / 4 * 4;
    //int fontHeight = gm.gmBlackBoxY;
    g_pD3DDev->CreateTexture(gm.gmBlackBoxX, gm.gmBlackBoxY, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL);
    // テクスチャにフォントビットマップ情報を書き込み
    D3DLOCKED_RECT lockedRect;
    pTex->LockRect(0, &lockedRect, NULL, 0);  // ロック
    DWORD* pTexBuf = (DWORD*)lockedRect.pBits;   // テクスチャメモリへのポインタ
    for (int y = 0; y < gm.gmBlackBoxY; y++) {
        for (int x = 0; x < gm.gmBlackBoxX; x++) {
            int i = y * gm.gmBlackBoxX + x;
            DWORD alpha = pMono[i] * 255 / grad;
            pTexBuf[i] = (alpha << 24) | 0x00ffffff;
        }
    }
    pTex->UnlockRect(0);  // アンロック
    delete[] pMono;
*/

    // 単位フォントポリゴン頂点作成
    Vtx vtx[4] = {
        //{0.0f, 1.0f, 1.0f,   0.0f, 1.0f},
        //{0.0f, 0.0f, 1.0f,   0.0f, 0.0f},
        //{1.0f, 1.0f, 1.0f,   1.0f, 1.0f},
        //{1.0f, 0.0f, 1.0f,   1.0f, 0.0f},
        {0.0f, -1.0f, 1.0f,   0.0f, 1.0f},
        {0.0f,  0.0f, 1.0f,   0.0f, 0.0f},
        {1.0f, -1.0f, 1.0f,   1.0f, 1.0f},
        {1.0f,  0.0f, 1.0f,   1.0f, 0.0f},
    };
    Vtx* p = 0;
    IDirect3DVertexBuffer9* pVertexBuffer = 0;
    g_pD3DDev->CreateVertexBuffer(sizeof(vtx), 0, 0, D3DPOOL_MANAGED, &pVertexBuffer, 0);
    pVertexBuffer->Lock(0, 0, (void**)&p, 0);
    memcpy(p, vtx, sizeof(vtx));
    pVertexBuffer->Unlock();
    g_pD3DDev->SetStreamSource(0, pVertexBuffer, 0, sizeof(Vtx));

/*
    // 各種行列
    XMMATRIX localScale=XMMatrixScaling( (float)gm.gmBlackBoxX, (float)gm.gmBlackBoxY, 1.0f);
    XMMATRIX localOffset=XMMatrixTranslation( (float)gm.gmptGlyphOrigin.x, (float)gm.gmptGlyphOrigin.y, 0.0f);
    XMMATRIX localMat = localScale * localOffset;
    int ox = -0, oy = -0;
    XMMATRIX world;
    XMMATRIX worldOffset=XMMatrixTranslation( (float)ox - 0.5f, (float)oy + 0.5f, 0.0f);
    world = localMat * worldOffset;
*/
    XMMATRIX ortho = XMMatrixOrthographicLH((float)screenW, (float)screenH, 0.0f, 1000.0f);
    //XMMATRIX ortho = XMMatrixOrthographicOffCenterLH( 0,(float)screenW, (float)screenH, 0, 0.0f, 1000.0f);
    g_pD3DDev->SetTransform(D3DTS_PROJECTION, &conv(ortho));
    
    const wchar_t c[] = L"ステキ";
    size_t num = wcslen(c);
    FONT* Font = new FONT[num];
    int size = 100;
    for (int i = 0; i < num; i++) {
        createFont(g_pD3DDev, &c[i], size, Font + i);
    }

    // メッセージ ループ
    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Direct3Dの処理
            g_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            g_pD3DDev->BeginScene();

            // ライトはオフで
            g_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);

            // αブレンド設定
            g_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            g_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            g_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

            // 描画
/*          g_pD3DDev->SetTransform(D3DTS_WORLD, &conv(world));
            g_pD3DDev->SetStreamSource(0, pVertexBuffer, 0, sizeof(Vtx));
            g_pD3DDev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
            g_pD3DDev->SetTexture(0, pTex);
            g_pD3DDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
*/
            // アライン情報描画
            //if (bShowTextMetrics)
                //drawTextMetrics(g_pD3DDev, tm, gm, ox, oy);
            int x = -300;
            for (int i = 0; i < num; i++) {
                drawFont(g_pD3DDev, (Font+i), x, 0);
                x += (Font+i)->gm.gmCellIncX;
            }

            g_pD3DDev->EndScene();
            g_pD3DDev->Present(NULL, NULL, NULL, NULL);
        }
    } while (msg.message != WM_QUIT);

    delete [] Font;
    //pTex->Release();
    pVertexBuffer->Release();
    g_pD3DDev->Release();
    g_pD3D->Release();

    return 0;
}

/*
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")

#include <windows.h>
#include <tchar.h>
#include <d3d9.h>
#include <directxmath.h>
#include <stdio.h>
using namespace DirectX;

TCHAR gName[100] = _T("改・高速フォント表示サンプルプログラム");
bool bShowTextMetrics = true;

D3DMATRIX conv(XMMATRIX& m) {
    D3DMATRIX d3dm = {
        m.r[0].m128_f32[0], m.r[0].m128_f32[1], m.r[0].m128_f32[2], m.r[0].m128_f32[3],
        m.r[1].m128_f32[0], m.r[1].m128_f32[1], m.r[1].m128_f32[2], m.r[1].m128_f32[3],
        m.r[2].m128_f32[0], m.r[2].m128_f32[1], m.r[2].m128_f32[2], m.r[2].m128_f32[3],
        m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2], m.r[3].m128_f32[3],
    };
    return d3dm;
}

struct Vtx {
    float x, y, z;
    float u, v;
};

struct LineVtx {
    float x, y, z;
    DWORD color;
};

void drawLineW(IDirect3DDevice9* dev, float sx, float sy, float sz, float ex, float ey, float ez, DWORD color) {
    LineVtx p[2] = {
            {sx, sy, sz, color},
            {ex, ey, ez, color}
    };
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_LINELIST, 1, &p, sizeof(LineVtx));
}

void drawRectW(IDirect3DDevice9* dev, float l, float t, float w, float h, DWORD color) {
    drawLineW(dev, l, t, 0.0f, l + w, t, 0.0f, color);
    drawLineW(dev, l + w, t, 0.0f, l + w, t - h, 0.0f, color);
    drawLineW(dev, l + w, t - h, 0.0f, l, t - h, 0.0f, color);
    drawLineW(dev, l, t - h, 0.0f, l, t, 0.0f, color);
}
void drawTextMetrics(IDirect3DDevice9* dev, TEXTMETRIC tm, GLYPHMETRICS gm, int ox, int oy) {
    XMMATRIX idn= XMMatrixIdentity();
    dev->SetTransform(D3DTS_WORLD, &conv(idn));
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Base Line
    D3DVIEWPORT9 vp;
    dev->GetViewport(&vp);
    drawLineW(dev, (float)vp.Width / -2.0f, (float)oy, 0.0f, (float)vp.Width / 2, (float)oy, 0.0f, 0xffff0000);

    // Ascent Line
    drawLineW(dev, (float)vp.Width / -2.0f, (float)(oy + tm.tmAscent), 0.0f, (float)vp.Width / 2, (float)(oy + tm.tmAscent), 0.0f, 0xffff0000);

    // Descent Line
    drawLineW(dev, (float)vp.Width / -2.0f, (float)(oy - tm.tmDescent), 0.0f, (float)vp.Width / 2, (float)(oy - tm.tmDescent), 0.0f, 0xffff0000);

    // Origin
    drawRectW(dev, (float)ox - 2.0f, (float)oy + 2.0f, 4.0f, 4.0f, 0xff00ff00);

    // Next Origin
    drawRectW(dev, (float)(ox + gm.gmCellIncX) - 2.0f, (float)oy + 2.0f, 4.0f, 4.0f, 0xffffff00);

    // BlackBox
    drawRectW(dev, (float)(ox + gm.gmptGlyphOrigin.x), (float)(oy + gm.gmptGlyphOrigin.y), (float)gm.gmBlackBoxX, (float)gm.gmBlackBoxY, 0x00ff0000ff);
};




LRESULT CALLBACK WndProc(HWND hWnd, UINT mes, WPARAM wParam, LPARAM lParam) {
    switch (mes) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if (wParam == 's')
            bShowTextMetrics = !bShowTextMetrics;
        break;
    }
    return DefWindowProc(hWnd, mes, wParam, lParam);
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    // アプリケーションの初期化
    MSG msg; HWND hWnd;
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, (TCHAR*)gName, NULL };
    if (!RegisterClassEx(&wcex))
        return 0;
    int screenW = 640, screenH = 480;
    RECT R = { 0, 0, screenW, screenH };
    ::AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
    if (!(hWnd = CreateWindow(gName, gName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, R.right - R.left, R.bottom - R.top, NULL, NULL, hInstance, NULL)))
        return 0;
    // Direct3Dの初期化
    LPDIRECT3D9 g_pD3D;
    LPDIRECT3DDEVICE9 g_pD3DDev;
    if (!(g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return 0;
    D3DPRESENT_PARAMETERS d3dpp = { screenW, screenH, D3DFMT_X8R8G8B8, 0, D3DMULTISAMPLE_NONE, 0, D3DSWAPEFFECT_DISCARD, NULL, TRUE, 0, D3DFMT_UNKNOWN, 0, 0 };
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pD3DDev))) {
        g_pD3D->Release();
        return 0;
    }
    ShowWindow(hWnd, SW_SHOW);

    // フォントの生成
    int fontSize = 240;
    int fontWeight = 500;
    //LOGFONT lf = { fontSize, 0, 0, 0, fontWeight, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("ＭＳ Ｐ明朝") };
    LOGFONT lf = {fontSize, 0, 0, 0, fontWeight, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, _T("ＭＳ Ｐゴシック")};
    HFONT hFont = CreateFontIndirect(&lf);
    if (hFont == NULL) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    // デバイスにフォントを持たせないとGetGlyphOutline関数はエラーとなる
    HDC hdc = GetDC(NULL);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    // フォントビットマップ取得
    const wchar_t c[] = L"楽";
    UINT code = (UINT)c[0];
    const int gradFlag = GGO_GRAY4_BITMAP; // GGO_GRAY2_BITMAP or GGO_GRAY4_BITMAP or GGO_GRAY8_BITMAP
    int grad = 0; // 階調の最大値
    switch (gradFlag) {
    case GGO_GRAY2_BITMAP: grad = 4; break;
    case GGO_GRAY4_BITMAP: grad = 16; break;
    case GGO_GRAY8_BITMAP: grad = 64; break;
    }
    if (grad == 0) { g_pD3DDev->Release(); g_pD3D->Release(); return 0; }
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    GLYPHMETRICS gm;
    CONST MAT2 mat = { {0,1},{0,0},{0,0},{0,1} };
    DWORD size = GetGlyphOutlineW(hdc, code, gradFlag, &gm, 0, NULL, &mat);
    BYTE* pMono = new BYTE[size];
    GetGlyphOutlineW(hdc, code, gradFlag, &gm, size, pMono, &mat);
    // デバイスコンテキストとフォントハンドルはもういらないので解放
    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);

    // テクスチャ作成
    IDirect3DTexture9* pTex = 0;
    int fontWidth = (gm.gmBlackBoxX + 3) / 4 * 4;
    int fontHeight = gm.gmBlackBoxY;
    g_pD3DDev->CreateTexture(fontWidth, fontHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL);
    // テクスチャにフォントビットマップ情報を書き込み
    D3DLOCKED_RECT lockedRect;
    pTex->LockRect(0, &lockedRect, NULL, 0);  // ロック
    DWORD* pTexBuf = (DWORD*)lockedRect.pBits;   // テクスチャメモリへのポインタ
    for (int y = 0; y < fontHeight; y++) {
        for (int x = 0; x < fontWidth; x++) {
            DWORD alpha = pMono[y * fontWidth + x] * 255 / grad;
            pTexBuf[y * fontWidth + x] = (alpha << 24) | 0x00ffffff;
        }
    }
    pTex->UnlockRect(0);  // アンロック
    delete[] pMono;


    // 単位フォントポリゴン頂点作成
    Vtx vtx[4] = {
            {0.0f, -1.0f, 1.0f,   0.0f, 1.0f},
            {0.0f,  0.0f, 1.0f,   0.0f, 0.0f},
            {1.0f, -1.0f, 1.0f,   1.0f, 1.0f},
            {1.0f,  0.0f, 1.0f,   1.0f, 0.0f},
    };
    Vtx* p = 0;
    IDirect3DVertexBuffer9* pVertexBuffer = 0;
    g_pD3DDev->CreateVertexBuffer(sizeof(vtx), 0, 0, D3DPOOL_MANAGED, &pVertexBuffer, 0);
    pVertexBuffer->Lock(0, 0, (void**)&p, 0);
    memcpy(p, vtx, sizeof(vtx));
    pVertexBuffer->Unlock();

    // 各種行列
    XMMATRIX localScale=XMMatrixScaling( (float)fontWidth, (float)fontHeight, 1.0f);
    XMMATRIX localOffset=XMMatrixTranslation( (float)gm.gmptGlyphOrigin.x, (float)gm.gmptGlyphOrigin.y, 0.0f);
    XMMATRIX localMat = localScale * localOffset;
    int ox = -0, oy = -0;
    XMMATRIX world;
    XMMATRIX worldOffset=XMMatrixTranslation( (float)ox - 0.5f, (float)oy + 0.5f, 0.0f);
    world = localMat * worldOffset;

    XMMATRIX ortho = XMMatrixOrthographicLH( (float)screenW, (float)screenH, 0.0f, 1000.0f);
    g_pD3DDev->SetTransform(D3DTS_PROJECTION, &conv(ortho));

    // メッセージ ループ
    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Direct3Dの処理
            g_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            g_pD3DDev->BeginScene();

            // ライトはオフで
            g_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);

            // αブレンド設定
            g_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            g_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            g_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

            // 描画
            g_pD3DDev->SetTransform(D3DTS_WORLD, &conv(world));
            g_pD3DDev->SetStreamSource(0, pVertexBuffer, 0, sizeof(Vtx));
            g_pD3DDev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
            g_pD3DDev->SetTexture(0, pTex);
            g_pD3DDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

            // アライン情報描画
            if (bShowTextMetrics)
                drawTextMetrics(g_pD3DDev, tm, gm, ox, oy);

            g_pD3DDev->EndScene();
            g_pD3DDev->Present(NULL, NULL, NULL, NULL);
        }
    } while (msg.message != WM_QUIT);

    pTex->Release();
    pVertexBuffer->Release();
    g_pD3DDev->Release();
    g_pD3D->Release();

    return 0;
}
*/