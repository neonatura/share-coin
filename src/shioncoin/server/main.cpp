
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

#include "shcoind.h"
#include "db.h"
#include "net.h"
#include "init.h"
#include "ui_interface.h"
#include "block.h"
#include "script.h"
#include "txsignature.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

//
// Global state
//


CCriticalSection cs_main;




// Settings
int64 nTransactionFee = 0;


CMedianFilter<int> cPeerBlockCounts(5, 0); // Amount of blocks that other nodes claim to have



bool CTransaction::ReadFromDisk(int ifaceIndex, COutPoint prevout)
{
  return (ReadTx(ifaceIndex, prevout.hash));
}

//
// Check transaction inputs, and make sure any
// pay-to-script-hash transactions are evaluating IsStandard scripts
//
// Why bother? To avoid denial-of-service attacks; an attacker
// can submit a standard HASH... OP_EQUAL transaction,
// which will get accepted into blocks. The redemption
// script can be anything; an attacker could use a very
// expensive-to-check-upon-redemption script like:
//   DUP CHECKSIG DROP ... repeated 100 times... OP_1
//
bool CTransaction::AreInputsStandard(int ifaceIndex, const MapPrevTx& mapInputs) const
{

  if (IsCoinBase())
    return true; // Coinbases don't use vin normally

  for (unsigned int i = 0; i < vin.size(); i++)
  {
    const CTxOut& prev = GetOutputFor(vin[i], mapInputs);

    vector<vector<unsigned char> > vSolutions;
    txnouttype whichType;
    // get the scriptPubKey corresponding to this input:
    const CScript& prevScript = prev.scriptPubKey;
    if (!Solver(prevScript, whichType, vSolutions)) {
      return false;
    }

    int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
    if (nArgsExpected < 0) {
      return false;
    }

    vector<vector<unsigned char> > stack;
    CTransaction *txSig = (CTransaction *)this;
    CSignature sig(ifaceIndex, txSig, i);
    if (!EvalScript(sig, stack, vin[i].scriptSig, SIGVERSION_BASE, 0)) {
      return false;
    }

    if (whichType == TX_SCRIPTHASH)
    {
      if (stack.empty()) {
        return false;
      }
      CScript subscript(stack.back().begin(), stack.back().end());
      vector<vector<unsigned char> > vSolutions2;
      txnouttype whichType2;
      if (!Solver(subscript, whichType2, vSolutions2)) {
        return (error(SHERR_INVAL, "CTransaction.AreInputStandard: TX_SCRIPT_HASH: !Solver\n"));
      }
      if (whichType2 == TX_SCRIPTHASH) {
        return (error(SHERR_INVAL, "CTransaction.AreInputStandard: TX_SCRIPT_HASH: whichType2 == TX_SCRIPTHASH\n"));
      }

      if (subscript.GetSigOpCount(true) > 15) {
        return (SHERR_INVAL, "AreInputsStandard: transaction exceeded 15 sig-ops for a P2SH script."); 
      }

      int tmpExpected;
      tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
      if (tmpExpected < 0) {
        return false;
      }
      nArgsExpected += tmpExpected;
    }

    if (stack.size() > (unsigned int)nArgsExpected) {
      return false;
    }
  }

  return true;
}

unsigned int
CTransaction::GetLegacySigOpCount() const
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

const CTxOut& CTransaction::GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const
{
  char errbuf[1024];

  MapPrevTx::const_iterator mi = inputs.find(input.prevout.hash);
  if (mi == inputs.end()) {
    /* TODO: check how critical this is */
    throw std::runtime_error("CTransaction::GetOutputFor() : prevout.hash not found");
  }

  const CTransaction& txPrev = (mi->second).second;
  if (input.prevout.n >= txPrev.vout.size()) {
    /* TODO: check how critical this is */
    sprintf(errbuf, "Transaction::GetOutputFor: prevout(%s).n(%d) out of range (>= %d)", input.prevout.hash.GetHex().c_str(), (int)input.prevout.n, (int)txPrev.vout.size());
    throw std::runtime_error((const char *)errbuf);
  }

  return txPrev.vout[input.prevout.n];
}

bool CTransaction::GetOutputFor(const CTxIn& input, tx_cache& inputs, CTxOut& retOut)
{
  char errbuf[1024];

  tx_cache::const_iterator mi = inputs.find(input.prevout.hash);
  if (mi == inputs.end())
    return (false); 

  const CTransaction& txPrev = (mi->second);
  if (input.prevout.n >= txPrev.vout.size())
    return (false);

  retOut = txPrev.vout[input.prevout.n];
  return (true);
}

