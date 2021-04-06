#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <unordered_map>
#include <thread>
#include <windows.h>
#undef max
#include "Random.hpp"

//�O���[�v���
struct GroupInfo
{
	std::wstring groupName;							//���Ȃ܂�
	std::vector<std::wstring> memberNames;	//���Ȃ܂�
	//�ő劄���\��, �f�t�H���g-1
	std::size_t maxMenbers = std::numeric_limits<std::size_t>::max();

	bool IsAssignmentable() { return memberNames.size() < maxMenbers; }
};

//----------------------------------------------------------------------------------
//[SplitString](�����Ă�����)
//string�ϐ����畡���̗v�f�𕪊�����
//�����������ʂ�Ԃ�l�ŕԋp,�Q�Ɠn���ŕύX����I�[�o�[���[�h����
//����1: ��������string
//����2: ���������؂蕶��
//����3: ���s���폜���邩
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete);

int main()
{
	//�O���[�v���
	std::unordered_map<std::size_t, GroupInfo> groupMap;
	//�����o�[���
	std::vector<std::wstring> memberNames;
	std::wstring password;			//�p�X���[�h
	std::wstring importTemplate; //�ǂݍ��񂾕�����

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));


	//�t�@�C�����J���Ă݂邤��
	try
	{
		//�t�@�C���I�[�v��, Unicode��
		std::wifstream stream(L"TemplateData.dat", std::ios::in);
		if (!stream.is_open()) throw std::exception();

		stream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
		//read
		importTemplate = std::wstring((std::istreambuf_iterator<wchar_t>(stream)), std::istreambuf_iterator<wchar_t>());
	}
	catch (...)
	{
		std::wcerr << L"\n File open failed." << std::flush;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 0;
	}


	//�p�X���[�h�m�F
	{
		//�p�X���[�h����
		std::wcout << L"\n Password(Hide strings): " << std::flush;

		HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode = 0;
		GetConsoleMode(stdHandle, &mode);
		SetConsoleMode(stdHandle, mode & (~ENABLE_ECHO_INPUT));

		std::wcin >> password;

		SetConsoleMode(stdHandle, mode);
		
		//����
		std::size_t toHash = std::hash<std::wstring>()(password);

		for (int i = 0, hashBit = 0, length = static_cast<int>(importTemplate.length()); i < length; ++i)
		{
			importTemplate[i] ^= static_cast<wchar_t>(toHash >> hashBit);

			hashBit = (hashBit + 2) % sizeof(std::size_t);
		}

		//�ƍ�
		if (password != importTemplate.substr(0, importTemplate.find(L"\n")))
		{
			std::wcerr << L"\n Invalid password." << std::flush;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			return 0;
		}
	}

	//���[�h
	//�ŏ��̍s�̓p�X���[�h
	//<G>�݂̂̍s���牺���O���[�v�u���b�N�Ƃ�1�O���[�v�ɂ���s-> number,name,maxMenbers,names[0],names[1]...�Ƃ���
	//<M>�݂̂̍s���牺�������o�[�u���b�N�Ƃ���s��-> names[0],names[1]...�Ƃ���
	{
		std::wstringstream stream(importTemplate);
		std::wstring line;
		int loadNumber = -1;

		//��s���ǂ݂܂�
		while (std::getline(stream, line))
		{
			if (line == L"<G>") { loadNumber = 0; continue; }
			else if (line == L"<M>") { loadNumber = 1; continue; }

			//,�ŕ���
			auto split = SplitString(line, L',', true);

			//�O���[�v���[�h
			if (loadNumber == 0)
			{
				//���X�g�ɒǉ�
				auto groupNumber = static_cast<std::size_t>(std::stoul(split[0]));
				groupMap.try_emplace(groupNumber, GroupInfo{ split[1],
					std::vector<std::wstring>(), static_cast<std::size_t>(std::stoul(split[2])) });
				
				//�Œ�Ȃ����O�ǉ����[�v
				for (std::size_t i = 3; i < split.size(); ++i)
					groupMap.at(groupNumber).memberNames.emplace_back(split[i]);
			}
			//�����o�[���[�h
			else if (loadNumber == 1)
			{			
				//���X�g�ɒǉ�
				for (auto& name : split)
					memberNames.emplace_back(name);
			}
		}
	}

	//�����_���f�o�C�X�쐬
	Random random(groupMap.size() - 1);
	//�������[�v
	for (auto& member : memberNames)
	{
		std::size_t randomValue;
		do
		{
			randomValue = random();	
		} while (!groupMap.at(randomValue).IsAssignmentable());
		groupMap.at(randomValue).memberNames.emplace_back(member);
	}
	

	//���ʕ\��
	std::wcout << L"\n\n <Result>" << std::endl;
	for (std::size_t i = 0; i < groupMap.size(); ++i)
	{
		std::wcout << L"\n "<< i << ". Group " << groupMap.at(i).groupName << L": " << std::endl;
		std::wcout << L"      Member: " << std::flush;
		for (auto& memberName : groupMap.at(i).memberNames)
			std::wcout << memberName << L", " << std::flush;
		std::wcout << std::endl;
	}

	std::rewind(stdin);
	std::getchar();

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