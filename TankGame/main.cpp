#include <windows.h>
#include <d2d1.h>
#include<dwrite.h>
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#include <wincodec.h>
#pragma comment(lib, "WindowsCodecs")
#include <vector>
#include <algorithm>
#include "DirectSoundHelper.h"
#include <string>

#include <sstream>

float tankX = 0;
float tankY = 0;

int tankDirection = 0; // �⺻������ ���� ������ ��Ÿ��

int numRectangles = 12;//�簢������


int targetNumber = 0; // �¾��� ���� Ƚ��
float sunX = 0.f;//���ʻ��
float sunY = 0.f;
float sunX2 = 0.f;//�����ϴ�
float sunY2 = 0.f;

bool endGame = false;



struct Bullet
{
	float x;
	float y;
	int direction;
};




// �ڿ� ���� ��ȯ ��ũ��.
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// ��Ʈ���� �ε��ϴ� �Լ� ����
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);




class DemoApp
{
public:
	DemoApp() {};
	~DemoApp() { DiscardDeviceResource(); DiscardAppResource(); }; // �Ҹ���. ��� �ڿ��� �ݳ���.
	bool Initialize(HINSTANCE hInstance); // ������ ����, CreateAppResource ȣ�� ����.
	void Render(); // ������ �׸���, CreateDevice ȣ�� ����.
	
	// ������ ó������ �ǵ��ư�
	bool Update(float timeDelta);
	//����ȭ��
	void RenderStartScreen(); // ���� ȭ�� �׸���

	//�Ѿ˺κ�
	std::vector<Bullet> bullets;
	void FireBullet();
	void RenderBullets(D2D1_SIZE_F rtSize);

	//�簢�� �浹
	bool isCheckRectangle(float tankX, float tankY, D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3);
	
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //������ ���ν���
	void OnResize(); //â �������� �ݹ� �Լ�
	bool CreateAppResource(); //�� �ڿ� ����
	void DiscardAppResource(); //�� �ڿ� ���
	bool CreateDeviceResource(); //��ġ �ڿ� ����
	void DiscardDeviceResource(); //��ġ �ڿ� ���

private:
	//Ŭ������ �������� ����
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;
	ID2D1SolidColorBrush* pCornflowerBlueBrush = NULL;
	IDWriteFactory* pDWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
	// ��
	ID2D1SolidColorBrush* pBlack = NULL;
	ID2D1SolidColorBrush* pWhite = NULL;
	ID2D1SolidColorBrush* pGreen = NULL;
	ID2D1SolidColorBrush* pRed = NULL;
	ID2D1SolidColorBrush* pYellow = NULL;

	//����ȭ��
	bool startScreen = true;

	// WIC
	IWICImagingFactory* pWICFactory = NULL;
	//��Ʈ��
	ID2D1Bitmap* pBitmapFromFile = NULL;
	ID2D1Bitmap* pBitmapFromResource = NULL;

	// DirectSound�� �Ҹ��� �����.
	CSoundManager* soundManager = NULL;

	//tank ������ ����ũ
	ID2D1Bitmap* pOrigBitmap=NULL;
	ID2D1Bitmap* pMaskBitmap=NULL;
	ID2D1Bitmap* pSpaceBitmap = NULL;
	ID2D1BitmapBrush* pOrigBitmapBrush=NULL;

	//��α���
	// ����
	ID2D1PathGeometry* pSunGeometry = NULL;
	ID2D1RadialGradientBrush* pRadialGradientBrush = NULL;

	// ����
	ID2D1PathGeometry* pPathGeometry = NULL;
	ID2D1PathGeometry* pObjectGeometry = NULL;


	
	float alphas[30]; // �� �ﰢ���� �ִϸ��̼� ���¸� �����ϴ� �迭

	// �ִϸ��̼� ����
	float timeDuration; // �ִϸ��̼� ���� �ð�. �и�� ���ǹǷ� 0�� �Ǹ� �ȵ�.
	float alpha; // �ִϸ��̼� ������ ���� ��ġ��, [0,1] ������ ��.
	float startPos;
	float endPos;

};


