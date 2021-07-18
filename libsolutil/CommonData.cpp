// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Common data and utils.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolutil/CommonData.h>
#include <libsolutil/Exceptions.h>
#include <libsolutil/Assertions.h>
#include <libsolutil/Keccak256.h>
#include <libsolutil/Blake2.h>
#include <libsolutil/FixedHash.h>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace solidity;
using namespace solidity::util;

namespace
{

static char const* upperHexChars = "0123456789ABCDEF";
static char const* lowerHexChars = "0123456789abcdef";

}

string solidity::util::toHex(uint8_t _data, HexCase _case)
{
	assertThrow(_case != HexCase::Mixed, BadHexCase, "Mixed case can only be used for byte arrays.");

	char const* chars = _case == HexCase::Upper ? upperHexChars : lowerHexChars;

	return std::string{
		chars[(unsigned(_data) / 16) & 0xf],
		chars[unsigned(_data) & 0xf]
	};
}

string solidity::util::toHex(bytes const& _data, HexPrefix _prefix, HexCase _case)
{
	std::string ret(_data.size() * 2 + (_prefix == HexPrefix::Add ? 2 : 0), 0);

	size_t i = 0;
	if (_prefix == HexPrefix::Add)
	{
		ret[i++] = '0';
		ret[i++] = 'x';
	}

	// Mixed case will be handled inside the loop.
	char const* chars = _case == HexCase::Upper ? upperHexChars : lowerHexChars;
	size_t rix = _data.size() - 1;
	for (uint8_t c: _data)
	{
		// switch hex case every four hexchars
		if (_case == HexCase::Mixed)
			chars = (rix-- & 2) == 0 ? lowerHexChars : upperHexChars;

		ret[i++] = chars[(static_cast<size_t>(c) >> 4ul) & 0xfu];
		ret[i++] = chars[c & 0xfu];
	}
	assertThrow(i == ret.size(), Exception, "");

	return ret;
}

int solidity::util::fromHex(char _i, WhenError _throw)
{
	if (_i >= '0' && _i <= '9')
		return _i - '0';
	if (_i >= 'a' && _i <= 'f')
		return _i - 'a' + 10;
	if (_i >= 'A' && _i <= 'F')
		return _i - 'A' + 10;
	if (_throw == WhenError::Throw)
		assertThrow(false, BadHexCharacter, to_string(_i));
	else
		return -1;
}

bytes solidity::util::fromHex(std::string const& _s, WhenError _throw)
{
	if (_s.empty())
		return {};

	unsigned s = (_s.size() >= 2 && _s[0] == '0' && _s[1] == 'x') ? 2 : 0;
	std::vector<uint8_t> ret;
	ret.reserve((_s.size() - s + 1) / 2);

	if (_s.size() % 2)
	{
		int h = fromHex(_s[s++], _throw);
		if (h != -1)
			ret.push_back(static_cast<uint8_t>(h));
		else
			return bytes();
	}
	for (unsigned i = s; i < _s.size(); i += 2)
	{
		int h = fromHex(_s[i], _throw);
		int l = fromHex(_s[i + 1], _throw);
		if (h != -1 && l != -1)
			ret.push_back(static_cast<uint8_t>(h * 16 + l));
		else
			return bytes();
	}
	return ret;
}


bool solidity::util::passesAddressChecksum(string const& _str, bool _strict)
{
	string s = _str.substr(0, 2) == "0x" ? _str : "0x" + _str;

	if (s.length() != 42)
		return false;

	if (!_strict && (
		s.find_first_of("abcdef") == string::npos ||
		s.find_first_of("ABCDEF") == string::npos
	))
		return true;

	return s == solidity::util::getChecksummedAddress(s);
}

// Solidity++: checksum
int solidity::util::checksum(string const& _data, string const& _checksum)
{
	auto dataBytes = fromHex(_data.data());
	auto checkSumBytes = fromHex(_checksum.data());
	unsigned length = (unsigned)checkSumBytes.size();
	h256 hash;
	blake2b_raw(hash.data(), length, dataBytes.data(), dataBytes.size(), nullptr, 0);

	bool isSame = true;
	bool isBitwiseNot = true;
	for (unsigned i = 0; i < length; ++i)
    {
		uint8_t b1 = checkSumBytes[i];
		uint8_t b2 = hash[i];
		uint8_t b2n = ~b2;

		isSame = isSame && b1 == b2;
		isBitwiseNot = isBitwiseNot && b1 == b2n;
    }

	if (isSame)
		return 1;
	else if(isBitwiseNot)
		return -1;	
	return 0;
}

