#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <limits>
#include <fstream>
#include <cassert>
#include <locale>
#include <codecvt>
#include <thread>
#include <windows.h>
#undef max

//�O���[�v���
struct GroupInfo
{
	std::vector<std::wstring> names;	//���Ȃ܂�
	//�ő劄���\��, �f�t�H���g-1
	std::size_t maxmembers = std::numeric_limits<std::size_t>::max();			
	//�ԍ�
	std::size_t groupNumber = -1;

	bool IsAssignmentable() { return names.size() <= maxmembers; }
};

//----------------------------------------------------------------------------------
//[SplitString](�����Ă�����)
//string�ϐ����畡���̗v�f�𕪊�����
//�����������ʂ�Ԃ�l�ŕԋp,�Q�Ɠn���ŕύX����I�[�o�[���[�h����
//����1: ��������string
//����2: ���������؂蕶��
//����3: ���s���폜���邩
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete);


//----------------------------------------------------------------------------------
int main()
{
	//���͕�������
	constexpr std::size_t cInputSize = 1024;

	//�O���[�v���
	std::unordered_map<std::wstring, GroupInfo> groupMap;
	//�����o�[���
	std::vector<std::wstring> memberNames;
	//�p�X���[�h
	std::wstring password;

	std::wstring exportTemplate;		//�o�͕�����
	wchar_t input[cInputSize] = {};	//���͕�����
	bool loopFlag = false;

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));

	std::wcout << std::endl;
	
	//�O���[�v������
	do
	{
		loopFlag = false;
		//�\��&����
		std::wcout << L" ��Groups command-> [specify max members(default: member max)] xxx-s (number)" << std::endl;
		std::wcout << L" Group names(comma separated): " << std::flush;
		std::wcin.getline(input, cInputSize);

		//���͂����q�𕪊����܂�
		auto split = SplitString(input, L',', true);
		//�\�z���[�v
		try
		{
			for (auto& str : split)
			{
				//�Ƃ肠�����R�}���h����
				std::size_t find = str.find(L"-s ");
				auto name = str.substr(0, find);

				//�}�b�v�ɏ���ǉ�
				if (groupMap.try_emplace(name).second == false)
					throw std::exception();
				//�ԍ�����
				groupMap.at(name).groupNumber = groupMap.size() - 1;

				//�R�}���h�m�F, ���������݂��L���Ȃ�����
				if (find != std::wstring::npos)
				{
					groupMap.at(name).maxmembers = std::stoi(std::wstring(1, str[find + 3]));
					if (groupMap.at(name).maxmembers <= 0) throw std::exception();
				}
			}
		}
		catch (...)
		{
			std::wcout << L" Invalid command.\n\n" << std::endl;
			groupMap.clear();
			loopFlag = true;
		}
	} while (loopFlag);


	//���O����
	do
	{
		loopFlag = false;
		//�\��&����
		std::wcout << L"\n ��Members command-> [fixed] xxx-f (fixed group name)" << std::endl;
		std::wcout << L" Members: " << std::flush;
		std::wcin.getline(input, cInputSize);

		//���͂����q�𕪊����܂�
		auto split = SplitString(input, L',', true);

		//�����ő吔���m�F���ē����
		for (auto& element : groupMap)
			if (element.second.maxmembers == std::numeric_limits<std::size_t>::max())
				element.second.maxmembers = split.size() / groupMap.size() + (split.size() % groupMap.size() > 0 ? 1 : 0);

		//�\�z���[�v
		try
		{
			for (auto& str : split)
			{
				//�Ƃ肠�����R�}���h����
				std::size_t find = str.find(L"-f ");
				
				//�R�}���h�m�F, �O���[�v�������݂��L���Ȃ�����
				if (find != std::wstring::npos)
				{
					auto groupName = str.substr(find + 3);

					if (groupMap.at(groupName).IsAssignmentable())
						groupMap.at(groupName).names.emplace_back(str.substr(0, find));
					else throw std::exception();
				}
				//�Œ肶��Ȃ���΃����o�[���X�g�ɒǉ�
				else
					memberNames.emplace_back(str);
			}
		}
		catch (...)
		{
			std::wcout << L" Invalid command.\n\n" << std::endl;
			groupMap.clear();
			loopFlag = true;
		}
	} while (loopFlag);


	//�p�X���[�h����
	std::wcout << L"\n Password(Hide strings): " << std::flush;

	HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(stdHandle, &mode);
	SetConsoleMode(stdHandle, mode & (~ENABLE_ECHO_INPUT));

	std::wcin >> password;

	SetConsoleMode(stdHandle, mode);


	//�o�� 

	//�ŏ��̍s�̓p�X���[�h
	//<G>�݂̂̍s���牺���O���[�v�u���b�N�Ƃ�1�O���[�v�ɂ���s-> number,name,maxmembers,names[0],names[1]...�Ƃ���
	//<M>�݂̂̍s���牺�������o�[�u���b�N�Ƃ���s��-> names[0],names[1]...�Ƃ���
	exportTemplate = password + L"\n";
	exportTemplate += L"<G>\n";
	for (auto& group : groupMap)
	{
		exportTemplate += std::to_wstring(group.second.groupNumber) + L',';
		exportTemplate += group.first + L',';
		exportTemplate += std::to_wstring(group.second.maxmembers) + L',';
		for (auto& name : group.second.names)
			exportTemplate += name + L',';
		exportTemplate += L"\n";
	}
	
	exportTemplate += L"<M>\n";
	for (auto& name : memberNames)
		exportTemplate += name + L',';


	//�ȒP�ɈÍ���
	std::size_t toHash = std::hash<std::wstring>()(password);
	for (int i = 0, hashBit = 0, length = static_cast<int>(exportTemplate.length()); i < length; ++i)
	{
		exportTemplate[i] ^= static_cast<wchar_t>(toHash >> hashBit);

		hashBit = (hashBit + 2) % sizeof(std::size_t);
	}


	//�t�@�C���I�[�v��, Unicode��
	std::wofstream stream(L"TemplateData.dat", std::ios::out);
	if (!stream.is_open())
	{
		std::wcout << L"\n\n File open failed." << std::flush;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 0;
	}
	
	stream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

	//�������݂��Ă����
	stream << exportTemplate;
	std::wcout << L"\n\n Export completed." << std::flush;
	std::this_thread::sleep_for(std::chrono::seconds(3));
	return 0;
}


//----------------------------------------------------------------------------------
//[SplitString]
//string�ϐ����畡���̗v�f�𕪊�����
//�����������ʂ�Ԃ�l�ŕԋp,�Q�Ɠn���ŕύX����I�[�o�[���[�h����
//����1: ��������string
//����2: ���������؂蕶��
//����3: ���s���폜���邩
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete)
{
	std::vector<std::wstring> ret;				//�Ԃ�l
	std::wstring getString = splitString;			//��������string
	std::wstring buf;									//getline�p�̃o�b�t�@
	std::wstringstream sStream(getString);	//getline�p��stream

	//���s�폜 = true �ōŌ�̕��������s�Ȃ�폜
	if (isNewLineDelete && getString[getString.size() - 1] == '\n')
		getString[getString.size() - 1] = '\0';

	//��؂蕶�������Ƃɗv�f�擾���s���v�f��string vector�ɒǉ�
	while (std::getline(sStream, buf, delimiter))
	{
		ret.emplace_back(buf);
	}

	return std::move(ret);
}