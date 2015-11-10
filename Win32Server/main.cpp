#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <tchar.h>

#define SIZE_BUFFER 1024
int const MATRIX_SIZE = 64;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ThreadProc1(LPVOID lpParameter);
DWORD WINAPI ThreadProc2(LPVOID lpParameter);
void FormatMatrix(TCHAR txt[], int m[]);
void TransformMatrix(int inM[], int outM[]);

HWND hWndMain;
HINSTANCE hInst;
volatile bool isPaint = false;

const _TCHAR TitleWindow[] = _T("Лабораторная работа 6 (Организация межпроцессного взаимодействия) [Сервер]");
const _TCHAR ProgrammName[] = _T("WinAPI_Lab6_server");

const _TCHAR DataClientEventName[] = _T("WinAPI_Lab6_Data_Client_EVENT");
const _TCHAR DataServerEventName[] = _T("WinAPI_Lab6_Data_Server_EVENT");
const _TCHAR ClipboardEventName[] = _T("WinAPI_Lab6_Clipboard_EVENT");

const _TCHAR DataFileMappingName[] = _T("WinAPI_Lab6_Data_File_Mapping");
const _TCHAR DataMailSlotName[] = _T("\\\\.\\mailslot\\WinAPI_Lab6_Data_Mail_Slot");


HANDLE thread1 = NULL, 
	   thread2 = NULL,
	   sendDataEvent = NULL,
	   recieveDataEvent = NULL,
	   clipboardEvent = NULL;


