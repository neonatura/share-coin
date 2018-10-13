
/*
 * @copyright
 *
 *  Copyright 2014 Neo Natura
 *
 *  This file is part of the Share Library.
 *  (https://github.com/neonatura/share)
 *        
 *  The Share Library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version. 
 *
 *  The Share Library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Share Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  @endcopyright
 */  

#undef GNULIB_NAMESPACE
#include "shcoind.h"

#include "init.h"
#include "ui_interface.h"
#include "base58.h"
#include "../server_iface.h" /* BLKERR_XXX */
#include "addrman.h"
#include "util.h"
#include "chain.h"
#include "rpc_proto.h"
#include "txmempool.h"
#include "color/color_pool.h"
#include "color/color_block.h"

using namespace std;
using namespace boost;
using namespace json_spirit;

extern int64 AmountFromValue(const Value& value);
extern string AccountFromValue(const Value& value);
extern CBlockIndex *GetBestColorBlockIndex(CIface *iface, uint160 hColor);
extern bool GetColorBlockHeight(CBlockIndex *pindex, unsigned int& nHeight);
extern double GetDifficulty(int ifaceIndex, const CBlockIndex* blockindex = NULL);


static uint160 rpc_alt_key_from_value(CIface *iface, Value val)
{
	string text = val.get_str();
	string strDesc;
	uint160 hColor;

	hColor = 0;
	if (text.size() == 40) {
		hColor = uint160(text);
	}
	if (hColor == 0) {
		hColor = GetAltColorHash(iface, text, strDesc);
	}

	return (hColor);
}

Value rpc_alt_addr(CIface *iface, const Array& params, bool fStratum) 
{
	CWallet *wallet = GetWallet(iface);
	string strAccount("");
	CPubKey pubkey;
	uint160 hColor;

  if (params.size() == 0 || params.size() > 2)
    throw runtime_error("rpc_alt_addr");

	hColor = rpc_alt_key_from_value(iface, params[0]);
	if (params.size() > 1)
		strAccount = AccountFromValue(params[1]);

	pubkey = GetAltChainAddr(hColor, strAccount, true);
	CCoinAddr addrRet(COLOR_COIN_IFACE, pubkey.GetID()); 
	return (addrRet.ToString());
}

Value rpc_alt_addrlist(CIface *iface, const Array& params, bool fStratum) 
{
  CWallet *wallet = GetWallet(COLOR_COIN_IFACE);
	string strAccount("");
	uint160 hColor;

  if (params.size() == 0 || params.size() > 2)
    throw runtime_error("rpc_alt_addrlist");

	hColor = rpc_alt_key_from_value(iface, params[0]); /* not used */
	if (params.size() > 1)
		strAccount = AccountFromValue(params[1]);

  Array ret;
  BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& item,
			wallet->mapAddressBook) {
    const CCoinAddr& address = CCoinAddr(COLOR_COIN_IFACE, item.first);
    const string& strName = item.second;
    if (strName == strAccount)
      ret.push_back(address.ToString());
  }

  if (strAccount.length() == 0) {
    std::set<CKeyID> keys;
    wallet->GetKeys(keys);
    BOOST_FOREACH(const CKeyID& key, keys) {
      if (wallet->mapAddressBook.count(key) == 0) { /* loner */
        CCoinAddr addr(COLOR_COIN_IFACE, key);
        ret.push_back(addr.ToString());
      }
    }
  }

  return ret;
}

Value rpc_alt_balance(CIface *iface, const Array& params, bool fStratum) 
{
	CWallet *wallet = GetWallet(iface);
	string strAccount;
	uint160 hColor;
	int64 nBalance = 0;
	int nMinDepth = 1;

  if (params.size() > 3)
    throw runtime_error("rpc_alt_balance");

	hColor = rpc_alt_key_from_value(iface, params[0]);
	if (params.size() > 1)
		strAccount = AccountFromValue(params[1]);
	if (params.size() > 2)
		nMinDepth = params[2].get_int();

	{
		vector <COutput> vCoins;
		wallet->AvailableAccountCoins(strAccount, vCoins, nMinDepth == 0 ? false : true, hColor);
		BOOST_FOREACH(const COutput& out, vCoins) {
			nBalance += out.tx->vout[out.i].nValue;
		}
	}

	return ((double)nBalance/COIN);
}

