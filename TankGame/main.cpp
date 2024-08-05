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

int tankDirection = 0; // 기본적으로 위쪽 방향을 나타냄

int numRectangles = 12;//사각형개수


int targetNumber = 0; // 태양이 맞은 횟수
float sunX = 0.f;//왼쪽상단
float sunY = 0.f;
float sunX2 = 0.f;//우측하단
float sunY2 = 0.f;

bool endGame = false;



struct Bullet
{
	float x;
	float y;
	int direction;
};




// 자원 안전 반환 매크로.
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// 비트맵을 로드하는 함수 원형
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);




class DemoApp
{
public:
	DemoApp() {};
	~DemoApp() { DiscardDeviceResource(); DiscardAppResource(); }; // 소멸자. 모든 자원을 반납함.
	bool Initialize(HINSTANCE hInstance); // 윈도우 생성, CreateAppResource 호출 포함.
	void Render(); // 내용을 그리기, CreateDevice 호출 포함.
	
	// 마지막 처음으로 되돌아감
	bool Update(float timeDelta);
	//시작화면
	void RenderStartScreen(); // 시작 화면 그리기

	//총알부분
	std::vector<Bullet> bullets;
	void FireBullet();
	void RenderBullets(D2D1_SIZE_F rtSize);

	//사각형 충돌
	bool isCheckRectangle(float tankX, float tankY, D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3);
	
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //윈도우 프로시져
	void OnResize(); //창 리사이즈 콜백 함수
	bool CreateAppResource(); //앱 자원 생성
	void DiscardAppResource(); //앱 자원 폐기
	bool CreateDeviceResource(); //장치 자원 생성
	void DiscardDeviceResource(); //장치 자원 폐기

private:
	//클래스의 변수들을 선언
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;
	ID2D1SolidColorBrush* pCornflowerBlueBrush = NULL;
	IDWriteFactory* pDWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
	// 붓
	ID2D1SolidColorBrush* pBlack = NULL;
	ID2D1SolidColorBrush* pWhite = NULL;
	ID2D1SolidColorBrush* pGreen = NULL;
	ID2D1SolidColorBrush* pRed = NULL;
	ID2D1SolidColorBrush* pYellow = NULL;

	//시작화면
	bool startScreen = true;

	// WIC
	IWICImagingFactory* pWICFactory = NULL;
	//비트맵
	ID2D1Bitmap* pBitmapFromFile = NULL;
	ID2D1Bitmap* pBitmapFromResource = NULL;

	// DirectSound로 소리를 재생함.
	CSoundManager* soundManager = NULL;

	//tank 불투명 마스크
	ID2D1Bitmap* pOrigBitmap=NULL;
	ID2D1Bitmap* pMaskBitmap=NULL;
	ID2D1Bitmap* pSpaceBitmap = NULL;
	ID2D1BitmapBrush* pOrigBitmapBrush=NULL;

	//경로기하
	// 기하
	ID2D1PathGeometry* pSunGeometry = NULL;
	ID2D1RadialGradientBrush* pRadialGradientBrush = NULL;

	// 나선
	ID2D1PathGeometry* pPathGeometry = NULL;
	ID2D1PathGeometry* pObjectGeometry = NULL;


	
	float alphas[30]; // 각 삼각형의 애니메이션 상태를 저장하는 배열

	// 애니메이션 변수
	float timeDuration; // 애니매이션 지속 시간. 분모로 사용되므로 0이 되면 안됨.
	float alpha; // 애니메이션 동안의 현재 위치값, [0,1] 사이의 값.
	float startPos;
	float endPos;

};


