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

std::string g_srcDir;			// 源文件夹
std::string g_dstDir;			// 目的文件夹

enum CompareType
{
	noCompare,					// 不比较，直接复制
	noEqual,					// 修改时间不相等就复制
	less						// 修改时间小于源文件就复制
};

CompareType g_compareType = less;

bool g_isChild = false;			// 默认忽略子文件夹

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
	std::cout << "将指定文件夹中的文件备份到目标文件夹中。" << std::endl << std::endl;

	std::cout << "backup srcDir dstDir [/?] [/help] [/c] [/C] [/a] [/A] [/!=] [/>]" << std::endl << std::endl;

	std::cout << "/? or /help: 帮助" << std::endl;
	std::cout << "/c or /C:    遍历子文件夹，默认忽略子文件夹" << std::endl;
	std::cout << "/a or /A:    全部复制，不比较文件的修改时间，默认小于则复制" << std::endl;
	std::cout << "/!=:         比较文件的修改时间，两者不相等则复制, 默认小于则复制" << std::endl;
	std::cout << "/>:          比较文件的修改时间，如果目标文件的修改时间小于源文件则复制，默认此项" << std::endl;
}

void Option(int argc, char * argv[])
{
	g_srcDir = argv[1];
	g_dstDir = argv[2];

	// 检查源文件夹
	if (_access(g_srcDir.c_str(), 0) == -1)
	{
		std::cout << "源文件夹路径错误！" << std::endl;

		exit(-1);
	}
	
	// 检查目的文件夹
	if (_access(g_dstDir.c_str(), 0) == -1)
	{
		if (_mkdir(g_dstDir.c_str()) == -1)
		{
			std::cout << "目的文件夹不存在，创建失败！" << std::endl;
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
			std::cout << "无效参数！" << std::endl;

			exit(0);
		}
	}
}

struct FileInfo
{
	std::string rePath;			// 文件所在的相对路径
	std::string fileName;		// 文件名
	FILETIME modifyTime;		// 文件修改时间
	bool isCopy;				// 是否拷贝
	//bool isExit;				// 目标文件是否存在，拷贝时需先删除

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

	// 遍历文件
	std::string find;
	std::string findDst;
	WIN32_FIND_DATA findData;
	while (rePaths.size() > 0)
	{
		std::string repath = rePaths.top();
		rePaths.pop();

		// 目标文件夹中没有此文件夹则新建
		findDst = g_dstDir + "\\" + repath;
		if (_access(findDst.c_str(), 0) == -1)
		{
			_mkdir(findDst.c_str());
		}

		find = g_srcDir + "\\" + repath + "\\*.*";

		HANDLE handle = FindFirstFile(find.c_str(), &findData);
		while (FindNextFile(handle, &findData) == TRUE)
		{
			// 文件夹
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
		// 目标文件存在
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
				// 修改时间不相等
				if (CompareFileTime(&fileInfo.modifyTime, &findData.ftLastWriteTime) != 0)
				{
					fileInfo.isCopy = true;
					copyFiles.push_back(fileInfo);
				}
			}break;

			case less:
			{
				// 目标文件的修改时间小于源文件的修改时间则复制
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

	std::cout << "本次需要复制" << copyFiles.size() << "个文件" << std::endl;
	std::string srcFile;

	LARGE_INTEGER frequency, start, end;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	for (int i = 0; i < copyFiles.size(); i++)
	{
		std::cout << "第" << i + 1 << "个文件, 剩余" << copyFiles.size() - i - 1 << "个文件: "
			<< copyFiles[i].fileName.c_str() << std::endl;

		srcFile = g_srcDir + "\\" + copyFiles[i].rePath + "\\" + copyFiles[i].fileName;
		dstFile = g_dstDir + "\\" + copyFiles[i].rePath + "\\" + copyFiles[i].fileName;

		CopyFile(srcFile.c_str(), dstFile.c_str(), FALSE);
	}

	QueryPerformanceCounter(&end);
	int time = static_cast<int>((end.QuadPart - start.QuadPart) / frequency.QuadPart);
	std::cout << "备份完成，共耗时" << time << "秒" << std::endl;
}
