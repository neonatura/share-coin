
/*
 * @copyright
 *
 *  Copyright 2018 Brian Burrell
 *
 *  This file is part of Shioncoin.
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

#include "shcoind.h"
#include "wallet.h"
#include "net.h"
#include "strlcpy.h"
#include "testnet_pool.h"
#include "testnet_block.h"
#include "testnet_wallet.h"
#include "testnet_txidx.h"
#include "chain.h"
#include "txsignature.h"

#ifdef WIN32
#include <string.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef fcntl
#undef fcntl
#endif


using namespace std;
using namespace boost;

extern bool testnet_ConnectInputs(CTransaction *tx, MapPrevTx inputs, map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx, const CBlockIndex* pindexBlock, bool fBlock, bool fMiner);
extern double color_CalculatePoolFeePriority(CPool *pool, CPoolTx *ptx, double dFeePrio);



static int64 testnet_CalculateFee(const CTransaction& tx)
{
  CIface *iface = GetCoinByIndex(TESTNET_COIN_IFACE);
  CWallet *wallet = GetWallet(TESTNET_COIN_IFACE);
  int64 nBytes;
  int64 nFee;

  nBytes = wallet->GetVirtualTransactionSize(tx);

  /* base fee */
	nFee = MIN_RELAY_TX_FEE(iface);
  nFee += MIN_TX_FEE(iface) * (nBytes / 1000);

  /* dust penalty */
  BOOST_FOREACH(const CTxOut& out, tx.vout) {
    if (out.nValue < CENT)
      nFee += MIN_TX_FEE(iface);
  }

  nFee = MIN(nFee, (int64)MAX_TRANSACTION_FEE(iface) - 1);

  return (nFee);
}

bool TESTNET_CTxMemPool::revert(CTransaction &tx)
{
  CWallet *pwallet = GetWallet(TESTNET_COIN_IFACE);
  pwallet->EraseFromWallet(tx.GetHash());
  return (tx.DisconnectInputs(TESTNET_COIN_IFACE));
}

int64_t TESTNET_CTxMemPool::GetSoftWeight()
{
  return (TESTNET_MAX_STANDARD_TX_WEIGHT);
}

int64_t TESTNET_CTxMemPool::GetSoftSigOpCost()
{
  return (TESTNET_MAX_STANDARD_TX_SIGOP_COST);
}

bool TESTNET_CTxMemPool::VerifyCoinStandards(CTransaction& tx, tx_cache& mapInputs)
{

  if (tx.IsCoinBase())
    return (true);

  /* SCRIPT_VERIFY_LOW_S */
  for (unsigned int i = 0; i < tx.vin.size(); i++) {
    CTxOut prev;
    if (!tx.GetOutputFor(tx.vin[i], mapInputs, prev))
      continue; /* only verifying what is currently available */

    /* ensure output script is valid */
    vector<vector<unsigned char> > vSolutions;
    txnouttype whichType;
    const CScript& prevScript = prev.scriptPubKey;
    if (!Solver(prevScript, whichType, vSolutions))
      return false;
    if (whichType == TX_NONSTANDARD)
      return false;

    /* evaluate signature */
    vector<vector<unsigned char> > stack;
    CTransaction *txSig = (CTransaction *)this;
    CSignature sig(TESTNET_COIN_IFACE, txSig, i);
    if (!EvalScript(sig, stack, tx.vin[i].scriptSig, SIGVERSION_BASE, SCRIPT_VERIFY_LOW_S)) {
      return (error(SHERR_INVAL, "(testnet) "
            "CTxMemPool.VerifyCoinStandards: error evaluating signature. [SCRIPT_VERIFY_LOW_S]"));
    }
  }

  return (true);
}

void TESTNET_CTxMemPool::EnforceCoinStandards(CTransaction& tx)
{
	CWallet *wallet = GetWallet(TESTNET_COIN_IFACE);

	if (tx.IsFinal(TESTNET_COIN_IFACE))
		return; /* n/a */

	/* detect notary tx */
	const uint256& hPrevTx = tx.vin[0].prevout.hash;
	if (tx.vin.size() == 1 && tx.vout.size() == 1 &&
			std::find(wallet->mapValidateTx.begin(), wallet->mapValidateTx.end(),
				hPrevTx) != wallet->mapValidateTx.end()) {
		CIface *iface = GetCoinByIndex(TESTNET_COIN_IFACE);
		CTransaction prev;

		if (!GetTransaction(iface, hPrevTx, prev, NULL))
			return;

		const CTxOut& out = prev.vout[tx.vin[0].prevout.n];
		if (tx.vout[0].nValue <= CTxMatrix::MAX_NOTARY_TX_VALUE)
			UpdateValidateNotaryTx(iface, tx, out.scriptPubKey);
	}

}

bool TESTNET_CTxMemPool::AcceptTx(CTransaction& tx)
{

	if (IsAltChainTx(tx)) {
		CommitAltChainPoolTx(GetCoinByIndex(TESTNET_COIN_IFACE), tx, true);

	}

  return true;
}


static int64 testnet_CalculateSoftFee(CTransaction& tx)
{
  CIface *iface = GetCoinByIndex(TESTNET_COIN_IFACE);
  CWallet *wallet = GetWallet(iface);
  int64 nBytes;
  int64 nFee;

  nBytes = wallet->GetVirtualTransactionSize(tx);

  /* base fee */
	nFee = (int64)MIN_RELAY_TX_FEE(iface);
  nFee += wallet->GetFeeRate() * (nBytes / 1000);

  /* dust penalty */
  BOOST_FOREACH(const CTxOut& out, tx.vout) {
    if (out.nValue < CENT)
      nFee += MIN_TX_FEE(iface);
  }

  nFee = MIN(nFee, (int64)MAX_TRANSACTION_FEE(iface) - 1);

  return (nFee);
}


int64 TESTNET_CTxMemPool::CalculateSoftFee(CTransaction& tx)
{
  return (testnet_CalculateSoftFee(tx));
}

int64 TESTNET_CTxMemPool::IsFreeRelay(CTransaction& tx, tx_cache& mapInputs)
{
	return (true);
}

double TESTNET_CTxMemPool::CalculateFeePriority(CPoolTx *ptx)
{
	double dFeePrio;

	dFeePrio = sqrt(ptx->dPriority + 1) * (double)ptx->nFee;

	/* alt-chain priority adjustment */
	dFeePrio = color_CalculatePoolFeePriority(this, ptx, dFeePrio);

	/* DEBUG: TODO: EXEC.. */

	return (dFeePrio);
}