// ���� ���α׷��� ������ �Լ�.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DemoApp demoApp;
	if (!demoApp.Initialize(hInstance)) return 1;

	// Time ����
	LARGE_INTEGER nPrevTime;
	LARGE_INTEGER nFrequency;
	// Time �ʱ�ȭ
	QueryPerformanceFrequency(&nFrequency);
	QueryPerformanceCounter(&nPrevTime);

	// ���� ������ �޽��� ������ ������.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Time ����
			LARGE_INTEGER CurrentTime;
			QueryPerformanceCounter(&CurrentTime);
			float timeDelta = (float)((double)(CurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = CurrentTime;


			// ���� �Լ��� ȣ���Ѵ�. escŰ ������ false�� ���ϵǾ� �����Ѵ�.
			if (!demoApp.Update(timeDelta)) break;
			demoApp.Render();
		}
	}

	return 0;
}
// ���� ���α׷��� �����츦 �����ϰ�, ��ġ ������ �ڿ��� ������.
bool DemoApp::Initialize(HINSTANCE hInstance)
{
	// ������ Ŭ������ �����.
	WNDCLASS wc = {};
	wc.lpfnWndProc = DemoApp::WndProc;
	wc.cbWndExtra = sizeof(LONG_PTR);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DemoApp";
	RegisterClass(&wc);

	// �����츦 ������.
	hWnd = CreateWindow(L"DemoApp", L"DemoApp", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, this);
	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// ��ġ ������ �ڿ��� ������.
	if (!CreateAppResource()) return false;
	

	timeDuration = 70.0f; // �ִϸ��̼� �ð� ����
	for (int i = 0; i < numRectangles; ++i)
	{
		alphas[i] = 0.0f;
	}


	//�ʱ� ��ũ ��ġ ����
	RECT rc;
	GetClientRect(hWnd, &rc);
	tankX = (rc.right / 2)-15;
	tankY = rc.bottom - 30;
	return true;
}

LRESULT CALLBACK DemoApp::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		DemoApp* pDemoApp = (DemoApp*)pCreate->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pDemoApp);
		return 1;
	}

	DemoApp* pDemoApp = (DemoApp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!pDemoApp)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_SIZE:
	{
		pDemoApp->OnResize();
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 1;
	}
	case WM_KEYDOWN:
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		if (!pDemoApp->startScreen) {
			switch (wParam) {
			case VK_DOWN:
				if (rc.bottom < tankY+40)
					tankY = tankY;
				else
					tankY += 20;
				tankDirection = 2; // �Ʒ��� ������ ��Ÿ��
				break;
			case VK_UP:
				if (rc.top > tankY-10)
					tankY = tankY;
				else
					tankY -= 20;
				tankDirection = 0; // ���� ������ ��Ÿ��
				break;
			case VK_RIGHT:
				if (rc.right < tankX+40)
					tankX = tankX;
				else
					tankX += 20;
				tankDirection = 1; // ������ ������ ��Ÿ��
				break;
			case VK_LEFT:
				if (rc.left > tankX-10)
					tankX = tankX;
				else
					tankX -= 20;
				tankDirection = 3; // ���� ������ ��Ÿ��
				break;
				//����ȭ�� �κ�
			}
			InvalidateRect(hWnd, NULL, TRUE); // ȭ���� �ٽ� �׸����� ��û
		}

		//����ȭ�� �κ�
		if (wParam == 'A') // 'A' Ű�� ������
		{
			if(pDemoApp->startScreen==false){
				pDemoApp->FireBullet(); // �Ѿ� �߻�
			}
			pDemoApp->startScreen = false;
			InvalidateRect(hWnd, NULL, TRUE); // ȭ���� �ٽ� �׸����� ��û
		}
		//����ȭ�� �κ�
		if (wParam == 'R') // 'R' Ű�� ������
		{
			if (endGame) {
				pDemoApp->startScreen = true;
				targetNumber = 0;
				//�ʱ� ��ũ ��ġ ����
				RECT rc;
				GetClientRect(hWnd, &rc);
				tankX = (rc.right / 2) - 15;
				tankY = rc.bottom - 30;

				InvalidateRect(hWnd, NULL, TRUE); // ȭ���� �ٽ� �׸����� ��û
			}

		}
		return 0;
	}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// â �������� �ݹ� �Լ�
void DemoApp::OnResize()
{
	if (!pRenderTarget) return;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	// ����Ÿ���� ũ�⸦ �ٽ� ������.
	pRenderTarget->Resize(size);

}
// ��ġ ������ �ڿ����� ������. �̵� �ڿ��� ������ ���� ���α׷��� ����Ǳ� ������ ��ȿ��.