int64 CTransaction::GetValueIn(tx_cache& inputs)
{

  if (IsCoinBase())
    return 0;

  int64 nResult = 0;
  for (unsigned int i = 0; i < vin.size(); i++) {
    CTxOut out;
    const CTxIn& in = vin[i];
    if (!GetOutputFor(in, inputs, out)) {
      error(SHERR_INVAL, "CTransaction.GetValueIn: warning: unknown input tx \"%s\" [x%d].", in.prevout.hash.GetHex().c_str(), inputs.size());
      continue;
    }
    nResult += out.nValue;
  }

  return nResult;
}

int64 CTransaction::GetValueIn(const MapPrevTx& inputs)
{
  int64 nResult = 0;
  for (unsigned int i = 0; i < vin.size(); i++)
  {
    nResult += GetOutputFor(vin[i], inputs).nValue;
  }
  return nResult;
}

unsigned int CTransaction::GetP2SHSigOpCount(const MapPrevTx& inputs) const
{
    if (IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        const CTxOut& prevout = GetOutputFor(vin[i], inputs);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
    }
    return nSigOps;
}


bool CheckDiskSpace(uint64 nAdditionalBytes)
{
    uint64 nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < nMinDiskSpace + nAdditionalBytes)
    {
        fShutdown = true;
        string strMessage = _("Warning: Disk space is low");
        strMiscWarning = strMessage;
        printf("*** %s\n", strMessage.c_str());
        StartServerShutdown();
        return false;
    }
    return true;
}

FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
{
    if ((nFile < 1) || (nFile == (unsigned int) -1))
        return NULL;
    FILE* file = fopen((GetDataDir() / strprintf("blk%04d.dat", nFile)).string().c_str(), pszMode);
    if (!file)
        return NULL;
    if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
    {
        if (fseek(file, nBlockPos, SEEK_SET) != 0)
        {
            fclose(file);
            return NULL;
        }
    }
    return file;
}

static unsigned int nCurrentBlockFile = 1;

FILE* AppendBlockFile(unsigned int& nFileRet)
{
    nFileRet = 0;
    loop
    {
        FILE* file = OpenBlockFile(nCurrentBlockFile, 0, "ab");
        if (!file)
            return NULL;
        if (fseek(file, 0, SEEK_END) != 0)
            return NULL;
        // FAT32 filesize max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
        if (ftell(file) < 0x7F000000 - MAX_SIZE)
        {
            nFileRet = nCurrentBlockFile;
            return file;
        }
        fclose(file);
        nCurrentBlockFile++;
    }
}









//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

string GetWarnings(int ifaceIndex, string strFor)
{
  CBlockIndex *pindexBest = GetBestBlockIndex(ifaceIndex);
  int nPriority = 0;
  string strStatusBar;
  string strRPC;
  if (GetBoolArg("-testsafemode"))
    strRPC = "test";

  // Misc warnings like out of disk space and clock is wrong
  if (strMiscWarning != "")
  {
    nPriority = 1000;
    strStatusBar = strMiscWarning;
  }

  // Alerts
  {
    LOCK(cs_mapAlerts);
    BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
    {
      const CAlert& alert = item.second;
      if (alert.AppliesToMe(ifaceIndex) && alert.nPriority > nPriority)
      {
        nPriority = alert.nPriority;
        strStatusBar = alert.strStatusBar;
      }
    }
  }

  if (strFor == "statusbar")
    return strStatusBar;
  else if (strFor == "rpc")
    return strRPC;
  //assert(!"GetWarnings() : invalid parameter");
  return "error";
}

CAlert CAlert::getAlertByHash(const uint256 &hash)
{
    CAlert retval;
    {
        LOCK(cs_mapAlerts);
        map<uint256, CAlert>::iterator mi = mapAlerts.find(hash);
        if(mi != mapAlerts.end())
            retval = mi->second;
    }
    return retval;
}