// Solidity++: check vite address checksum
bool solidity::util::passesViteAddressChecksum(string const& _str)
{	
	if (_str.size() != 55)
		return false;
	string hex = _str.substr(5);
	string value = hex.substr(0, 40);
	string checkSum = hex.substr(40, 10);

	return checksum(value, checkSum) != 0;
}

// Solidity++: check vite token id checksum
bool solidity::util::passesViteTokenIdChecksum(string const& _str)
{
	if (_str.size() != 28)
		return false;
	
	string hex = _str.substr(4);
	string value = hex.substr(0, 20);
	string checkSum = hex.substr(20, 4);

	return checksum(value, checkSum) != 0;
}

// Solidity++: get vite address in hex string
string solidity::util::getViteAddressHex(string const& _addr)
{
	if (_addr.size() != 55)
		return nullptr;

	string hex = _addr.substr(5);
	string value = hex.substr(0, 40);
	string checkSum = hex.substr(40, 10);

	int addressType = checksum(value, checkSum);
	if (addressType == 0)
		return nullptr;
	string postfix = addressType > 0 ? "00" : "01";
	return "0x" + value + postfix;
}

// Solidity++: get vite token id in hex string
string solidity::util::getViteTokenIdHex(string const& _tokenid)
{
	if (_tokenid.size() != 28)
		return nullptr;
	string hex = _tokenid.substr(4);
	string value = hex.substr(0, 20);
	string checkSum = hex.substr(20, 4);

	int result = checksum(value, checkSum);
	if (result == 0)
		return nullptr;
	return "0x" + value;
}

string solidity::util::getChecksummedAddress(string const& _addr)
{
	string s = _addr.substr(0, 2) == "0x" ? _addr.substr(2) : _addr;
	assertThrow(s.length() == 40, InvalidAddress, "");
	assertThrow(s.find_first_not_of("0123456789abcdefABCDEF") == string::npos, InvalidAddress, "");

	h256 hash = keccak256(boost::algorithm::to_lower_copy(s, std::locale::classic()));

	string ret = "0x";
	for (unsigned i = 0; i < 40; ++i)
	{
		char addressCharacter = s[i];
		uint8_t nibble = hash[i / 2u] >> (4u * (1u - (i % 2u))) & 0xf;
		if (nibble >= 8)
			ret += static_cast<char>(toupper(addressCharacter));
		else
			ret += static_cast<char>(tolower(addressCharacter));
	}
	return ret;
}

bool solidity::util::isValidHex(string const& _string)
{
	if (_string.substr(0, 2) != "0x")
		return false;
	if (_string.find_first_not_of("0123456789abcdefABCDEF", 2) != string::npos)
		return false;
	return true;
}

bool solidity::util::isValidDecimal(string const& _string)
{
	if (_string.empty())
		return false;
	if (_string == "0")
		return true;
	// No leading zeros
	if (_string.front() == '0')
		return false;
	if (_string.find_first_not_of("0123456789") != string::npos)
		return false;
	return true;
}

string solidity::util::formatAsStringOrNumber(string const& _value)
{
	assertThrow(_value.length() <= 32, StringTooLong, "String to be formatted longer than 32 bytes.");

	for (auto const& c: _value)
		if (c <= 0x1f || c >= 0x7f || c == '"')
			return "0x" + h256(_value, h256::AlignLeft).hex();

	return escapeAndQuoteString(_value);
}


string solidity::util::escapeAndQuoteString(string const& _input)
{
	string out;

	for (char c: _input)
		if (c == '\\')
			out += "\\\\";
		else if (c == '"')
			out += "\\\"";
		else if (c == '\b')
			out += "\\b";
		else if (c == '\f')
			out += "\\f";
		else if (c == '\n')
			out += "\\n";
		else if (c == '\r')
			out += "\\r";
		else if (c == '\t')
			out += "\\t";
		else if (c == '\v')
			out += "\\v";
		else if (!isprint(c, locale::classic()))
		{
			ostringstream o;
			o << "\\x" << std::hex << setfill('0') << setw(2) << (unsigned)(unsigned char)(c);
			out += o.str();
		}
		else
			out += c;

	return "\"" + std::move(out) + "\"";
}
