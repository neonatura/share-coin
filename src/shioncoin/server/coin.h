
/*
 * @copyright
 *
 *  Copyright 2014 Neo Natura
 *
 *  This file is part of ShionCoin.
 *  (https://github.com/neonatura/shioncoin)
 *        
 *  ShionCoin is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version. 
 *
 *  ShionCoin is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ShionCoin.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  @endcopyright
 */  

#ifndef __SERVER__COIN_H__
#define __SERVER__COIN_H__

typedef int64 CAmount;

bool core_VerifyCoinInputs(int ifaceIndex, CTransaction& tx, unsigned int nIn, CTxOut& prev);

bool core_ConnectBlock(CBlock *block, CBlockIndex* pindex);

bool core_DisconnectInputs(int ifaceIndex, CTransaction *tx);

bool core_AcceptPoolTx(int ifaceIndex, CTransaction& tx, bool fCheckInputs);

bool EraseTxCoins(CIface *iface, uint256 hash);

bool WriteTxCoins(uint256 hash, int ifaceIndex, const vector<uint256>& vOuts);

void WriteHashBestChain(CIface *iface, uint256 hash);

bool ReadHashBestChain(CIface *iface, uint256& ret_hash);

bool core_Truncate(CIface *iface, uint256 hash);

bool HasTxCoins(CIface *iface, uint256 hash);

bool UpdateBlockCoins(CBlock& block);

string FormatMoney(int64 n);

bool ParseMoney(const string& str, CAmount& nRet);

bool ParseMoney(const char* pszIn, CAmount& nRet);

bool core_ConnectCoinInputs(int ifaceIndex, CTransaction *tx, const CBlockIndex* pindexBlock, tx_map& mapOutput, map<uint256, CTransaction>& mapTx, int& nSigOps, int64& nFees, bool fVerifySig, bool fVerifyInputs, bool fRequireInputs, CBlock *pBlock);

CIface *GetCoinByHash(uint160 hash);

uint160 GetCoinHash(string name);

#endif /* ndef __SERVER_COIN_H__ */

