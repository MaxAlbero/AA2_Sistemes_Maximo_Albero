#include "ICodable.h"

std::string ICodable::DecodeKey()
{
	return "ICodableType";
}

void ICodable::SaveDecodeProcess(std::string className, SubClassDecode decodeProcess)
{
	ICodable::GetDecodeMap()->emplace(className, decodeProcess);
}

ICodable::DecodeMap* ICodable::GetDecodeMap() {
	static DecodeMap* map = new DecodeMap();

	return map;
}