// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <common/run_command.h>
#include <util/string.h>

#include <tinyformat.h>
#include <univalue.h>

#ifdef ENABLE_EXTERNAL_SIGNER
#include <util/subprocess.hpp>
#endif // ENABLE_EXTERNAL_SIGNER

UniValue RunCommandParseJSON(const std::vector<std::string>& str_command, const std::string& str_std_in)
{
#ifdef ENABLE_EXTERNAL_SIGNER
    namespace sp = subprocess;

    UniValue result_json;
    std::istringstream stdout_stream;
    std::istringstream stderr_stream;

    if (str_command.empty()) return UniValue::VNULL;

    auto c = sp::Popen(str_command, sp::input{sp::PIPE}, sp::output{sp::PIPE}, sp::error{sp::PIPE});
    if (!str_std_in.empty()) {
        c.send(str_std_in);
    }
    auto [out_res, err_res] = c.communicate();

    std::cerr << "= START ================== OUT: " << out_res.buf.data() << std::endl;
    std::cerr << "========================== ERR: " << err_res.buf.data() << std::endl;

    stdout_stream.str(std::string{out_res.buf.begin(), out_res.buf.end()});
    stderr_stream.str(std::string{err_res.buf.begin(), err_res.buf.end()});

    std::string result;
    std::string error;
    std::getline(stdout_stream, result);
    std::getline(stderr_stream, error);

    std::cerr << "========================== result: " << result << std::endl;
    std::cerr << "========================== error: " << error << std::endl;

    // auto ret_code_from_wait = c.wait();
    // std::cerr << "========================== ret_code_from_wait: " << ret_code_from_wait  << std::endl;

    // const int n_error = c.retcode();
    const int n_error = c.wait();
    std::cerr << "= FINISH ================= n_error: " << n_error << std::endl << std::endl << std::endl;

    if (n_error) throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", Join(str_command, " "), n_error, error));
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
#else
    throw std::runtime_error("Compiled without external signing support (required for external signing).");
#endif // ENABLE_EXTERNAL_SIGNER
}