bool DemoApp::CreateAppResource()
{
	
	//��Ʈ�ʺκ�
	// CoCreateInstance �Լ��� ���� ������ CoInitialize�� ȣ�����־�� ��.
	CoInitialize(NULL); // CreateAppResource �Լ��� ó������ ȣ��.
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	// CoCreateInstance �Լ��� ����Ͽ� WIC ���丮�� ������.
	HRESULT br = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
	if (FAILED(br)) return false;



	// D2D ���丮�� ������.
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) return false;

	// DirectWrite ���丮�� ������.
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	if (FAILED(hr)) return false;

	//DirectWrite �ؽ�Ʈ ���� ��ü�� ������.
	hr = pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 50, L"", &pTextFormat);
	if (FAILED(hr)) return false;

	// �ؽ�Ʈ�� �������� �߾� �����ϰ� �������ε� �߾� ������.
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


	//���� �κ�
	soundManager = new CSoundManager;
	hr = soundManager->init(hWnd); // ��ȿ�� hWnd�� Ȯ���� �ڿ� ȣ��Ǿ�� ��.
	if (FAILED(hr)) return false;

	// ���� ������ �߰���.
	int id;  // ����Ŭ�� ��Ϲ�ȣ, id=0���� ������.
	hr = soundManager->add(const_cast<LPTSTR>(L"startBgSound.wav"), &id); //id=0
	if (FAILED(hr)) return false;

	hr = soundManager->add(const_cast<LPTSTR>(L"bullet.wav"), &id); //id=1
	if (FAILED(hr)) return false;

	hr = soundManager->add(const_cast<LPTSTR>(L"mainBgSound.wav"), &id); //id=2
	if (FAILED(hr)) return false;

	hr = soundManager->add(const_cast<LPTSTR>(L"victory.wav"), &id); //id=3
	if (FAILED(hr)) return false;

	//soundManager->play(0, FALSE); //play id=3
	//soundManager->play(1,TRUE);


	



	//����
	pD2DFactory->CreatePathGeometry(&pPathGeometry);
	ID2D1GeometrySink* aSink = NULL;
	pPathGeometry->Open(&aSink);

	D2D1_POINT_2F currentLocation = { 0, 0 };

	aSink->BeginFigure(currentLocation, D2D1_FIGURE_BEGIN_FILLED);

	D2D1_POINT_2F locDelta = { 2, 2 };
	float radius = 3;

	for (UINT i = 0; i < 80; ++i)
	{
		currentLocation.x += radius * locDelta.x;
		currentLocation.y += radius * locDelta.y;

		aSink->AddArc(
			D2D1::ArcSegment(
				currentLocation,
				D2D1::SizeF(2 * radius, 2 * radius), // radiusx/y
				0.0f, // rotation angle
				D2D1_SWEEP_DIRECTION_CLOCKWISE,
				D2D1_ARC_SIZE_SMALL
			)
		);

		locDelta = D2D1::Point2F(-locDelta.y, locDelta.x);

		radius += 3;
	}

	aSink->EndFigure(D2D1_FIGURE_END_OPEN);
	aSink->Close();
	SAFE_RELEASE(aSink);



	// �ִϸ��̼� �ʱ�ȭ
	timeDuration = 3.0f;
	alpha = 0;

	float length = 0;
	startPos = length * 0.5;
	pPathGeometry->ComputeLength(NULL, &length);
	endPos = length * 0.8;



	return true;
}

// �� �ڿ� ���
void DemoApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pWICFactory);
	SAFE_DELETE(soundManager);

	//��Ʈ��
	// CoInitialize �Լ��� ȣ��� ������ �������� �� �Լ��� ȣ���� �־�� ��.
	CoUninitialize(); // DiscardAppResource �Լ��� ���������� ȣ��.

	//��α���
	SAFE_RELEASE(pSunGeometry);
	//����
	SAFE_RELEASE(pPathGeometry);
	SAFE_RELEASE(pObjectGeometry);
}

// ��ġ ������ �ڿ����� ������. ��ġ�� �ҽǵǴ� ��쿡�� �̵� �ڿ��� �ٽ� �����ؾ� ��.
bool DemoApp::CreateDeviceResource()
{
	// ����Ÿ���� ��ȿ�ϴٸ� �� �Լ��� ������ �ʿ䰡 ����
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

	// D2D ����Ÿ���� ������.
	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
	if (FAILED(hr)) return false;



	// ������ ���� ������.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlack);
	if (FAILED(hr)) return false;
	// �Ͼ�� ���� ������.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);
	if (FAILED(hr)) return false;
	//���� ��
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pRed);
	// ��� ��
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &pYellow);
	// �ʷ� ��
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow), &pGreen);

	//�˻� �� üũ

	HRESULT br = S_OK;
	// �ܺ� ���Ϸκ��� ��Ʈ�� ��ü pAnotherBitmap�� ������.
	br = LoadBitmapFromFile(pRenderTarget, pWICFactory, L".\\startBG.jpg", 0, 0, &pBitmapFromFile);
	if (FAILED(br)) return false;

	// ���� ���α׷� ���ҽ��κ��� ��Ʈ�� ��ü pBitmap�� ������.
	br = LoadBitmapFromResource(pRenderTarget, pWICFactory, L"startBG", L"Image", 0, 150, &pBitmapFromResource);
	if (FAILED(br)) return false;


	//�ܵ� ��Ʈ�� ����
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"space", L"Image", 0, 0, &pSpaceBitmap);


	// ��Ʈ���� ������.
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"tankSkin", L"Image", 0, 0, &pOrigBitmap);
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"tankMask", L"Image", 0, 0, &pMaskBitmap);
	// ��Ʈ�� ���� ������.
	D2D1_BITMAP_BRUSH_PROPERTIES propertiesXClampYClamp = D2D1::BitmapBrushProperties(
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
	);
	br = pRenderTarget->CreateBitmapBrush(pOrigBitmap, propertiesXClampYClamp, &pOrigBitmapBrush);
	if (FAILED(br) || pOrigBitmapBrush == NULL) {
		OutputDebugString(L"Failed to create bitmap brush\n");
		return false;
	}


	// ����� ���� ���� ������.
	// D2D1_GRADIENT_STOP ����ü �迭�� ������.
	ID2D1GradientStopCollection* pGradientStops = NULL;

	D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Gold, 1);
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Orange, 0.8f);
	gradientStops[1].position = 0.85f;
	gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::OrangeRed, 0.7f);
	gradientStops[2].position = 1.0f;

	// D2D1_GRADIENT_STOP ����ü �迭�κ��� ID2D1GradientStopCollection�� ������.
	pRenderTarget->CreateGradientStopCollection(gradientStops, 3, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);

	// ID2D1GradientStopCollection�κ��� ����� ���� ���� ������.
	pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(330, 330), D2D1::Point2F(140, 140), 140, 140), pGradientStops, &pRadialGradientBrush);




	return true;
}