// 응용 프로그램의 진입점 함수.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DemoApp demoApp;
	if (!demoApp.Initialize(hInstance)) return 1;

	// Time 선언
	LARGE_INTEGER nPrevTime;
	LARGE_INTEGER nFrequency;
	// Time 초기화
	QueryPerformanceFrequency(&nFrequency);
	QueryPerformanceCounter(&nPrevTime);

	// 메인 윈도우 메시지 루프를 실행함.
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
			// Time 갱신
			LARGE_INTEGER CurrentTime;
			QueryPerformanceCounter(&CurrentTime);
			float timeDelta = (float)((double)(CurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = CurrentTime;


			// 갱신 함수를 호출한다. esc키 누르면 false가 리턴되어 종료한다.
			if (!demoApp.Update(timeDelta)) break;
			demoApp.Render();
		}
	}

	return 0;
}
// 응용 프로그램의 원도우를 생성하고, 장치 독립적 자원을 생성함.
bool DemoApp::Initialize(HINSTANCE hInstance)
{
	// 윈도우 클래스를 등록함.
	WNDCLASS wc = {};
	wc.lpfnWndProc = DemoApp::WndProc;
	wc.cbWndExtra = sizeof(LONG_PTR);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DemoApp";
	RegisterClass(&wc);

	// 윈도우를 생성함.
	hWnd = CreateWindow(L"DemoApp", L"DemoApp", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, this);
	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// 장치 독립적 자원을 생성함.
	if (!CreateAppResource()) return false;
	

	timeDuration = 70.0f; // 애니메이션 시간 증가
	for (int i = 0; i < numRectangles; ++i)
	{
		alphas[i] = 0.0f;
	}


	//초기 탱크 위치 설정
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
				tankDirection = 2; // 아래쪽 방향을 나타냄
				break;
			case VK_UP:
				if (rc.top > tankY-10)
					tankY = tankY;
				else
					tankY -= 20;
				tankDirection = 0; // 위쪽 방향을 나타냄
				break;
			case VK_RIGHT:
				if (rc.right < tankX+40)
					tankX = tankX;
				else
					tankX += 20;
				tankDirection = 1; // 오른쪽 방향을 나타냄
				break;
			case VK_LEFT:
				if (rc.left > tankX-10)
					tankX = tankX;
				else
					tankX -= 20;
				tankDirection = 3; // 왼쪽 방향을 나타냄
				break;
				//시작화면 부분
			}
			InvalidateRect(hWnd, NULL, TRUE); // 화면을 다시 그리도록 요청
		}

		//시작화면 부분
		if (wParam == 'A') // 'A' 키가 눌리면
		{
			if(pDemoApp->startScreen==false){
				pDemoApp->FireBullet(); // 총알 발사
			}
			pDemoApp->startScreen = false;
			InvalidateRect(hWnd, NULL, TRUE); // 화면을 다시 그리도록 요청
		}
		//시작화면 부분
		if (wParam == 'R') // 'R' 키가 눌리면
		{
			if (endGame) {
				pDemoApp->startScreen = true;
				targetNumber = 0;
				//초기 탱크 위치 설정
				RECT rc;
				GetClientRect(hWnd, &rc);
				tankX = (rc.right / 2) - 15;
				tankY = rc.bottom - 30;

				InvalidateRect(hWnd, NULL, TRUE); // 화면을 다시 그리도록 요청
			}

		}
		return 0;
	}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 창 리사이즈 콜백 함수
void DemoApp::OnResize()
{
	if (!pRenderTarget) return;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	// 렌더타겟의 크기를 다시 설정함.
	pRenderTarget->Resize(size);

}
// 장치 독립적 자원들을 생성함. 이들 자원의 수명은 응용 프로그램이 종료되기 전까지 유효함.

bool DemoApp::CreateAppResource()
{
	
	//비트맵부분
	// CoCreateInstance 함수가 사용될 때에는 CoInitialize를 호출해주어야 함.
	CoInitialize(NULL); // CreateAppResource 함수의 처음에서 호출.
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	// CoCreateInstance 함수를 사용하여 WIC 팩토리를 생성함.
	HRESULT br = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
	if (FAILED(br)) return false;



	// D2D 팩토리를 생성함.
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) return false;

	// DirectWrite 팩토리를 생성함.
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	if (FAILED(hr)) return false;

	//DirectWrite 텍스트 포맷 객체를 생성함.
	hr = pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 50, L"", &pTextFormat);
	if (FAILED(hr)) return false;

	// 텍스트를 수평으로 중앙 정렬하고 수직으로도 중앙 정렬함.
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


	//사운드 부분
	soundManager = new CSoundManager;
	hr = soundManager->init(hWnd); // 유효한 hWnd가 확보된 뒤에 호출되어야 함.
	if (FAILED(hr)) return false;

	// 사운드 파일을 추가함.
	int id;  // 사운드클립 등록번호, id=0부터 시작함.
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


	



	//나선
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



	// 애니메이션 초기화
	timeDuration = 3.0f;
	alpha = 0;

	float length = 0;
	startPos = length * 0.5;
	pPathGeometry->ComputeLength(NULL, &length);
	endPos = length * 0.8;



	return true;
}

