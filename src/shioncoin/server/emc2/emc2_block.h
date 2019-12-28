
/*
 * @copyright
 *
 *  Copyright 2014 Brian Burrell
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

#ifndef __EMC2_BLOCK_H__
#define __EMC2_BLOCK_H__


/**
 * @ingroup sharecoin_emc2
 * @{
 */

#ifdef EMC2_SERVICE

#include <boost/assign/list_of.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <share.h>

class EMC2Block : public CBlock
{
	public:
		// header
		static const int CURRENT_VERSION=2;
		static EMC2_CTxMemPool mempool; 
		static CBlockIndex *pindexBest;
		static CBlockIndex *pindexGenesisBlock;
		static int64 nTimeBestReceived;

		EMC2Block()
		{
			ifaceIndex = EMC2_COIN_IFACE;
			SetNull();
		}

		EMC2Block(const CBlock &block)
		{
			ifaceIndex = EMC2_COIN_IFACE;
			SetNull();
			*((CBlock*)this) = block;
		}

		EMC2Block(const CBlockHeader &header)
		{
			ifaceIndex = EMC2_COIN_IFACE;
			SetNull();
			*((CBlockHeader*)this) = header;
		}

		void SetNull()
		{
			nVersion = EMC2Block::CURRENT_VERSION;
			CBlock::SetNull();
		}

		void InvalidChainFound(CBlockIndex* pindexNew);
		unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast);
		bool AcceptBlock();
		bool IsBestChain();
		CScript GetCoinbaseFlags();
		bool AddToBlockIndex();
		bool CheckBlock();
		bool ReadBlock(uint64_t nHeight);
		bool ReadArchBlock(uint256 hash);
		bool IsOrphan();
		bool Truncate();
		bool VerifyCheckpoint(int nHeight);
		uint64_t GetTotalBlocksEstimate();

		int64_t GetBlockWeight();

		bool CreateCheckpoint(); 

		int GetAlgo() const { return (0); }

		bool SetBestChain(CBlockIndex* pindexNew);

		bool ConnectBlock(CBlockIndex* pindex);

		bool DisconnectBlock(CBlockIndex* pindex);

};



/**
 * A memory pool where an inventory of pending block transactions are stored.
 */
extern EMC2_CTxMemPool EMC2_mempool;

/**
 * The best known tail of the EMC2 block-chain.
 */
extern CBlockIndex* EMC2_pindexBest;

/**
 * The initial block in the EMC2 block-chain's index reference.
 */
extern CBlockIndex* EMC2_pindexGenesisBlock;

/**
 * The block hash of the initial block in the EMC2 block-chain.
 */
extern uint256 emc2_hashGenesisBlock;


extern int EMC2_nBestHeight;
extern uint256 EMC2_hashBestChain;
extern int64 EMC2_nTimeBestReceived;

/**
 * Create a block template with pending inventoried transactions.
 */
CBlock* emc2_CreateNewBlock(const CPubKey& rkey);

/**
 * Generate the inital EMC2 block in the block-chain.
 */
bool emc2_CreateGenesisBlock();

/**
 * Set the best known block hash.
 */
bool emc2_SetBestChain(CBlock *block);

/**
 * Attempt to process an incoming block from a remote EMC2 coin service.
 */
bool emc2_ProcessBlock(CNode* pfrom, CBlock* pblock);

int64 emc2_GetBlockValue(int nHeight, int64 nFees);

CBlockIndex *emc2_GetLastCheckpoint();

bool emc2_IsOrphanBlock(const uint256& hash); 
void emc2_AddOrphanBlock(CBlock *block); 
void emc2_RemoveOrphanBlock(const uint256& hash); 
bool emc2_GetOrphanPrevHash(const uint256& hash, uint256& retPrevHash); 
CBlock *emc2_GetOrphanBlock(const uint256& hash); 
uint256 emc2_GetOrphanRoot(uint256 hash); 

#endif /* def EMC2_SERVICE */

/**
 * @}
 */

#endif /* ndef __EMC2_BLOCK_H__ */