// ��ġ ������ �ڿ����� �ݳ���. ��ġ�� �ҽǵǸ� �̵� �ڿ��� �ٽ� �����ؾ� ��.
void DemoApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pBlack);
	SAFE_RELEASE(pWhite);
	SAFE_RELEASE(pRed);
	SAFE_RELEASE(pYellow);
	SAFE_RELEASE(pGreen);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);

	//��Ʈ��
	SAFE_RELEASE(pBitmapFromFile);
	SAFE_RELEASE(pBitmapFromResource);
	SAFE_RELEASE(pOrigBitmap);
	SAFE_RELEASE(pMaskBitmap);
	SAFE_RELEASE(pSpaceBitmap);

	//��α���
	SAFE_RELEASE(pRadialGradientBrush);
	
}

float GetPosInterp(float t, float s, float e)
{
	float u;

	// linear
	u = t;

	// easein cubic
	//u = t * t * t;

	// easeout cubic
	//u = (float)(1 - pow(1 - t, 3));

	// easeinout cubic
	//u = (t < 0.5) ? (4 * t * t * t) : (float)(1 - pow(-2 * t + 2, 3) / 2);

	return (s + ((e - s) * u));
}


bool DemoApp::Update(float timeDelta)
{
	// ���� ���α׷��� �����ϴ� ��쿡 false�� �����ϰ� �� �ܿ��� �׻� true�� ������.

	// �Է� ���¸� ������.


	static const float offsetTime = timeDuration / numRectangles;
	static float timeElapsed = 0;
	timeElapsed += timeDelta;

	if (timeElapsed >= timeDuration) timeElapsed = 0.0f;

	for (int i = 0; i < numRectangles; ++i)
	{
		float currentElapsed = timeElapsed - i * offsetTime;
		if (currentElapsed < 0) currentElapsed += timeDuration;
		alphas[i] = currentElapsed / timeDuration;
	}
	//����ȭ���϶� �Ҹ� �ݺ�
	if (startScreen) {
		soundManager->stop(0);
		soundManager->stop(3);
		soundManager->play(0, TRUE);
	}
	else {
		soundManager->stop(0);
		if (endGame == true) {
			soundManager->stop(2);
			soundManager->play(3, TRUE);
		}
		else {
			soundManager->stop(2);
			soundManager->play(2, TRUE);
		}
	}

	//a�� ������ �� ȿ���� �ѹ���
	if (GetAsyncKeyState('A') & 0x8000f) {
		soundManager->play(1, FALSE);
	}


	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000f) return false; //���� ���α׷��� ������

	return true;
}