// 앱 자원 폐기
void DemoApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pWICFactory);
	SAFE_DELETE(soundManager);

	//비트맵
	// CoInitialize 함수가 호출될 때에는 마지막에 이 함수도 호출해 주어야 함.
	CoUninitialize(); // DiscardAppResource 함수의 마지막에서 호출.

	//경로기하
	SAFE_RELEASE(pSunGeometry);
	//나선
	SAFE_RELEASE(pPathGeometry);
	SAFE_RELEASE(pObjectGeometry);
}

// 장치 의존적 자원들을 생성함. 장치가 소실되는 경우에는 이들 자원을 다시 생성해야 함.
bool DemoApp::CreateDeviceResource()
{
	// 렌더타겟이 유효하다면 이 함수를 실행할 필요가 없음
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

	// D2D 렌더타겟을 생성함.
	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
	if (FAILED(hr)) return false;



	// 검은색 붓을 생성함.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlack);
	if (FAILED(hr)) return false;
	// 하얀색 붓을 생성함.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);
	if (FAILED(hr)) return false;
	//빨강 붓
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pRed);
	// 노랑 붓
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &pYellow);
	// 초록 붓
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow), &pGreen);

	//검사 및 체크

	HRESULT br = S_OK;
	// 외부 파일로부터 비트맵 객체 pAnotherBitmap를 생성함.
	br = LoadBitmapFromFile(pRenderTarget, pWICFactory, L".\\startBG.jpg", 0, 0, &pBitmapFromFile);
	if (FAILED(br)) return false;

	// 응용 프로그램 리소스로부터 비트맵 객체 pBitmap를 생성함.
	br = LoadBitmapFromResource(pRenderTarget, pWICFactory, L"startBG", L"Image", 0, 150, &pBitmapFromResource);
	if (FAILED(br)) return false;


	//잔디 비트맵 생성
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"space", L"Image", 0, 0, &pSpaceBitmap);


	// 비트맵을 생성함.
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"tankSkin", L"Image", 0, 0, &pOrigBitmap);
	LoadBitmapFromResource(pRenderTarget, pWICFactory, L"tankMask", L"Image", 0, 0, &pMaskBitmap);
	// 비트맵 붓을 생성함.
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


	// 방사형 계조 붓을 생성함.
	// D2D1_GRADIENT_STOP 구조체 배열을 생성함.
	ID2D1GradientStopCollection* pGradientStops = NULL;

	D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Gold, 1);
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Orange, 0.8f);
	gradientStops[1].position = 0.85f;
	gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::OrangeRed, 0.7f);
	gradientStops[2].position = 1.0f;

	// D2D1_GRADIENT_STOP 구조체 배열로부터 ID2D1GradientStopCollection을 생성함.
	pRenderTarget->CreateGradientStopCollection(gradientStops, 3, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);

	// ID2D1GradientStopCollection로부터 방사형 계조 붓을 생성함.
	pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(330, 330), D2D1::Point2F(140, 140), 140, 140), pGradientStops, &pRadialGradientBrush);




	return true;
}

