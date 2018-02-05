#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <stack>
#include <direct.h>
#include <io.h>
#include <windows.h>

void ShowHelp();
void Option(int argc, char *argv[]);
void DoBackup();

std::string g_srcDir;			// Դ�ļ���
std::string g_dstDir;			// Ŀ���ļ���

enum CompareType
{
	noCompare,					// ���Ƚϣ�ֱ�Ӹ���
	noEqual,					// �޸�ʱ�䲻��Ⱦ͸���
	less						// �޸�ʱ��С��Դ�ļ��͸���
};

CompareType g_compareType = less;

bool g_isChild = false;			// Ĭ�Ϻ������ļ���

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		ShowHelp();

		return -1;
	}
	else
	{
		Option(argc, argv);
	}

	DoBackup();

	return 0;
}

void ShowHelp()
{
	std::cout << "��ָ���ļ����е��ļ����ݵ�Ŀ���ļ����С�" << std::endl << std::endl;

	std::cout << "backup srcDir dstDir [/?] [/help] [/c] [/C] [/a] [/A] [/!=] [/>]" << std::endl << std::endl;

	std::cout << "/? or /help: ����" << std::endl;
	std::cout << "/c or /C:    �������ļ��У�Ĭ�Ϻ������ļ���" << std::endl;
	std::cout << "/a or /A:    ȫ�����ƣ����Ƚ��ļ����޸�ʱ�䣬Ĭ��С������" << std::endl;
	std::cout << "/!=:         �Ƚ��ļ����޸�ʱ�䣬���߲��������, Ĭ��С������" << std::endl;
	std::cout << "/>:          �Ƚ��ļ����޸�ʱ�䣬���Ŀ���ļ����޸�ʱ��С��Դ�ļ����ƣ�Ĭ�ϴ���" << std::endl;
}

void Option(int argc, char * argv[])
{
	g_srcDir = argv[1];
	g_dstDir = argv[2];

	// ���Դ�ļ���
	if (_access(g_srcDir.c_str(), 0) == -1)
	{
		std::cout << "Դ�ļ���·������" << std::endl;

		exit(-1);
	}
	
	// ���Ŀ���ļ���
	if (_access(g_dstDir.c_str(), 0) == -1)
	{
		if (_mkdir(g_dstDir.c_str()) == -1)
		{
			std::cout << "Ŀ���ļ��в����ڣ�����ʧ�ܣ�" << std::endl;
		}
	}

	for (int i = 3; i < argc; ++i)
	{
		if ((_stricmp(argv[i], "/help") == 0) || (_stricmp(argv[i], "/?") == 0))
		{ 
			ShowHelp();

			exit(0);
		}
		else if (_stricmp(argv[i], "/c") == 0)
		{
			g_isChild = true;
		}
		else if (_stricmp(argv[i], "/a") == 0)
		{
			g_compareType = noCompare;
		}
		else if (_stricmp(argv[i], "/!=") == 0)
		{
			g_compareType = noEqual;
		}
		else if (_stricmp(argv[i], "/>") == 0)
		{
			g_compareType = less;
		}
		else
		{
			std::cout << "��Ч������" << std::endl;

			exit(0);
		}
	}
}

struct FileInfo
{
	std::string rePath;			// �ļ����ڵ����·��
	std::string fileName;		// �ļ���
	FILETIME modifyTime;		// �ļ��޸�ʱ��
	bool isCopy;				// �Ƿ񿽱�
	//bool isExit;				// Ŀ���ļ��Ƿ���ڣ�����ʱ����ɾ��

	FileInfo()
	{
		isCopy = false;
		//isExit = false;
	}
};

void DoBackup()
{
	std::vector<FileInfo> fileInfos;
	std::stack<std::string> rePaths;

	rePaths.push(".");

	// �����ļ�
	std::string find;
	std::string findDst;
	WIN32_FIND_DATA findData;
	while (rePaths.size() > 0)
	{
		std::string repath = rePaths.top();
		rePaths.pop();

		// Ŀ���ļ�����û�д��ļ������½�
		findDst = g_dstDir + "\\" + repath;
		if (_access(findDst.c_str(), 0) == -1)
		{
			_mkdir(findDst.c_str());
		}

		find = g_srcDir + "\\" + repath + "\\*.*";

		HANDLE handle = FindFirstFile(find.c_str(), &findData);
		while (FindNextFile(handle, &findData) == TRUE)
		{
			// �ļ���
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				if (g_isChild)
				{
					if ((strcmp(findData.cFileName, ".") != 0) &&
						(strcmp(findData.cFileName, "..") != 0))
					{
						rePaths.push(repath + "\\" + findData.cFileName);
					}
				}
			}
			else
			{
				FileInfo fileInfo;
				fileInfo.rePath = repath;
				fileInfo.fileName = findData.cFileName;
				fileInfo.modifyTime = findData.ftLastWriteTime;

				fileInfos.push_back(fileInfo);
			}
		}
	}

	std::vector<FileInfo> copyFiles;
	std::string dstFile;
	for (int i = 0; i < fileInfos.size(); i++)
	{
		FileInfo fileInfo = fileInfos[i];

		dstFile = g_dstDir + "\\" + fileInfos[i].rePath + "\\" + fileInfos[i].fileName;
		// Ŀ���ļ�����
		if (FindFirstFile(dstFile.c_str(), &findData) != INVALID_HANDLE_VALUE)
		{
			switch (g_compareType)
			{
			case noCompare:
			{
				fileInfo.isCopy = true;

				copyFiles.push_back(fileInfo);
			}break;

			case noEqual:
			{
				// �޸�ʱ�䲻���
				if (CompareFileTime(&fileInfo.modifyTime, &findData.ftLastWriteTime) != 0)
				{
					fileInfo.isCopy = true;
					copyFiles.push_back(fileInfo);
				}
			}break;

			case less:
			{
				// Ŀ���ļ����޸�ʱ��С��Դ�ļ����޸�ʱ������
				if (CompareFileTime(&fileInfo.modifyTime, &findData.ftLastWriteTime) == 1)
				{
					fileInfo.isCopy = true;
					copyFiles.push_back(fileInfo);
				}
			}break;

			default:
				break;
			}
		}
		else
		{
			copyFiles.push_back(fileInfo);
		}
	}

	std::cout << "������Ҫ����" << copyFiles.size() << "���ļ�" << std::endl;
	std::string srcFile;

	LARGE_INTEGER frequency, start, end;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	for (int i = 0; i < copyFiles.size(); i++)
	{
		std::cout << "��" << i + 1 << "���ļ�, ʣ��" << copyFiles.size() - i - 1 << "���ļ�: "
			<< copyFiles[i].fileName.c_str() << std::endl;

		srcFile = g_srcDir + "\\" + copyFiles[i].rePath + "\\" + copyFiles[i].fileName;
		dstFile = g_dstDir + "\\" + copyFiles[i].rePath + "\\" + copyFiles[i].fileName;

		CopyFile(srcFile.c_str(), dstFile.c_str(), FALSE);
	}

	QueryPerformanceCounter(&end);
	int time = static_cast<int>((end.QuadPart - start.QuadPart) / frequency.QuadPart);
	std::cout << "������ɣ�����ʱ" << time << "��" << std::endl;
}