// �׸� ������ �׸���.
void DemoApp::Render()
{
	if (!CreateDeviceResource()) return;


	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// ����Ÿ���� ũ�⸦ ����.
	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	

	if (startScreen){	
		RenderStartScreen();
	}
	else
	{

		//���
		D2D1_POINT_2F base = D2D1::Point2F(0.f, 0.f);
		pRenderTarget->DrawBitmap(pSpaceBitmap, D2D1::RectF(base.x, base.y, base.x + rtSize.width, base.y + rtSize.height));



		//����
		if(targetNumber>0){
			D2D1_RECT_F bar = D2D1::RectF(5, 10, rtSize.width-5, 40);

			// ���� �ܺθ� �ʷϻ����� ä��
			D2D1_RECT_F barFill = D2D1::RectF(5, 10, rtSize.width - 5, 40);
			pRenderTarget->FillRectangle(barFill, pGreen);

			if (targetNumber < 31) {
				// ���� ���θ� ���������� ä��
				D2D1_RECT_F barChange = D2D1::RectF(5, 10, (rtSize.width - 5) * (targetNumber / 30.0f), 40);
				pRenderTarget->FillRectangle(barChange, pRed);
			}
			else {
				D2D1_RECT_F barFill = D2D1::RectF(5, 10, rtSize.width - 5, 40);
				pRenderTarget->FillRectangle(barFill, pRed);
			}

			pRenderTarget->DrawRectangle(bar, pBlack);
		}
		

		// �¾� ��α��� �����
		ID2D1GeometrySink* pSink = NULL;
		RECT rc;
		GetClientRect(hWnd, &rc);
		float centerX = (rc.right - rc.left) / 2;
		float centerY = (rc.bottom - rc.top) / 2;

		pD2DFactory->CreatePathGeometry(&pSunGeometry);
		pSunGeometry->Open(&pSink);
		pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

		pSink->BeginFigure(D2D1::Point2F(centerX - 55, centerY + 60), D2D1_FIGURE_BEGIN_FILLED);
		pSink->AddArc(D2D1::ArcSegment(D2D1::Point2F(centerX + 55, centerY + 60), D2D1::SizeF(85, 85), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_LARGE));
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);

		if (targetNumber < 10) {
			// �� ����
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 30, centerY - 20), D2D1::Point2F(centerX - 20, centerY - 30), D2D1::Point2F(centerX - 10, centerY - 20)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX + 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX + 30, centerY - 20), D2D1::Point2F(centerX + 20, centerY - 30), D2D1::Point2F(centerX + 10, centerY - 20)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			// ������
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 30), D2D1::Point2F(centerX, centerY + 40), D2D1::Point2F(centerX + 20, centerY + 30)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else if (targetNumber < 20) {
			//��ǥ�� - -  ǥ�� 
			//        -
			// ���� ��
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 20));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// ������ ��
			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 20));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// ��
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 20, centerY + 30));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else if(targetNumber < 30) {
			// ȭ�� ǥ��
			// ���� ��
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 25), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 15));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);


			// ������ ��

			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 15), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 25));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			// ��
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 40), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 40), D2D1::Point2F(centerX, centerY + 20), D2D1::Point2F(centerX + 20, centerY + 40)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else {
			//���ʴ�
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX - 10, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 30, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// ������ ��
			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX + 30, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 10, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// ��
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 30), D2D1::Point2F(centerX, centerY + 20), D2D1::Point2F(centerX + 20, centerY + 30)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);







			endGame = true;

			// ������ �׸��� ���� �غ�
			pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			IDWriteTextFormat* pTextFormatEnd = nullptr;
			pDWriteFactory->CreateTextFormat(
				L"Arial",                   // Font family name
				NULL,                       // Font collection (NULL sets it to use the system font collection)
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				20.0f,                      // Font size
				L"en-us",
				&pTextFormatEnd
			);

			std::wstring endText = L"�¸�!! R�� ������ �ʱ�ȭ������ ���ư��ϴ�!";

			// �ؽ�Ʈ ������
			D2D1_RECT_F textRect = D2D1::RectF(rtSize.width/2-200, 50, rtSize.width - 10, rtSize.height - 10);
			pRenderTarget->DrawText(
				endText.c_str(),
				endText.length(),
				pTextFormatEnd,
				textRect,
				pWhite
			);

			// ���ҽ� ����
			pTextFormatEnd->Release();
		}

		
		pSink->Close();
		SAFE_RELEASE(pSink);



	
		

		

		pRenderTarget->FillGeometry(pSunGeometry, pRadialGradientBrush);
		pRenderTarget->DrawGeometry(pSunGeometry, pBlack, 1.f);


		sunX = centerX - 70;
		sunY = centerY - 80;
		sunX2 = centerX + 70;
		sunY2 = centerY + 60;
		//// ������ ä�� �簢�� ����� ������.
		//D2D1_RECT_F sunbg = D2D1::RectF(centerX-70, centerY-80, centerX+70, centerY + 60);
		//pRenderTarget->DrawRectangle(sunbg, pWhite);



		// ������ ä�� �簢�� ����� ������.
		D2D1_RECT_F rcBrushRect = D2D1::RectF(0, 0, 30, 30);

		// �߽����� �������� ȸ�� �� ��ȯ ����
		D2D1_POINT_2F center = D2D1::Point2F(15, 15); // �̹����� �߽�
		D2D1_MATRIX_3X2_F transform;

		switch (tankDirection)
		{
		case 0: // ����
			transform = D2D1::Matrix3x2F::Rotation(90, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 1: // ������
			transform = D2D1::Matrix3x2F::Rotation(180, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 2: // �Ʒ���
			transform = D2D1::Matrix3x2F::Rotation(270, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 3: // ����
			transform = D2D1::Matrix3x2F::Rotation(0, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		}

		pRenderTarget->SetTransform(transform);

		// FillOpacityMask �Լ��� �ùٸ��� �����Ϸ��� ������ D2D1_ANTIALIAS_MODE_ALIASED�� �����ؾ� ��.
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		pRenderTarget->FillOpacityMask(pMaskBitmap, pOrigBitmapBrush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, &rcBrushRect);
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		



		//// �ؽ�Ʈ�� �׸��� ���� �غ�
		//pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());


		//IDWriteTextFormat* pTextFormat = nullptr;
		//pDWriteFactory->CreateTextFormat(
		//	L"Arial",                   // Font family name
		//	NULL,                       // Font collection (NULL sets it to use the system font collection)
		//	DWRITE_FONT_WEIGHT_REGULAR,
		//	DWRITE_FONT_STYLE_NORMAL,
		//	DWRITE_FONT_STRETCH_NORMAL,
		//	12.0f,                      // Font size
		//	L"en-us",
		//	&pTextFormat
		//);



		//// ��ũ �߽� ��ǥ �ؽ�Ʈ ����
		//std::wstring tankPosText = L"Tank Position: (" + std::to_wstring(tankX + center.x) + L", " + std::to_wstring(tankY + center.y) + L")";

		//// �ؽ�Ʈ ������
		//D2D1_RECT_F textRect = D2D1::RectF(10, 10, rtSize.width - 10, rtSize.height - 10);
		//pRenderTarget->DrawText(
		//	tankPosText.c_str(),
		//	tankPosText.length(),
		//	pTextFormat,
		//	textRect,
		//	pWhite
		//);

		//// ���ҽ� ����
		//pTextFormat->Release();


		// �̵� ���� �׸���

		// �̵� ���� ���� ��ΰ� ȭ�� �߽ɿ� �׷������� ��.
		D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
		float minWidthHeightScale = min(rtSize.width, rtSize.height) / 512;
		D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(minWidthHeightScale, minWidthHeightScale);
		D2D1::Matrix3x2F translation = D2D1::Matrix3x2F::Translation(rtSize.width / 2, rtSize.height / 2);
		pRenderTarget->SetTransform(scale * translation);

		//pRenderTarget->DrawGeometry(pPathGeometry, pRed); // �̵� ������ ���������� �׸�.

		if (targetNumber < 30) {
			// �簢���� ���� ����
			if (targetNumber < 10) {
				numRectangles = 4;
			}
			else if (targetNumber < 20) {
				numRectangles = 8;
			}
			else if (targetNumber < 30) {
				numRectangles = 12;
			}

			for (int i = 0; i < numRectangles; ++i) {
				float length = GetPosInterp(alphas[i], startPos, endPos); // �ִϸ��̼��� ���� ���� ��ġ ���
				D2D1_POINT_2F point;
				D2D1_POINT_2F tangent;
				pPathGeometry->ComputePointAtLength(length, NULL, &point, &tangent);

				D2D1_MATRIX_3X2_F rectangleMatrix;
				rectangleMatrix = D2D1::Matrix3x2F(
					tangent.x, tangent.y,
					-tangent.y, tangent.x,
					point.x, point.y);
				pRenderTarget->SetTransform(rectangleMatrix * scale * translation);

				// �簢���� ��ǥ ����
				D2D1_RECT_F rect = D2D1::RectF(-10.0f, -10.0f, 10.0f, 10.0f);

				// �簢���� ��������� �׸�
				pRenderTarget->FillRectangle(rect, pYellow);

				// ��ȯ�� �簢���� ��ǥ ���
				D2D1_POINT_2F originalPoints[4] = {
					D2D1::Point2F(-10.f, -10.f),
					D2D1::Point2F(-10.f, 10.f),
					D2D1::Point2F(10.f, 10.f),
					D2D1::Point2F(10.f, -10.f)
				};

				D2D1_POINT_2F transformedPoints[4];
				for (int j = 0; j < 4; ++j) {
					transformedPoints[j].x = rectangleMatrix._11 * originalPoints[j].x + rectangleMatrix._21 * originalPoints[j].y + rectangleMatrix._31;
					transformedPoints[j].y = rectangleMatrix._12 * originalPoints[j].x + rectangleMatrix._22 * originalPoints[j].y + rectangleMatrix._32;
				}

				// �浹 ����
				if (isCheckRectangle(tankX, tankY, transformedPoints[0], transformedPoints[1], transformedPoints[2], transformedPoints[3])) {
					startScreen = true;
					targetNumber = 0;
					// �ʱ� ��ũ ��ġ ����
					RECT rc;
					GetClientRect(hWnd, &rc);
					tankX = (rc.right / 2) - 15;
					tankY = rc.bottom - 30;
					soundManager->stop(2);
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}
			}
		}


		if (tankX+15 > sunX && tankY+15 > sunY && tankX < sunX2 && tankY < sunY2) {
			startScreen = true;
			targetNumber = 0;
			//�ʱ� ��ũ ��ġ ����
			RECT rc;
			GetClientRect(hWnd, &rc);
			tankX = (rc.right / 2) - 15;
			tankY = rc.bottom - 30;
			soundManager->stop(2);
			InvalidateRect(hWnd, NULL, TRUE);
		}

		//�Ѿ� ������
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		RenderBullets(rtSize);


	}

	HRESULT hr = pRenderTarget->EndDraw();
	// �� �Լ��� ����Ǵ� ���ȿ� ��ġ�� �ҽǵǸ� ��ġ �ڿ����� �ݳ���.
	// ������ Render ȣ�� �ÿ� ���ο��� ȣ��Ǵ� CreateDeviceResource ���� �ݳ��� ��ġ �ڿ����� �ٽ� ������.
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}



// �浹 ���� �Լ� ����
bool DemoApp::isCheckRectangle(float px, float py, D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) {
	// ��ũ �߽� ��ǥ
	D2D1_POINT_2F tankCenter = D2D1::Point2F(px + 15, py + 15);

	// ���� ���� ���� �̿��� �浹 ����
	auto crossProduct = [](D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) {
		return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
	};

	bool b1 = crossProduct(p0, p1, tankCenter) >= 0.0f;
	bool b2 = crossProduct(p1, p2, tankCenter) >= 0.0f;
	bool b3 = crossProduct(p2, p3, tankCenter) >= 0.0f;
	bool b4 = crossProduct(p3, p0, tankCenter) >= 0.0f;

	return (b1 == b2) && (b2 == b3) && (b3 == b4);
}




// �Ѿ� �߻� �Լ�
void DemoApp::FireBullet()
{
	float bulletX = tankX;
	float bulletY = tankY;

	// �� ���⿡ ���� �Ѿ��� �ʱ� ��ġ�� ����
	// ��ũ ���� ������ ����
	switch (tankDirection)
	{
	case 0: // ����
		bulletX += 15.f;
		break;
	case 1: // ������
		bulletX += 30.f; 
		bulletY += 15.f;
		break;
	case 2: // �Ʒ���
		bulletY += 30.f;
		bulletX += 15.f;
		break;
	case 3: // ����
		bulletY += 15.f;
		break;
	}
	
	Bullet bullet = { bulletX, bulletY, tankDirection };
	bullets.push_back(bullet);

}

// �Ѿ� ������ �Լ�
void DemoApp::RenderBullets(D2D1_SIZE_F rtSize)
{
	for (auto it = bullets.begin(); it != bullets.end(); )
	{
		switch (it->direction)
		{
		case 0: // ����
			it->y -= 3;
			if (it->y < 0) {
				it = bullets.erase(it); // ȭ�� ������ ������ ����
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // �¾� ������ ����
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // ���� �Ѿ˷� �̵�
			}
			break;
		case 1: // ������
			it->x += 3;
			if (it->x > rtSize.width) {
				it = bullets.erase(it); // ȭ�� ������ ������ ����
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // �¾� ������ ����
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // ���� �Ѿ˷� �̵�
			}
			break;
		case 2: // �Ʒ���
			it->y += 3;
			if (it->y > rtSize.height) {
				it = bullets.erase(it); // ȭ�� ������ ������ ����
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // �¾� ������ ����
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // ���� �Ѿ˷� �̵�
			}
			break;
		case 3: // ����
			it->x -= 3;
			if (it->x < 0) {
				it = bullets.erase(it); // ȭ�� ������ ������ ����
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // �¾� ������ ����
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // ���� �Ѿ˷� �̵�
			}
			break;
		}
	}
}










// ���� ȭ���� �׸���.
void DemoApp::RenderStartScreen()
{
	endGame = false;// ���� �ٽ� false�� ����
	
	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	// ��Ʈ�� �׸���
	D2D1_POINT_2F base = D2D1::Point2F(0.f, 0.f);
	pRenderTarget->DrawBitmap(pBitmapFromResource, D2D1::RectF(base.x, base.y, base.x + rtSize.width, base.y + rtSize.height));

	const WCHAR titleText[] = L"TankGame";
	const WCHAR instructionsText[] = L"��aŰ ������ ����!\n�̵�Ű : ��,��,��,�� \n �Ѿ� �߻� : a";

	// Ÿ��Ʋ �ؽ�Ʈ �׸���
	pRenderTarget->DrawText(
		titleText,
		ARRAYSIZE(titleText) - 1,
		pTextFormat,
		D2D1::RectF(0, 0, rtSize.width, rtSize.height),
		pWhite
	);

	// ��ɾ� �ؽ�Ʈ ���� ����
	IDWriteTextFormat* pCommandFormat = NULL;
	pDWriteFactory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20,
		L"",
		&pCommandFormat
	);

	pCommandFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	pCommandFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	// ��ɾ� �ؽ�Ʈ �׸���
	pRenderTarget->DrawText(
		instructionsText,
		ARRAYSIZE(instructionsText) - 1,
		pCommandFormat,
		D2D1::RectF(0, rtSize.height - 80, rtSize.width - 10, rtSize.height),
		pWhite
	);

	SAFE_RELEASE(pCommandFormat);
}





// !{! LoadBitmapFromFile �Լ� ���� �ڵ�. ���� �ڵ��� ��ġ�� LoadBitmap/main.cpp.
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// �� �Լ��� ���Ϸκ��� �̹����� �ε��Ͽ� D2D ��Ʈ�� ID2D1Bitmap ��ü�� ������.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// ù ��° �������� ����.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// �̹��� ������ 32bppPBGRA�� ��ȯ�� (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// �ʺ�� ���� �߿� �ϳ� �̻��� ��õǾ� ������, IWICBitmapScaler�� �����ϰ� �̸� ����ؼ� �̹����� ����������.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // ���̰� ��õ� ������ ��������, �ʺ�� ������ŭ �ڵ����� ����.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // �ʺ� ��õ� ������ ��������, ���̴� ������ŭ �ڵ����� ����.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
			destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
		}

		hr = pIWICFactory->CreateBitmapScaler(&pScaler);
		if (FAILED(hr)) return hr;

		hr = pScaler->Initialize(pSource, destinationWidth, destinationHeight, WICBitmapInterpolationModeCubic);
		if (FAILED(hr)) return hr;

		hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}
	else // �ʺ�� ���̰� ��� 0�̸�, ũ�� ��ȭ�� ������ �ǹ��ϸ�, �̹����� ���������� ����.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC ��Ʈ�����κ��� D2D ��Ʈ���� ID2D1Bitmap ��ü�� ������.
	hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	if (FAILED(hr)) return hr;

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);
	SAFE_RELEASE(pScaler);

	return hr;
}
// !}!

