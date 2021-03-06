
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

#include "shcoind.h"
#include "walletdb.h"
#include "wallet.h"
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;


static uint64 nAccountingEntryNumber = 0;

//
// CWalletDB
//

bool CWalletDB::WriteName(const string& strAddress, const string& strName)
{
    nWalletDBUpdated++;
    return Write(make_pair(string("name"), strAddress), strName);
}

bool CWalletDB::EraseName(const string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    nWalletDBUpdated++;
    return Erase(make_pair(string("name"), strAddress));
}

bool CWalletDB::ReadAccount(const string& strAccount, CAccount& account)
{
    account.SetNull();
    return Read(make_pair(string("acc"), strAccount), account);
}

bool CWalletDB::WriteAccount(const string& strAccount, const CAccount& account)
{
    return Write(make_pair(string("acc"), strAccount), account);
}

bool CWalletDB::WriteAccountingEntry(const CAccountingEntry& acentry)
{
    return Write(boost::make_tuple(string("acentry"), acentry.strAccount, ++nAccountingEntryNumber), acentry);
}

#if 0
bool CWalletDB::WriteHDChain(const CHDChain& chain)
{
	return Write(std::string("hdchain"), chain);
}
#endif

int64 CWalletDB::GetAccountCreditDebit(const string& strAccount)
{
    list<CAccountingEntry> entries;
    ListAccountCreditDebit(strAccount, entries);

    int64 nCreditDebit = 0;
    BOOST_FOREACH (const CAccountingEntry& entry, entries)
        nCreditDebit += entry.nCreditDebit;

    return nCreditDebit;
}

void CWalletDB::ListAccountCreditDebit(const string& strAccount, list<CAccountingEntry>& entries)
{
    bool fAllAccounts = (strAccount == "*");

    Dbc* pcursor = GetCursor();
    if (!pcursor)
        throw runtime_error("CWalletDB::ListAccountCreditDebit() : cannot create DB cursor");
    unsigned int fFlags = DB_SET_RANGE;
    loop
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (fFlags == DB_SET_RANGE)
            ssKey << boost::make_tuple(string("acentry"), (fAllAccounts? string("") : strAccount), uint64(0));
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
            break;
        else if (ret != 0)
        {
            pcursor->close();
            throw runtime_error("CWalletDB::ListAccountCreditDebit() : error scanning DB");
        }

        // Unserialize
        string strType;
        ssKey >> strType;
        if (strType != "acentry")
            break;
        CAccountingEntry acentry;
        ssKey >> acentry.strAccount;
        if (!fAllAccounts && acentry.strAccount != strAccount)
            break;

        ssValue >> acentry;
        entries.push_back(acentry);
    }

    pcursor->close();
}


