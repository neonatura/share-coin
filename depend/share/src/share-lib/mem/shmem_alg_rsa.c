/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
/*
 *  The following sources were referenced in the design of this implementation
 *  of the RSA algorithm:
 *
 *  [1] A method for obtaining digital signatures and public-key cryptosystems
 *      R Rivest, A Shamir, and L Adleman
 *      http://people.csail.mit.edu/rivest/pubs.html#RSA78
 *
 *  [2] Handbook of Applied Cryptography - 1997, Chapter 8
 *      Menezes, van Oorschot and Vanstone
 *
 */

#include "share.h"

//#define SHMEM_ALG_RSA_PKCS_V15 SHRSA_PKCS_V15
//#define SHMEM_ALG_RSA_PKCS_V21 SHRSA_PKCS_V21

/*
 * Initialize an RSA context
 */
void shrsa_init( shrsa_t *ctx,
               int padding,
               int hash_id )
{
    memset( ctx, 0, sizeof( shrsa_t ) );

    shrsa_set_padding( ctx, padding, hash_id );

}

/*
 * Set padding for an existing RSA context
 */
void shrsa_set_padding( shrsa_t *ctx, int padding, int hash_id )
{
    ctx->padding = padding;
    ctx->hash_id = hash_id;
}

#if defined(SHMEM_ALG_GENPRIME)

/*
 * Generate an RSA keypair
 */