Value rpc_alt_color(CIface *iface, const Array& params, bool fStratum) 
{
	CBlockIndex *pindex;
	uint160 hColor;
	string strTitle;
	string strDesc;
	uint32_t r, g, b, a;
	char buf[256];

  if (params.size() != 1)
    throw runtime_error("rpc_alt_color");

	strTitle = params[0].get_str();
	hColor = GetAltColorHash(iface, strTitle, strDesc); 

	Object ret;

#if 0
	pindex = GetBestColorBlockIndex(iface, hColor);
	if (pindex) {
		ret.push_back(Pair("block", pindex->GetBlockHash().GetHex()));
	}
#endif

	GetAltColorCode(hColor, &r, &g, &b, &a);
	sprintf(buf, "#%-2.2X%-2.2X%-2.2X", (r >> 24), (g >> 24), (b >> 24));
	ret.push_back(Pair("code", string(buf)));

	ret.push_back(Pair("colorhash", hColor.GetHex()));
	ret.push_back(Pair("color name", strDesc));
	ret.push_back(Pair("symbol", GetAltColorHashAbrev(hColor)));
	ret.push_back(Pair("title", strTitle));

	return ret;
}

static double print_rpc_difficulty(CBigNum val)
{
{
	unsigned int nBits = val.GetCompact();
	int nShift = (nBits >> 24) & 0xff;

	double dDiff =
		(double)0x0000ffff / (double)(nBits & 0x00ffffff);

	while (nShift < 29)
	{
		dDiff *= 256.0;
		nShift++;
	}
	while (nShift > 29)
	{
		dDiff /= 256.0;
		nShift--;
	}

	return dDiff;
}

}

Value rpc_alt_info(CIface *iface, const Array& params, bool fStratum) 
{
	CIface *alt_iface = GetCoinByIndex(COLOR_COIN_IFACE);
	CWallet *alt_wallet = GetWallet(COLOR_COIN_IFACE);
	CBlockIndex *pindex;
	uint160 hColor = 0;
	unsigned int nHeight = 0;
	uint32_t r, g, b, a;
	char buf[256];

  if (params.size() != 1)
    throw runtime_error("rpc_alt_info");

	hColor = rpc_alt_key_from_value(iface, params[0]);
	if (hColor == 0) {
    throw JSONRPCError(SHERR_INVAL, "invalid color hash");
	}

	pindex = GetBestColorBlockIndex(iface, hColor);
	if (!pindex) {
    throw JSONRPCError(SHERR_INVAL, "unestablished block-chain");
	}

	GetColorBlockHeight(pindex, nHeight);
	GetAltColorCode(hColor, &r, &g, &b, &a);


	/* custom options */
	int64 nBlockValueRate = color_GetBlockValueRate(hColor);
	int64 nBlockValueBase = color_GetBlockValueBase(hColor);
	int64 nBlockTarget = color_GetBlockTarget(hColor);
	int64 nCoinbaseMaturity = color_GetCoinbaseMaturity(hColor);
	int64 nMinTxFee = color_GetMinTxFee(hColor);
	CBigNum bnMinDifficulty = color_GetMinDifficulty(hColor);


	Object obj;

	sprintf(buf, "#%-2.2X%-2.2X%-2.2X", (r >> 24), (g >> 24), (b >> 24));
	obj.push_back(Pair("colorcode", string(buf)));

	obj.push_back(Pair("colorhash", hColor.GetHex()));

	obj.push_back(Pair("blockversion",  (int)alt_iface->block_ver));

	obj.push_back(Pair("blocks", (int)nHeight));

	obj.push_back(Pair("blocktarget", (int)nBlockTarget));
	obj.push_back(Pair("blockvaluerate", (int)nBlockValueRate));
	obj.push_back(Pair("blockvaluebase", ((double)nBlockValueBase/COIN)));

	obj.push_back(Pair("currentblockhash", pindex->GetBlockHash().GetHex()));

	obj.push_back(Pair("difficulty",
				(double)GetDifficulty(COLOR_COIN_IFACE, pindex)));

	obj.push_back(Pair("min-difficulty", 
				print_rpc_difficulty(bnMinDifficulty)));

	obj.push_back(Pair("min-txfee", ((double)nMinTxFee/COIN)));

	obj.push_back(Pair("maturity", (int)nCoinbaseMaturity));

	CTxMemPool *pool = GetTxMemPool(alt_iface);
	obj.push_back(Pair("pooledtx",      (uint64_t)pool->size()));

	obj.push_back(Pair("symbol", GetAltColorHashAbrev(hColor)));

	obj.push_back(Pair("version",       (int)alt_iface->proto_ver));

	obj.push_back(Pair("walletversion", alt_wallet->GetVersion()));

	int nAddrTotal = 0;
	string hex = hColor.GetHex();
  BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& item, alt_wallet->mapAddressBook)
  {
    const CTxDestination& address = item.first;
    const string& strName = item.second;
    if (strName == hex) {
			nAddrTotal++;
		}
  }
	obj.push_back(Pair("walletaddr", (int)nAddrTotal));

	int nTxTotal = 0;
	BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, alt_wallet->mapWallet) {
		CWalletTx& wtx = item.second;
		if (wtx.GetColor() == hColor)
			nTxTotal++;
	}
	obj.push_back(Pair("wallettx", (int)nTxTotal));

	return (obj);
}

