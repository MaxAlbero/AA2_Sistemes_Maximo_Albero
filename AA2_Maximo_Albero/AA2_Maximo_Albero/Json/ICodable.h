#pragma once
#include "../dist/json/json.h"
#include <functional>
#include <string>

class ICodable
{
public:
	typedef std::function<ICodable* ()> SubClassDecode;
	typedef std::map<std::string, SubClassDecode> DecodeMap;
public:

	static std::string DecodeKey();
	static void SaveDecodeProcess(std::string className, SubClassDecode decodeProcess);

	template<typename T, typename = typename std::enable_if<std::is_base_of<ICodable, T>::value>::type>
	static void SaveDecodeProcess() {
		SaveDecodeProcess(typeid(T).name(), []() {
			return new T();
		});
	}

	virtual Json::Value Code() = 0;
	virtual void Decode(Json::Value json) = 0;


	template<typename T, typename = typename std::enable_if<std::is_base_of<ICodable, T>::value>::type>
	static T* FromJson(Json::Value json) {
		std::string className = json[DecodeKey()].asString();

		ICodable* codable = (*GetDecodeMap())[className]();

		//DecodeMap* decodeMap = GetDecodeMap();
		//SubClassDecode decodeLambda = (*decodeMap)[className];
		//ICodable codable = decodeLambda();

		T* codableCasted = dynamic_cast<T*>(codable);

		codableCasted->Decode(json);

		return codableCasted;
	}


protected:

	template<typename T, typename = typename std::enable_if<std::is_base_of<ICodable, T>::value>::type>
	void CodeSubClassType(Json::Value& json) {
		json[DecodeKey()] = typeid(T).name();
	}

private:

	static DecodeMap* GetDecodeMap();
};

