
/*
 * @copyright
 *
 *  Copyright 2016 Neo Natura
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

#ifndef __COIN_PROTO_H__
#define __COIN_PROTO_H__

#include "blockchain/bc.h"




#ifdef __cplusplus
#include <vector>
extern "C" {
#endif

#define BASE58_DEFAULT_PUBKEY_ADDRESS 62 /* S */
#define BASE58_DEFAULT_SCRIPT_ADDRESS 5
#define BASE58_DEFAULT_SECRET_KEY 176

#define COIN_IFACE_VERSION(_maj, _min, _rev, _bui) \
  ( \
   (1000000 * (_maj)) + \
   (10000 * (_min)) + \
   (100 * (_rev)) + \
   (1 * (_bui)) \
  )


#define SCALE_FACTOR 4


#define MAX_BLOCK_SIGOPS(_iface) \
  ((_iface)->max_sigops)

#define MAX_BLOCK_SIGOP_COST(_iface) \
  ((_iface)->max_sigops * 4)

#define MAX_TX_SIGOP_COST(_iface) \
  (MAX_BLOCK_SIGOP_COST(_iface) / 5)

#define MAX_BLOCK_SIZE(_iface) \
  ((_iface)->max_block_size)
#define DEFAULT_MAX_BLOCK_SIZE(_iface) \
  ((_iface)->def_max_block_size)

#define MAX_BLOCK_WEIGHT(_iface) \
  ((_iface)->max_block_size * 4)

#define MAX_ORPHAN_TRANSACTIONS(_iface) \
  ((_iface)->max_orphan_tx)

#define MAX_TRANSACTION_WEIGHT(_iface) \
  ((_iface)->max_tx_weight)

#define MAX_TX_FEE(_iface) \
  ((_iface)->max_tx_fee)

#define MAX_TRANSACTION_FEE(_iface) \
  (MAX_TX_FEE(_iface))

#define MAX_MONEY(_iface) \
  ((_iface)->max_money)

#define MIN_TX_FEE(_iface) \
  (int64)((_iface) ? ((_iface)->min_tx_fee) : 0)

#define MIN_TX_FEE_RATE(_iface) \
  (MIN_TX_FEE(_iface))

/**
 * The minimum fee applied to a tranaction.
 */
#define MIN_RELAY_TX_FEE(_iface) \
	(int64)( !(_iface) ? 0 : (_iface)->min_relay_fee )
#define DEFAULT_MIN_RELAY_TX_FEE(_iface) \
	(int64)( !(_iface) ? 0 : (_iface)->min_relay_fee )

#define DUST_RELAY_TX_FEE(_iface) \
	(int64)((_iface)->min_relay_fee * 3)

/**
 * The minimum coin value allowed to be transfered in a single transaction.
 */
#define MIN_INPUT_VALUE(_iface) \
  (int64)(iface ? ((_iface)->min_input) : 0)

#define COIN_SERVICES(_iface) \
  ((_iface) ? (_iface)->services : 0)

#define MAX_FREE_TX_SIZE(_iface) \
  ((_iface)->max_free_tx_size)


/** A self-test interface for verifying stability. */
#define TEST_COIN_IFACE 0

/** The "ShionCoin" virtual currency. */
#define SHC_COIN_IFACE 1

/** The "EMC2" (Einstienium) virtual currency. */
#define EMC2_COIN_IFACE 3

/** The "LTC" (Litecoin) virtual currency. */
#define LTC_COIN_IFACE 4

/** The "ShionCoin" Test Network */
#define TESTNET_COIN_IFACE 5

/** The "ShionCoin Color" alternate block-chain. */
#define COLOR_COIN_IFACE 6

#define MAX_COIN_IFACE 7


#define COINF_DL_SCAN (1 << 0)
#define COINF_DL_SYNC (1 << 1)
#define COINF_WALLET_SCAN (1 << 3)
#define COINF_WALLET_SYNC (1 << 4)
#define COINF_PEER_SCAN (1 << 5)
#define COINF_PEER_SYNC (1 << 6)
#define COINF_VALIDATE_SCAN (1 << 7)
#define COINF_VALIDATE_SYNC (1 << 8)


