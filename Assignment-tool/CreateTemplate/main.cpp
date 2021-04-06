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

//グループ情報
struct GroupInfo
{
	std::vector<std::wstring> names;	//おなまえ
	//最大割当可能数, デフォルト-1
	std::size_t maxmembers = std::numeric_limits<std::size_t>::max();			
	//番号
	std::size_t groupNumber = -1;

	bool IsAssignmentable() { return names.size() <= maxmembers; }
};

//----------------------------------------------------------------------------------
//[SplitString](もってきたこ)
//string変数から複数の要素を分割する
//分割した結果を返り値で返却,参照渡しで変更するオーバーロードあり
//引数1: 分割するstring
//引数2: 分割する区切り文字
//引数3: 改行を削除するか
std::vector<std::wstring> SplitString(const std::wstring& splitString, wchar_t delimiter, bool isNewLineDelete);


//----------------------------------------------------------------------------------
int main()
{
	//入力文字列上限
	constexpr std::size_t cInputSize = 1024;

	//グループ情報
	std::unordered_map<std::wstring, GroupInfo> groupMap;
	//メンバー情報
	std::vector<std::wstring> memberNames;
	//パスワード
	std::wstring password;

	std::wstring exportTemplate;		//出力文字列
	wchar_t input[cInputSize] = {};	//入力文字列
	bool loopFlag = false;

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));

	std::wcout << std::endl;
	
	//グループ数入力
	do
	{
		loopFlag = false;
		//表示&入力
		std::wcout << L" ※Groups command-> [specify max members(default: member max)] xxx-s (number)" << std::endl;
		std::wcout << L" Group names(comma separated): " << std::flush;
		std::wcin.getline(input, cInputSize);

		//入力した子を分割します
		auto split = SplitString(input, L',', true);
		//構築ループ
		try
		{
			for (auto& str : split)
			{
				//とりあえずコマンド検索
				std::size_t find = str.find(L"-s ");
				auto name = str.substr(0, find);

				//マップに情報を追加
				if (groupMap.try_emplace(name).second == false)
					throw std::exception();
				//番号割当
				groupMap.at(name).groupNumber = groupMap.size() - 1;

				//コマンド確認, 数字が存在し有効なら入れる
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


	//名前入力
	do
	{
		loopFlag = false;
		//表示&入力
		std::wcout << L"\n ※Members command-> [fixed] xxx-f (fixed group name)" << std::endl;
		std::wcout << L" Members: " << std::flush;
		std::wcin.getline(input, cInputSize);

		//入力した子を分割します
		auto split = SplitString(input, L',', true);

		//割当最大数を確認して入れる
		for (auto& element : groupMap)
			if (element.second.maxmembers == std::numeric_limits<std::size_t>::max())
				element.second.maxmembers = split.size() / groupMap.size() + (split.size() % groupMap.size() > 0 ? 1 : 0);

		//構築ループ
		try
		{
			for (auto& str : split)
			{
				//とりあえずコマンド検索
				std::size_t find = str.find(L"-f ");
				
				//コマンド確認, グループ名が存在し有効なら入れる
				if (find != std::wstring::npos)
				{
					auto groupName = str.substr(find + 3);

					if (groupMap.at(groupName).IsAssignmentable())
						groupMap.at(groupName).names.emplace_back(str.substr(0, find));
					else throw std::exception();
				}
				//固定じゃなければメンバーリストに追加
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


	//パスワード入力
	std::wcout << L"\n Password(Hide strings): " << std::flush;

	HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(stdHandle, &mode);
	SetConsoleMode(stdHandle, mode & (~ENABLE_ECHO_INPUT));

	std::wcin >> password;

	SetConsoleMode(stdHandle, mode);


	//出力 

	//最初の行はパスワード
	//<G>のみの行から下をグループブロックとし1グループにつき一行-> number,name,maxmembers,names[0],names[1]...とする
	//<M>のみの行から下をメンバーブロックとし一行で-> names[0],names[1]...とする
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


	//簡単に暗号化
	std::size_t toHash = std::hash<std::wstring>()(password);
	for (int i = 0, hashBit = 0, length = static_cast<int>(exportTemplate.length()); i < length; ++i)
	{
		exportTemplate[i] ^= static_cast<wchar_t>(toHash >> hashBit);

		hashBit = (hashBit + 2) % sizeof(std::size_t);
	}


	//ファイルオープン, Unicode化
	std::wofstream stream(L"TemplateData.dat", std::ios::out);
	if (!stream.is_open())
	{
		std::wcout << L"\n\n File open failed." << std::flush;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 0;
	}
	
	stream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

	//かきこみしておわり
	stream << exportTemplate;
	std::wcout << L"\n\n Export completed." << std::flush;
	std::this_thread::sleep_for(std::chrono::seconds(3));
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