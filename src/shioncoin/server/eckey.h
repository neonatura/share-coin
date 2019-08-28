
/*
 * @copyright
 *
 *  Copyright 2017 Neo Natura
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

#ifndef __SERVER__ECKEY_H__
#define __SERVER__ECKEY_H__

/* libsecp256k1 */
#include "secp256k1.h"
#include "secp256k1_recovery.h"

#ifdef __cplusplus
#include "key.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void INIT_SECP256K1(void);

secp256k1_context *SECP256K1_VERIFY_CONTEXT(void);

secp256k1_context *SECP256K1_SIGN_CONTEXT(void);

void TERM_SECP256K1(void);
#ifdef __cplusplus
};
#endif


#ifdef __cplusplus
#include <stdexcept>
#include <vector>

#include "allocators.h"
#include "serialize.h"
#include "uint256.h"
#include "util.h"

const unsigned int BIP32_EXTKEY_SIZE = 74;

/** An encapsulated Elliptic Curve key (public and/or private) */
class ECKey : public CKey
{

	public:

    ECKey()
		{
			SetNull();
		}

    ECKey(CSecret secret, bool fCompressed = true)
		{
			SetNull();
			SetSecret(secret, fCompressed);
		}

    ECKey(const ECKey& b)
		{
			SetNull();
			Init(b);
		}

    ECKey& operator=(const ECKey& b)
		{
			Init(b);
			return (*this);
		}

    bool IsNull() const;

    bool SetPrivKey(const CPrivKey& vchPrivKey, bool fCompressed = false);

    bool SetSecret(const CSecret& vchSecret, bool fCompressed = false);

    CPrivKey GetPrivKey() const;

    bool SetPubKey(const CPubKey& vchPubKey);

    CPubKey GetPubKey() const; /* CKey */

    bool Sign(uint256 hash, std::vector<unsigned char>& vchSig); /* CKey */

    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool SignCompact(uint256 hash, std::vector<unsigned char>& vchSig); /* CKey */

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig); /* CKey */

    bool Verify(uint256 hash, const std::vector<unsigned char>& vchSig); /* CKey */

    // Verify a compact signature
    bool VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig); /* CKey */

    void MakeNewKey(bool fCompressed); /* CKey */

    bool IsValid(); /* CKey */

    void MergeKey(CKey& childKey, cbuff tag);

		bool Derive(CKey& keyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const;

};

#if 0
struct ECExtPubKey 
{
	unsigned char nDepth;
	unsigned char vchFingerprint[4];
	unsigned int nChild;
	ChainCode chaincode;
	CPubKey pubkey;

	friend bool operator==(const ECExtPubKey &a, const ECExtPubKey &b)
	{
		return a.nDepth == b.nDepth &&
			memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], sizeof(vchFingerprint)) == 0 &&
			a.nChild == b.nChild &&
			a.chaincode == b.chaincode &&
			a.pubkey == b.pubkey;
	}

	bool Derive(ECExtPubKey& out, unsigned int nChild) const;

	IMPLEMENT_SERIALIZE
	(
		if (fRead) {
			READWRITE(FLATDATA(code));
			nDepth = code[0];
			memcpy(vchFingerprint, code+1, 4);
			nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
			memcpy(chaincode.begin(), code+9, 32);
			pubkey.Set(cbuff(code+41, code+BIP32_EXTKEY_SIZE));
		} else {
			code[0] = nDepth;
			memcpy(code+1, vchFingerprint, 4);
			code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
			code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
			memcpy(code+9, chaincode.begin(), 32);
//			assert(pubkey.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE);
			memcpy(code+41, pubkey.begin(), CPubKey::COMPRESSED_PUBLIC_KEY_SIZE);
			READWRITE(FLATDATA(code));
		}
	)

};

struct ECExtKey
{
	unsigned char nDepth;
	unsigned char vchFingerprint[4];
	unsigned int nChild;
	ChainCode chaincode;
	CKey key;


	friend bool operator==(const ECExtKey& a, const ECExtKey& b)
	{
		return a.nDepth == b.nDepth &&
			memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], sizeof(vchFingerprint)) == 0 &&
			a.nChild == b.nChild &&
			a.chaincode == b.chaincode &&
			a.key == b.key;
	}

	bool Derive(ECExtKey& out, unsigned int nChild) const;

	ECExtPubKey Neuter() const;

	void SetMaster(const unsigned char* seed, unsigned int nSeedLen);

	IMPLEMENT_SERIALIZE
	(
		unsigned char code[BIP32_EXTKEY_SIZE];
		if (fRead) {
			READWRITE(FLATDATA(code));
			nDepth = code[0];
			memcpy(vchFingerprint, code+1, 4);
			nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
			memcpy(chaincode.begin(), code+9, 32);
			key.Set(cbuff(code+42, code+BIP32_EXTKEY_SIZE), true);
		} else {
			code[0] = nDepth;
			memcpy(code+1, vchFingerprint, 4);
			code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
			code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
			memcpy(code+9, chaincode.begin(), 32);
			code[41] = 0;
//			assert(key.size() == 32);
			memcpy(code+42, key.begin(), 32);
			READWRITE(FLATDATA(code));
		}
	)

};
#endif

#endif /* __cplusplus */


#endif /* ndef __SERVER__ECKEY_H__ */
