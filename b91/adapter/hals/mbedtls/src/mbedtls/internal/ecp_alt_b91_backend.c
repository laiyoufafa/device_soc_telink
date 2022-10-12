/*
 *  Elliptic curves over GF(p): generic functions
 *
 *  Copyright The Mbed TLS Contributors
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
 */

/*
 * References:
 *
 * SEC1 http://www.secg.org/index.php?action=secg,docs_secg
 * GECC = Guide to Elliptic Curve Cryptography - Hankerson, Menezes, Vanstone
 * FIPS 186-3 http://csrc.nist.gov/publications/fips/fips186-3/fips_186-3.pdf
 * RFC 4492 for the related TLS structures and constants
 * RFC 7748 for the Curve448 and Curve25519 curve definitions
 *
 * [Curve25519] http://cr.yp.to/ecdh/curve25519-20060209.pdf
 *
 * [2] CORON, Jean-S'ebastien. Resistance against differential power analysis
 *     for elliptic curve cryptosystems. In : Cryptographic Hardware and
 *     Embedded Systems. Springer Berlin Heidelberg, 1999. p. 292-302.
 *     <http://link.springer.com/chapter/10.1007/3-540-48059-5_25>
 *
 * [3] HEDABOU, Mustapha, PINEL, Pierre, et B'EN'ETEAU, Lucien. A comb method to
 *     render ECC resistant against Side Channel Attacks. IACR Cryptology
 *     ePrint Archive, 2004, vol. 2004, p. 342.
 *     <http://eprint.iacr.org/2004/342.pdf>
 */

#include "common.h"

#if defined( MBEDTLS_ECP_C )

#include "mbedtls/ecp.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"
#include "pke.h"
#include "multithread.h"
#include <string.h>

#if defined( MBEDTLS_ECP_ALT )

#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)   ||   \
    defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)   ||   \
    defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)   ||   \
    defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED) ||   \
    defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
#define ECP_SHORTWEIERSTRASS
#endif

#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED) || \
    defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
#define ECP_MONTGOMERY
#endif

/*
 * Curve types: internal for now, might be exposed later
 */
typedef enum
{
    ECP_TYPE_NONE = 0,
    ECP_TYPE_SHORT_WEIERSTRASS,    /* y^2 = x^3 + a x + b      */
    ECP_TYPE_MONTGOMERY,           /* y^2 = x^3 + a x^2 + x    */
} ecp_curve_type;

/*
 * Get the type of a curve
 */
static inline ecp_curve_type ecp_get_type( const mbedtls_ecp_group *grp )
{
    if( grp->G.X.p == NULL )
        return( ECP_TYPE_NONE );

    if( grp->G.Y.p == NULL )
        return( ECP_TYPE_MONTGOMERY );
    else
        return( ECP_TYPE_SHORT_WEIERSTRASS );
}

/****************************************************************
 * HW unit curve data constants definition
 ****************************************************************/

#if defined( MBEDTLS_ECP_DP_SECP256R1_ENABLED )
static eccp_curve_t secp256r1 =
{
    .eccp_p_bitLen = 256,
    .eccp_p = ( unsigned int[] )
    {
        0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x00000003, 0x00000000, 0xffffffff, 0xfffffffb, 0xfffffffe, 0xffffffff, 0xfffffffd, 0x00000004
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x00000001
    },
    .eccp_a = ( unsigned int[] )
    {
        0xfffffffc, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xffffffff
    },
    .eccp_b = ( unsigned int[] )
    {
        0x27d2604b, 0x3bce3c3e, 0xcc53b0f6, 0x651d06b0, 0x769886bc, 0xb3ebbd55, 0xaa3a93e7, 0x5ac635d8
    }
};
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */

