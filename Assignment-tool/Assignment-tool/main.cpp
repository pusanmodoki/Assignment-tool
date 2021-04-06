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

//グループ情報
struct GroupInfo
{
	std::wstring groupName;							//おなまえ
	std::vector<std::wstring> memberNames;	//おなまえ
	//最大割当可能数, デフォルト-1
	std::size_t maxMenbers = std::numeric_limits<std::size_t>::max();

	bool IsAssignmentable() { return memberNames.size() < maxMenbers; }
};

//----------------------------------------------------------------------------------
//[SplitString](もってきたこ)
//string変数から複数の要素を分割する
//分割した結果を返り値で返却,参照渡しで変更するオーバーロードあり
//引数1: 分割するstring
//引数2: 分割する区切り文字
//引数3: 改行を削除するか
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete);

int main()
{
	//グループ情報
	std::unordered_map<std::size_t, GroupInfo> groupMap;
	//メンバー情報
	std::vector<std::wstring> memberNames;
	std::wstring password;			//パスワード
	std::wstring importTemplate; //読み込んだ文字列

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));


	//ファイルを開いてみるうよ
	try
	{
		//ファイルオープン, Unicode化
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


	//パスワード確認
	{
		//パスワード入力
		std::wcout << L"\n Password(Hide strings): " << std::flush;

		HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode = 0;
		GetConsoleMode(stdHandle, &mode);
		SetConsoleMode(stdHandle, mode & (~ENABLE_ECHO_INPUT));

		std::wcin >> password;

		SetConsoleMode(stdHandle, mode);
		
		//複合
		std::size_t toHash = std::hash<std::wstring>()(password);

		for (int i = 0, hashBit = 0, length = static_cast<int>(importTemplate.length()); i < length; ++i)
		{
			importTemplate[i] ^= static_cast<wchar_t>(toHash >> hashBit);

			hashBit = (hashBit + 2) % sizeof(std::size_t);
		}

		//照合
		if (password != importTemplate.substr(0, importTemplate.find(L"\n")))
		{
			std::wcerr << L"\n Invalid password." << std::flush;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			return 0;
		}
	}

	//ロード
	//最初の行はパスワード
	//<G>のみの行から下をグループブロックとし1グループにつき一行-> number,name,maxMenbers,names[0],names[1]...とする
	//<M>のみの行から下をメンバーブロックとし一行で-> names[0],names[1]...とする
	{
		std::wstringstream stream(importTemplate);
		std::wstring line;
		int loadNumber = -1;

		//一行ずつ読みます
		while (std::getline(stream, line))
		{
			if (line == L"<G>") { loadNumber = 0; continue; }
			else if (line == L"<M>") { loadNumber = 1; continue; }

			//,で分割
			auto split = SplitString(line, L',', true);

			//グループロード
			if (loadNumber == 0)
			{
				//リストに追加
				auto groupNumber = static_cast<std::size_t>(std::stoul(split[0]));
				groupMap.try_emplace(groupNumber, GroupInfo{ split[1],
					std::vector<std::wstring>(), static_cast<std::size_t>(std::stoul(split[2])) });
				
				//固定なお名前追加ループ
				for (std::size_t i = 3; i < split.size(); ++i)
					groupMap.at(groupNumber).memberNames.emplace_back(split[i]);
			}
			//メンバーロード
			else if (loadNumber == 1)
			{			
				//リストに追加
				for (auto& name : split)
					memberNames.emplace_back(name);
			}
		}
	}

	//ランダムデバイス作成
	Random random(groupMap.size() - 1);
	//割当ループ
	for (auto& member : memberNames)
	{
		std::size_t randomValue;
		do
		{
			randomValue = random();	
		} while (!groupMap.at(randomValue).IsAssignmentable());
		groupMap.at(randomValue).memberNames.emplace_back(member);
	}
	

	//結果表示
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
//string変数から複数の要素を分割する
//分割した結果を返り値で返却,参照渡しで変更するオーバーロードあり
//引数1: 分割するstring
//引数2: 分割する区切り文字
//引数3: 改行を削除するか
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete)
{
	std::vector<std::wstring> ret;				//返り値
	std::wstring getString = splitString;			//分割するstring
	std::wstring buf;									//getline用のバッファ
	std::wstringstream sStream(getString);	//getline用のstream

	//改行削除 = true で最後の文字が改行なら削除
	if (isNewLineDelete && getString[getString.size() - 1] == '\n')
		getString[getString.size() - 1] = '\0';

	//区切り文字をもとに要素取得を行い要素をstring vectorに追加
	while (std::getline(sStream, buf, delimiter))
	{
		ret.emplace_back(buf);
	}

	return std::move(ret);
}