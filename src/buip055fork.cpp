// Copyright (c) 2017 The Bitcoin Unlimited Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "buip055fork.h"
#include "chain.h"
#include "primitives/block.h"
#include "script/interpreter.h"
#include "unlimited.h"

#include <inttypes.h>
#include <vector>

static std::string invalidOpRetString("Bitcoin: A Peer-to-Peer Electronic Cash System");
std::vector<unsigned char> invalidOpReturn(invalidOpRetString.begin(), invalidOpRetString.end());

bool UpdateBUIP055Globals(CBlockIndex *activeTip)
{
    if (activeTip)
    {
        if ((miningForkTime.value != 0) && activeTip->forkAtNextBlock(miningForkTime.value))
        {
            excessiveBlockSize = miningForkEB.value;
            maxGeneratedBlock = miningForkMG.value;
            return true;
        }
    }
    return false;
}

bool ValidateBUIP055Block(const CBlock &block, CValidationState &state)
{
    // Validate transactions are HF compatible
    for (const CTransaction &tx : block.vtx)
    {
        if (IsTxOpReturnInvalid(tx))
            return state.DoS(100,
                             error("transaction is invalid on BUIP055 chain"), REJECT_INVALID, "bad-txns-wrong-fork");
    }
    return true;
}


bool IsTxBUIP055Only(const CTransaction& tx)
{
    if (tx.sighashType & SIGHASH_FORKID)
    {
        LogPrintf("txn is BUIP055-specific\n");
        return true;
    }
    return false;
}

bool IsTxOpReturnInvalid(const CTransaction &tx)
{
    for (auto txout : tx.vout)
    {
        int idx = txout.scriptPubKey.Find(OP_RETURN);
        if (idx)
        {
            CScript::const_iterator pc(txout.scriptPubKey.begin());
            opcodetype op;
            for (;pc != txout.scriptPubKey.end();)
            {
                if (txout.scriptPubKey.GetOp(pc, op))
                {
                    if (op == OP_RETURN) break;
                }
            }
            if (pc != txout.scriptPubKey.end())
            {
                std::vector<unsigned char> data;
                if (txout.scriptPubKey.GetOp(pc, op, data))
                {
                    // Note this code only works if the size <= 75 (or we'd have OP_PUSHDATAn instead)
                    if (op == invalidOpReturn.size())
                    {
                        if (data == invalidOpReturn)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