Value rpc_alt_mine(CIface *iface, const Array& params, bool fStratum) 
{
  CWallet *wallet = GetWallet(iface);
  int ifaceIndex = GetCoinIndex(iface);
	string strAccount;
	uint160 hColor;
	int64 nValue;
	int err;

  if (params.size() == 0)
    throw runtime_error("rpc_alt_mine");

	hColor = rpc_alt_key_from_value(iface, params[0]);
	if (params.size() == 1)
		strAccount = "";
	else
		strAccount = AccountFromValue(params[1]);

	CWalletTx wtx;
	vector<CTransaction> vAltTx;
	err = update_altchain_tx(iface, strAccount, hColor, vAltTx, wtx); 
	if (err)
    throw JSONRPCError(err, "update_altchain_tx");

	return wtx.ToValue(ifaceIndex);
}

static void _split_token(string tok, string& mode_str, int& val)
{
	string delim("=");

	mode_str = "";
	val = 0;

	if (tok.find(delim) == string::npos)
		return;

	mode_str = tok.substr(0, tok.find(delim));
	val = atoi(tok.substr(tok.find(delim) + 1).c_str());
}

Value rpc_alt_new(CIface *iface, const Array& params, bool fStratum) 
{
  CWallet *wallet = GetWallet(iface);
  int ifaceIndex = GetCoinIndex(iface);
	string strAccount;
	uint160 hColor;
	int64 nValue;
	int err;

  if (params.size() == 0)
    throw runtime_error("rpc_alt_mine");

	/* block-chain color */
	hColor = rpc_alt_key_from_value(iface, params[0]);

	/* account on main-chain to deduct altchain-tx fee. */
	if (params.size() == 1)
		strAccount = "";
	else
		strAccount = AccountFromValue(params[1]);

	/* establish block-chain parameters. */
	color_opt opt;
	for (int i = 2; i < params.size(); i++) {
		string tok = params[i].get_str();
		string mode_str;
		int val;
		_split_token(tok, mode_str, val);
		if (mode_str == "" || val == 0)
			continue;
		if (mode_str == "difficulty")
			SetColorOpt(opt, CLROPT_DIFFICULTY, MIN(8, val));
		else if (mode_str == "blocktarget")
			SetColorOpt(opt, CLROPT_BLOCKTARGET, MIN(15, val));
		else if (mode_str == "maturity")
			SetColorOpt(opt, CLROPT_MATURITY, MIN(8, val));
		else if (mode_str == "rewardbase")
			SetColorOpt(opt, CLROPT_REWARDBASE, MIN(10, val));
		else if (mode_str == "rewardhalf")
			SetColorOpt(opt, CLROPT_REWARDHALF, MIN(15, val));
		else if (mode_str == "txfee")
			SetColorOpt(opt, CLROPT_TXFEE, MIN(8, val));
	}

	CWalletTx wtx;
	err = init_altchain_tx(iface, strAccount, hColor, opt, wtx);
	if (err)
    throw JSONRPCError(err, "init_altchain_tx");

	return wtx.ToValue(ifaceIndex);
}