#define STAT_BLOCK_ACCEPTS(_iface) (_iface)->stat.tot_block_accept
#define STAT_BLOCK_SUBMITS(_iface) (_iface)->stat.tot_block_submit
#define STAT_BLOCK_ORPHAN(_iface) (_iface)->stat.tot_block_orphan
#define STAT_TX_ACCEPTS(_iface) (_iface)->stat.tot_tx_accept
#define STAT_TX_SUBMITS(_iface) (_iface)->stat.tot_tx_submit
#define STAT_TX_RETURNS(_iface) (_iface)->stat.tot_tx_return
#define STAT_TX_MINT(_iface) (_iface)->stat.tot_tx_mint

struct coin_iface_t;
typedef int (*coin_f)(struct coin_iface_t * /*iface*/, void * /* arg */);
#define COINF(_f) ((coin_f)(_f))

#define HEADER_PREFIX(_iface) \
  ((_iface)->hdr_magic)


#define BASE58_PUBKEY_ADDRESS(_iface) \
	((_iface) ? (_iface)->base58_pubkey_address : BASE58_DEFAULT_PUBKEY_ADDRESS)
#define BASE58_SCRIPT_ADDRESS(_iface) \
	((_iface) ? (_iface)->base58_script_address : BASE58_DEFAULT_SCRIPT_ADDRESS)
#define BASE58_SCRIPT_ADDRESS_2(_iface) \
	((_iface) ? (_iface)->base58_script_address2 : 0)
#define BASE58_SECRET_KEY(_iface) \
	((_iface) ? (_iface)->base58_secret_key : (BASE58_DEFAULT_PUBKEY_ADDRESS + 128))
#define BASE58_EXT_PUBLIC_KEY(_iface) \
	((_iface)->base58_ext_public_key)
#define BASE58_EXT_SECRET_KEY(_iface) \
	((_iface)->base58_ext_secret_key)


#define MAX_SCRIPT_SIZE(_iface) \
  ((_iface) ? (_iface)->max_script_size : 10000)

#define MAX_SCRIPT_ELEMENT_SIZE(_iface) \
  ((_iface) ? (_iface)->max_script_element_size : 520)


/* shared traits for services that support */
/** Number of blocks that can be requested at any given time from a single peer. */ 
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  *  less than this number, we reached their tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *   *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *    *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    DEPLOYMENT_RESERVED_0,
    DEPLOYMENT_RESERVED_1,
    DEPLOYMENT_RESERVED_2,
    DEPLOYMENT_ALGO,
    DEPLOYMENT_PARAM,
    DEPLOYMENT_BOLO,
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
  /** Bit position to select the particular bit in nVersion. */
  int bit;
  /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
  int64_t nStartTime;
  /** Timeout/expiry MedianTime for the deployment attempt. */
  int64_t nTimeout;
};
typedef struct BIP9Deployment BIP9Deployment;



/**
 * A coin interface provides a specialized means to perform service operations.
 */