int APIENTRY WinMain(HINSTANCE hInstance,     // Дескриптор экземпляра приложения
	HINSTANCE hPrevInstance, // в Win32 не используется
	LPSTR lpCmdLine,		  // Нужен для запуска окна в режиме командной строки
	int nCmdShow)			  // Режим отображения окна
{
	hInst = hInstance;
	HWND hWnd;
	MSG msg;
	WNDCLASS wnd = { 0 }; // Полное обнуление структуры
						  // Заполнение оконного класса
	wnd.lpszClassName = ProgrammName;
	wnd.hInstance = hInstance;
	wnd.lpfnWndProc = (WNDPROC)WndProc;
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd.lpszMenuName = NULL;
	wnd.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;

	if (!RegisterClass(&wnd))
		return -1;

	hWnd = CreateWindow(ProgrammName,
		TitleWindow,
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		0, 0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.lParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg) 
	{
		case WM_CREATE:
		{
			hWndMain = hWnd;

			// Создаем событие отправки
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = FALSE;

			sendDataEvent = CreateEvent(&sa, TRUE, FALSE, DataServerEventName);
			
			// Создаем и запускаем дочерний поток для матрицы
			thread1 = CreateThread(NULL, 512, &ThreadProc1, NULL, NULL, NULL);

			// Создаем и запускаем дочерний поток для буфера обмена
			thread2 = CreateThread(NULL, 512, &ThreadProc2, NULL, NULL, NULL);
		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		case WM_PAINT:
		{
			// Проверка на первую отрисовку формы
			if (!isPaint)
				return DefWindowProc(hWnd, msg, wParam, lParam);

			HGLOBAL hgMem;
			HDC hdcMem;
			RECT rt;
			HDC hdc;
			PAINTSTRUCT ps;
			hdc = BeginPaint(hWnd, &ps);
			
			GetClientRect(hWnd, &rt);
			while (!OpenClipboard(hWnd))
				Sleep(10);
			hdcMem = CreateCompatibleDC(hdc);
			hgMem = (HBITMAP)GetClipboardData(CF_BITMAP);

			SelectObject(hdcMem, hgMem);
			//BitBlt(hdc, rt.top + 500, rt.left, rt.bottom, rt.right, hdcMem, 0, 0, SRCCOPY);
			StretchBlt(hdc, rt.top, rt.left, 500, 500, hdcMem, 0, 0, 500, 500, SRCCOPY);
			DeleteDC(hdcMem);
			CloseClipboard();

			EndPaint(hWnd, &ps);
		}
		break;

		default:
		{
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

	}

	return 0;
}

DWORD WINAPI ThreadProc1(LPVOID lpParameter)
{
	// Исходная и преобразованная матрицы
	int srcMatrix[MATRIX_SIZE] = { 0 }, dstMatrix[MATRIX_SIZE] = { 0 };

	// Находим событие клиента для получения данных
	recieveDataEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, DataClientEventName);
				//if (!recieveDataEvent)
				//	PostMessage(hWndMain, WM_DESTROY, NULL, NULL);

	// Открываем проекцию страничного файла
	HANDLE hFileMapping = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, DataFileMappingName);
				//if (!hFileMapping)
				//	PostMessage(hWndMain, WM_DESTROY, NULL, NULL);
	
	// Получаем указатель на проекцию
	HANDLE hFMap = MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
				//if (!hFMap)
				//	PostMessage(hWndMain, WM_DESTROY, NULL, NULL);
	
	// Открываем почтовую ячейку
	HANDLE mailSlot = CreateFile(DataMailSlotName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	while (1)
	{
		// Ожидаем события от клиента, для получения исходной матрицы
		WaitForSingleObject(recieveDataEvent, INFINITE);
		ResetEvent(recieveDataEvent);

					//DWORD outBytes = 42;
					//ReadFile(hFMap, srcMatrix, MATRIX_SIZE * sizeof(int), &outBytes, NULL);
		// Получаем исходную матрицу с проекции страничного файла
		memcpy_s(srcMatrix, MATRIX_SIZE * sizeof(int), hFMap, MATRIX_SIZE * sizeof(int));
		
					//TCHAR tmp[100];
					//_stprintf_s(tmp, 100, _T("%lu"), (unsigned long)hFMap);
					//MessageBox(hWndMain, tmp, _T("DEBUG"), NULL);
		

					//TCHAR txt[SIZE_BUFFER] = { 0 };
					//FormatMatrix(txt, srcMatrix);
					//MessageBox(hWndMain, txt, _T("Matrix"), NULL);

		// Преобразовываем матрицу
		TransformMatrix(srcMatrix, dstMatrix);

		// Записываем в преобразованную матрицу в почтовую ячейку
		DWORD outBytes = 0;
		WriteFile(mailSlot, dstMatrix, MATRIX_SIZE * sizeof(int), &outBytes, NULL);

		// Уведомляем клиента
		SetEvent(sendDataEvent);

	}
	return 0;
}

void FormatMatrix(TCHAR txt[], int m[])
{
	TCHAR temp[SIZE_BUFFER >> 4];

	for (int i = 0; i < MATRIX_SIZE; i += 8)
	{
		_stprintf_s(temp,
			SIZE_BUFFER >> 4,
			_T("%3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d\r\n"),
			m[i], m[i + 1], m[i + 2], m[i + 3],
			m[i + 4], m[i + 5], m[i + 6], m[i + 7]);

		_tcscat_s(txt, SIZE_BUFFER, temp);
	}
}

void TransformMatrix(int inM[], int outM[])
{
	int COUNT_IN_LINE = 8;

	int posMax = 0, posMin = 0;
	int max = inM[0], min = inM[0];
	// Поиск максимума и мимнимума
	for (int i = 1; i < MATRIX_SIZE; ++i)
	{
		if (inM[i] > max)
		{
			max = inM[i];
			posMax = i;
		}

		if (inM[i] < min)
		{
			min = inM[i];
			posMin = i;
		}
	}

	// Нахождение строки с макс. и мин. элементом
	int lineMax = posMax / COUNT_IN_LINE;
	int lineMin = posMin / COUNT_IN_LINE;

	// Преобразовывание матрицы
	// Копирование всех элементов, кроме тех двух строк
	for (int i = 0; i < MATRIX_SIZE; ++i)
		if (i != lineMax || i != lineMin)
			outM[i] = inM[i];

	// Копирование строк с мин и макс элементами
	for (int i = 0; i < COUNT_IN_LINE; ++i)
	{
		outM[i + lineMax * COUNT_IN_LINE] = inM[i + lineMin * COUNT_IN_LINE];
		outM[i + lineMin * COUNT_IN_LINE] = inM[i + lineMax * COUNT_IN_LINE];
	}
}

DWORD WINAPI ThreadProc2(LPVOID lpParameter)
{
	clipboardEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, ClipboardEventName);
	WaitForSingleObject(clipboardEvent, INFINITE);
	isPaint = true;
	//MessageBox(hWndMain, _T("asd"), NULL, NULL);
	// Перерисовываем форму
	RECT r;
	GetClientRect(hWndMain, &r);
	InvalidateRect(hWndMain, &r, FALSE);

	return 0;
}