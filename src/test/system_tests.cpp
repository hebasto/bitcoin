// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include <bitcoin-build-config.h> // IWYU pragma: keep
#include <test/util/setup_common.h>
#include <common/run_command.h>
#include <univalue.h>
#include <util/fs.h>

#ifdef ENABLE_EXTERNAL_SIGNER
#include <util/subprocess.h>
#endif // ENABLE_EXTERNAL_SIGNER

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <fstream>
#include <string>

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

#ifdef ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_CASE(run_command)
{
    BOOST_TEST_MESSAGE(strprintf("==== m_args: %s", fs::PathToString(m_args.GetDataDirBase())));

    {
        const UniValue result = RunCommandParseJSON("");
        BOOST_CHECK(result.isNull());
    }
    {
#ifdef WIN32
        const UniValue result = RunCommandParseJSON("cmd.exe /c echo {\"success\": true}");
#else
        const UniValue result = RunCommandParseJSON("echo {\"success\": true}");
#endif
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
    {
        // An invalid command is handled by cpp-subprocess
#ifdef WIN32
        const std::string expected{"CreateProcess failed: "};
#else
        const std::string expected{"execve failed: "};
#endif
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON("invalid_command"), subprocess::CalledProcessError, HasReason(expected));
    }
    {
        // Return non-zero exit code, no output to stderr
#ifdef WIN32
        const std::string command{"cmd.exe /c exit 1"};
#else
        const std::string command{"false"};
#endif
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what{e.what()};
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned 1: \n", command)) != std::string::npos);
            return true;
        });
    }
    {
        // Return non-zero exit code, with error message for stderr
        const std::string expected_message{"oops"};
#ifdef WIN32
        const std::string command{strprintf("cmd.exe /c \"echo %s 1>&2 && exit 1\"", expected_message)};
#else
        // Not using the test data dir to avoid a space in the path.
        const fs::path script_path{fs::temp_directory_path() / "script.sh"};
        const std::string script_name{fs::PathToString(script_path)};
        std::ofstream script{script_path};
        BOOST_REQUIRE_MESSAGE(script, strprintf("failed to create: %s", script_name));
        script < "#!/bin/sh\n";
        script << "echo $1 >&2\n";
        script << "exit 1\n";
        script.close();
        int exit_status = std::system(("chmod +x \"" + script_name + "\"").data());
        BOOST_REQUIRE_MESSAGE(exit_status == 0, strprintf("failed to chmod: %s", script_name));
        const std::string command{script_name + " " + expected_message};
#endif
        const std::string expected_error{strprintf("RunCommandParseJSON error: process(%s) returned 1: ", command)};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            std::string what(e.what());

            BOOST_TEST_MESSAGE(strprintf("==== what: %s", what));

            BOOST_CHECK(what.find(expected_error) != std::string::npos);
            what.erase(0, expected_error.size());
            BOOST_CHECK(what.find(expected_message) != std::string::npos);
            return true;
        });
        fs::remove(script_path);
    }
    {
        // Unable to parse JSON
#ifdef WIN32
        const std::string command{"cmd.exe /c echo {"};
#else
        const std::string command{"echo {"};
#endif
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, HasReason("Unable to parse JSON: {"));
    }
#ifndef WIN32
    {
        // Test stdin
        const UniValue result = RunCommandParseJSON("cat", "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
#endif
}
#endif // ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_SUITE_END()