typedef struct coin_iface_t
{
  /* lowercase 'common' name of currency */
  char name[MAX_SHARE_NAME_LENGTH];
  int enabled;
  int client_ver;
  int block_ver;
  int proto_ver;

  /* socket */
  int port;


  unsigned char hdr_magic[4];

	unsigned char base58_pubkey_address;
	unsigned char base58_script_address;
	unsigned char base58_script_address2;
	unsigned char base58_secret_key;
	unsigned char base58_ext_public_key[4];
	unsigned char base58_ext_secret_key[4];

  uint64_t services; /* NODE_XXX */
  uint64_t min_input;
  uint64_t max_block_size;
  uint64_t def_max_block_size;
  uint64_t max_orphan_tx;
  uint64_t max_tx_weight;
  uint64_t min_tx_fee;
  uint64_t min_relay_fee;
  uint64_t def_min_relay_fee;
  uint64_t max_tx_fee;
  uint64_t max_free_tx_size;
  uint64_t max_money;
  uint64_t coinbase_maturity;
  uint64_t max_sigops;
	uint64_t max_script_size;
	uint64_t max_script_element_size;

  /* coin operations */
  coin_f op_init;
  coin_f op_bind;
  coin_f op_term;
  coin_f op_msg_recv;
  coin_f op_msg_send;
  coin_f op_peer_add;
  coin_f op_peer_recv;
  coin_f op_block_new;
  coin_f op_block_process;
  coin_f op_block_templ;
  coin_f op_tx_new;
  coin_f op_tx_pool;

	/** Block height at which BIP16 becomes active */
	int BIP16Height;
	/** Block height and hash at which BIP30 becomes active */
	int BIP30Height;
	/** Block height and hash at which BIP34 becomes active */
	int BIP34Height;
	/** Block height at which BIP65 becomes active */
	int BIP65Height;
	/** Block height at which BIP66 becomes active */
	int BIP66Height;
  /* BIP consensus */
  uint32_t nRuleChangeActivationThreshold;
  uint32_t nMinerConfirmationWindow;
  BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];  

  bc_t *bc_block;
  bc_t *bc_tx;
  bc_t *bc_wtx;
  bc_t *bc_coin;
  double blk_diff; /* next block difficulty */
  uint64_t tx_tot; /* nTransactionsUpdated */
  bc_hash_t block_besthash; /* best block hash */
  time_t net_valid;
  time_t net_invalid;
  uint32_t blockscan_max;
  time_t work_stamp;
  int flags;

  struct coin_stat_t {
    uint64_t tot_block_submit;
    uint64_t tot_block_accept;
    uint64_t tot_block_orphan;
    uint64_t tot_tx_submit;
    uint64_t tot_tx_accept;
		uint64_t tot_tx_return;
		uint64_t tot_tx_mint;
    uint64_t tot_spring_submit;
    uint64_t tot_spring_accept;
  } stat;


} coin_iface_t;

typedef struct coin_iface_t CIface;

typedef struct coinhdr_t
{
  unsigned char magic[4];
  char cmd[12];
  uint32_t size;
  uint32_t crc;
} coinhdr_t;
#define SIZEOF_COINHDR_T 24

/**
 * Obtain a numerical attribute for a coin interface
 */
int GetCoinAttr(const char *name, char *attr);

/**
 * Get the defined index for a specified coin interface.
 */
int GetCoinIndex(coin_iface_t *iface);

/**
 * Obtain a coin interface by it's defined index.
 * @see SHC_COIN_IFACE
 */
coin_iface_t *GetCoinByIndex(int index);

/**
 * Obtain a coin interface by it's common lowercase code.
 */
coin_iface_t *GetCoin(const char *name);


/**
 * The SHC currency coin service.
 * @ingroup sharecoin
 * @defgroup sharecoin_shc The SHC currency coin service.
 * @{
 */
#include "shc_proto.h"
/**
 * @}
 */

/**
 * The "LTC" (Litecoin) virtual currency.
 * @ingroup sharecoin
 * @defgroup sharecoin_ltc The "LTC" (Litecoin) virtual currency.
 * @{
 */
#include "ltc_proto.h"
/**
 * @}
 */

/**
 * The SHC currency Test Network service.
 * @ingroup sharecoin
 * @defgroup sharecoin_testnet The SHC currency Test Network service.
 * @{
 */
#include "testnet_proto.h"
/**
 * @}
 */

/**
 * The SHC currency Color alternate block-chain.
 * @ingroup sharecoin
 * @defgroup sharecoin_color The SHC currency Color alternate block-chain.
 * @{
 */
#include "color_proto.h"
/**
 * @}
 */


bc_t *GetBlockChain(CIface *iface);

bc_t *GetBlockTxChain(CIface *iface);

bc_t *GetWalletTxChain(CIface *iface);

bc_t *GetBlockCoinChain(CIface *iface);


#ifdef __cplusplus
}

class CNode;
typedef std::vector<CNode *> NodeList;
NodeList& GetNodeList(int ifaceIndex);
NodeList& GetNodeList(CIface *iface);
#endif




#endif /* ndef __COIN_PROTO_H__ */