int shrsa_gen_key( shrsa_t *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 unsigned int nbits, int exponent )
{
    int ret;
    shmpi P1, Q1, H, G;

    if( f_rng == NULL || nbits < 128 || exponent < 3 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    if( nbits % 2 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    shmpi_init( &P1 ); shmpi_init( &Q1 );
    shmpi_init( &H ); shmpi_init( &G );

    /*
     * find primes P and Q with Q < P so that:
     * GCD( E, (P-1)*(Q-1) ) == 1
     */
    MPI_CHK( shmpi_lset( &ctx->E, exponent ) );

    do
    {
        MPI_CHK( shmpi_gen_prime( &ctx->P, nbits >> 1, 0,
                                f_rng, p_rng ) );

        MPI_CHK( shmpi_gen_prime( &ctx->Q, nbits >> 1, 0,
                                f_rng, p_rng ) );

        if( shmpi_cmp_mpi( &ctx->P, &ctx->Q ) == 0 )
            continue;

        MPI_CHK( shmpi_mul_mpi( &ctx->N, &ctx->P, &ctx->Q ) );
        if( shmpi_bitlen( &ctx->N ) != nbits )
            continue;

        if( shmpi_cmp_mpi( &ctx->P, &ctx->Q ) < 0 )
                                shmpi_swap( &ctx->P, &ctx->Q );

        MPI_CHK( shmpi_sub_int( &P1, &ctx->P, 1 ) );
        MPI_CHK( shmpi_sub_int( &Q1, &ctx->Q, 1 ) );
        MPI_CHK( shmpi_mul_mpi( &H, &P1, &Q1 ) );
        MPI_CHK( shmpi_gcd( &G, &ctx->E, &H  ) );
    }
    while( shmpi_cmp_int( &G, 1 ) != 0 );

    /*
     * D  = E^-1 mod ((P-1)*(Q-1))
     * DP = D mod (P - 1)
     * DQ = D mod (Q - 1)
     * QP = Q^-1 mod P
     */
    MPI_CHK( shmpi_inv_mod( &ctx->D , &ctx->E, &H  ) );
    MPI_CHK( shmpi_mod_mpi( &ctx->DP, &ctx->D, &P1 ) );
    MPI_CHK( shmpi_mod_mpi( &ctx->DQ, &ctx->D, &Q1 ) );
    MPI_CHK( shmpi_inv_mod( &ctx->QP, &ctx->Q, &ctx->P ) );

    ctx->len = ( shmpi_bitlen( &ctx->N ) + 7 ) >> 3;

cleanup:

    shmpi_free( &P1 ); shmpi_free( &Q1 ); shmpi_free( &H ); shmpi_free( &G );

    if( ret != 0 )
    {
        shrsa_free( ctx );
        return( SHRSA_ERR_KEY_GEN_FAILED + ret );
    }

    return( 0 );
}

#endif /* SHMEM_ALG_GENPRIME */

/*
 * Check a public RSA key
 */
int shrsa_check_pubkey( const shrsa_t *ctx )
{
    if( !ctx->N.p || !ctx->E.p )
        return( SHRSA_ERR_KEY_CHECK_FAILED );

    if( ( ctx->N.p[0] & 1 ) == 0 ||
        ( ctx->E.p[0] & 1 ) == 0 )
        return( SHRSA_ERR_KEY_CHECK_FAILED );

    if( shmpi_bitlen( &ctx->N ) < 128 ||
        shmpi_bitlen( &ctx->N ) > SHMPI_MAX_BITS )
        return( SHRSA_ERR_KEY_CHECK_FAILED );

    if( shmpi_bitlen( &ctx->E ) < 2 ||
        shmpi_cmp_mpi( &ctx->E, &ctx->N ) >= 0 )
        return( SHRSA_ERR_KEY_CHECK_FAILED );

    return( 0 );
}

/*
 * Check a private RSA key
 */
int shrsa_check_privkey( const shrsa_t *ctx )
{
    int ret;
    shmpi PQ, DE, P1, Q1, H, I, G, G2, L1, L2, DP, DQ, QP;

    if( ( ret = shrsa_check_pubkey( ctx ) ) != 0 )
        return( ret );

    if( !ctx->P.p || !ctx->Q.p || !ctx->D.p )
        return( SHRSA_ERR_KEY_CHECK_FAILED );

    shmpi_init( &PQ ); shmpi_init( &DE ); shmpi_init( &P1 ); shmpi_init( &Q1 );
    shmpi_init( &H  ); shmpi_init( &I  ); shmpi_init( &G  ); shmpi_init( &G2 );
    shmpi_init( &L1 ); shmpi_init( &L2 ); shmpi_init( &DP ); shmpi_init( &DQ );
    shmpi_init( &QP );

    MPI_CHK( shmpi_mul_mpi( &PQ, &ctx->P, &ctx->Q ) );
    MPI_CHK( shmpi_mul_mpi( &DE, &ctx->D, &ctx->E ) );
    MPI_CHK( shmpi_sub_int( &P1, &ctx->P, 1 ) );
    MPI_CHK( shmpi_sub_int( &Q1, &ctx->Q, 1 ) );
    MPI_CHK( shmpi_mul_mpi( &H, &P1, &Q1 ) );
    MPI_CHK( shmpi_gcd( &G, &ctx->E, &H  ) );

    MPI_CHK( shmpi_gcd( &G2, &P1, &Q1 ) );
    MPI_CHK( shmpi_div_mpi( &L1, &L2, &H, &G2 ) );
    MPI_CHK( shmpi_mod_mpi( &I, &DE, &L1  ) );

    MPI_CHK( shmpi_mod_mpi( &DP, &ctx->D, &P1 ) );
    MPI_CHK( shmpi_mod_mpi( &DQ, &ctx->D, &Q1 ) );
    MPI_CHK( shmpi_inv_mod( &QP, &ctx->Q, &ctx->P ) );
    /*
     * Check for a valid PKCS1v2 private key
     */
    if( shmpi_cmp_mpi( &PQ, &ctx->N ) != 0 ||
        shmpi_cmp_mpi( &DP, &ctx->DP ) != 0 ||
        shmpi_cmp_mpi( &DQ, &ctx->DQ ) != 0 ||
        shmpi_cmp_mpi( &QP, &ctx->QP ) != 0 ||
        shmpi_cmp_int( &L2, 0 ) != 0 ||
        shmpi_cmp_int( &I, 1 ) != 0 ||
        shmpi_cmp_int( &G, 1 ) != 0 )
    {
        ret = SHRSA_ERR_KEY_CHECK_FAILED;
    }

cleanup:
    shmpi_free( &PQ ); shmpi_free( &DE ); shmpi_free( &P1 ); shmpi_free( &Q1 );
    shmpi_free( &H  ); shmpi_free( &I  ); shmpi_free( &G  ); shmpi_free( &G2 );
    shmpi_free( &L1 ); shmpi_free( &L2 ); shmpi_free( &DP ); shmpi_free( &DQ );
    shmpi_free( &QP );

    if( ret == SHRSA_ERR_KEY_CHECK_FAILED )
        return( ret );

    if( ret != 0 )
        return( SHRSA_ERR_KEY_CHECK_FAILED + ret );

    return( 0 );
}

/*
 * Check if contexts holding a public and private key match
 */
int shrsa_check_pub_priv( const shrsa_t *pub, const shrsa_t *prv )
{
    if( shrsa_check_pubkey( pub ) != 0 ||
        shrsa_check_privkey( prv ) != 0 )
    {
        return( SHRSA_ERR_KEY_CHECK_FAILED );
    }

    if( shmpi_cmp_mpi( &pub->N, &prv->N ) != 0 ||
        shmpi_cmp_mpi( &pub->E, &prv->E ) != 0 )
    {
        return( SHRSA_ERR_KEY_CHECK_FAILED );
    }

    return( 0 );
}

/*
 * Do an RSA public key operation
 */
int shrsa_public( shrsa_t *ctx,
                const unsigned char *input,
                unsigned char *output )
{
    int ret;
    size_t olen;
    shmpi T;

    shmpi_init( &T );


    MPI_CHK( shmpi_read_binary( &T, input, ctx->len ) );

    if( shmpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        ret = SHMPI_ERR_BAD_INPUT_DATA;
        goto cleanup;
    }

    olen = ctx->len;
    MPI_CHK( shmpi_exp_mod( &T, &T, &ctx->E, &ctx->N, &ctx->RN ) );
    MPI_CHK( shmpi_write_binary( &T, output, olen ) );

cleanup:

    shmpi_free( &T );

    if( ret != 0 )
        return( SHRSA_ERR_PUBLIC_FAILED + ret );

    return( 0 );
}

/*
 * Generate or update blinding values, see section 10 of:
 *  KOCHER, Paul C. Timing attacks on implementations of Diffie-Hellman, RSA,
 *  DSS, and other systems. In : Advances in Cryptology-CRYPTO'96. Springer
 *  Berlin Heidelberg, 1996. p. 104-113.
 */
static int rsa_prepare_blinding( shrsa_t *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret, count = 0;

    if( ctx->Vf.p != NULL )
    {
        /* We already have blinding values, just update them by squaring */
        MPI_CHK( shmpi_mul_mpi( &ctx->Vi, &ctx->Vi, &ctx->Vi ) );
        MPI_CHK( shmpi_mod_mpi( &ctx->Vi, &ctx->Vi, &ctx->N ) );
        MPI_CHK( shmpi_mul_mpi( &ctx->Vf, &ctx->Vf, &ctx->Vf ) );
        MPI_CHK( shmpi_mod_mpi( &ctx->Vf, &ctx->Vf, &ctx->N ) );

        goto cleanup;
    }

    /* Unblinding value: Vf = random number, invertible mod N */
    do {
        if( count++ > 10 )
            return( SHRSA_ERR_RNG_FAILED );

        MPI_CHK( shmpi_fill_random( &ctx->Vf, ctx->len - 1, f_rng, p_rng ) );
        MPI_CHK( shmpi_gcd( &ctx->Vi, &ctx->Vf, &ctx->N ) );
    } while( shmpi_cmp_int( &ctx->Vi, 1 ) != 0 );

    /* Blinding value: Vi =  Vf^(-e) mod N */
    MPI_CHK( shmpi_inv_mod( &ctx->Vi, &ctx->Vf, &ctx->N ) );
    MPI_CHK( shmpi_exp_mod( &ctx->Vi, &ctx->Vi, &ctx->E, &ctx->N, &ctx->RN ) );


cleanup:
    return( ret );
}

/*
 * Do an RSA private key operation
 */
int shrsa_private( shrsa_t *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 const unsigned char *input,
                 unsigned char *output )
{
    int ret;
    size_t olen;
    shmpi T, T1, T2;

    /* Make sure we have private key info, prevent possible misuse */
    if( ctx->P.p == NULL || ctx->Q.p == NULL || ctx->D.p == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    shmpi_init( &T ); shmpi_init( &T1 ); shmpi_init( &T2 );


    MPI_CHK( shmpi_read_binary( &T, input, ctx->len ) );
    if( shmpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        ret = SHMPI_ERR_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( f_rng != NULL )
    {
        /*
         * Blinding
         * T = T * Vi mod N
         */
        MPI_CHK( rsa_prepare_blinding( ctx, f_rng, p_rng ) );
        MPI_CHK( shmpi_mul_mpi( &T, &T, &ctx->Vi ) );
        MPI_CHK( shmpi_mod_mpi( &T, &T, &ctx->N ) );
    }

#if defined(SHMEM_ALG_RSA_NO_CRT)
    MPI_CHK( shmpi_exp_mod( &T, &T, &ctx->D, &ctx->N, &ctx->RN ) );
#else
    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( shmpi_exp_mod( &T1, &T, &ctx->DP, &ctx->P, &ctx->RP ) );
    MPI_CHK( shmpi_exp_mod( &T2, &T, &ctx->DQ, &ctx->Q, &ctx->RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( shmpi_sub_mpi( &T, &T1, &T2 ) );
    MPI_CHK( shmpi_mul_mpi( &T1, &T, &ctx->QP ) );
    MPI_CHK( shmpi_mod_mpi( &T, &T1, &ctx->P ) );

    /*
     * T = T2 + T * Q
     */
    MPI_CHK( shmpi_mul_mpi( &T1, &T, &ctx->Q ) );
    MPI_CHK( shmpi_add_mpi( &T, &T2, &T1 ) );
#endif /* SHMEM_ALG_RSA_NO_CRT */

    if( f_rng != NULL )
    {
        /*
         * Unblind
         * T = T * Vf mod N
         */
        MPI_CHK( shmpi_mul_mpi( &T, &T, &ctx->Vf ) );
        MPI_CHK( shmpi_mod_mpi( &T, &T, &ctx->N ) );
    }

    olen = ctx->len;
    MPI_CHK( shmpi_write_binary( &T, output, olen ) );

cleanup:

    shmpi_free( &T ); shmpi_free( &T1 ); shmpi_free( &T2 );

    if( ret != 0 )
        return( SHRSA_ERR_PRIVATE_FAILED + ret );

    return( 0 );
}

#if defined(SHMEM_ALG_RSA_PKCS_V21)
/**
 * Generate and apply the MGF1 operation (from PKCS#1 v2.1) to a buffer.
 *
 * \param dst       buffer to mask
 * \param dlen      length of destination buffer
 * \param src       source of the mask generation
 * \param slen      length of the source buffer
 * \param md_ctx    message digest context to use
 */
static void mgf_mask( unsigned char *dst, size_t dlen, unsigned char *src,
                      size_t slen, mbedtls_md_context_t *md_ctx )
{
    unsigned char mask[SHRSA_MD_MAX_SIZE];
    unsigned char counter[4];
    unsigned char *p;
    unsigned int hlen;
    size_t i, use_len;

    memset( mask, 0, SHRSA_MD_MAX_SIZE );
    memset( counter, 0, 4 );

    hlen = mbedtls_md_get_size( md_ctx->md_info );

    /* Generate and apply dbMask */
    p = dst;

    while( dlen > 0 )
    {
        use_len = hlen;
        if( dlen < hlen )
            use_len = dlen;

        mbedtls_md_starts( md_ctx );
        mbedtls_md_update( md_ctx, src, slen );
        mbedtls_md_update( md_ctx, counter, 4 );
        mbedtls_md_finish( md_ctx, mask );

        for( i = 0; i < use_len; ++i )
            *p++ ^= mask[i];

        counter[3]++;

        dlen -= use_len;
    }
}
#endif /* SHMEM_ALG_RSA_PKCS_V21 */

#if defined(SHMEM_ALG_RSA_PKCS_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-ENCRYPT function
 */
int shrsa_rsaes_oaep_encrypt( shrsa_t *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t ilen,
                            const unsigned char *input,
                            unsigned char *output )
{
    size_t olen;
    int ret;
    unsigned char *p = output;
    unsigned int hlen;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V21 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    if( f_rng == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    md_info = mbedtls_md_info_from_type( (shrsa_md_type_t) ctx->hash_id );
    if( md_info == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    olen = ctx->len;
    hlen = mbedtls_md_get_size( md_info );

    /* first comparison checks for overflow */
    if( ilen + 2 * hlen + 2 < ilen || olen < ilen + 2 * hlen + 2 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    memset( output, 0, olen );

    *p++ = 0;

    /* Generate a random octet string seed */
    if( ( ret = f_rng( p_rng, p, hlen ) ) != 0 )
        return( SHRSA_ERR_RNG_FAILED + ret );

    p += hlen;

    /* Construct DB */
    mbedtls_md( md_info, label, label_len, p );
    p += hlen;
    p += olen - 2 * hlen - 2 - ilen;
    *p++ = 1;
    memcpy( p, input, ilen );

    mbedtls_md_init( &md_ctx );
    if( ( ret = mbedtls_md_setup( &md_ctx, md_info, 0 ) ) != 0 )
    {
        mbedtls_md_free( &md_ctx );
        return( ret );
    }

    /* maskedDB: Apply dbMask to DB */
    mgf_mask( output + hlen + 1, olen - hlen - 1, output + 1, hlen,
               &md_ctx );

    /* maskedSeed: Apply seedMask to seed */
    mgf_mask( output + 1, hlen, output + hlen + 1, olen - hlen - 1,
               &md_ctx );

    mbedtls_md_free( &md_ctx );

    return( ( mode == SHRSA_PUBLIC )
            ? shrsa_public(  ctx, output, output )
            : shrsa_private( ctx, f_rng, p_rng, output, output ) );
}
#endif /* SHMEM_ALG_RSA_PKCS_V21 */

#if defined(SHMEM_ALG_RSA_PKCS_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-ENCRYPT function
 */
int shrsa_rsaes_pkcs1_v15_encrypt( shrsa_t *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t ilen,
                                 const unsigned char *input,
                                 unsigned char *output )
{
    size_t nb_pad, olen;
    int ret;
    unsigned char *p = output;

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V15 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    // We don't check p_rng because it won't be dereferenced here
    if( f_rng == NULL || input == NULL || output == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    olen = ctx->len;

    /* first comparison checks for overflow */
    if( ilen + 11 < ilen || olen < ilen + 11 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    nb_pad = olen - 3 - ilen;

    *p++ = 0;
    if( mode == SHRSA_PUBLIC )
    {
        *p++ = SHRSA_CRYPT;

        while( nb_pad-- > 0 )
        {
            int rng_dl = 100;

            do {
                ret = f_rng( p_rng, p, 1 );
            } while( *p == 0 && --rng_dl && ret == 0 );

            /* Check if RNG failed to generate data */
            if( rng_dl == 0 || ret != 0 )
                return( SHRSA_ERR_RNG_FAILED + ret );

            p++;
        }
    }
    else
    {
        *p++ = SHRSA_SIGN;

        while( nb_pad-- > 0 )
            *p++ = 0xFF;
    }

    *p++ = 0;
    memcpy( p, input, ilen );

    return( ( mode == SHRSA_PUBLIC )
            ? shrsa_public(  ctx, output, output )
            : shrsa_private( ctx, f_rng, p_rng, output, output ) );
}
#endif /* SHMEM_ALG_RSA_PKCS_V15 */

/*
 * Add the message padding, then do an RSA operation
 */
int shrsa_pkcs1_encrypt( shrsa_t *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t ilen,
                       const unsigned char *input,
                       unsigned char *output )
{
  switch( ctx->padding )
  {
#if defined(SHMEM_ALG_RSA_PKCS_V15)
    case SHMEM_ALG_RSA_PKCS_V15:
      return shrsa_rsaes_pkcs1_v15_encrypt( ctx, f_rng, p_rng, mode, ilen,
          input, output );
#endif
#if defined(SHMEM_ALG_RSA_PKCS_V21)
    case SHMEM_ALG_RSA_PKCS_V21:
      return shrsa_rsaes_oaep_encrypt( ctx, f_rng, p_rng, mode, NULL, 0,
          ilen, input, output );
#endif
    default:
      return( SHRSA_ERR_INVALID_PADDING );
  }
}

#if defined(SHMEM_ALG_RSA_PKCS_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-DECRYPT function
 */
int shrsa_rsaes_oaep_decrypt( shrsa_t *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t *olen,
                            const unsigned char *input,
                            unsigned char *output,
                            size_t output_max_len )
{
    int ret;
    size_t ilen, i, pad_len;
    unsigned char *p, bad, pad_done;
    unsigned char buf[SHMPI_MAX_SIZE];
    unsigned char lhash[SHRSA_MD_MAX_SIZE];
    unsigned int hlen;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    /*
     * Parameters sanity checks
     */
    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V21 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    md_info = mbedtls_md_info_from_type( (shrsa_md_type_t) ctx->hash_id );
    if( md_info == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    hlen = mbedtls_md_get_size( md_info );

    // checking for integer underflow
    if( 2 * hlen + 2 > ilen )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    /*
     * RSA operation
     */
    ret = ( mode == SHRSA_PUBLIC )
          ? shrsa_public(  ctx, input, buf )
          : shrsa_private( ctx, f_rng, p_rng, input, buf );

    if( ret != 0 )
        return( ret );

    /*
     * Unmask data and generate lHash
     */
    mbedtls_md_init( &md_ctx );
    if( ( ret = mbedtls_md_setup( &md_ctx, md_info, 0 ) ) != 0 )
    {
        mbedtls_md_free( &md_ctx );
        return( ret );
    }


    /* Generate lHash */
    mbedtls_md( md_info, label, label_len, lhash );

    /* seed: Apply seedMask to maskedSeed */
    mgf_mask( buf + 1, hlen, buf + hlen + 1, ilen - hlen - 1,
               &md_ctx );

    /* DB: Apply dbMask to maskedDB */
    mgf_mask( buf + hlen + 1, ilen - hlen - 1, buf + 1, hlen,
               &md_ctx );

    mbedtls_md_free( &md_ctx );

    /*
     * Check contents, in "constant-time"
     */
    p = buf;
    bad = 0;

    bad |= *p++; /* First byte must be 0 */

    p += hlen; /* Skip seed */

    /* Check lHash */
    for( i = 0; i < hlen; i++ )
        bad |= lhash[i] ^ *p++;

    /* Get zero-padding len, but always read till end of buffer
     * (minus one, for the 01 byte) */
    pad_len = 0;
    pad_done = 0;
    for( i = 0; i < ilen - 2 * hlen - 2; i++ )
    {
        pad_done |= p[i];
        pad_len += ((pad_done | (unsigned char)-pad_done) >> 7) ^ 1;
    }

    p += pad_len;
    bad |= *p++ ^ 0x01;

    /*
     * The only information "leaked" is whether the padding was correct or not
     * (eg, no data is copied if it was not correct). This meets the
     * recommendations in PKCS#1 v2.2: an opponent cannot distinguish between
     * the different error conditions.
     */
    if( bad != 0 )
        return( SHRSA_ERR_INVALID_PADDING );

    if( ilen - ( p - buf ) > output_max_len )
        return( SHRSA_ERR_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}
#endif /* SHMEM_ALG_RSA_PKCS_V21 */

#if defined(SHMEM_ALG_RSA_PKCS_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-DECRYPT function
 */
int shrsa_rsaes_pkcs1_v15_decrypt( shrsa_t *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t *olen,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 size_t output_max_len)
{
    int ret;
    size_t ilen, pad_count = 0, i;
    unsigned char *p, bad, pad_done = 0;
    unsigned char buf[SHMPI_MAX_SIZE];

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V15 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    ret = ( mode == SHRSA_PUBLIC )
          ? shrsa_public(  ctx, input, buf )
          : shrsa_private( ctx, f_rng, p_rng, input, buf );

    if( ret != 0 )
        return( ret );

    p = buf;
    bad = 0;

    /*
     * Check and get padding len in "constant-time"
     */
    bad |= *p++; /* First byte must be 0 */

    /* This test does not depend on secret data */
    if( mode == SHRSA_PRIVATE )
    {
        bad |= *p++ ^ SHRSA_CRYPT;

        /* Get padding len, but always read till end of buffer
         * (minus one, for the 00 byte) */
        for( i = 0; i < ilen - 3; i++ )
        {
            pad_done  |= ((p[i] | (unsigned char)-p[i]) >> 7) ^ 1;
            pad_count += ((pad_done | (unsigned char)-pad_done) >> 7) ^ 1;
        }

        p += pad_count;
        bad |= *p++; /* Must be zero */
    }
    else
    {
        bad |= *p++ ^ SHRSA_SIGN;

        /* Get padding len, but always read till end of buffer
         * (minus one, for the 00 byte) */
        for( i = 0; i < ilen - 3; i++ )
        {
            pad_done |= ( p[i] != 0xFF );
            pad_count += ( pad_done == 0 );
        }

        p += pad_count;
        bad |= *p++; /* Must be zero */
    }

    bad |= ( pad_count < 8 );

    if( bad )
        return( SHRSA_ERR_INVALID_PADDING );

    if( ilen - ( p - buf ) > output_max_len )
        return( SHRSA_ERR_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}
#endif /* SHMEM_ALG_RSA_PKCS_V15 */

/*
 * Do an RSA operation, then remove the message padding
 */
int shrsa_pkcs1_decrypt( shrsa_t *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t *olen,
                       const unsigned char *input,
                       unsigned char *output,
                       size_t output_max_len)
{
    switch( ctx->padding )
    {
#if defined(SHMEM_ALG_RSA_PKCS_V15)
        case SHMEM_ALG_RSA_PKCS_V15:
            return shrsa_rsaes_pkcs1_v15_decrypt( ctx, f_rng, p_rng, mode, olen,
                                                input, output, output_max_len );
#endif

#if defined(SHMEM_ALG_RSA_PKCS_V21)
        case SHMEM_ALG_RSA_PKCS_V21:
            return shrsa_rsaes_oaep_decrypt( ctx, f_rng, p_rng, mode, NULL, 0,
                                           olen, input, output,
                                           output_max_len );
#endif

        default:
            return( SHRSA_ERR_INVALID_PADDING );
    }
}

#if defined(SHMEM_ALG_RSA_PKCS_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-SIGN function
 */
int shrsa_rsassa_pss_sign( shrsa_t *ctx,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
                         int mode,
                         shrsa_md_type_t md_alg,
                         unsigned int hashlen,
                         const unsigned char *hash,
                         unsigned char *sig )
{
    size_t olen;
    unsigned char *p = sig;
    unsigned char salt[SHRSA_MD_MAX_SIZE];
    unsigned int slen, hlen, offset = 0;
    int ret;
    size_t msb;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V21 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    if( f_rng == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    olen = ctx->len;

    if( md_alg != SHRSA_MD_NONE )
    {
        /* Gather length of hash to sign */
        md_info = mbedtls_md_info_from_type( md_alg );
        if( md_info == NULL )
            return( SHRSA_ERR_BAD_INPUT_DATA );

        hashlen = mbedtls_md_get_size( md_info );
    }

    md_info = mbedtls_md_info_from_type( (shrsa_md_type_t) ctx->hash_id );
    if( md_info == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    hlen = mbedtls_md_get_size( md_info );
    slen = hlen;

    if( olen < hlen + slen + 2 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    memset( sig, 0, olen );

    /* Generate salt of length slen */
    if( ( ret = f_rng( p_rng, salt, slen ) ) != 0 )
        return( SHRSA_ERR_RNG_FAILED + ret );

    /* Note: EMSA-PSS encoding is over the length of N - 1 bits */
    msb = shmpi_bitlen( &ctx->N ) - 1;
    p += olen - hlen * 2 - 2;
    *p++ = 0x01;
    memcpy( p, salt, slen );
    p += slen;

    mbedtls_md_init( &md_ctx );
    if( ( ret = mbedtls_md_setup( &md_ctx, md_info, 0 ) ) != 0 )
    {
        mbedtls_md_free( &md_ctx );
        return( ret );
    }

    /* Generate H = Hash( M' ) */
    mbedtls_md_starts( &md_ctx );
    mbedtls_md_update( &md_ctx, p, 8 );
    mbedtls_md_update( &md_ctx, hash, hashlen );
    mbedtls_md_update( &md_ctx, salt, slen );
    mbedtls_md_finish( &md_ctx, p );

    /* Compensate for boundary condition when applying mask */
    if( msb % 8 == 0 )
        offset = 1;

    /* maskedDB: Apply dbMask to DB */
    mgf_mask( sig + offset, olen - hlen - 1 - offset, p, hlen, &md_ctx );

    mbedtls_md_free( &md_ctx );

    msb = shmpi_bitlen( &ctx->N ) - 1;
    sig[0] &= 0xFF >> ( olen * 8 - msb );

    p += hlen;
    *p++ = 0xBC;

    return( ( mode == SHRSA_PUBLIC )
            ? shrsa_public(  ctx, sig, sig )
            : shrsa_private( ctx, f_rng, p_rng, sig, sig ) );
}
#endif /* SHMEM_ALG_RSA_PKCS_V21 */

#if defined(SHMEM_ALG_RSA_PKCS_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-V1_5-SIGN function
 */
/*
 * Do an RSA operation to sign the message digest
 */
int shrsa_rsassa_pkcs1_v15_sign( shrsa_t *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               shrsa_md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               unsigned char *sig )
{
    size_t nb_pad, olen, oid_size = 0;
    unsigned char *p = sig;
    const char *oid = NULL;
    unsigned char *sig_try = NULL, *verif = NULL;
    size_t i;
    unsigned char diff;
    volatile unsigned char diff_no_optimize;
    int ret;

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V15 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    olen = ctx->len;
    nb_pad = olen - 3;

    if( md_alg != SHRSA_MD_NONE )
    {
        const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type( md_alg );
        if( md_info == NULL )
            return( SHRSA_ERR_BAD_INPUT_DATA );

        if( mbedtls_oid_get_oid_by_md( md_alg, &oid, &oid_size ) != 0 )
            return( SHRSA_ERR_BAD_INPUT_DATA );

        nb_pad -= 10 + oid_size;

        hashlen = mbedtls_md_get_size( md_info );
    }

    nb_pad -= hashlen;

    if( ( nb_pad < 8 ) || ( nb_pad > olen ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    *p++ = 0;
    *p++ = SHRSA_SIGN;
    memset( p, 0xFF, nb_pad );
    p += nb_pad;
    *p++ = 0;

    if( md_alg == SHRSA_MD_NONE )
    {
        memcpy( p, hash, hashlen );
    }
    else
    {
        /*
         * DigestInfo ::= SEQUENCE {
         *   digestAlgorithm DigestAlgorithmIdentifier,
         *   digest Digest }
         *
         * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
         *
         * Digest ::= OCTET STRING
         */
        *p++ = SHMEM_ALG_ASN1_SEQUENCE | SHMEM_ALG_ASN1_CONSTRUCTED;
        *p++ = (unsigned char) ( 0x08 + oid_size + hashlen );
        *p++ = SHMEM_ALG_ASN1_SEQUENCE | SHMEM_ALG_ASN1_CONSTRUCTED;
        *p++ = (unsigned char) ( 0x04 + oid_size );
        *p++ = SHMEM_ALG_ASN1_OID;
        *p++ = oid_size & 0xFF;
        memcpy( p, oid, oid_size );
        p += oid_size;
        *p++ = SHMEM_ALG_ASN1_NULL;
        *p++ = 0x00;
        *p++ = SHMEM_ALG_ASN1_OCTET_STRING;
        *p++ = hashlen;
        memcpy( p, hash, hashlen );
    }

    if( mode == SHRSA_PUBLIC )
        return( shrsa_public(  ctx, sig, sig ) );

    /*
     * In order to prevent Lenstra's attack, make the signature in a
     * temporary buffer and check it before returning it.
     */
    sig_try = calloc( 1, ctx->len );
    if( sig_try == NULL )
        return( SHMEM_ALG_ERR_MPI_ALLOC_FAILED );

    verif   = calloc( 1, ctx->len );
    if( verif == NULL )
    {
        free( sig_try );
        return( SHMEM_ALG_ERR_MPI_ALLOC_FAILED );
    }

    MPI_CHK( shrsa_private( ctx, f_rng, p_rng, sig, sig_try ) );
    MPI_CHK( shrsa_public( ctx, sig_try, verif ) );

    /* Compare in constant time just in case */
    for( diff = 0, i = 0; i < ctx->len; i++ )
        diff |= verif[i] ^ sig[i];
    diff_no_optimize = diff;

    if( diff_no_optimize != 0 )
    {
        ret = SHRSA_ERR_PRIVATE_FAILED;
        goto cleanup;
    }

    memcpy( sig, sig_try, ctx->len );

cleanup:
    free( sig_try );
    free( verif );

    return( ret );
}
#endif /* SHMEM_ALG_RSA_PKCS_V15 */

/*
 * Do an RSA operation to sign the message digest
 */
int shrsa_pkcs1_sign( shrsa_t *ctx,
                    int (*f_rng)(void *, unsigned char *, size_t),
                    void *p_rng,
                    int mode,
                    shrsa_md_type_t md_alg,
                    unsigned int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig )
{
    switch( ctx->padding )
    {
#if defined(SHMEM_ALG_RSA_PKCS_V15)
        case SHMEM_ALG_RSA_PKCS_V15:
            return shrsa_rsassa_pkcs1_v15_sign( ctx, f_rng, p_rng, mode, md_alg,
                                              hashlen, hash, sig );
#endif

#if defined(SHMEM_ALG_RSA_PKCS_V21)
        case SHMEM_ALG_RSA_PKCS_V21:
            return shrsa_rsassa_pss_sign( ctx, f_rng, p_rng, mode, md_alg,
                                        hashlen, hash, sig );
#endif

        default:
            return( SHRSA_ERR_INVALID_PADDING );
    }
}

#if defined(SHMEM_ALG_RSA_PKCS_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int shrsa_rsassa_pss_verify_ext( shrsa_t *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               shrsa_md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               shrsa_md_type_t mgf1_hash_id,
                               int expected_salt_len,
                               const unsigned char *sig )
{
    int ret;
    size_t siglen;
    unsigned char *p;
    unsigned char result[SHRSA_MD_MAX_SIZE];
    unsigned char zeros[8];
    unsigned int hlen;
    size_t slen, msb;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;
    unsigned char buf[SHMPI_MAX_SIZE];

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V21 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    siglen = ctx->len;

    if( siglen < 16 || siglen > sizeof( buf ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    ret = ( mode == SHRSA_PUBLIC )
          ? shrsa_public(  ctx, sig, buf )
          : shrsa_private( ctx, f_rng, p_rng, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( buf[siglen - 1] != 0xBC )
        return( SHRSA_ERR_INVALID_PADDING );

    if( md_alg != SHRSA_MD_NONE )
    {
        /* Gather length of hash to sign */
        md_info = mbedtls_md_info_from_type( md_alg );
        if( md_info == NULL )
            return( SHRSA_ERR_BAD_INPUT_DATA );

        hashlen = mbedtls_md_get_size( md_info );
    }

    md_info = mbedtls_md_info_from_type( mgf1_hash_id );
    if( md_info == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    hlen = mbedtls_md_get_size( md_info );
    slen = siglen - hlen - 1; /* Currently length of salt + padding */

    memset( zeros, 0, 8 );

    /*
     * Note: EMSA-PSS verification is over the length of N - 1 bits
     */
    msb = shmpi_bitlen( &ctx->N ) - 1;

    /* Compensate for boundary condition when applying mask */
    if( msb % 8 == 0 )
    {
        p++;
        siglen -= 1;
    }
    if( buf[0] >> ( 8 - siglen * 8 + msb ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    mbedtls_md_init( &md_ctx );
    if( ( ret = mbedtls_md_setup( &md_ctx, md_info, 0 ) ) != 0 )
    {
        mbedtls_md_free( &md_ctx );
        return( ret );
    }

    mgf_mask( p, siglen - hlen - 1, p + siglen - hlen - 1, hlen, &md_ctx );

    buf[0] &= 0xFF >> ( siglen * 8 - msb );

    while( p < buf + siglen && *p == 0 )
        p++;

    if( p == buf + siglen ||
        *p++ != 0x01 )
    {
        mbedtls_md_free( &md_ctx );
        return( SHRSA_ERR_INVALID_PADDING );
    }

    /* Actual salt len */
    slen -= p - buf;

    if( expected_salt_len != SHMEM_ALG_RSA_SALT_LEN_ANY &&
        slen != (size_t) expected_salt_len )
    {
        mbedtls_md_free( &md_ctx );
        return( SHRSA_ERR_INVALID_PADDING );
    }

    /*
     * Generate H = Hash( M' )
     */
    mbedtls_md_starts( &md_ctx );
    mbedtls_md_update( &md_ctx, zeros, 8 );
    mbedtls_md_update( &md_ctx, hash, hashlen );
    mbedtls_md_update( &md_ctx, p, slen );
    mbedtls_md_finish( &md_ctx, result );

    mbedtls_md_free( &md_ctx );

    if( memcmp( p + slen, result, hlen ) == 0 )
        return( 0 );
    else
        return( SHRSA_ERR_VERIFY_FAILED );
}

/*
 * Simplified PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int shrsa_rsassa_pss_verify( shrsa_t *ctx,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng,
                           int mode,
                           shrsa_md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           const unsigned char *sig )
{
    shrsa_md_type_t mgf1_hash_id = ( ctx->hash_id != SHRSA_MD_NONE )
                             ? (shrsa_md_type_t) ctx->hash_id
                             : md_alg;

    return( shrsa_rsassa_pss_verify_ext( ctx, f_rng, p_rng, mode,
                                       md_alg, hashlen, hash,
                                       mgf1_hash_id, SHMEM_ALG_RSA_SALT_LEN_ANY,
                                       sig ) );

}
#endif /* SHMEM_ALG_RSA_PKCS_V21 */

#if defined(SHMEM_ALG_RSA_PKCS_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-v1_5-VERIFY function
 */
int shrsa_rsassa_pkcs1_v15_verify( shrsa_t *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode,
                                 shrsa_md_type_t md_alg,
                                 unsigned int hashlen,
                                 const unsigned char *hash,
                                 const unsigned char *sig )
{
    int ret;
    size_t len, siglen, asn1_len;
    unsigned char *p, *end;
    shrsa_md_type_t msg_md_alg;
    const mbedtls_md_info_t *md_info;
    mbedtls_asn1_buf oid;
    unsigned char buf[SHMPI_MAX_SIZE];

    if( mode == SHRSA_PRIVATE && ctx->padding != SHMEM_ALG_RSA_PKCS_V15 )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    siglen = ctx->len;

    if( siglen < 16 || siglen > sizeof( buf ) )
        return( SHRSA_ERR_BAD_INPUT_DATA );

    ret = ( mode == SHRSA_PUBLIC )
          ? shrsa_public(  ctx, sig, buf )
          : shrsa_private( ctx, f_rng, p_rng, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( *p++ != 0 || *p++ != SHRSA_SIGN )
        return( SHRSA_ERR_INVALID_PADDING );

    while( *p != 0 )
    {
        if( p >= buf + siglen - 1 || *p != 0xFF )
            return( SHRSA_ERR_INVALID_PADDING );
        p++;
    }
    p++;

    len = siglen - ( p - buf );

    if( len == hashlen && md_alg == SHRSA_MD_NONE )
    {
        if( memcmp( p, hash, hashlen ) == 0 )
            return( 0 );
        else
            return( SHRSA_ERR_VERIFY_FAILED );
    }

    md_info = mbedtls_md_info_from_type( md_alg );
    if( md_info == NULL )
        return( SHRSA_ERR_BAD_INPUT_DATA );
    hashlen = mbedtls_md_get_size( md_info );

    end = p + len;

    /*
     * Parse the ASN.1 structure inside the PKCS#1 v1.5 structure
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &asn1_len,
            SHMEM_ALG_ASN1_CONSTRUCTED | SHMEM_ALG_ASN1_SEQUENCE ) ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( asn1_len + 2 != len )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &asn1_len,
            SHMEM_ALG_ASN1_CONSTRUCTED | SHMEM_ALG_ASN1_SEQUENCE ) ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( asn1_len + 6 + hashlen != len )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &oid.len, SHMEM_ALG_ASN1_OID ) ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    oid.p = p;
    p += oid.len;

    if( mbedtls_oid_get_md_alg( &oid, &msg_md_alg ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( md_alg != msg_md_alg )
        return( SHRSA_ERR_VERIFY_FAILED );

    /*
     * assume the algorithm parameters must be NULL
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &asn1_len, SHMEM_ALG_ASN1_NULL ) ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &asn1_len, SHMEM_ALG_ASN1_OCTET_STRING ) ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( asn1_len != hashlen )
        return( SHRSA_ERR_VERIFY_FAILED );

    if( memcmp( p, hash, hashlen ) != 0 )
        return( SHRSA_ERR_VERIFY_FAILED );

    p += hashlen;

    if( p != end )
        return( SHRSA_ERR_VERIFY_FAILED );

    return( 0 );
}
#endif /* SHMEM_ALG_RSA_PKCS_V15 */

/*
 * Do an RSA operation and check the message digest
 */
int shrsa_pkcs1_verify( shrsa_t *ctx,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng,
                      int mode,
                      shrsa_md_type_t md_alg,
                      unsigned int hashlen,
                      const unsigned char *hash,
                      const unsigned char *sig )
{
    switch( ctx->padding )
    {
#if defined(SHMEM_ALG_RSA_PKCS_V15)
        case SHMEM_ALG_RSA_PKCS_V15:
            return shrsa_rsassa_pkcs1_v15_verify( ctx, f_rng, p_rng, mode, md_alg,
                                                hashlen, hash, sig );
#endif

#if defined(SHMEM_ALG_RSA_PKCS_V21)
        case SHMEM_ALG_RSA_PKCS_V21:
            return shrsa_rsassa_pss_verify( ctx, f_rng, p_rng, mode, md_alg,
                                          hashlen, hash, sig );
#endif

        default:
            return( SHRSA_ERR_INVALID_PADDING );
    }
}

/*
 * Copy the components of an RSA key
 */
int shrsa_copy( shrsa_t *dst, const shrsa_t *src )
{
    int ret;

    dst->ver = src->ver;
    dst->len = src->len;

    MPI_CHK( shmpi_copy( &dst->N, &src->N ) );
    MPI_CHK( shmpi_copy( &dst->E, &src->E ) );

    MPI_CHK( shmpi_copy( &dst->D, &src->D ) );
    MPI_CHK( shmpi_copy( &dst->P, &src->P ) );
    MPI_CHK( shmpi_copy( &dst->Q, &src->Q ) );
    MPI_CHK( shmpi_copy( &dst->DP, &src->DP ) );
    MPI_CHK( shmpi_copy( &dst->DQ, &src->DQ ) );
    MPI_CHK( shmpi_copy( &dst->QP, &src->QP ) );

    MPI_CHK( shmpi_copy( &dst->RN, &src->RN ) );
    MPI_CHK( shmpi_copy( &dst->RP, &src->RP ) );
    MPI_CHK( shmpi_copy( &dst->RQ, &src->RQ ) );

    MPI_CHK( shmpi_copy( &dst->Vi, &src->Vi ) );
    MPI_CHK( shmpi_copy( &dst->Vf, &src->Vf ) );

    dst->padding = src->padding;
    dst->hash_id = src->hash_id;

cleanup:
    if( ret != 0 )
        shrsa_free( dst );

    return( ret );
}

/*
 * Free the components of an RSA key
 */
void shrsa_free( shrsa_t *ctx )
{
    shmpi_free( &ctx->Vi ); shmpi_free( &ctx->Vf );
    shmpi_free( &ctx->RQ ); shmpi_free( &ctx->RP ); shmpi_free( &ctx->RN );
    shmpi_free( &ctx->QP ); shmpi_free( &ctx->DQ ); shmpi_free( &ctx->DP );
    shmpi_free( &ctx->Q  ); shmpi_free( &ctx->P  ); shmpi_free( &ctx->D );
    shmpi_free( &ctx->E  ); shmpi_free( &ctx->N  );

}








/* self test routine */

#define KEY_LEN 128

#define RSA_N   "9292758453063D803DD603D5E777D788" \
                "8ED1D5BF35786190FA2F23EBC0848AEA" \
                "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
                "7130B9CED7ACDF54CFC7555AC14EEBAB" \
                "93A89813FBF3C4F8066D2D800F7C38A8" \
                "1AE31942917403FF4946B0A83D3D3E05" \
                "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
                "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
                "66CA472BC44D253102F8B4A9D3BFA750" \
                "91386C0077937FE33FA3252D28855837" \
                "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
                "DF79C5CE07EE72C7F123142198164234" \
                "CABB724CF78B8173B9F880FC86322407" \
                "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
                "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
                "2C01CAD19EA484A87EA4377637E75500" \
                "FCB2005C5C7DD6EC4AC023CDA285D796" \
                "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
                "E211C2B9E5DB1ED0BF61D0D9899620F4" \
                "910E4168387E3C30AA1E00C339A79508" \
                "8452DD96A9A5EA5D9DCA68DA636032AF"

#define RSA_DP  "C1ACF567564274FB07A0BBAD5D26E298" \
                "3C94D22288ACD763FD8E5600ED4A702D" \
                "F84198A5F06C2E72236AE490C93F07F8" \
                "3CC559CD27BC2D1CA488811730BB5725"

#define RSA_DQ  "4959CBF6F8FEF750AEE6977C155579C7" \
                "D8AAEA56749EA28623272E4F7D0592AF" \
                "7C1F1313CAC9471B5C523BFE592F517B" \
                "407A1BD76C164B93DA2D32A383E58357"

#define RSA_QP  "9AE7FBC99546432DF71896FC239EADAE" \
                "F38D18D2B2F0E2DD275AA977E2BF4411" \
                "F5A3B2A5D33605AEBBCCBA7FEB9F2D2F" \
                "A74206CEC169D74BF5A8C50D6F48EA08"

#define PT_LEN  24
#define RSA_PT  "\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF" \
                "\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD"

static int myrand( void *rng_state, unsigned char *output, size_t len )
{
#if !defined(__OpenBSD__)
    size_t i;

    if( rng_state != NULL )
        rng_state  = NULL;

    for( i = 0; i < len; ++i )
        output[i] = rand();
#else
    if( rng_state != NULL )
        rng_state = NULL;

    arc4random_buf( output, len );
#endif /* !OpenBSD */

    return( 0 );
}
_TEST(shrsa)
{
#if defined(SHMEM_ALG_RSA_PKCS_V15)
  int ret = 0;
  size_t len;
  shrsa_t rsa;
  unsigned char rsa_plaintext[PT_LEN];
  unsigned char rsa_decrypted[PT_LEN];
  unsigned char rsa_ciphertext[KEY_LEN];
  int err;

  shrsa_init( &rsa, SHMEM_ALG_RSA_PKCS_V15, 0 );

  rsa.len = KEY_LEN;
  _TRUE(0 == shmpi_read_string( &rsa.N , 16, RSA_N  ));
  _TRUE(0 == shmpi_read_string( &rsa.E , 16, RSA_E  ));
  _TRUE(0 == shmpi_read_string( &rsa.D , 16, RSA_D  ));
  _TRUE(0 == shmpi_read_string( &rsa.P , 16, RSA_P  ));
  _TRUE(0 == shmpi_read_string( &rsa.Q , 16, RSA_Q  ));
  _TRUE(0 == shmpi_read_string( &rsa.DP, 16, RSA_DP ));
  _TRUE(0 == shmpi_read_string( &rsa.DQ, 16, RSA_DQ ));
  _TRUE(0 == shmpi_read_string( &rsa.QP, 16, RSA_QP ));

  _TRUE(0 == shrsa_check_pubkey(  &rsa ));
  _TRUE(0 == shrsa_check_privkey( &rsa ));

  memcpy( rsa_plaintext, RSA_PT, PT_LEN );
  err = shrsa_pkcs1_encrypt( &rsa, myrand, NULL, SHRSA_PUBLIC, PT_LEN, rsa_plaintext, rsa_ciphertext );
if (err) fprintf(stderr, "DEBUG: %d = shrsa_pkcs1_encrypt()\n", err);

  _TRUE(0 == shrsa_pkcs1_decrypt( &rsa, myrand, NULL, SHRSA_PRIVATE, &len, rsa_ciphertext, rsa_decrypted, sizeof(rsa_decrypted) ));

  _TRUE(0 == memcmp( rsa_decrypted, rsa_plaintext, len ));
#endif
}