// 장치 의존적 자원들을 반납함. 장치가 소실되면 이들 자원을 다시 생성해야 함.
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

	//비트맵
	SAFE_RELEASE(pBitmapFromFile);
	SAFE_RELEASE(pBitmapFromResource);
	SAFE_RELEASE(pOrigBitmap);
	SAFE_RELEASE(pMaskBitmap);
	SAFE_RELEASE(pSpaceBitmap);

	//경로기하
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
	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.

	// 입력 상태를 갱신함.


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
	//시작화면일때 소리 반복
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

	//a를 눌렀을 때 효과음 한번씩
	if (GetAsyncKeyState('A') & 0x8000f) {
		soundManager->play(1, FALSE);
	}


	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000f) return false; //응용 프로그램을 종료함

	return true;
}




// 그릴 내용을 그리기.
void DemoApp::Render()
{
	if (!CreateDeviceResource()) return;


	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 렌더타켓의 크기를 얻음.
	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	

	if (startScreen){	
		RenderStartScreen();
	}
	else
	{

		//배경
		D2D1_POINT_2F base = D2D1::Point2F(0.f, 0.f);
		pRenderTarget->DrawBitmap(pSpaceBitmap, D2D1::RectF(base.x, base.y, base.x + rtSize.width, base.y + rtSize.height));



		//피통
		if(targetNumber>0){
			D2D1_RECT_F bar = D2D1::RectF(5, 10, rtSize.width-5, 40);

			// 바의 외부를 초록색으로 채움
			D2D1_RECT_F barFill = D2D1::RectF(5, 10, rtSize.width - 5, 40);
			pRenderTarget->FillRectangle(barFill, pGreen);

			if (targetNumber < 31) {
				// 바의 내부를 빨간색으로 채움
				D2D1_RECT_F barChange = D2D1::RectF(5, 10, (rtSize.width - 5) * (targetNumber / 30.0f), 40);
				pRenderTarget->FillRectangle(barChange, pRed);
			}
			else {
				D2D1_RECT_F barFill = D2D1::RectF(5, 10, rtSize.width - 5, 40);
				pRenderTarget->FillRectangle(barFill, pRed);
			}

			pRenderTarget->DrawRectangle(bar, pBlack);
		}
		

		// 태양 경로기하 만들기
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
			// 눈 웃음
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 30, centerY - 20), D2D1::Point2F(centerX - 20, centerY - 30), D2D1::Point2F(centerX - 10, centerY - 20)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX + 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX + 30, centerY - 20), D2D1::Point2F(centerX + 20, centerY - 30), D2D1::Point2F(centerX + 10, centerY - 20)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			// 스마일
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 30), D2D1::Point2F(centerX, centerY + 40), D2D1::Point2F(centerX + 20, centerY + 30)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else if (targetNumber < 20) {
			//무표정 - -  표정 
			//        -
			// 왼쪽 눈
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 20));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// 오른쪽 눈
			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 20), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 20));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// 입
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 20, centerY + 30));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else if(targetNumber < 30) {
			// 화난 표정
			// 왼쪽 눈
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 25), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 15));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);


			// 오른쪽 눈

			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 15), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 25));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			// 입
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 40), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 40), D2D1::Point2F(centerX, centerY + 20), D2D1::Point2F(centerX + 20, centerY + 40)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
		}
		else {
			//왼쪽눈
			pSink->BeginFigure(D2D1::Point2F(centerX - 30, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 10, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX - 10, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX - 30, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// 오른쪽 눈
			pSink->BeginFigure(D2D1::Point2F(centerX + 10, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 30, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			pSink->BeginFigure(D2D1::Point2F(centerX + 30, centerY - 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(centerX + 10, centerY - 10));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);

			// 입
			pSink->BeginFigure(D2D1::Point2F(centerX - 20, centerY + 30), D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(centerX - 20, centerY + 30), D2D1::Point2F(centerX, centerY + 20), D2D1::Point2F(centerX + 20, centerY + 30)));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);







			endGame = true;

			// 끝남을 그리기 위한 준비
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

			std::wstring endText = L"승리!! R을 누르면 초기화면으로 돌아갑니다!";

			// 텍스트 렌더링
			D2D1_RECT_F textRect = D2D1::RectF(rtSize.width/2-200, 50, rtSize.width - 10, rtSize.height - 10);
			pRenderTarget->DrawText(
				endText.c_str(),
				endText.length(),
				pTextFormatEnd,
				textRect,
				pWhite
			);

			// 리소스 해제
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
		//// 붓으로 채울 사각형 모양을 정의함.
		//D2D1_RECT_F sunbg = D2D1::RectF(centerX-70, centerY-80, centerX+70, centerY + 60);
		//pRenderTarget->DrawRectangle(sunbg, pWhite);



		// 붓으로 채울 사각형 모양을 정의함.
		D2D1_RECT_F rcBrushRect = D2D1::RectF(0, 0, 30, 30);

		// 중심점을 기준으로 회전 및 변환 설정
		D2D1_POINT_2F center = D2D1::Point2F(15, 15); // 이미지의 중심
		D2D1_MATRIX_3X2_F transform;

		switch (tankDirection)
		{
		case 0: // 위쪽
			transform = D2D1::Matrix3x2F::Rotation(90, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 1: // 오른쪽
			transform = D2D1::Matrix3x2F::Rotation(180, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 2: // 아래쪽
			transform = D2D1::Matrix3x2F::Rotation(270, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		case 3: // 왼쪽
			transform = D2D1::Matrix3x2F::Rotation(0, center) * D2D1::Matrix3x2F::Translation(tankX, tankY);
			break;
		}

		pRenderTarget->SetTransform(transform);

		// FillOpacityMask 함수가 올바르게 동작하려면 직전에 D2D1_ANTIALIAS_MODE_ALIASED로 지정해야 함.
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		pRenderTarget->FillOpacityMask(pMaskBitmap, pOrigBitmapBrush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, &rcBrushRect);
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		



		//// 텍스트를 그리기 위한 준비
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



		//// 탱크 중심 좌표 텍스트 생성
		//std::wstring tankPosText = L"Tank Position: (" + std::to_wstring(tankX + center.x) + L", " + std::to_wstring(tankY + center.y) + L")";

		//// 텍스트 렌더링
		//D2D1_RECT_F textRect = D2D1::RectF(10, 10, rtSize.width - 10, rtSize.height - 10);
		//pRenderTarget->DrawText(
		//	tankPosText.c_str(),
		//	tankPosText.length(),
		//	pTextFormat,
		//	textRect,
		//	pWhite
		//);

		//// 리소스 해제
		//pTextFormat->Release();


		// 이동 동선 그리기

		// 이동 동선 기하 경로가 화면 중심에 그려지도록 함.
		D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
		float minWidthHeightScale = min(rtSize.width, rtSize.height) / 512;
		D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(minWidthHeightScale, minWidthHeightScale);
		D2D1::Matrix3x2F translation = D2D1::Matrix3x2F::Translation(rtSize.width / 2, rtSize.height / 2);
		pRenderTarget->SetTransform(scale * translation);

		//pRenderTarget->DrawGeometry(pPathGeometry, pRed); // 이동 동선을 붉은색으로 그림.

		if (targetNumber < 30) {
			// 사각형의 개수 설정
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
				float length = GetPosInterp(alphas[i], startPos, endPos); // 애니메이션을 위한 현재 위치 계산
				D2D1_POINT_2F point;
				D2D1_POINT_2F tangent;
				pPathGeometry->ComputePointAtLength(length, NULL, &point, &tangent);

				D2D1_MATRIX_3X2_F rectangleMatrix;
				rectangleMatrix = D2D1::Matrix3x2F(
					tangent.x, tangent.y,
					-tangent.y, tangent.x,
					point.x, point.y);
				pRenderTarget->SetTransform(rectangleMatrix * scale * translation);

				// 사각형의 좌표 정의
				D2D1_RECT_F rect = D2D1::RectF(-10.0f, -10.0f, 10.0f, 10.0f);

				// 사각형을 노란색으로 그림
				pRenderTarget->FillRectangle(rect, pYellow);

				// 변환된 사각형의 좌표 계산
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

				// 충돌 감지
				if (isCheckRectangle(tankX, tankY, transformedPoints[0], transformedPoints[1], transformedPoints[2], transformedPoints[3])) {
					startScreen = true;
					targetNumber = 0;
					// 초기 탱크 위치 설정
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
			//초기 탱크 위치 설정
			RECT rc;
			GetClientRect(hWnd, &rc);
			tankX = (rc.right / 2) - 15;
			tankY = rc.bottom - 30;
			soundManager->stop(2);
			InvalidateRect(hWnd, NULL, TRUE);
		}

		//총알 랜더링
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		RenderBullets(rtSize);


	}

	HRESULT hr = pRenderTarget->EndDraw();
	// 이 함수는 실행되는 동안에 장치가 소실되면 장치 자원들을 반납함.
	// 다음번 Render 호출 시에 내부에서 호출되는 CreateDeviceResource 에서 반납된 장치 자원들이 다시 생성됨.
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}



// 충돌 감지 함수 수정
bool DemoApp::isCheckRectangle(float px, float py, D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) {
	// 탱크 중심 좌표
	D2D1_POINT_2F tankCenter = D2D1::Point2F(px + 15, py + 15);

	// 벡터 교차 곱을 이용한 충돌 감지
	auto crossProduct = [](D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) {
		return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
	};

	bool b1 = crossProduct(p0, p1, tankCenter) >= 0.0f;
	bool b2 = crossProduct(p1, p2, tankCenter) >= 0.0f;
	bool b3 = crossProduct(p2, p3, tankCenter) >= 0.0f;
	bool b4 = crossProduct(p3, p0, tankCenter) >= 0.0f;

	return (b1 == b2) && (b2 == b3) && (b3 == b4);
}




// 총알 발사 함수
void DemoApp::FireBullet()
{
	float bulletX = tankX;
	float bulletY = tankY;

	// 각 방향에 따라 총알의 초기 위치를 조정
	// 탱크 왼쪽 꼭짓점 기준
	switch (tankDirection)
	{
	case 0: // 위쪽
		bulletX += 15.f;
		break;
	case 1: // 오른쪽
		bulletX += 30.f; 
		bulletY += 15.f;
		break;
	case 2: // 아래쪽
		bulletY += 30.f;
		bulletX += 15.f;
		break;
	case 3: // 왼쪽
		bulletY += 15.f;
		break;
	}
	
	Bullet bullet = { bulletX, bulletY, tankDirection };
	bullets.push_back(bullet);

}

// 총알 렌더링 함수
void DemoApp::RenderBullets(D2D1_SIZE_F rtSize)
{
	for (auto it = bullets.begin(); it != bullets.end(); )
	{
		switch (it->direction)
		{
		case 0: // 위쪽
			it->y -= 3;
			if (it->y < 0) {
				it = bullets.erase(it); // 화면 밖으로 나가면 삭제
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // 태양 맞으면 삭제
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // 다음 총알로 이동
			}
			break;
		case 1: // 오른쪽
			it->x += 3;
			if (it->x > rtSize.width) {
				it = bullets.erase(it); // 화면 밖으로 나가면 삭제
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // 태양 맞으면 삭제
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // 다음 총알로 이동
			}
			break;
		case 2: // 아래쪽
			it->y += 3;
			if (it->y > rtSize.height) {
				it = bullets.erase(it); // 화면 밖으로 나가면 삭제
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // 태양 맞으면 삭제
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // 다음 총알로 이동
			}
			break;
		case 3: // 왼쪽
			it->x -= 3;
			if (it->x < 0) {
				it = bullets.erase(it); // 화면 밖으로 나가면 삭제
			}
			else if (it->x > sunX && it->y > sunY && it->x < sunX2 && it->y < sunY2) {
				it = bullets.erase(it); // 태양 맞으면 삭제
				targetNumber++;
			}
			else {
				D2D1_ELLIPSE bullet = D2D1::Ellipse(D2D1::Point2F(it->x, it->y), 5.0f, 5.0f);
				pRenderTarget->DrawEllipse(bullet, pWhite, 5.f, nullptr);
				++it; // 다음 총알로 이동
			}
			break;
		}
	}
}










// 시작 화면을 그리기.
void DemoApp::RenderStartScreen()
{
	endGame = false;// 종료 다시 false로 지정
	
	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	// 비트맵 그리기
	D2D1_POINT_2F base = D2D1::Point2F(0.f, 0.f);
	pRenderTarget->DrawBitmap(pBitmapFromResource, D2D1::RectF(base.x, base.y, base.x + rtSize.width, base.y + rtSize.height));

	const WCHAR titleText[] = L"TankGame";
	const WCHAR instructionsText[] = L"※a키 누르면 시작!\n이동키 : ↑,↓,←,→ \n 총알 발사 : a";

	// 타이틀 텍스트 그리기
	pRenderTarget->DrawText(
		titleText,
		ARRAYSIZE(titleText) - 1,
		pTextFormat,
		D2D1::RectF(0, 0, rtSize.width, rtSize.height),
		pWhite
	);

	// 명령어 텍스트 포맷 설정
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

	// 명령어 텍스트 그리기
	pRenderTarget->DrawText(
		instructionsText,
		ARRAYSIZE(instructionsText) - 1,
		pCommandFormat,
		D2D1::RectF(0, rtSize.height - 80, rtSize.width - 10, rtSize.height),
		pWhite
	);

	SAFE_RELEASE(pCommandFormat);
}





// !{! LoadBitmapFromFile 함수 구현 코드. 원본 코드의 위치는 LoadBitmap/main.cpp.
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// 이 함수는 파일로부터 이미지를 로드하여 D2D 비트맵 ID2D1Bitmap 객체를 생성함.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// 첫 번째 프레임을 얻음.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// 이미지 포맷을 32bppPBGRA로 변환함 (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// 너비와 높이 중에 하나 이상이 명시되어 있으면, IWICBitmapScaler를 생성하고 이를 사용해서 이미지를 리사이즈함.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // 높이가 명시된 값으로 정해지고, 너비는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // 너비가 명시된 값으로 정해지고, 높이는 비율만큼 자동으로 계산됨.
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
	else // 너비와 높이가 모두 0이면, 크기 변화가 없음을 의미하며, 이미지를 스케일하지 않음.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC 비트맵으로부터 D2D 비트맵인 ID2D1Bitmap 객체를 생성함.
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

// !{! LoadBitmapFromResource 함수 구현 코드. 원본 코드의 위치는 LoadBitmap/main.cpp.
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// 이 함수는 응용 프로그램의 리소스로부터 이미지를 로드하여 D2D 비트맵 ID2D1Bitmap 객체를 생성함.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;

	// 리소스를 찾음.
	imageResHandle = FindResourceW(NULL, resourceName, resourceType);
	HRESULT hr = imageResHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 로드함.
	imageResDataHandle = LoadResource(NULL, imageResHandle);
	hr = imageResDataHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 잠그고 메모리 포인터를 얻어옴.
	pImageFile = LockResource(imageResDataHandle);
	hr = pImageFile ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 이미지 크기를 계산함
	imageFileSize = SizeofResource(NULL, imageResHandle);
	hr = imageFileSize ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 메모리에 매핑하기 위해서 WIC 스트림 객체를 생성함.
	hr = pIWICFactory->CreateStream(&pStream);
	if (FAILED(hr)) return hr;

	// 스트림을 초기화함.
	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
	if (FAILED(hr)) return hr;

	// 스트림에 대한 디코더를 생성함.
	hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// 첫 번째 프레임을 얻음.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// 이미지 포맷을 32bppPBGRA로 변환함 (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// 너비와 높이 중에 하나 이상이 명시되어 있으면, IWICBitmapScaler를 생성하고 이를 사용해서 이미지를 리사이즈함.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // 높이가 명시된 값으로 정해지고, 너비는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // 너비가 명시된 값으로 정해지고, 높이는 비율만큼 자동으로 계산됨.
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
	else // 너비와 높이가 모두 0이면, 크기 변화가 없음을 의미하며, 이미지를 스케일하지 않고 원본 크기로 생성함.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC 비트맵으로부터 D2D 비트맵인 ID2D1Bitmap 객체를 생성함.
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