// !{! LoadBitmapFromResource �Լ� ���� �ڵ�. ���� �ڵ��� ��ġ�� LoadBitmap/main.cpp.
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// �� �Լ��� ���� ���α׷��� ���ҽ��κ��� �̹����� �ε��Ͽ� D2D ��Ʈ�� ID2D1Bitmap ��ü�� ������.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;

	// ���ҽ��� ã��.
	imageResHandle = FindResourceW(NULL, resourceName, resourceType);
	HRESULT hr = imageResHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// ���ҽ��� �ε���.
	imageResDataHandle = LoadResource(NULL, imageResHandle);
	hr = imageResDataHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// ���ҽ��� ��װ� �޸� �����͸� ����.
	pImageFile = LockResource(imageResDataHandle);
	hr = pImageFile ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// �̹��� ũ�⸦ �����
	imageFileSize = SizeofResource(NULL, imageResHandle);
	hr = imageFileSize ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// ���ҽ��� �޸𸮿� �����ϱ� ���ؼ� WIC ��Ʈ�� ��ü�� ������.
	hr = pIWICFactory->CreateStream(&pStream);
	if (FAILED(hr)) return hr;

	// ��Ʈ���� �ʱ�ȭ��.
	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
	if (FAILED(hr)) return hr;

	// ��Ʈ���� ���� ���ڴ��� ������.
	hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// ù ��° �������� ����.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// �̹��� ������ 32bppPBGRA�� ��ȯ�� (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// �ʺ�� ���� �߿� �ϳ� �̻��� ��õǾ� ������, IWICBitmapScaler�� �����ϰ� �̸� ����ؼ� �̹����� ����������.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // ���̰� ��õ� ������ ��������, �ʺ�� ������ŭ �ڵ����� ����.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // �ʺ� ��õ� ������ ��������, ���̴� ������ŭ �ڵ����� ����.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
			destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
		}

		hr = pIWICFactory->CreateBitmapScaler(&pScaler);
		if (FAILED(hr)) return hr;

		hr = pScaler->Initialize(pSource, destinationWidth, destinationHeight, WICBitmapInterpolationModeCubic);
		if (FAILED(hr)) return hr;

		hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}
	else // �ʺ�� ���̰� ��� 0�̸�, ũ�� ��ȭ�� ������ �ǹ��ϸ�, �̹����� ���������� �ʰ� ���� ũ��� ������.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC ��Ʈ�����κ��� D2D ��Ʈ���� ID2D1Bitmap ��ü�� ������.
	hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	if (FAILED(hr)) return hr;

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);
	SAFE_RELEASE(pScaler);

	return hr;
}
// !}!

