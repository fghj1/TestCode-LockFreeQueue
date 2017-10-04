// TestCode-LockFreeQueue.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "LockFreeQueue.h"

#define WORKERTHREAD_MAX 13


HANDLE g_hMainThreadDestroyEvent = NULL;
HANDLE g_hWorkerThreadDestroyEvent[WORKERTHREAD_MAX] = {NULL,};
CLockFreeQueue<int> g_Q;

unsigned __stdcall EnqueueThreadCallback( LPVOID lpParameter );
unsigned __stdcall DequeueThreadCallback( LPVOID lpParameter );

// TODO: CLockFreeQueue가 필요로 하는 라이브러리(vector, map, algorithm)가 없을 경우 경고 메시지로 출력해줄 수 있는 방법을 찾아본다.
//		 어떤 외부 라이브러리는 뭔가 필요한 것이 없어 에러가 발생할 경우 무엇이 필요한지 경고 메시지로 출력하는 것을 본 적이 있다.
// TODO: 64bits에서도 실행될 수 있도록 본 프로그램을 수정할 것
// TODO: CLockFreeQueue가 처리되는 상황을 본래 성능에 영향을 주지 않고 도트 그래픽으로 표현할 수 있다면 좋을 듯.
int _tmain( int argc, _TCHAR* argv[] )
{
	//_CrtSetBreakAlloc( ? );  // TEST
	//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );  // TEST
	//{
		g_hMainThreadDestroyEvent = CreateEvent( NULL, TRUE, FALSE, NULL );  // Manual-reset, Non-signal
		if( NULL == g_hMainThreadDestroyEvent )
		{
			char szError[256] = {0,};
			sprintf_s( szError, 256, "[ERROR] Failed to create the worker thread destroy event. ErrNo.%d\n", GetLastError() );
			OutputDebugStringA( szError );
			return 0;
		}

		INT iWorkerThreadIndex = 0;
		char szTest[256] = {0,};
		unsigned ( __stdcall *pThreadCallback )( LPVOID );
		HANDLE hWorkerThread = NULL;
		for( iWorkerThreadIndex = 0; iWorkerThreadIndex < WORKERTHREAD_MAX; ++iWorkerThreadIndex )
		{
			sprintf_s( szTest, 256, "iWorkerThreadIndex : %d\n", iWorkerThreadIndex );
			OutputDebugStringA( szTest );

			// EnqueueThread : DequeueThread = 12 : 1
			//if( iWorkerThreadIndex < ( WORKERTHREAD_MAX - 1 ) )
			//	pThreadCallback = EnqueueThreadCallback;
			//else
			//	pThreadCallback = DequeueThreadCallback;

			// EnqueueThread : DequeueThread = 6 : 7
			if( 0 != iWorkerThreadIndex % 2 )
				pThreadCallback = EnqueueThreadCallback;
			else
				pThreadCallback = DequeueThreadCallback;

			hWorkerThread = ( HANDLE )_beginthreadex( NULL, 0, pThreadCallback, ( LPVOID )&iWorkerThreadIndex, 0, NULL );
			if( NULL == hWorkerThread )
			{
				char szError[256] = {0,};
				sprintf_s( szError, 256, "[ERROR] Failed to create the thread(%d).\n", hWorkerThread );
				OutputDebugStringA( szError );
				return 0;
			}

			Sleep( 10 );
		}

		INT iTermKey = 0, iAnswer = 0;
		LONG lQSize = 0;
		while( true )
		{
			iTermKey = _gettch();
			if( 0x00000013 == iTermKey )  // Ctrl+S : 0x00000013
			{
				lQSize = g_Q.Size();
				_tprintf_s( _T( "Q size : %d\n" ), lQSize );

				sprintf_s( szTest, 256, "Q size : %d\n", lQSize );
				OutputDebugStringA( szTest );
			}
			else if( 0x00000018 == iTermKey )  // Ctrl+X : 0x00000018
			{
				_tprintf_s( _T( "Do you really want to shutdown the program? y/n\n" ) );
				iAnswer = _gettch();
				if( 'y' == iAnswer )
				{
					_tprintf_s( _T( "Program is shutting down. Please wait...\n" ) );
					SetEvent( g_hMainThreadDestroyEvent );
					break;
				}
				else
					_tprintf_s( _T( "Shutdown process has been canceled.\n" ) );
			}
		}

		WaitForMultipleObjects( WORKERTHREAD_MAX, g_hWorkerThreadDestroyEvent, TRUE, INFINITE );

		if( 0 == CloseHandle( hWorkerThread ) )
		{
			char szError[256] = {0,};
			sprintf_s( szError, 256, "[ERROR] Failed to close the thread handle. ErrNo.%d\n", GetLastError() );
			OutputDebugStringA( szError );
		}
		hWorkerThread = NULL;

		for( iWorkerThreadIndex = 0; iWorkerThreadIndex < WORKERTHREAD_MAX; ++iWorkerThreadIndex )
		{
			if( g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] )
			{
				CloseHandle( g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] );
				g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] = NULL;
			}
		}
		if( g_hMainThreadDestroyEvent )
		{
			CloseHandle( g_hMainThreadDestroyEvent );
			g_hMainThreadDestroyEvent = NULL;
		}
	//}
	//_CrtDumpMemoryLeaks();  // TEST

	return 0;
}

