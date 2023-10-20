#include <windows.h>
#include <iostream>


int length_of_array;
int* array;
CRITICAL_SECTION cs;
HANDLE* Start;
HANDLE* Stop;
HANDLE* Finish;


DWORD WINAPI marker(LPVOID thread_index)
{
	int marked_numbers_counter = 0;

	srand((int)thread_index);

	while (true)
	{
		int random_number = rand() % length_of_array;

		if (array[random_number] == 0)
		{
			EnterCriticalSection(&cs);
			Sleep(5);
			array[random_number] = (int)thread_index + 1;
			marked_numbers_counter++;
			Sleep(5);
			LeaveCriticalSection(&cs);
		}
		else
		{
			EnterCriticalSection(&cs);
			std::cout << "\nIndex number of thread = " << (int)thread_index + 1 <<
				"\nNumber of marked elements = " << marked_numbers_counter <<
				"\nIndex of not marked element = " << random_number << "\n";
			LeaveCriticalSection(&cs);

			SetEvent(Stop[(int)thread_index]);
			ResetEvent(Start[(int)thread_index]);

			HANDLE StartFinish[]{ Start[(int)thread_index], Finish[(int)thread_index] };

			if (WaitForMultipleObjects(2, StartFinish, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
			{
				EnterCriticalSection(&cs);
				for (int i = 0; i < length_of_array; i++)
				{
					if (array[i] == (int)thread_index + 1)
						array[i] = 0;
				}
				LeaveCriticalSection(&cs);
				ExitThread(NULL);
			}
			else
			{
				EnterCriticalSection(&cs);
				marked_numbers_counter = 0;
				ResetEvent(Stop[(int)thread_index]);
				LeaveCriticalSection(&cs);
				continue;
			}
		}
	}
}


int main()
{
	std::cout << "Enter the size of array:\n";
	std::cin >> length_of_array;

	array = new int[length_of_array];

	for (int i = 0; i < length_of_array;i++)
		array[i] = 0;

	std::cout << "Enter amount of threads:\n";

	int number_of_threads;
	std::cin >> number_of_threads;

	InitializeCriticalSection(&cs);

	HANDLE* hThreads = new HANDLE[number_of_threads];
	DWORD* IDThreads = new DWORD[number_of_threads];
	Start = new HANDLE[number_of_threads];
	Stop = new HANDLE[number_of_threads];
	Finish = new HANDLE[number_of_threads];

	HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);

	for (int i = 0; i < number_of_threads; i++)
	{
		hThreads[i] = CreateThread(NULL, 1, marker, (LPVOID)i, NULL, &IDThreads[i]);
		if (hThreads[i] == NULL)
			return GetLastError();

		Start[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (Start == NULL)
			return GetLastError();

		Stop[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (Stop == NULL)
			return GetLastError();

		Finish[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (Finish == NULL)
			return GetLastError();
	}

	for (int i = 0; i < number_of_threads; i++)
		SetEvent(Start[i]);

	int amount_of_terminated_threads = 0;

	bool* hThreadsNotFinish = new bool[number_of_threads];

	for (int i = 0; i < number_of_threads;i++)
		hThreadsNotFinish[i] = true;

	while (amount_of_terminated_threads < number_of_threads)
	{
		if (WaitForMultipleObjects(number_of_threads, Stop, TRUE, INFINITE) == WAIT_FAILED)
		{
			std::cout << "Wait for multiple objects failed.\n";
			std::cout << "Press any key to exit.\n";
		}

		WaitForSingleObject(hMutex, INFINITE);

		std::cout << "\n";

		for (int i = 0; i < length_of_array; i++)
			std::cout << array[i] << " ";

		std::cout << "\n\n";

		ReleaseMutex(hMutex);

		std::cout << "Enter index number of thread to finish:\n";

		int index_finished_thread;
		std::cin >> index_finished_thread;

		hThreadsNotFinish[index_finished_thread - 1] = false;
		amount_of_terminated_threads++;
	
		SetEvent(Finish[index_finished_thread - 1]);

		WaitForSingleObject(hThreads[index_finished_thread - 1], INFINITE);

		CloseHandle(hThreads[index_finished_thread - 1]);
		CloseHandle(Finish[index_finished_thread - 1]);
		CloseHandle(Start[index_finished_thread - 1]);

		hMutex = OpenMutex(NULL, FALSE, NULL);

		WaitForSingleObject(hMutex, INFINITE);

		std::cout << "\n";

		for (int i = 0; i < length_of_array; i++)
			std::cout << array[i] << " ";

		std::cout << "\n\n";

		ReleaseMutex(hMutex);

		for (int i = 0; i < number_of_threads; i++)
		{
			if (hThreadsNotFinish[i])
			{
				ResetEvent(Stop[i]);
				SetEvent(Start[i]);
			}
		}
	}

	for (int i = 0; i < number_of_threads; i++)
		CloseHandle(Stop[i]);

	DeleteCriticalSection(&cs);

	return 0;
}