#if defined( MBEDTLS_ECP_DP_SECP256K1_ENABLED )
static eccp_curve_t secp256k1 =
{
    .eccp_p_bitLen = 256,
    .eccp_p = ( unsigned int[] )
    {
        0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x000e90a1, 0x000007a2, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0xd2253531
    },
    .eccp_a = ( unsigned int[] )
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_b = ( unsigned int[] )
    {
        0x00000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
#endif /* MBEDTLS_ECP_DP_SECP256K1_ENABLED */

#if defined( MBEDTLS_ECP_DP_BP256R1_ENABLED )
static eccp_curve_t BP256r1 =
{
    .eccp_p_bitLen = 256,
    .eccp_p = ( unsigned int[] )
    {
        0x1f6e5377, 0x2013481d, 0xd5262028, 0x6e3bf623, 0x9d838d72, 0x3e660a90, 0xa1eea9bc, 0xa9fb57db
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0xa6465b6c, 0x8cfedf7b, 0x614d4f4d, 0x5cce4c26, 0x6b1ac807, 0xa1ecdacd, 0xe5957fa8, 0x4717aa21
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0xcefd89b9
    },
    .eccp_a = ( unsigned int[] )
    {
        0xf330b5d9, 0xe94a4b44, 0x26dc5c6c, 0xfb8055c1, 0x417affe7, 0xeef67530, 0xfc2c3057, 0x7d5a0975
    },
    .eccp_b = ( unsigned int[] )
    {
        0xff8c07b6, 0x6bccdc18, 0x5cf7e1ce, 0x95841629, 0xbbd77cbf, 0xf330b5d9, 0xe94a4b44, 0x26dc5c6c
    }
};
#endif /* MBEDTLS_ECP_DP_BP256R1_ENABLED */

#if defined( MBEDTLS_ECP_DP_SECP224R1_ENABLED )
static eccp_curve_t secp224r1 =
{
    .eccp_p_bitLen = 224,
    .eccp_p = ( unsigned int[] )
    {
        0x00000001, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x00000001, 0x00000000, 0x00000000, 0xfffffffe, 0xffffffff, 0xffffffff, 0x00000000
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0xffffffff
    },
    .eccp_a = ( unsigned int[] )
    {
        0xfffffffe, 0xffffffff, 0xffffffff, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_b = ( unsigned int[] )
    {
        0x2355ffb4, 0x270b3943, 0xd7bfd8ba, 0x5044b0b7, 0xf5413256, 0x0c04b3ab, 0xb4050a85
    }
};
#endif /* MBEDTLS_ECP_DP_SECP224R1_ENABLED */

#if defined( MBEDTLS_ECP_DP_SECP224K1_ENABLED )
static eccp_curve_t secp224k1 =
{
    .eccp_p_bitLen = 224,
    .eccp_p = ( unsigned int[] )
    {
        0xffffe56d, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x02c23069, 0x00003526, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x198d139b
    },
    .eccp_a = ( unsigned int[] )
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_b = ( unsigned int[] )
    {
        0x00000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
#endif /* MBEDTLS_ECP_DP_SECP224K1_ENABLED */

#if defined( MBEDTLS_ECP_DP_SECP192R1_ENABLED )
static eccp_curve_t secp192r1 =
{
    .eccp_p_bitLen = 192,
    .eccp_p = ( unsigned int[] )
    {
        0xffffffff, 0xffffffff, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000001, 0x00000000
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x00000001
    },
    .eccp_a = ( unsigned int[] )
    {
        0xfffffffc, 0xffffffff, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_b = ( unsigned int[] )
    {
        0xc146b9b1, 0xfeb8deec, 0x72243049, 0x0fa7e9ab, 0xe59c80e7, 0x64210519
    }
};
#endif /* MBEDTLS_ECP_DP_SECP192R1_ENABLED */

#if defined( MBEDTLS_ECP_DP_SECP192K1_ENABLED )
static eccp_curve_t secp192k1 =
{
    .eccp_p_bitLen = 192,
    .eccp_p = ( unsigned int[] )
    {
        0xffffee37, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x013c4fd1, 0x00002392, 0x00000001, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_p_h = ( unsigned int[] )
    {
        0x7446d879
    },
    .eccp_a = ( unsigned int[] )
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .eccp_b = ( unsigned int[] )
    {
        0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
#endif /* MBEDTLS_ECP_DP_SECP192K1_ENABLED */

#if defined( MBEDTLS_ECP_DP_CURVE25519_ENABLED )
static mont_curve_t x25519 =
{
    .mont_p_bitLen = 255,
    .mont_p = ( unsigned int[] )
    {
        0xffffffed, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff
    },
    .mont_p_h = ( unsigned int[] )
    {
        0x000005a4, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    },
    .mont_p_n1 = ( unsigned int[] )
    {
        0x286bca1b
    },
    .mont_a24 = ( unsigned int[] )
    {
        0x0001db41, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    }
};
#endif /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */


/****************************************************************
 * Linking mbedtls to HW unit curve data
 ****************************************************************/

#if defined( ECP_SHORTWEIERSTRASS )
static const struct
{
    mbedtls_ecp_group_id    group;
    eccp_curve_t *          curve_dat;
}
eccp_curve_linking[] =
{
#if defined( MBEDTLS_ECP_DP_SECP256R1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP256R1,
        .curve_dat = &secp256r1
    },
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */
#if defined( MBEDTLS_ECP_DP_SECP256K1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP256K1,
        .curve_dat = &secp256k1
    },
#endif /* MBEDTLS_ECP_DP_SECP256K1_ENABLED */
#if defined( MBEDTLS_ECP_DP_BP256R1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_BP256R1,
        .curve_dat = &BP256r1
    },
#endif /* MBEDTLS_ECP_DP_BP256R1_ENABLED */
#if defined( MBEDTLS_ECP_DP_SECP224R1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP224R1,
        .curve_dat = &secp224r1
    },
#endif /* MBEDTLS_ECP_DP_SECP224R1_ENABLED */
#if defined( MBEDTLS_ECP_DP_SECP224K1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP224K1,
        .curve_dat = &secp224k1
    },
#endif /* MBEDTLS_ECP_DP_SECP224K1_ENABLED */
#if defined( MBEDTLS_ECP_DP_SECP192R1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP192R1,
        .curve_dat = &secp192r1
    },
#endif /* MBEDTLS_ECP_DP_SECP192R1_ENABLED */
#if defined( MBEDTLS_ECP_DP_SECP192K1_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_SECP192K1,
        .curve_dat = &secp192k1
    }
#endif /* MBEDTLS_ECP_DP_SECP192K1_ENABLED */
};
#endif /* ECP_SHORTWEIERSTRASS */

#if defined( ECP_MONTGOMERY )
static const struct
{
    mbedtls_ecp_group_id    group;
    mont_curve_t *          curve_dat;
}
mont_curve_linking[] =
{
#if defined( MBEDTLS_ECP_DP_CURVE25519_ENABLED )
    {
        .group = MBEDTLS_ECP_DP_CURVE25519,
        .curve_dat = &x25519
    }
#endif /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */
};
#endif /* ECP_MONTGOMERY */


/****************************************************************
 * Private functions declaration
 ****************************************************************/

#define ciL    (sizeof(mbedtls_mpi_uint))         /* chars in limb  */

/* Get a specific byte, without range checks. */
#define GET_BYTE( X, i )                                \
    ( ( ( X )->p[( i ) / ciL] >> ( ( ( i ) % ciL ) * 8 ) ) & 0xff )

#define CHARS_TO_LIMBS(i) ( (i) / ciL + ( (i) % ciL != 0 ) )

/*
 * Export X into unsigned binary data, little endian
 */
int mbedtls_mpi_write_binary_le( const mbedtls_mpi *X,
                                 unsigned char *buf, size_t buflen )
{
    size_t stored_bytes = X->n * ciL;
    size_t bytes_to_copy;
    size_t i;

    if( stored_bytes < buflen )
    {
        bytes_to_copy = stored_bytes;
    }
    else
    {
        bytes_to_copy = buflen;

        /* The output buffer is smaller than the allocated size of X.
         * However X may fit if its leading bytes are zero. */
        for( i = bytes_to_copy; i < stored_bytes; i++ )
        {
            if( GET_BYTE( X, i ) != 0 )
                return( MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL );
        }
    }

    for( i = 0; i < bytes_to_copy; i++ )
        buf[i] = GET_BYTE( X, i );

    if( stored_bytes < buflen )
    {
        /* Write trailing 0 bytes */
        memset( buf + stored_bytes, 0, buflen - stored_bytes );
    }

    return( 0 );
}

/*
 * Import X from unsigned binary data, little endian
 */
int mbedtls_mpi_read_binary_le( mbedtls_mpi *X,
                                const unsigned char *buf, size_t buflen )
{
    int ret;
    size_t i;
    size_t const limbs = CHARS_TO_LIMBS( buflen );

    /* Ensure that target MPI has exactly the necessary number of limbs */
    // MBEDTLS_MPI_CHK( mbedtls_mpi_resize_clear( X, limbs ) );

    /* Ensure that target MPI has exactly the necessary number of limbs */
    if( X->n != limbs )
    {
        mbedtls_mpi_free( X );
        mbedtls_mpi_init( X );
        MBEDTLS_MPI_CHK( mbedtls_mpi_grow( X, limbs ) );
    }
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( X, 0 ) );

    for( i = 0; i < buflen; i++ )
        X->p[i / ciL] |= ((mbedtls_mpi_uint) buf[i]) << ((i % ciL) << 3);

cleanup:

    /*
     * This function is also used to import keys. However, wiping the buffers
     * upon failure is not necessary because failure only can happen before any
     * input is copied.
     */
    return( ret );
}

#if defined( ECP_SHORTWEIERSTRASS )
static eccp_curve_t * eccp_curve_get( const mbedtls_ecp_group *grp )
{
    eccp_curve_t * eccp_curve = NULL;

    for ( size_t i = 0; i < sizeof( eccp_curve_linking ) / sizeof( eccp_curve_linking[0] ); i++ )
    {
        if ( eccp_curve_linking[i].group == grp->id )
        {
            eccp_curve = eccp_curve_linking[i].curve_dat;
            break;
        }
    }

    return eccp_curve;
}
#endif /* ECP_SHORTWEIERSTRASS */

#if defined( ECP_MONTGOMERY )
static mont_curve_t * mont_curve_get( const mbedtls_ecp_group *grp )
{
    mont_curve_t * mont_curve = NULL;

    for ( size_t i = 0; i < sizeof( mont_curve_linking ) / sizeof( mont_curve_linking[0] ); i++ )
    {
        if ( mont_curve_linking[i].group == grp->id )
        {
            mont_curve = mont_curve_linking[i].curve_dat;
            break;
        }
    }

    return mont_curve;
}
#endif /* ECP_MONTGOMERY */


/****************************************************************
 * Public functions declaration
 ****************************************************************/

int ecp_alt_b91_backend_check_pubkey( const mbedtls_ecp_group *grp, const mbedtls_ecp_point *pt )
{
    int result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

    if ( grp != NULL && pt != NULL )
    {
        // Tl_printf("%s:%d\r\n", __func__, __LINE__);
        result = MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
        const unsigned int word_len = GET_WORD_LEN( grp->pbits );

        if ( word_len <= PKE_OPERAND_MAX_WORD_LEN )
        {
            unsigned int Qx[word_len], Qy[word_len];

#if defined( ECP_SHORTWEIERSTRASS )
            if ( ecp_get_type( grp ) == ECP_TYPE_SHORT_WEIERSTRASS )
            {
                eccp_curve_t * eccp_curve = eccp_curve_get( grp );
                if ( eccp_curve != NULL )
                {
                    ( void ) mbedtls_mpi_write_binary_le( &pt->X, ( unsigned char * )Qx, sizeof( Qx ) );
                    ( void ) mbedtls_mpi_write_binary_le( &pt->Y, ( unsigned char * )Qy, sizeof( Qy ) );

                    mbedtls_ecp_lock();
                    if ( pke_eccp_point_verify( eccp_curve, Qx, Qy ) == PKE_SUCCESS )
                        result = 0;
                    else
                        result = MBEDTLS_ERR_ECP_INVALID_KEY;
                    mbedtls_ecp_unlock();
                }
            }
#endif /* ECP_SHORTWEIERSTRASS */

            memset( Qx, 0, sizeof( Qx ) );
            memset( Qy, 0, sizeof( Qy ) );
        }
    }
    return result;
}

int ecp_alt_b91_backend_mul( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                             const mbedtls_mpi *m, const mbedtls_ecp_point *P )
{
    int result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

    if ( grp != NULL && R != NULL && m != NULL && P != NULL )
    {

        // Tl_printf("%s:%d\r\n", __func__, __LINE__);
        result = MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
        const unsigned int word_len = GET_WORD_LEN( grp->pbits );

        if ( word_len <= PKE_OPERAND_MAX_WORD_LEN )
        {
            unsigned int ms[word_len], Qx[word_len], Qy[word_len];

            // Tl_printf("%s:%d\r\n", __func__, __LINE__);
#if defined( ECP_SHORTWEIERSTRASS )
            // Tl_printf("type: %d\r\n",  ecp_get_type( grp ) );
            if ( ecp_get_type( grp ) == ECP_TYPE_SHORT_WEIERSTRASS )
            {
        // Tl_printf("%s:%d\r\n", __func__, __LINE__);
                eccp_curve_t * eccp_curve = eccp_curve_get( grp );
                if ( eccp_curve != NULL )
                {
                    ( void ) mbedtls_mpi_write_binary_le( m, ( unsigned char * )ms, sizeof( ms ) );
                    ( void ) mbedtls_mpi_write_binary_le( &P->X, ( unsigned char * )Qx, sizeof( Qx ) );
                    ( void ) mbedtls_mpi_write_binary_le( &P->Y, ( unsigned char * )Qy, sizeof( Qy ) );

                    mbedtls_ecp_lock();
                    if ( pke_eccp_point_mul( eccp_curve, ms, Qx, Qy, Qx, Qy ) == PKE_SUCCESS )
                    {
                        ( void ) mbedtls_mpi_read_binary_le( &R->X, ( const unsigned char * )Qx, sizeof( Qx ) );
                        ( void ) mbedtls_mpi_read_binary_le( &R->Y, ( const unsigned char * )Qy, sizeof( Qy ) );
                        ( void ) mbedtls_mpi_lset( &R->Z, 1 );
                        result = 0;
                    }
                    else
                        result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
                    mbedtls_ecp_unlock();
                }
            }
#endif /* ECP_SHORTWEIERSTRASS */
#if defined( ECP_MONTGOMERY )
            if ( ecp_get_type( grp ) == ECP_TYPE_MONTGOMERY )
            {
        // Tl_printf("%s:%d\r\n", __func__, __LINE__);
                mont_curve_t * mont_curve = mont_curve_get( grp );
                if ( mont_curve != NULL )
                {
                    ( void ) mbedtls_mpi_write_binary_le( m, ( unsigned char * )ms, sizeof( ms ) );
                    ( void ) mbedtls_mpi_write_binary_le( &P->X, ( unsigned char * )Qx, sizeof( Qx ) );

                    mbedtls_ecp_lock();
                    if ( pke_x25519_point_mul( mont_curve, ms, Qx, Qx ) == PKE_SUCCESS )
                    {
                        ( void ) mbedtls_mpi_read_binary_le( &R->X, ( const unsigned char * )Qx, sizeof( Qx ) );
                        ( void ) mbedtls_mpi_lset( &R->Y, 0 );
                        ( void ) mbedtls_mpi_lset( &R->Z, 1 );
                        result = 0;
                    }
                    else
                        result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
                    mbedtls_ecp_unlock();
                }
            }
#endif /* ECP_MONTGOMERY */

            memset( ms, 0, sizeof( ms ) );
            memset( Qx, 0, sizeof( Qx ) );
            memset( Qy, 0, sizeof( Qy ) );
        }
    }
    return result;
}

int ecp_alt_b91_backend_muladd( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                                const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                                const mbedtls_mpi *n, const mbedtls_ecp_point *Q )
{


    int result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

    if ( grp != NULL && R != NULL && m != NULL && P != NULL && n != NULL && Q != NULL )
    {

        // Tl_printf("%s:%d\r\n", __func__, __LINE__);
        result = MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
        const unsigned int word_len = GET_WORD_LEN( grp->pbits );

        if ( word_len <= PKE_OPERAND_MAX_WORD_LEN )
        {
            unsigned int ms[word_len], Q1x[word_len], Q1y[word_len], Q2x[word_len], Q2y[word_len];

#if defined( ECP_SHORTWEIERSTRASS )
            if ( ecp_get_type( grp ) == ECP_TYPE_SHORT_WEIERSTRASS )
            {
                eccp_curve_t * eccp_curve = eccp_curve_get( grp );
                if ( eccp_curve != NULL )
                {
                    ( void ) mbedtls_mpi_write_binary_le( &P->X, ( unsigned char * )Q1x, sizeof( Q1x ) );
                    ( void ) mbedtls_mpi_write_binary_le( &P->Y, ( unsigned char * )Q1y, sizeof( Q1y ) );
                    ( void ) mbedtls_mpi_write_binary_le( &Q->X, ( unsigned char * )Q2x, sizeof( Q2x ) );
                    ( void ) mbedtls_mpi_write_binary_le( &Q->Y, ( unsigned char * )Q2y, sizeof( Q2y ) );

                    result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

                    mbedtls_ecp_lock();

                    do
                    {
                        ( void ) mbedtls_mpi_write_binary_le( m, ( unsigned char * )ms, sizeof( ms ) );
                        if ( pke_eccp_point_mul( eccp_curve, ms, Q1x, Q1y, Q1x, Q1y ) != PKE_SUCCESS )
                            break;

                        ( void ) mbedtls_mpi_write_binary_le( n, ( unsigned char * )ms, sizeof( ms ) );
                        if ( pke_eccp_point_mul( eccp_curve, ms, Q2x, Q2y, Q2x, Q2y ) != PKE_SUCCESS )
                            break;

                        if ( pke_eccp_point_add( eccp_curve, Q1x, Q1y, Q2x, Q2y, Q1x, Q1y ) != PKE_SUCCESS )
                            break;

                        ( void ) mbedtls_mpi_read_binary_le( &R->X, ( const unsigned char * )Q1x, sizeof( Q1x ) );
                        ( void ) mbedtls_mpi_read_binary_le( &R->Y, ( const unsigned char * )Q1y, sizeof( Q1y ) );
                        ( void ) mbedtls_mpi_lset( &R->Z, 1 );
                        result = 0;

                    }
                    while ( 0 );

                    mbedtls_ecp_unlock();
                }
            }
#endif /* ECP_SHORTWEIERSTRASS */

            memset( ms, 0, sizeof( ms ) );
            memset( Q1x, 0, sizeof( Q1x ) );
            memset( Q1y, 0, sizeof( Q1y ) );
            memset( Q2x, 0, sizeof( Q2x ) );
            memset( Q2y, 0, sizeof( Q2y ) );
        }
    }
    return result;
}

#endif /* MBEDTLS_ECP_ALT */

#endif /* MBEDTLS_ECP_C */