unsigned __stdcall EnqueueThreadCallback( LPVOID lpParameter )
{
	INT iWorkerThreadIndex = *( ( INT* )lpParameter );

	g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] = CreateEvent( NULL, TRUE, FALSE, NULL );  // Manual-reset, Non-signal
	if( NULL == g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] )
	{
		char szError[256] = {0,};
		sprintf_s( szError, 256, "[ERROR] Failed to create the %d's worker thread destroy event. ErrNo.%d\n", iWorkerThreadIndex, GetLastError() );
		OutputDebugStringA( szError );
		return 0;
	}

	_tprintf_s( _T( "[%7d] Create the %d's Enqueue thread.\n" ), GetCurrentThreadId(), iWorkerThreadIndex );

	DWORD Result = 0;
	char szTest[256] = {0,};
	INT iLoop = 0, iMaxLoop = ( iWorkerThreadIndex + 1 ) * 10000, iPushItem = 0;
	while( true )
	{
		Result = WaitForSingleObject( g_hMainThreadDestroyEvent, 1 );
		if( WAIT_OBJECT_0 == Result )
		{
			sprintf_s( szTest, 256, "[%7d] Close the %d's Enqueue thread.\n", GetCurrentThreadId(), iWorkerThreadIndex );
			OutputDebugStringA( szTest );

			SetEvent( g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] );

			return 0;
		}

		iPushItem = ( iLoop % 10000 ) + iMaxLoop;
		g_Q.Enqueue( iPushItem );
		++iLoop;
		if( 10000 == iLoop )
			iLoop = 0;
	}

	_endthreadex( 0 );
	return 0;
}

unsigned __stdcall DequeueThreadCallback( LPVOID lpParameter )
{
	INT iWorkerThreadIndex = *( ( INT* )lpParameter );

	g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] = CreateEvent( NULL, TRUE, FALSE, NULL );  // Manual-reset, Non-signal
	if( NULL == g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] )
	{
		char szError[256] = {0,};
		sprintf_s( szError, 256, "[ERROR] Failed to create the %d's worker thread destroy event. ErrNo.%d\n", iWorkerThreadIndex, GetLastError() );
		OutputDebugStringA( szError );
		return 0;
	}

	_tprintf_s( _T( "[%7d] Create the %d's Dequeue thread.\n" ), GetCurrentThreadId(), iWorkerThreadIndex );

	INT iPopItem = 0;
	DWORD Result = 0;
	char szTest[256] = {0,};
	while( true )
	{
		if( false == g_Q.Dequeue( iPopItem ) )
		{
			Result = WaitForSingleObject( g_hMainThreadDestroyEvent, 1 );
			if( WAIT_OBJECT_0 == Result )
			{
				sprintf_s( szTest, 256, "[%7d] Close the %d's Dequeue thread.\n", GetCurrentThreadId(), iWorkerThreadIndex );
				OutputDebugStringA( szTest );

				SetEvent( g_hWorkerThreadDestroyEvent[iWorkerThreadIndex] );

				return 0;
			}

			continue;
		}

		//_tprintf_s( _T( "[%7d] Extracted value : %7d\n" ), GetCurrentThreadId(), iPopItem );  // TODO: 처리 속도를 위해 주석 처리
	}

	_endthreadex( 0 );
	return 0;
}
