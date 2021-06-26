// SPDX-License-Identifier: GPL-3.0
/**
 * Unit tests for blake2b.
 */
#include <libsolutil/Blake2.h>

#include <boost/test/unit_test.hpp>

using namespace std;

namespace solidity::util::test
{

BOOST_AUTO_TEST_SUITE(Blake2b, *boost::unit_test::label("nooptions"))

BOOST_AUTO_TEST_CASE(empty)
{
	BOOST_CHECK_EQUAL(
		blake2b(bytes()),
		FixedHash<32>("0x0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8")
	);
}

BOOST_AUTO_TEST_CASE(zeros)
{
	BOOST_CHECK_EQUAL(
		blake2b(bytes(1, '\0')),
		FixedHash<32>("0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314")
	);
	BOOST_CHECK_EQUAL(
		blake2b(bytes(2, '\0')),
		FixedHash<32>("0x9ee6dfb61a2fb903df487c401663825643bb825d41695e63df8af6162ab145a6")
	);
	BOOST_CHECK_EQUAL(
		blake2b(bytes(5, '\0')),
		FixedHash<32>("0x569ed9e4a5463896190447e6ffe37c394c4d77ce470aa29ad762e0286b896832")
	);
	BOOST_CHECK_EQUAL(
		blake2b(bytes(10, '\0')),
		FixedHash<32>("0x8a833f318209f48a635f8fdd18653d1e761ea488ebb83e1617c6ec4d9344b1a9")
	);
}

BOOST_AUTO_TEST_CASE(strings)
{
	BOOST_CHECK_EQUAL(
		blake2b("test"),
		FixedHash<32>("0x928b20366943e2afd11ebc0eae2e53a93bf177a4fcf35bcc64d503704e65e202")
	);
	BOOST_CHECK_EQUAL(
		blake2b("longer test string"),
		FixedHash<32>("0x48091eeb0e3243e1b33e215045a62b0a3c08fb93bdcd05cff3e6dbc8668ac3c3")
	);
	BOOST_CHECK_EQUAL(
		blake2b("SayHello()"),
		FixedHash<32>("0xe1d87589dc56e6c63187f37d35c7215d9570ca599ad00511baff28b7ffdce0a5")
	);
}

BOOST_AUTO_TEST_SUITE_END()

}