int CWalletDB::LoadWallet(CWallet* pwallet)
{
  pwallet->vchDefaultKey = CPubKey();
  int nFileVersion = 0;
  bool fIsEncrypted = false;

	{
		LOCK(pwallet->cs_wallet);

		int nMinVersion = 0;
		if (Read((string)"minversion", nMinVersion))
		{
#if 0
			if (nMinVersion > CLIENT_VERSION)
				return DB_TOO_NEW;
#endif
			//      pwallet->LoadMinVersion(nMinVersion);
		}

		// Get cursor
		Dbc* pcursor = GetCursor();
		if (!pcursor)
		{
			//fprintf(stderr, "Error getting wallet database cursor\n");
			return DB_CORRUPT;
		}

		loop
		{
			// Read next record
			CDataStream ssKey(SER_DISK, CLIENT_VERSION);
			CDataStream ssValue(SER_DISK, CLIENT_VERSION);
			int ret = ReadAtCursor(pcursor, ssKey, ssValue);
			if (ret == DB_NOTFOUND)
				break;
			else if (ret != 0)
			{
				//fprintf(stderr, "Error reading next record from wallet database\n");
				return DB_CORRUPT;
			}

			// Unserialize
			// Taking advantage of the fact that pair serialization
			// is just the two items serialized one after the other
			string strType;
			ssKey >> strType;
			if (strType == "name")
			{
				string strAddress;
				ssKey >> strAddress;
				ssValue >> pwallet->mapAddressBook[CCoinAddr(pwallet->ifaceIndex, strAddress).Get()];
			}
			else if (strType == "tx")
			{
				uint256 hash;
				ssKey >> hash;
				CWalletTx& wtx = pwallet->mapWallet[hash];
				ssValue >> wtx;
				wtx.BindWallet(pwallet);
				pwallet->InitSpent(wtx);
			}
			else if (strType == "acentry")
			{
				string strAccount;
				ssKey >> strAccount;
				uint64 nNumber;
				ssKey >> nNumber;
				if (nNumber > nAccountingEntryNumber)
					nAccountingEntryNumber = nNumber;
			} else if (strType == "eckey") {
				vector<unsigned char> vchPubKey;
				ECKey key;

				ssKey >> vchPubKey;
				ssValue >> key;
				const CPubKey& pubkey = key.GetPubKey();
				if (pubkey != vchPubKey)
					return DB_CORRUPT;

				//				key.SetPubKey(pubkey);
				if (!pwallet->LoadKey(key))
					return DB_CORRUPT;
#if 0
				pwallet->LoadKeyMetadata(pubkey.GetID(), key.meta);
#endif
			} else if (strType == "dikey") {
				vector<unsigned char> vchPubKey;
				DIKey key;

				ssKey >> vchPubKey;
				ssValue >> key;
				const CPubKey& pubkey = key.GetPubKey();
				if (pubkey != vchPubKey)
					return DB_CORRUPT;

				//				key.SetPubKey(pubkey);
				if (!pwallet->LoadKey(key))
					return DB_CORRUPT;
#if 0
				pwallet->LoadKeyMetadata(pubkey.GetID(), key.meta);
#endif
			}
			else if (strType == "key" || strType == "wkey")
			{
				vector<unsigned char> vchPubKey;
				ssKey >> vchPubKey;
				ECKey key;
				if (strType == "key")
				{
					CPrivKey pkey;
					ssValue >> pkey;
					key.SetPubKey(vchPubKey);
					key.SetPrivKey(pkey);
					if (key.GetPubKey() != vchPubKey)
					{
#if 0
						HDPrivKey hdkey;
						hdkey.SetPrivKey(pkey);
						if (hdkey.GetPubKey() != vchPubKey) {
							//              fprintf(stderr, "Error reading wallet database: CPrivKey pubkey inconsistency\n");
							return DB_CORRUPT;
						}
						if (!hdkey.IsValid()) {
							//fprintf(stderr, "Error reading wallet database: invalid HDPrivKey\n");
							return DB_CORRUPT;
						}
#endif
						return DB_CORRUPT;
					} else if (!key.IsValid()) {
						//fprintf(stderr, "Error reading wallet database: invalid CPrivKey\n");
						return DB_CORRUPT;
					}
				}
				else
				{
					CWalletKey wkey;
					ssValue >> wkey;
					key.SetPubKey(vchPubKey);
					key.SetPrivKey(wkey.vchPrivKey);
					if (key.GetPubKey() != vchPubKey)
					{
						//fprintf(stderr, "Error reading wallet database: CWalletKey pubkey inconsistency\n");
						return DB_CORRUPT;
					}
					if (!key.IsValid())
					{
						//fprintf(stderr, "Error reading wallet database: invalid CWalletKey\n");
						return DB_CORRUPT;
					}
				}
				if (!pwallet->LoadKey(key))
				{
					//fprintf(stderr, "Error reading wallet database: LoadKey failed\n");
					return DB_CORRUPT;
				}
			}
#if 0
			else if (strType == "mkey")
			{
				unsigned int nID;
				ssKey >> nID;
				CMasterKey kMasterKey;
				ssValue >> kMasterKey;
				if(pwallet->mapMasterKeys.count(nID) != 0)
				{
					//fprintf(stderr, "Error reading wallet database: duplicate CMasterKey id %u\n", nID);
					return DB_CORRUPT;
				}
				pwallet->mapMasterKeys[nID] = kMasterKey;
				if (pwallet->nMasterKeyMaxID < nID)
					pwallet->nMasterKeyMaxID = nID;
			}
#endif
#if 0
			else if (strType == "ckey")
			{
				vector<unsigned char> vchPubKey;
				ssKey >> vchPubKey;
				vector<unsigned char> vchPrivKey;
				ssValue >> vchPrivKey;
				if (!pwallet->LoadCryptedKey(vchPubKey, vchPrivKey))
				{
					//fprintf(stderr, "Error reading wallet database: LoadCryptedKey failed\n");
					return DB_CORRUPT;
				}
				fIsEncrypted = true;
			}
#endif
			else if (strType == "defaultkey")
			{
				ssValue >> pwallet->vchDefaultKey;
			}
#if 0
			else if (strType == "pool")
			{
				int64 nIndex;
				ssKey >> nIndex;
				pwallet->setKeyPool.insert(nIndex);
			}
#endif
			else if (strType == "version")
			{
				ssValue >> nFileVersion;
				if (nFileVersion == 10300)
					nFileVersion = 300;
			}
			else if (strType == "cscript")
			{
				uint160 hash;
				ssKey >> hash;
				CScript script;
				ssValue >> script;
				if (!pwallet->LoadCScript(script))
				{
					//fprintf(stderr, "Error reading wallet database: LoadCScript failed\n");
					return DB_CORRUPT;
				}
#if 0
			} else if (strType == "hdchain") {
				CHDChain chain;
				ssValue >> chain;
				pwallet->SetHDChain(chain, true);
#endif
			}

		}
		pcursor->close();
	}

  if (nFileVersion < CLIENT_VERSION) // Update
    WriteVersion(CLIENT_VERSION);

  return DB_LOAD_OK;
}