bool CAlert::ProcessAlert(int ifaceIndex)
{
    if (!CheckSignature(ifaceIndex))
        return false;
    if (!IsInEffect())
        return false;

    {
        LOCK(cs_mapAlerts);
        // Cancel previous alerts
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
        {
            const CAlert& alert = (*mi).second;
            if (Cancels(alert))
            {
                printf("cancelling alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            }
            else if (!alert.IsInEffect())
            {
                printf("expiring alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            }
            else
                mi++;
        }

        // Check if this alert has been cancelled
        BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.Cancels(*this))
            {
                printf("alert already cancelled by %d\n", alert.nID);
                return false;
            }
        }

        // Add to mapAlerts
        mapAlerts.insert(make_pair(GetHash(), *this));
        // Notify UI if it applies to me
    }

    printf("accepted alert %d, AppliesToMe()=%d\n", nID, AppliesToMe(ifaceIndex));
    return true;
}









//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

int FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    SHA256_CTX ctx;
    unsigned char data[64];

    SHA256_Init(&ctx);

    for (int i = 0; i < 16; i++)
        ((uint32_t*)data)[i] = ByteReverse(((uint32_t*)pinput)[i]);

    for (int i = 0; i < 8; i++)
        ctx.h[i] = ((uint32_t*)pinit)[i];

    SHA256_Update(&ctx, data, sizeof(data));
    for (int i = 0; i < 8; i++)
        ((uint32_t*)pstate)[i] = ctx.h[i];
}

//
// ScanHash scans nonces looking for a hash with at least some zero bits.
// It operates on big endian data.  Caller does the byte reversing.
// All input buffers are 16-byte aligned.  nNonce is usually preserved
// between calls, but periodically or if nNonce is 0xffff0000 or above,
// the block is rebuilt and nNonce starts over at zero.
//
unsigned int static ScanHash_CryptoPP(char* pmidstate, char* pdata, char* phash1, char* phash, unsigned int& nHashesDone)
{
    unsigned int& nNonce = *(unsigned int*)(pdata + 12);
    for (;;)
    {
        // Crypto++ SHA-256
        // Hash pdata using pmidstate as the starting state into
        // preformatted buffer phash1, then hash phash1 into phash
        nNonce++;
        SHA256Transform(phash1, pdata, pmidstate);
        SHA256Transform(phash, phash1, pSHA256InitState);

        // Return the nonce if the hash has at least some zero bits,
        // caller will check if it has enough to reach the target
        if (((unsigned short*)phash)[14] == 0)
            return nNonce;

        // If nothing found after trying for a while, return -1
        if ((nNonce & 0xffff) == 0)
        {
            nHashesDone = 0xffff+1;
            return (unsigned int) -1;
        }
    }
}


uint64 nLastBlockTx = 0;
uint64 nLastBlockSize = 0;



#if 0
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    pblock->vtx[0].vin[0].scriptSig = (CScript() << pblock->nTime << CBigNum(nExtraNonce)) + pblock->GetCoinbaseFlags();
    assert(pblock->vtx[0].vin[0].scriptSig.size() <= 100);

    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

void SetExtraNonce(CBlock* pblock, const char *xn_hex)
{

  pblock->vtx[0].vin[0].scriptSig = (CScript() << CBigNum(pblock->nTime) << ParseHex(xn_hex)) + pblock->GetCoinbaseFlags();
  if (pblock->vtx[0].vin[0].scriptSig.size() > 100) {
    char errbuf[1024];

    sprintf(errbuf, "SetExtraNonce: warning: scriptSig byte size is %u [which is over 100]. (xn_hex: %s)", 
      (unsigned int)pblock->vtx[0].vin[0].scriptSig.size(), xn_hex);
    shcoind_log(errbuf);
  }

}
#endif



void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1)
{
    //
    // Prebuild hash buffers
    //
    struct
    {
        struct unnamed2
        {
            int nVersion;
            uint256 hashPrevBlock;
            uint256 hashMerkleRoot;
            unsigned int nTime;
            unsigned int nBits;
            unsigned int nNonce;
        }
        block;
        unsigned char pchPadding0[64];
        uint256 hash1;
        unsigned char pchPadding1[64];
    }
    tmp;
    memset(&tmp, 0, sizeof(tmp));

    tmp.block.nVersion       = pblock->nVersion;
    tmp.block.hashPrevBlock  = pblock->hashPrevBlock;
    tmp.block.hashMerkleRoot = pblock->hashMerkleRoot;
    tmp.block.nTime          = pblock->nTime;
    tmp.block.nBits          = pblock->nBits;
    tmp.block.nNonce         = pblock->nNonce;

    FormatHashBlocks(&tmp.block, sizeof(tmp.block));
    FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));

    // Byte swap all the input buffer
    for (unsigned int i = 0; i < sizeof(tmp)/4; i++)
        ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

    // Precalc the first half of the first hash, which stays constant
    SHA256Transform(pmidstate, &tmp.block, pSHA256InitState);

    memcpy(pdata, &tmp.block, 128);
    memcpy(phash1, &tmp.hash1, 64);
}


bool CheckWork(CBlock* pblock, CWallet& wallet)
{
  CIface *iface = GetCoinByIndex(pblock->ifaceIndex); 
  uint256 hash = pblock->GetPoWHash();
  uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

  if (hash > hashTarget)
    return false;

  // Found a solution
  {
    LOCK(cs_main);
    if (pblock->hashPrevBlock != GetBestBlockChain(iface))
      return error(SHERR_INVAL, "BitcoinMiner : generated block is stale");

#if 0
    // Remove key from key pool
    reservekey.KeepKey();
#endif

    // Track how many getdata requests this block gets
    {
      LOCK(wallet.cs_wallet);
      wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    // Process this block the same as if we had received it from another node
    if (!ProcessBlock(NULL, pblock))
      return error(SHERR_INVAL, "BitcoinMiner : ProcessBlock, block not accepted");
  }

  return true;
}


