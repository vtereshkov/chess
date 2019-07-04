// Chess GUI by Vasiliy Tereshkov, 2019

#include <stdio.h>
#include <windows.h>
#include "ChessEngine.h"



const int squareSize = 64, boardSize = 8 * squareSize;

typedef BOOL (WINGDIAPI WINAPI *TransparentBlt2Ptr)(IN HDC,IN int,IN int,IN int,IN int,IN HDC,IN int,IN int,IN int,IN int,IN UINT);
TransparentBlt2Ptr TransparentBlt2;

typedef HBITMAP PieceBitmap[2][16];
PieceBitmap pieceBitmap;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow)
  {
  // load Windows API functions missing in static libraries
  HINSTANCE hLibMsimg32 = LoadLibrary("Msimg32.dll");
  TransparentBlt2 = (TransparentBlt2Ptr)GetProcAddress(hLibMsimg32, "TransparentBlt");
  
  // register the window class
  const char className[]  = "Main Window Class";

  WNDCLASS wc = {};

  wc.lpfnWndProc   = WindowProc;
  wc.hInstance     = hInstance;
  wc.lpszClassName = className;
  wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);

  RegisterClass(&wc);

  // create the window
  RECT boardRect = {0, 0, boardSize, boardSize};
  AdjustWindowRect(&boardRect, WS_OVERLAPPEDWINDOW, FALSE);

  HWND hWnd = CreateWindowEx(
      0,                                 // optional styles
      className,                         // class
      "Chess",                           // text
      WS_OVERLAPPEDWINDOW,               // style
      CW_USEDEFAULT,                     // position X
      CW_USEDEFAULT,                     // position Y
      boardRect.right - boardRect.left,  // width
      boardRect.bottom - boardRect.top,  // height
      NULL,                              // parent window
      NULL,                              // menu
      hInstance,                         // handle
      NULL                               // additional application data
      );

  ShowWindow(hWnd, nCmdShow);

  // run the message loop
  MSG msg = {};

  while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }


  FreeLibrary(hLibMsimg32);

  return 0;
  }



void LoadPieceBitmaps(PieceBitmap pieceBitmap)
  {
  for (int clr = 0; clr <= 1; clr++)
    for (int i = 0; i < 6; i++)
      {
      char fileName[256];
      sprintf(fileName, "pieces\\%d%d.bmp", clr, i);

      pieceBitmap[clr][i] = (HBITMAP)LoadImage(NULL, fileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
      }
  }   



void DrawBitmap(HDC hdc, HBITMAP hBitmap, int xStart, int yStart) 
  {
  BITMAP bmp;
  HDC hdcMem;

  hdcMem = CreateCompatibleDC(hdc);
  SelectObject(hdcMem, hBitmap);
  SetMapMode(hdcMem, GetMapMode(hdc));
  GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp);

  TransparentBlt2(hdc, xStart, yStart, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, bmp.bmWidth, bmp.bmHeight, 0x000000FF);

  DeleteDC(hdcMem);
  }



void DrawBoard(HDC hdc)
  {
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++)
      {
      RECT square = {x * squareSize, y * squareSize, (x + 1) * squareSize, (y + 1) * squareSize};
      HBRUSH brush = CreateSolidBrush(((x + y) % 2) ? 0x00005F9F : 0x00BFDFFF);
      FillRect(hdc, &square, brush);
      }
  }



void DrawPieces(HDC hdc, Board &board)
  {
  for (int clr = BLACK; clr <= WHITE; clr++)
    for (int i = 0; i < 16; i++)
      {
      Piece &p = board.piece[clr][i];
      if (p.alive)
        DrawBitmap(hdc, pieceBitmap[clr][p.type], p.x * squareSize, (7 - p.y) * squareSize);
      }
  }



bool TestForCheckmate(HWND hWnd, Board &board)
  {
  const char *colorStr[] = {"White wins", "Black wins"};

  if (board.checkmate[BLACK] || board.checkmate[WHITE])
    {
    MessageBox(hWnd, colorStr[board.checkmate[BLACK] ? 0 : 1], "Checkmate", MB_OK | MB_ICONEXCLAMATION);
    board.reset();
    InvalidateRect(hWnd, NULL, FALSE);
    return true;
    }

  return false;
  }
  


// event handlers

Board board;
Piece *selectedPiece = NULL;
bool playerMove = true;



void OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  LoadPieceBitmaps(pieceBitmap);
  }



void OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
  
  DrawBoard(hdc);
  DrawPieces(hdc, board);
  
  EndPaint(hWnd, &ps);
  
  PostMessageA(hWnd, WM_USER, 0, 0);
  }



void OnLButtonDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  int xWnd = lParam & 0xFFFF;
  int yWnd = (lParam >> 16) & 0xFFFF;

  int xBrd = xWnd / squareSize;
  int yBrd = (boardSize - yWnd) / squareSize;

  selectedPiece = board.pieceAt(xBrd, yBrd);
  if (selectedPiece && (!selectedPiece->alive || selectedPiece->color != WHITE))
    selectedPiece = NULL;
  }



void OnLButtonUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  int xWnd = lParam & 0xFFFF;
  int yWnd = (lParam >> 16) & 0xFFFF;

  int xBrd = xWnd / squareSize;
  int yBrd = (boardSize - yWnd) / squareSize;
  
  // player's move
  if (playerMove && selectedPiece)                    
    {
    ValidityMap map;
    memset(&map, 0, sizeof(map));
    board.findValidMoves(*selectedPiece, map);
    
    if (map[xBrd][yBrd])
      {
      board.makeMove(*selectedPiece, xBrd, yBrd);
      playerMove = false;  
      if (TestForCheckmate(hWnd, board))
        playerMove = true;        
      
      InvalidateRect(hWnd, NULL, FALSE);
      }
    }
  }
  
  
  
void OnUser(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  // computer's move  
  if (!playerMove)
    {
    SetWindowTextA(hWnd, "Chess (Thinking)");
    
    board.makeBestMove(BLACK);
    playerMove = true;
    TestForCheckmate(hWnd, board);  

    InvalidateRect(hWnd, NULL, FALSE);
    
    SetWindowTextA(hWnd, "Chess");
    }
  }
  


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
  switch (uMsg)
    {
    case WM_CREATE:
      OnCreate(hWnd, uMsg, wParam, lParam);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

    case WM_PAINT:
      OnPaint(hWnd, uMsg, wParam, lParam);
      break;

    case WM_LBUTTONDOWN:
      OnLButtonDown(hWnd, uMsg, wParam, lParam);
      break;

    case WM_LBUTTONUP:
      OnLButtonUp(hWnd, uMsg, wParam, lParam);
      break;
      
    case WM_USER:
      OnUser(hWnd, uMsg, wParam, lParam);
      break;
    }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

