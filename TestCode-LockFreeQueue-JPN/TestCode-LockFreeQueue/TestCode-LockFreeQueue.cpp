// TestCode-LockFreeQueue.cpp : コンソール応用プログラムに対する進入点を定義します。
//

#include "stdafx.h"
#include "LockFreeQueue.h"

#define WORKERTHREAD_MAX 13


HANDLE g_hMainThreadDestroyEvent = NULL;
HANDLE g_hWorkerThreadDestroyEvent[WORKERTHREAD_MAX] = {NULL,};
CLockFreeQueue<int> g_Q;

unsigned __stdcall EnqueueThreadCallback( LPVOID lpParameter );
unsigned __stdcall DequeueThreadCallback( LPVOID lpParameter );

// TODO: CLockFreeQueueが必要とするライブラリー(vector, map, algorithm)がない場合警告メッセージを出力する方法を探してみる。
//		 とある外部ライブラリーは何か必要なのがないとエラーが発生するとき何が必要か警告メッセジーで知らせてくれるのを見たことがある。
// TODO: CLockFreeQueueが処理している状況を本来の性能に影響せずにドットグラフィックで表現できたらいいね。
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

		//_tprintf_s( _T( "[%7d] Extracted value : %7d\n" ), GetCurrentThreadId(), iPopItem );  // TODO: 処理速度のためにコメント処理
	}

	_endthreadex( 0 );
	return 0;
}