Value rpc_alt_send(CIface *iface, const Array& params, bool fStratum) 
{
  CWallet *wallet = GetWallet(iface);
  int ifaceIndex = GetCoinIndex(iface);
	string strAccount;
	uint160 hColor;
	int64 nValue;
	int err;

  if (params.size() < 3 || params.size() > 4)
    throw runtime_error("rpc_alt_send");

	hColor = rpc_alt_key_from_value(iface, params[0]);
	nValue = AmountFromValue(params[2]);

	strAccount = "";
	if (params.size() == 4)
		strAccount = AccountFromValue(params[3]);

	CWalletTx wtx;
	CCoinAddr addr(params[1].get_str());
	err = update_altchain_tx(iface, strAccount, hColor, addr, nValue, wtx); 
	if (err)
    throw JSONRPCError(err, "update_altchain_tx");

	return wtx.ToValue(ifaceIndex);
}

/* commit a pre-formed block to an alt block-chain */
Value rpc_alt_commit(CIface *iface, const Array& params, bool fStratum) 
{
  CWallet *pwalletMain = GetWallet(iface);
  int ifaceIndex = GetCoinIndex(iface);

	if (fStratum)
		throw runtime_error("unsupported operation");

  if (params.size() == 0 || params.size() > 2)
    throw runtime_error(
        "alt.commit data [coinbase]\n"
        );

#if 0
  if (IsInitialBlockDownload(ifaceIndex))
    throw JSONRPCError(-10, "coin service is downloading blocks...");
#endif

  typedef map<uint256, pair<CBlock*, CScript> > mapNewBlock_t;
  static mapNewBlock_t mapNewBlock;
  static vector<CBlock*> vNewBlock;

	// Parse parameters
	vector<unsigned char> vchData = ParseHex(params[0].get_str());
	vector<unsigned char> coinbase;

	if(params.size() == 2)
		coinbase = ParseHex(params[1].get_str());

	if (vchData.size() != 128)
		throw JSONRPCError(-8, "Invalid parameter");

	CBlock* pdata = (CBlock*)&vchData[0];

	// Byte reverse
	for (int i = 0; i < 128/4; i++)
		((unsigned int*)pdata)[i] = ByteReverse(((unsigned int*)pdata)[i]);

	// Get saved block
	if (!mapNewBlock.count(pdata->hashMerkleRoot))
		return false;
	CBlock* pblock = mapNewBlock[pdata->hashMerkleRoot].first;

	pblock->nTime = pdata->nTime;
	pblock->nNonce = pdata->nNonce;

	if(coinbase.size() == 0)
		pblock->vtx[0].vin[0].scriptSig = mapNewBlock[pdata->hashMerkleRoot].second;
	else
		CDataStream(coinbase, SER_NETWORK, PROTOCOL_VERSION(iface)) >> pblock->vtx[0]; // FIXME - HACK!

	pblock->hashMerkleRoot = pblock->BuildMerkleTree();

	return CheckWork(pblock, *pwalletMain);
}


Value rpc_alt_block(CIface *iface, const Array& params, bool fStratum)
{
	CIface *alt_iface = GetCoinByIndex(COLOR_COIN_IFACE);
	CBlockIndex *pindex;
	CBlock *block;

  if (params.size() != 1)
    throw runtime_error(
        "alt.block block-hash\n"
        );

	const uint256& hashBlock = uint256(params[0].get_str());
	pindex = GetBlockIndexByHash(COLOR_COIN_IFACE, hashBlock);
	if (!pindex) {
		throw JSONRPCError(SHERR_NOENT, "unknown block hash");
	}

	block = GetBlockByHash(alt_iface, hashBlock);
	if (!block) {
		throw JSONRPCError(SHERR_IO, "error loading block from alt-chain");
	}

	Value ret = block->ToValue();
	delete block;

	return (ret);
}

Value rpc_alt_tx(CIface *iface, const Array& params, bool fStratum)
{
	CIface *alt_iface = GetCoinByIndex(COLOR_COIN_IFACE);
	CBlockIndex *pindex;
	CBlock *block;

  if (params.size() != 1)
    throw runtime_error(
        "alt.tx tx-hash\n"
        );

	const uint256& hashTx = uint256(params[0].get_str());

	CTransaction tx;
	uint256 hBlock;
	if (!GetTransaction(alt_iface, hashTx, tx, &hBlock))
		throw JSONRPCError(SHERR_IO, "error loading tx from alt-chain");

	Value val(tx.ToValue(COLOR_COIN_IFACE));
	Object obj = val.get_obj();
	obj.push_back(Pair("blockhash", hBlock.GetHex()));

	return (obj);
}
