#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

#define IDM_BUTTON_SEND_DATA 0x00000001
#define IDM_BUTTON_SEND_BMP  0x00000002
#define IDM_EDITBOX_MATRIX_SRC   0x00000003
#define IDM_EDITBOX_MATRIX_DST   0x00000004

#define SIZE_BUFFER 1024
int const MATRIX_SIZE = 64;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ThreadProc1(LPVOID lpParameter);
void FormatMatrix(TCHAR text[], int m[]);

HWND hWndMain;
HINSTANCE hInst;


const _TCHAR TitleWindow[] = _T("������������ ������ 6 (����������� �������������� ��������������) [������]");
const _TCHAR ProgrammName[] = _T("WinAPI_Lab6_client");

const _TCHAR DataClientEventName[] = _T("WinAPI_Lab6_Data_Client_EVENT");
const _TCHAR DataServerEventName[] = _T("WinAPI_Lab6_Data_Server_EVENT");
const _TCHAR ClipboardEventName[] = _T("WinAPI_Lab6_Clipboard_EVENT");

const _TCHAR DataFileMappingName[] = _T("WinAPI_Lab6_Data_File_Mapping");
const _TCHAR DataMailSlotName[] = _T("\\\\.\\mailslot\\WinAPI_Lab6_Data_Mail_Slot");

HANDLE thread1 = NULL, 
	   sendDataEvent = NULL, 
	   recieveDataEvent = NULL,
	   threadEvent = NULL;

HANDLE hFileMapping = NULL, hFMap = NULL;
HANDLE mailSlot;

HBITMAP hBMP = NULL;
HANDLE clipboardEvent = NULL;
bool isPaint = false;

