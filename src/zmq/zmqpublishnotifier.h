// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
#define BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H

#include <sync.h>
#include <zmq/zmqabstractnotifier.h>

#include <cstddef>
#include <cstdint>
#include <functional>

class CBlock;
class CBlockIndex;
class CTransaction;

class CZMQAbstractPublishNotifier : public CZMQAbstractNotifier
{
private:
    Mutex m_socket_mutex;
    void* psocket GUARDED_BY(m_socket_mutex){nullptr};

    uint32_t nSequence {0U}; //!< upcounting per message sequence number

public:
    ~CZMQAbstractPublishNotifier();
    void* GetSocket() EXCLUSIVE_LOCKS_REQUIRED(!m_socket_mutex);

    /* send zmq multipart message
       parts:
          * command
          * data
          * message sequence number
    */
    bool SendZmqMessage(const char *command, const void* data, size_t size) EXCLUSIVE_LOCKS_REQUIRED(!m_socket_mutex);

    bool Initialize(void *pcontext) override EXCLUSIVE_LOCKS_REQUIRED(!m_socket_mutex);
    void Shutdown() override EXCLUSIVE_LOCKS_REQUIRED(!m_socket_mutex);
};

class CZMQPublishHashBlockNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishHashTransactionNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};

class CZMQPublishRawBlockNotifier : public CZMQAbstractPublishNotifier
{
private:
    const std::function<bool(CBlock&, const CBlockIndex&)> m_get_block_by_index;

public:
    CZMQPublishRawBlockNotifier(std::function<bool(CBlock&, const CBlockIndex&)> get_block_by_index)
        : m_get_block_by_index{std::move(get_block_by_index)} {}
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishRawTransactionNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};

class CZMQPublishSequenceNotifier : public CZMQAbstractPublishNotifier
{
public:
    bool NotifyBlockConnect(const CBlockIndex *pindex) override;
    bool NotifyBlockDisconnect(const CBlockIndex *pindex) override;
    bool NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence) override;
    bool NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence) override;
};

#endif // BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