int APIENTRY WinMain(HINSTANCE hInstance,     // ���������� ���������� ����������
	HINSTANCE hPrevInstance, // � Win32 �� ������������
	LPSTR lpCmdLine,		  // ����� ��� ������� ���� � ������ ��������� ������
	int nCmdShow)			  // ����� ����������� ����
{
	hInst = hInstance;
	HWND hWnd;
	MSG msg;
	WNDCLASS wnd = { 0 }; // ������ ��������� ���������
						  // ���������� �������� ������
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
	// �������� �� ������ ��������� �����
	if (msg == WM_PAINT && !isPaint)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg) 
	{

		case WM_CREATE:
		{
			hWndMain = hWnd;
			
			CreateWindow(
				TEXT("EDIT"),  // Predefined class; Unicode assumed 
				TEXT("�������� �������\r\n"),
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,  // Styles 
				10,         // x position 
				200,         // y position 
				200,        // Button width
				200,        // Button height
				hWnd,     // Parent window
				HMENU(IDM_EDITBOX_MATRIX_SRC),
				hInst,
				NULL);

			CreateWindow(
				TEXT("EDIT"),  // Predefined class; Unicode assumed 
				TEXT("��������������� �������\r\n"),
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,  // Styles 
				220,         // x position 
				200,         // y position 
				200,        // Button width
				200,        // Button height
				hWnd,     // Parent window
				HMENU(IDM_EDITBOX_MATRIX_DST),
				hInst,
				NULL);

			CreateWindow(
				TEXT("BUTTON"),
				TEXT("�������� ������� �� ������"),
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_MULTILINE,  // Styles 
				10,         // x position 
				10,         // y position 
				150,        // Button width
				100,        // Button height
				hWnd,     // Parent window
				HMENU(IDM_BUTTON_SEND_DATA),
				hInst,
				NULL);

			CreateWindow(
				TEXT("BUTTON"),
				TEXT("�������� ����� ����� ������ BMP-�������"),
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_MULTILINE,  // Styles 
				210,         // x position 
				10,         // y position 
				150,        // Button width
				100,        // Button height
				hWnd,     // Parent window
				HMENU(IDM_BUTTON_SEND_BMP),
				hInst,
				NULL);

			HFONT defaultFont;
			defaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			SendMessage(GetDlgItem(hWnd, IDM_EDITBOX_MATRIX_SRC), WM_SETFONT, WPARAM(defaultFont), TRUE);
			SendMessage(GetDlgItem(hWnd, IDM_EDITBOX_MATRIX_DST), WM_SETFONT, WPARAM(defaultFont), TRUE);
			SendMessage(GetDlgItem(hWnd, IDM_BUTTON_SEND_DATA), WM_SETFONT, WPARAM(defaultFont), TRUE);
			SendMessage(GetDlgItem(hWnd, IDM_BUTTON_SEND_BMP), WM_SETFONT, WPARAM(defaultFont), TRUE);

			// ������� ������� ��� ��������
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = FALSE; 
			
			sendDataEvent = CreateEvent(&sa, TRUE, FALSE, DataClientEventName);

			// ������� ������������ ������
			hFileMapping = CreateFileMapping((HANDLE)0xFFFFFFFF,
				NULL, PAGE_READWRITE, 0, MATRIX_SIZE * sizeof(int), DataFileMappingName);
			
			hFMap = MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			
			// ������� �������� ������
			mailSlot = CreateMailslot(DataMailSlotName, 0, MAILSLOT_WAIT_FOREVER, NULL);
			
			// ������� ������� ��� ������ ������
			clipboardEvent = CreateEvent(NULL, TRUE, FALSE, ClipboardEventName);

			TCHAR buf[1024];
			OPENFILENAME a;
			ZeroMemory(&a, sizeof(a));
			a.lStructSize = sizeof(a);
			a.lpstrFile = buf;
			a.lpstrFile[0] = '\0';
			a.nMaxFile = 1024;
			a.nFilterIndex = 1;
			a.hInstance = hInst;
			a.hwndOwner = hWnd;

			a.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // | OFN_ALLOWMULTISELECT;

			GetOpenFileName(&a);

			// ��������� ������� �������
			LPSECURITY_ATTRIBUTES secProcAtr = { 0 };
			LPSECURITY_ATTRIBUTES secThrAtr = { 0 };

			STARTUPINFO cif;
			cif.cb = sizeof(STARTUPINFO);
			ZeroMemory(&cif, sizeof(STARTUPINFO));

			PROCESS_INFORMATION pi;

			CreateProcess(
				//_T("D:\\���\\4 ����\\7 �������\\��������� ����������������\\Lab6\\Win32IPC\\Debug\\Win32Server.exe"),
				a.lpstrFile,
				NULL,
				secProcAtr,
				secThrAtr,
				TRUE,
				NULL,
				NULL,
				NULL,
				&cif,
				&pi);


			// ������� ������� ��� ������ ���������� �������
			threadEvent = CreateEvent(&sa, TRUE, FALSE, NULL); //CreateMutex(&sa, TRUE, NULL);
			// ������� � ��������� �����
			thread1 = CreateThread(NULL, 512, &ThreadProc1, NULL, NULL, NULL);

		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDM_BUTTON_SEND_DATA:
				{
					// ���������� �����
					SetEvent(threadEvent);
					// ��������� ������
					EnableWindow(GetDlgItem(hWnd, IDM_BUTTON_SEND_DATA), FALSE);
				}
				break;

				case IDM_BUTTON_SEND_BMP:
				{
					if (hBMP != NULL)
						DeleteObject(hBMP);

					hBMP = (HBITMAP)LoadImage(NULL, 
						_T("D:\\���\\4 ����\\7 �������\\��������� ����������������\\Lab6\\Win32IPC\\Debug\\pic.bmp"), 
						IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
					
					// �������� � ����� ������
					OpenClipboard(hWnd);
					SetClipboardData(CF_BITMAP, hBMP);
					CloseClipboard();

					// �������������� �����
					RECT r;
					GetClientRect(hWnd, &r);
					InvalidateRect(hWnd, &r, FALSE);
					//SendMessage(hWnd, WM_PAINT, 0, 0);
					
					isPaint = true;
					SetEvent(clipboardEvent);
				}
				break;
			}
		}

		case WM_PAINT:
		{
			HGLOBAL hgMem;
			HDC hdcMem;
			RECT rt;
			HDC hdc;
			PAINTSTRUCT ps;
			hdc = BeginPaint(hWnd, &ps);
			 
			GetClientRect(hWnd, &rt);
			OpenClipboard(hWnd);
			hdcMem = CreateCompatibleDC(hdc);
			hgMem = (HBITMAP)GetClipboardData(CF_BITMAP);
			
			SelectObject(hdcMem, hgMem);
			//BitBlt(hdc, rt.top + 500, rt.left, rt.bottom, rt.right, hdcMem, 0, 0, SRCCOPY);
			StretchBlt(hdc, rt.top + 500, rt.left, 500, 500, hdcMem, 0, 0, 500, 500, SRCCOPY);
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
	// �������� � ��������������� �������
	int srcMatrix[MATRIX_SIZE] = { 0 }, dstMatrix[MATRIX_SIZE] = { 0 };
	DWORD t; // �����
	// ������ ��� ������ ������ � ����
	TCHAR txt[SIZE_BUFFER] = { 0 };
	int outLength; // ����� ������ � ����

	HWND hWndEdit = GetDlgItem(hWndMain, IDM_EDITBOX_MATRIX_SRC);
	HWND hWndEdit2 = GetDlgItem(hWndMain, IDM_EDITBOX_MATRIX_DST);
	HWND hWndButton = GetDlgItem(hWndMain, IDM_BUTTON_SEND_DATA);
	
	while (1)
	{
		// ������� ������� �� ������
	    WaitForSingleObject(threadEvent, INFINITE);
		ResetEvent(threadEvent); // ���������� �������

		// �������� �������
		ZeroMemory(srcMatrix, MATRIX_SIZE * sizeof(int));
		ZeroMemory(dstMatrix, MATRIX_SIZE * sizeof(int));

		SetWindowText(hWndEdit, _TEXT("�������� �������\r\n"));
		SetWindowText(hWndEdit2, _TEXT("��������������� �������\r\n"));

		// ���������� �������� ������� � ��������� -9 �� 9
		t = time(NULL);
		srand(t);
		for (int i = 0; i < MATRIX_SIZE; ++i)
		{
			srcMatrix[i] = -9 + rand() % 19;
		}

		// ������� �������� ������� � ����
		ZeroMemory(txt, SIZE_BUFFER * sizeof(TCHAR));
		FormatMatrix(txt, srcMatrix);
		outLength = GetWindowTextLength(hWndEdit);
		SendMessage(hWndEdit, EM_SETSEL, outLength, outLength + 1);
		SendMessage(hWndEdit, EM_REPLACESEL, TRUE, (LPARAM)txt);

		
							//DWORD outBytes = 0;
							//WriteFile(hFileMapping, srcMatrix, MATRIX_SIZE * sizeof(int), &outBytes, NULL);

		// �������� �������� ������� � �������� ������������� �����
		memcpy_s(hFMap, MATRIX_SIZE * sizeof(int), srcMatrix, MATRIX_SIZE * sizeof(int));

		// ���������� ������, ��� ������ ����������������
		SetEvent(sendDataEvent);

		// ������� ������� ������� ��� ��������� ��������������� �������
		recieveDataEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, DataServerEventName);
		//if (!recieveDataEvent)
		//PostMessage(hWndMain, WM_DESTROY, NULL, NULL);

		// ������� ����������� �� �������
		WaitForSingleObject(recieveDataEvent, INFINITE);
		ResetEvent(recieveDataEvent);

		// �������� ��������������� ������� �� �������, ����� �������� ������
		DWORD outBytes = 0;
		ReadFile(mailSlot, dstMatrix, MATRIX_SIZE * sizeof(int), &outBytes, NULL);

		// ������� ��������������� ������� � ������ ����
		ZeroMemory(txt, SIZE_BUFFER * sizeof(TCHAR));
		FormatMatrix(txt, dstMatrix);
		outLength = GetWindowTextLength(hWndEdit2);
		SendMessage(hWndEdit2, EM_SETSEL, outLength, outLength + 1);
		SendMessage(hWndEdit2, EM_REPLACESEL, TRUE, (LPARAM)txt);

		// ������������ ������
		EnableWindow(hWndButton, TRUE);
	}

	return 0;
}

void FormatMatrix(TCHAR txt[], int m[])
{
	TCHAR temp[SIZE_BUFFER >> 4];

	for (int i = 0; i < MATRIX_SIZE; i+=8)
	{
		_stprintf_s(temp,
			SIZE_BUFFER >> 4,
			_T("%3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d\r\n"),
			m[i],     m[i + 1], m[i + 2], m[i + 3],
			m[i + 4], m[i + 5], m[i + 6], m[i + 7]);

		_tcscat_s(txt, SIZE_BUFFER, temp);
	}
}