/********************************************************************************************************
 * @file    eccp_curve.c
 *
 * @brief   This is the source file for TL323X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include <stdio.h>

#include "lib/include/crypto_common/eccp_curve.h"


/* CAUTION:
 * for PKE_LP and PKE_SECURE, secp160r1, secp160r2, secp160k1 and secp224k1 are
 * so particular, their n are 1 bit longer than p, and n are 1 word longer than
 * p, but the IP montgomery parameter width is 32 bits, so we have to set the
 * curve parameter p width as n length(this affects the p_h value), to support
 * point multiplication while the scalar may be longer than p. but for PKE_HP,
 * PKE_UHP, the IP montgomery parameter width is 256 bits, so there is no
 * problem.
 */

/**************************** brainpoolp160r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP160R1
static const unsigned int brainpoolp160r1_p[5] = {
    0x9515620FU, 0x95B3D813U, 0x60DFC7ADU, 0x737059DCU, 0xE95E4A5FU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp160r1_p_h[5] = {
    0x532B7BEBU, 0xC57E4353U, 0xB4BA7FB8U, 0x4CC30F3BU, 0xB3945136U,
};
#else
static const unsigned int brainpoolp160r1_p_h[5] = {
    0x25BC14FFU, 0xB333F8D6U, 0xFED717E0U, 0xC0CA7EF8U, 0x6CF12F81U,
};
static const unsigned int brainpoolp160r1_p_n0[1] = {
    0xADBCB311U,
};
#endif
static const unsigned int brainpoolp160r1_a[5] = {
    0xE8F7C300U, 0xDA745D97U, 0xE2BE61BAU, 0xA280EB74U, 0x340E7BE2U,
};
static const unsigned int brainpoolp160r1_b[5] = {
    0xD8675E58U, 0xBDEC95C8U, 0x134FAA2DU, 0x95423412U, 0x1E589A85U,
};
static const unsigned int brainpoolp160r1_Gx[5] = {
    0xBDBCDBC3U, 0x31EB5AF7U, 0x62938C46U, 0xEA3F6A4FU, 0xBED5AF16U,
};
static const unsigned int brainpoolp160r1_Gy[5] = {
    0x16DA6321U, 0x669C9763U, 0x38F94741U, 0x7A1A8EC3U, 0x1667CB47U,
};
static const unsigned int brainpoolp160r1_n[5] = {
    0x9E60FC09U, 0xD4502940U, 0x60DF5991U, 0x737059DCU, 0xE95E4A5FU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp160r1_n_h[5] = {
    0x9ADFB54BU, 0xE00DFA53U, 0x0E7C2B8DU, 0x5C5494B1U, 0x9B44D4F6U,
};
#else
static const unsigned int brainpoolp160r1_n_h[5] = {
    0x1FDF90EAU, 0xFC61D435U, 0x9E31FE16U, 0xFC9BE6F6U, 0x2BC73851U,
};
static const unsigned int brainpoolp160r1_n_n0[1] = {
    0x5C7AADC7U,
};
#endif

//[2^80]G
#ifdef PKE_HP
static const unsigned int brainpoolp160r1_2_80_Gx[5] = {
    0xB19BD5A1U, 0xD3B8210CU, 0xBB518725U, 0x39FC8E94U, 0x8E63BD39U,
};
static const unsigned int brainpoolp160r1_2_80_Gy[5] = {
    0x68DE4448U, 0xAF1015A3U, 0x8E138836U, 0x17A68390U, 0x4D1E977BU,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp160r1[1] = {
    {
        160u,
        160u,
        brainpoolp160r1_p,
        brainpoolp160r1_p_h, // NULL,//
        brainpoolp160r1_a,
        brainpoolp160r1_b,
        brainpoolp160r1_Gx,
        brainpoolp160r1_Gy,
        brainpoolp160r1_n,
        brainpoolp160r1_n_h, // NULL,//
        brainpoolp160r1_2_80_Gx,
        brainpoolp160r1_2_80_Gy,
    },
};
#else
const eccp_curve_t brainpoolp160r1[1] = {
    {
        160u,
        160u,
        brainpoolp160r1_p,
        brainpoolp160r1_p_h,
        brainpoolp160r1_p_n0,
        brainpoolp160r1_a,
        brainpoolp160r1_b,
        brainpoolp160r1_Gx,
        brainpoolp160r1_Gy,
        brainpoolp160r1_n,
        brainpoolp160r1_n_h,
        brainpoolp160r1_n_n0,
    },
};
#endif
#endif

/**************************** brainpoolp192r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP192R1
static const unsigned int brainpoolp192r1_p[6] = {
    0xE1A86297U, 0x8FCE476DU, 0x93D18DB7U, 0xA7A34630U, 0x932A36CDU, 0xC302F41DU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp192r1_p_h[6] = {
    0xF1375215U, 0x01E662B2U, 0x62E2228FU, 0x6AE9CFFAU, 0x2760DC78U, 0xB0AFEBA3U,
};
#else
static const unsigned int brainpoolp192r1_p_h[6] = {
    0x72C7B21AU, 0xE2474C69U, 0x02C3FE69U, 0x33BF4846U, 0xEED34F10U, 0xB6225126U,
};
static const unsigned int brainpoolp192r1_p_n0[1] = {
    0x56A2C2D9U,
};
#endif
static const unsigned int brainpoolp192r1_a[6] = {
    0xC69A28EFU, 0xCAE040E5U, 0xFE8685C1U, 0x9C39C031U, 0x76B1E0E1U, 0x6A911740U,
};
static const unsigned int brainpoolp192r1_b[6] = {
    0x6FBF25C9U, 0xCA7EF414U, 0x4F4496BCU, 0xDC721D04U, 0x7C28CCA3U, 0x469A28EFU,
};
static const unsigned int brainpoolp192r1_Gx[6] = {
    0x53375FD6U, 0x0A2F5C48U, 0x6CB0F090U, 0x53B033C5U, 0xAAB6A487U, 0xC0A0647EU,
};
static const unsigned int brainpoolp192r1_Gy[6] = {
    0xFA299B8FU, 0xE6773FA2U, 0xC1490002U, 0x8B5F4828U, 0x6ABD5BB8U, 0x14B69086U,
};
static const unsigned int brainpoolp192r1_n[6] = {
    0x9AC4ACC1U, 0x5BE8F102U, 0x9E9E916BU, 0xA7A3462FU, 0x932A36CDU, 0xC302F41DU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp192r1_n_h[6] = {
    0xE7FCB8CEU, 0x6ECF9194U, 0x9F23B907U, 0xCFB52741U, 0x85FD1D24U, 0xB2C3A70BU,
};
#else
static const unsigned int brainpoolp192r1_n_h[6] = {
    0xE407E8F8U, 0xB4727C80U, 0xBF53AFF0U, 0xBF4AFD5DU, 0xE772102BU, 0x98769B9CU,
};
static const unsigned int brainpoolp192r1_n_n0[1] = {
    0x75DE1CBFU,
};
#endif

//[2^96]G
#ifdef PKE_HP
static const unsigned int brainpoolp192r1_2_96_Gx[6] = {
    0x6E55384CU, 0xB4006CD5U, 0xB5B55527U, 0x086B6F1DU, 0x1E972E92U, 0x773D0297U,
};
static const unsigned int brainpoolp192r1_2_96_Gy[6] = {
    0x3014234CU, 0x32828E3DU, 0x6634C2CEU, 0xA3D9346EU, 0x01CE0488U, 0x807F615CU,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp192r1[1] = {
    {
        192u,
        192u,
        brainpoolp192r1_p,
        brainpoolp192r1_p_h, // NULL,//
        brainpoolp192r1_a,
        brainpoolp192r1_b,
        brainpoolp192r1_Gx,
        brainpoolp192r1_Gy,
        brainpoolp192r1_n,
        brainpoolp192r1_n_h, // NULL,//
        brainpoolp192r1_2_96_Gx,
        brainpoolp192r1_2_96_Gy,
    },
};
#else
const eccp_curve_t brainpoolp192r1[1] = {
    {
        192u, 192u, brainpoolp192r1_p, brainpoolp192r1_p_h, brainpoolp192r1_p_n0, brainpoolp192r1_a, brainpoolp192r1_b, brainpoolp192r1_Gx, brainpoolp192r1_Gy, brainpoolp192r1_n,
        brainpoolp192r1_n_h,  // NULL,//
        brainpoolp192r1_n_n0, // NULL,//
    },
};
#endif
#endif

/**************************** brainpoolp224r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP224R1
static const unsigned int brainpoolp224r1_p[7] = {
    0x7EC8C0FFU, 0x97DA89F5U, 0xB09F0757U, 0x75D1D787U, 0x2A183025U, 0x26436686U, 0xD7C134AAU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp224r1_p_h[7] = {
    0x6B3D58FFU, 0x3BFBC9BFU, 0xF691D105U, 0x76F9EE46U, 0x77E3D7E4U, 0x7EA5577CU, 0x2B3D40DDU,
};
#else
static const unsigned int brainpoolp224r1_p_h[7] = {
    0x64DCD04FU, 0x7867CA80U, 0x43C20E72U, 0x96AF774CU, 0x3FE8A2AAU, 0x2E6A6CE4U, 0x0578FD59U,
};
static const unsigned int brainpoolp224r1_p_n0[1] = {
    0xE149C101U,
};
#endif
static const unsigned int brainpoolp224r1_a[7] = {
    0xCAD29F43U, 0xB0042A59U, 0x4E182AD8U, 0xC1530B51U, 0x299803A6U, 0xA9CE6C1CU, 0x68A5E62CU,
};
static const unsigned int brainpoolp224r1_b[7] = {
    0x386C400BU, 0x66DBB372U, 0x3E2135D2U, 0xA92369E3U, 0x870713B1U, 0xCFE44138U, 0x2580F63CU,
};
static const unsigned int brainpoolp224r1_Gx[7] = {
    0xEE12C07DU, 0x4C1E6EFDU, 0x9E4CE317U, 0xA87DC68CU, 0x340823B2U, 0x2C7E5CF4U, 0x0D9029ADU,
};
static const unsigned int brainpoolp224r1_Gy[7] = {
    0x761402CDU, 0xCAA3F6D3U, 0x354B9E99U, 0x4ECDAC24U, 0x24C6B89EU, 0x72C0726FU, 0x58AA56F7U,
};
static const unsigned int brainpoolp224r1_n[7] = {
    0xA5A7939FU, 0x6DDEBCA3U, 0xD116BC4BU, 0x75D0FB98U, 0x2A183025U, 0x26436686U, 0xD7C134AAU,
};
#ifdef PKE_HP
static const unsigned int brainpoolp224r1_n_h[7] = {
    0xE0D86B49U, 0xF3D67605U, 0xBAB96B21U, 0xF0F40A07U, 0xA35371E1U, 0xB4581327U, 0x5234FE17U,
};
#else
static const unsigned int brainpoolp224r1_n_h[7] = {
    0x486CA401U, 0xADDAF8AAU, 0x9399652CU, 0x9F24919BU, 0x1E9CAE24U, 0x3211A561U, 0x4A73A656U,
};
static const unsigned int brainpoolp224r1_n_n0[1] = {
    0x6CFB37A1U,
};
#endif

//[2^112]G
#ifdef PKE_HP
static const unsigned int brainpoolp224r1_2_112_Gx[7] = {
    0x18671CACU, 0x794ADD48U, 0x619F1D35U, 0xEE591079U, 0xD7C84B0EU, 0x0699B4A5U, 0xB3CDA5BCU,
};
static const unsigned int brainpoolp224r1_2_112_Gy[7] = {
    0xBB4A5447U, 0x6E6080C1U, 0x4D88B767U, 0x3B07A68CU, 0xD6D750A1U, 0xFEF32E40U, 0xBC2DC034U,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp224r1[1] = {
    {
        224u,
        224u,
        brainpoolp224r1_p,
        brainpoolp224r1_p_h,
        brainpoolp224r1_a,
        brainpoolp224r1_b,
        brainpoolp224r1_Gx,
        brainpoolp224r1_Gy,
        brainpoolp224r1_n,
        brainpoolp224r1_n_h,
        brainpoolp224r1_2_112_Gx,
        brainpoolp224r1_2_112_Gy,
    },
};
#else
const eccp_curve_t brainpoolp224r1[1] = {
    {
        224u,
        224u,
        brainpoolp224r1_p,
        brainpoolp224r1_p_h,
        brainpoolp224r1_p_n0,
        brainpoolp224r1_a,
        brainpoolp224r1_b,
        brainpoolp224r1_Gx,
        brainpoolp224r1_Gy,
        brainpoolp224r1_n,
        brainpoolp224r1_n_h,
        brainpoolp224r1_n_n0,
    },
};
#endif
#endif

/**************************** brainpoolp256r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP256R1
static const unsigned int brainpoolp256r1_p[8] = {
    0x1F6E5377U, 0x2013481DU, 0xD5262028U, 0x6E3BF623U, 0x9D838D72U, 0x3E660A90U, 0xA1EEA9BCU, 0xA9FB57DBU,
};
static const unsigned int brainpoolp256r1_p_h[8] = {
    0xA6465B6CU, 0x8CFEDF7BU, 0x614D4F4DU, 0x5CCE4C26U, 0x6B1AC807U, 0xA1ECDACDU, 0xE5957FA8U, 0x4717AA21U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int brainpoolp256r1_p_n0[1] = {
    0xCEFD89B9U,
};
#endif
static const unsigned int brainpoolp256r1_a[8] = {
    0xF330B5D9U, 0xE94A4B44U, 0x26DC5C6CU, 0xFB8055C1U, 0x417AFFE7U, 0xEEF67530U, 0xFC2C3057U, 0x7D5A0975U,
};
static const unsigned int brainpoolp256r1_b[8] = {
    0xFF8C07B6U, 0x6BCCDC18U, 0x5CF7E1CEU, 0x95841629U, 0xBBD77CBFU, 0xF330B5D9U, 0xE94A4B44U, 0x26DC5C6CU,
};
static const unsigned int brainpoolp256r1_Gx[8] = {
    0x9ACE3262U, 0x3A4453BDU, 0xE3BD23C2U, 0xB9DE27E1U, 0xFC81B7AFU, 0x2C4B482FU, 0xCB7E57CBU, 0x8BD2AEB9U,
};
static const unsigned int brainpoolp256r1_Gy[8] = {
    0x2F046997U, 0x5C1D54C7U, 0x2DED8E54U, 0xC2774513U, 0x14611DC9U, 0x97F8461AU, 0xC3DAC4FDU, 0x547EF835U,
};
static const unsigned int brainpoolp256r1_n[8] = {
    0x974856A7U, 0x901E0E82U, 0xB561A6F7U, 0x8C397AA3U, 0x9D838D71U, 0x3E660A90U, 0xA1EEA9BCU, 0xA9FB57DBU,
};
static const unsigned int brainpoolp256r1_n_h[8] = {
    0x3312FCA6U, 0xE1D8D8DEU, 0x1134E4A0U, 0xF35D176AU, 0x6C815CB0U, 0x9B7F25E7U, 0xC3236762U, 0x0B25F1B9U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int brainpoolp256r1_n_n0[1] = {
    0xCBB40EE9U,
};
#endif

//[2^128]G
#ifdef PKE_HP
static const unsigned int brainpoolp256r1_2_128_Gx[8] = {
    0xF58472C9U, 0xEB6B651CU, 0x11006590U, 0x1200CA9BU, 0x7F87ED9DU, 0xB4438511U, 0x3B856C94U, 0x4A14C030U,
};
static const unsigned int brainpoolp256r1_2_128_Gy[8] = {
    0x28F852D1U, 0x529C5CD6U, 0xCD732117U, 0xD544A068U, 0x8B47CC5EU, 0xE6387349U, 0xDAE2D5EFU, 0x7B81E470U,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp256r1[1] = {
    {
        256u,
        256u,
        brainpoolp256r1_p,
        brainpoolp256r1_p_h,
        brainpoolp256r1_a,
        brainpoolp256r1_b,
        brainpoolp256r1_Gx,
        brainpoolp256r1_Gy,
        brainpoolp256r1_n,
        brainpoolp256r1_n_h, // NULL,//
        brainpoolp256r1_2_128_Gx,
        brainpoolp256r1_2_128_Gy,
    },
};
#else
const eccp_curve_t brainpoolp256r1[1] = {
    {
        256u, 256u, brainpoolp256r1_p, brainpoolp256r1_p_h, brainpoolp256r1_p_n0, brainpoolp256r1_a, brainpoolp256r1_b, brainpoolp256r1_Gx, brainpoolp256r1_Gy, brainpoolp256r1_n,
        brainpoolp256r1_n_h,  // NULL,//
        brainpoolp256r1_n_n0, // NULL,//
    },
};
#endif
#endif

/**************************** brainpoolp320r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP320R1
static const unsigned int brainpoolp320r1_p[10] = {
    0xF1B32E27U, 0xFCD412B1U, 0x7893EC28U, 0x4F92B9ECU, 0xF6F40DEFU, 0xF98FCFA6U, 0xD201E065U, 0xE13C785EU, 0x36BC4FB7U, 0xD35E4720U,
};
#ifdef PKE_HP
static const unsigned int brainpoolp320r1_p_h[10] = {
    0x173FD2B9U, 0xB487F1A2U, 0x6246117AU, 0x639B0116U, 0xEA02CBC0U, 0x44C86B0DU, 0x9289E4ABU, 0xD6D9D773U, 0xD1FEA1C3U, 0xD2E2BD5AU,
};
#else
static const unsigned int brainpoolp320r1_p_h[10] = {
    0x743B52F9U, 0x994EE88AU, 0x906978EFU, 0xC2478A8DU, 0x30C5B676U, 0x1F4C881FU, 0xE614D6D2U, 0x5455A964U, 0x6C2D9252U, 0xA259BA4AU,
};
static const unsigned int brainpoolp320r1_p_n0[1] = {
    0x2A8A9E69U,
};
#endif
static const unsigned int brainpoolp320r1_a[10] = {
    0x7D860EB4U, 0x92F375A9U, 0x85FFA9F4U, 0x66190EB0U, 0xF5EB79DAU, 0xA2A73513U, 0x6D3F3BB8U, 0x83CCEBD4U, 0x8FBAB0F8U, 0x3EE30B56U,
};
static const unsigned int brainpoolp320r1_b[10] = {
    0x8FB1F1A6U, 0x6F5EB4ACU, 0x88453981U, 0xCC31DCCDU, 0x9554B49AU, 0xE13F4134U, 0x40688A6FU, 0xD3AD1986U, 0x9DFDBC42U, 0x52088394U,
};
static const unsigned int brainpoolp320r1_Gx[10] = {
    0x39E20611U, 0x10AF8D0DU, 0x10A599C7U, 0xE7871E2AU, 0x0A087EB6U, 0xF20137D1U, 0x8EE5BFE6U, 0x5289BCC4U, 0xFB53D8B8U, 0x43BD7E9AU,
};
static const unsigned int brainpoolp320r1_Gy[10] = {
    0x692E8EE1U, 0xD35245D1U, 0xAAAC6AC7U, 0xA9C77877U, 0x117182EAU, 0x0743FFEDU, 0x7F77275EU, 0xAB409324U, 0x45EC1CC8U, 0x14FDD055U,
};
static const unsigned int brainpoolp320r1_n[10] = {
    0x44C59311U, 0x8691555BU, 0xEE8658E9U, 0x2D482EC7U, 0xB68F12A3U, 0xF98FCFA5U, 0xD201E065U, 0xE13C785EU, 0x36BC4FB7U, 0xD35E4720U,
};
#ifdef PKE_HP
static const unsigned int brainpoolp320r1_n_h[10] = {
    0x69C0E896U, 0x7C11F348U, 0xE18227E6U, 0xC61AF312U, 0x60B04341U, 0x6666D6A0U, 0xB83FEB01U, 0x66ED2F8BU, 0x017FEA5BU, 0x2606C905U,
};
#else
static const unsigned int brainpoolp320r1_n_h[10] = {
    0x2513E4CDU, 0x679D29DFU, 0xE0E16805U, 0x91C3001BU, 0xAF86C409U, 0x86B330BCU, 0x4E6390FEU, 0xE30D3524U, 0x3200B14FU, 0x31EC87C7U,
};
static const unsigned int brainpoolp320r1_n_n0[1] = {
    0xFC62420FU,
};
#endif

//[2^160]G
#ifdef PKE_HP
static const unsigned int brainpoolp320r1_2_160_Gx[10] = {
    0x7C841CBCU, 0x7723D2D1U, 0x13C4A1FAU, 0x20619B2AU, 0xB7ED9FD8U, 0x41C14B62U, 0xD324B0FFU, 0x37C47C97U, 0x675062D3U, 0x3BC19CAEU,
};
static const unsigned int brainpoolp320r1_2_160_Gy[10] = {
    0x45BEE1AAU, 0xEB2238EEU, 0x4F8EA777U, 0x68A3020AU, 0x2422C609U, 0xA45BC518U, 0x6B9BA3E5U, 0x704D6FB7U, 0xE7B2DC94U, 0x8C1DE9E0U,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp320r1[1] = {
    {
        320u,
        320u,
        brainpoolp320r1_p,
        brainpoolp320r1_p_h,
        brainpoolp320r1_a,
        brainpoolp320r1_b,
        brainpoolp320r1_Gx,
        brainpoolp320r1_Gy,
        brainpoolp320r1_n,
        brainpoolp320r1_n_h,
        brainpoolp320r1_2_160_Gx,
        brainpoolp320r1_2_160_Gy,
    },
};
#else
const eccp_curve_t brainpoolp320r1[1] = {
    {
        320u,
        320u,
        brainpoolp320r1_p,
        brainpoolp320r1_p_h,
        brainpoolp320r1_p_n0,
        brainpoolp320r1_a,
        brainpoolp320r1_b,
        brainpoolp320r1_Gx,
        brainpoolp320r1_Gy,
        brainpoolp320r1_n,
        brainpoolp320r1_n_h,
        brainpoolp320r1_n_n0,
    },
};
#endif
#endif

/**************************** brainpoolp384r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP384R1
static const unsigned int brainpoolp384r1_p[12] = {
    0x3107EC53U, 0x87470013U, 0x901D1A71U, 0xACD3A729U, 0x7FB71123U, 0x12B1DA19U, 0xED5456B4U, 0x152F7109U, 0x50E641DFU, 0x0F5D6F7EU, 0xA3386D28U, 0x8CB91E82U,
};
#ifdef PKE_HP
static const unsigned int brainpoolp384r1_p_h[12] = {
    0xF2E00133U, 0x41FB3B28U, 0x62CAC6F4U, 0xDC038BB7U, 0xD1773502U, 0x07B40011U, 0x37CF40F3U, 0x2DEB1A30U, 0xEB9AA2CAU, 0xCCE5246AU, 0x96900F16U, 0x273B1C6CU,
};
#else
static const unsigned int brainpoolp384r1_p_h[12] = {
    0x40B64BDEU, 0x087CEFFFU, 0x3D7FD965U, 0x53528334U, 0xC9940899U, 0x8E28F99CU, 0x9918D5AFU, 0x62140191U, 0xA57E052CU, 0xD5C6EF3BU, 0x178DF842U, 0x36BF6883U,
};
static const unsigned int brainpoolp384r1_p_n0[1] = {
    0xEA9EC825U,
};
#endif
static const unsigned int brainpoolp384r1_a[12] = {
    0x22CE2826U, 0x04A8C7DDU, 0x503AD4EBU, 0x8AA5814AU, 0xBA91F90FU, 0x139165EFU, 0x4FB22787U, 0xC2BEA28EU, 0xCE05AFA0U, 0x3C72080AU, 0x3D8C150CU, 0x7BC382C6U,
};
static const unsigned int brainpoolp384r1_b[12] = {
    0xFA504C11U, 0x3AB78696U, 0x95DBC994U, 0x7CB43902U, 0x3EEB62D5U, 0x2E880EA5U, 0x07DCD2A6U, 0x2FB77DE1U, 0x16F0447CU, 0x8B39B554U, 0x22CE2826U, 0x04A8C7DDU,
};
static const unsigned int brainpoolp384r1_Gx[12] = {
    0x47D4AF1EU, 0xEF87B2E2U, 0x36D646AAU, 0xE826E034U, 0x0CBD10E8U, 0xDB7FCAFEU, 0x7EF14FE3U, 0x8847A3E7U, 0xB7C13F6BU, 0xA2A63A81U, 0x68CF45FFU, 0x1D1C64F0U,
};
static const unsigned int brainpoolp384r1_Gy[12] = {
    0x263C5315U, 0x42820341U, 0x77918111U, 0x0E464621U, 0xF9912928U, 0xE19C054FU, 0xFEEC5864U, 0x62B70B29U, 0x95CFD552U, 0x5CB1EB8EU, 0x20F9C2A4U, 0x8ABE1D75U,
};
static const unsigned int brainpoolp384r1_n[12] = {
    0xE9046565U, 0x3B883202U, 0x6B7FC310U, 0xCF3AB6AFU, 0xAC0425A7U, 0x1F166E6CU, 0xED5456B3U, 0x152F7109U, 0x50E641DFU, 0x0F5D6F7EU, 0xA3386D28U, 0x8CB91E82U,
};
#ifdef PKE_HP
static const unsigned int brainpoolp384r1_n_h[12] = {
    0x4894533BU, 0x2FC7D3C1U, 0x16DB4EF9U, 0x18D9AF26U, 0x02483F80U, 0x6A0C5704U, 0x8D80258BU, 0xB8A756A8U, 0x3F1936AFU, 0xFDA24123U, 0x143E3D5CU, 0x354DFF02U,
};
#else
static const unsigned int brainpoolp384r1_n_h[12] = {
    0xDE771C8EU, 0xAC4ED3A2U, 0x2F2B6B6EU, 0x37264E20U, 0x9802688AU, 0x2A927E3BU, 0x52D748FFU, 0x574A74CBU, 0x65165FDBU, 0x8F886DC9U, 0x614E97C2U, 0x0CE8941AU,
};
static const unsigned int brainpoolp384r1_n_n0[1] = {
    0x5CB5BB93U,
};
#endif

//[2^192]G
#ifdef PKE_HP
static const unsigned int brainpoolp384r1_2_192_Gx[12] = {
    0x4D994B04U, 0x6BF550B0U, 0x5345A946U, 0x02A353BCU, 0x02C7F727U, 0xE18A5D2DU, 0xBB1FDC04U, 0xDF4DDACCU, 0x8E81AF98U, 0x8974B568U, 0x397C99F1U, 0x2369DBB6U,
};
static const unsigned int brainpoolp384r1_2_192_Gy[12] = {
    0x14504C85U, 0x0EE188BBU, 0x500CB2C1U, 0x43F143E6U, 0x3334CE66U, 0xCD41CAE9U, 0x0742FA8FU, 0x127368F1U, 0x925FF0A8U, 0x93B08775U, 0xAA3B124FU, 0x6F47B11DU,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp384r1[1] = {
    {
        384u,
        384u,
        brainpoolp384r1_p,
        brainpoolp384r1_p_h,
        brainpoolp384r1_a,
        brainpoolp384r1_b,
        brainpoolp384r1_Gx,
        brainpoolp384r1_Gy,
        brainpoolp384r1_n,
        brainpoolp384r1_n_h,
        brainpoolp384r1_2_192_Gx,
        brainpoolp384r1_2_192_Gy,
    },
};
#else
const eccp_curve_t brainpoolp384r1[1] = {
    {
        384u,
        384u,
        brainpoolp384r1_p,
        brainpoolp384r1_p_h,
        brainpoolp384r1_p_n0,
        brainpoolp384r1_a,
        brainpoolp384r1_b,
        brainpoolp384r1_Gx,
        brainpoolp384r1_Gy,
        brainpoolp384r1_n,
        brainpoolp384r1_n_h,
        brainpoolp384r1_n_n0,
    },
};
#endif
#endif

/**************************** brainpoolp512r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP512R1
static const unsigned int brainpoolp512r1_p[16] = {
    0x583A48F3U, 0x28AA6056U, 0x2D82C685U, 0x2881FF2FU, 0xE6A380E6U, 0xAECDA12AU, 0x9BC66842U, 0x7D4D9B00U,
    0x70330871U, 0xD6639CCAU, 0xB3C9D20EU, 0xCB308DB3U, 0x33C9FC07U, 0x3FD4E6AEU, 0xDBE9C48BU, 0xAADD9DB8U,
};
static const unsigned int brainpoolp512r1_p_h[16] = {
    0x6158F205U, 0x49AD144AU, 0x27157905U, 0x793FB130U, 0x905AFFD3U, 0x53B7F9BCU, 0x83514A25U, 0xE0C19A77U,
    0xD5898057U, 0x19486FD8U, 0xD42BFF83U, 0xA16DAA5FU, 0x2056EECCU, 0x202E1940U, 0xA9FF6450U, 0x3C4C9D05U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int brainpoolp512r1_p_n0[1] = {
    0x7D89EFC5U,
};
#endif
static const unsigned int brainpoolp512r1_a[16] = {
    0x77FC94CAU, 0xE7C1AC4DU, 0x2BF2C7B9U, 0x7F1117A7U, 0x8B9AC8B5U, 0x0A2EF1C9U, 0xA8253AA1U, 0x2DED5D5AU,
    0xEA9863BCU, 0xA83441CAU, 0x3DF91610U, 0x94CBDD8DU, 0xAC234CC5U, 0xE2327145U, 0x8B603B89U, 0x7830A331U,
};
static const unsigned int brainpoolp512r1_b[16] = {
    0x8016F723U, 0x2809BD63U, 0x5EBAE5DDU, 0x984050B7U, 0xDC083E67U, 0x77FC94CAU, 0xE7C1AC4DU, 0x2BF2C7B9U,
    0x7F1117A7U, 0x8B9AC8B5U, 0x0A2EF1C9U, 0xA8253AA1U, 0x2DED5D5AU, 0xEA9863BCU, 0xA83441CAU, 0x3DF91610U,
};
static const unsigned int brainpoolp512r1_Gx[16] = {
    0xBCB9F822U, 0x8B352209U, 0x406A5E68U, 0x7C6D5047U, 0x93B97D5FU, 0x50D1687BU, 0xE2D0D48DU, 0xFF3B1F78U,
    0xF4D0098EU, 0xB43B62EEU, 0xB5D916C1U, 0x85ED9F70U, 0x9C4C6A93U, 0x5A21322EU, 0xD82ED964U, 0x81AEE4BDU,
};
static const unsigned int brainpoolp512r1_Gy[16] = {
    0x3AD80892U, 0x78CD1E0FU, 0xA8F05406U, 0xD1CA2B2FU, 0x8A2763AEU, 0x5BCA4BD8U, 0x4A5F485EU, 0xB2DCDE49U,
    0x881F8111U, 0xA000C55BU, 0x24A57B1AU, 0xF209F700U, 0xCF7822FDU, 0xC0EABFA9U, 0x566332ECU, 0x7DDE385DU,
};
static const unsigned int brainpoolp512r1_n[16] = {
    0x9CA90069U, 0xB5879682U, 0x085DDADDU, 0x1DB1D381U, 0x7FAC1047U, 0x41866119U, 0x4CA92619U, 0x553E5C41U,
    0x70330870U, 0xD6639CCAU, 0xB3C9D20EU, 0xCB308DB3U, 0x33C9FC07U, 0x3FD4E6AEU, 0xDBE9C48BU, 0xAADD9DB8U,
};
static const unsigned int brainpoolp512r1_n_h[16] = {
    0xCDA81671U, 0xD2A3681EU, 0x95283DDDU, 0x0886B758U, 0x33B7627FU, 0x3EC64BD0U, 0x2F0207E8U, 0xA6F230C7U,
    0x3B790DE3U, 0xD7F9CC26U, 0x2F16BBDFU, 0x723C37A2U, 0x194B2E56U, 0x95DF1B4CU, 0x718407B0U, 0xA794586AU,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int brainpoolp512r1_n_n0[1] = {
    0x0F1B7027U,
};
#endif

//[2^256]G
#ifdef PKE_HP
static const unsigned int brainpoolp512r1_2_256_Gx[16] = {
    0xEF66EFB5U, 0xCA448127U, 0x08AC7DA5U, 0x9D189C54U, 0x730A3721U, 0x97502F33U, 0xACA3B94EU, 0x1BAE0F47U,
    0x945E8460U, 0x79A83D9BU, 0x8F284870U, 0xC056FD65U, 0xB8F1B2AFU, 0xD5262CE1U, 0x66C4ED95U, 0x7B5913F7U,
};
static const unsigned int brainpoolp512r1_2_256_Gy[16] = {
    0xC2F45872U, 0xD610453CU, 0xDA0092C9U, 0x3E9470F8U, 0x4EB496C3U, 0x24EC6F84U, 0xBC578877U, 0xFB4EDB0DU,
    0x5FC450A8U, 0xE46656A7U, 0x61306AF0U, 0xC21FCA91U, 0xEA20B708U, 0x3727EA1EU, 0x4FFC593AU, 0x3B4D8E45U,
};
#endif

#ifdef PKE_HP
const eccp_curve_t brainpoolp512r1[1] = {
    {
        512u,
        512u,
        brainpoolp512r1_p,
        brainpoolp512r1_p_h,
        brainpoolp512r1_a,
        brainpoolp512r1_b,
        brainpoolp512r1_Gx,
        brainpoolp512r1_Gy,
        brainpoolp512r1_n,
        brainpoolp512r1_n_h, // NULL,//
        brainpoolp512r1_2_256_Gx,
        brainpoolp512r1_2_256_Gy,
    },
};
#else
const eccp_curve_t brainpoolp512r1[1] = {
    {
        512u, 512u, brainpoolp512r1_p, brainpoolp512r1_p_h, brainpoolp512r1_p_n0, brainpoolp512r1_a, brainpoolp512r1_b, brainpoolp512r1_Gx, brainpoolp512r1_Gy, brainpoolp512r1_n,
        brainpoolp512r1_n_h,  // NULL,//
        brainpoolp512r1_n_n0, // NULL,//
    },
};
#endif
#endif

/**************************** secp160r1 ******************************/
#ifdef SUPPORT_SECP160R1
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r1_p[5] = {
    0x7FFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r1_p_h[5] = {
    0x00000000U, 0x80000001U, 0xC0000001U, 0x20000000U, 0x00000000U,
};
#elif (defined(PKE_LP))
static const unsigned int secp160r1_p[6] = {
    0x7FFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U,
};
static const unsigned int secp160r1_p_h[6] = {
    0x00000000U, 0x00000000U, 0x00000001U, 0x40000001U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp160r1_p_n0[1] = {
    0x80000001U,
};
#elif (defined(PKE_SECURE))
static const unsigned int secp160r1_p[5] = {
    0x7FFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r1_p_h[5] = {
    0x00000001U, 0x40000001U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp160r1_p_n0[1] = {
    0x80000001U,
};
#endif
static const unsigned int secp160r1_a[5] = {
    0x7FFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r1_b[5] = {
    0xC565FA45U, 0x81D4D4ADU, 0x65ACF89FU, 0x54BD7A8BU, 0x1C97BEFCU,
};
static const unsigned int secp160r1_Gx[5] = {
    0x13CBFC82U, 0x68C38BB9U, 0x46646989U, 0x8EF57328U, 0x4A96B568U,
};
static const unsigned int secp160r1_Gy[5] = {
    0x7AC5FB32U, 0x04235137U, 0x59DCC912U, 0x3168947DU, 0x23A62855U,
};
static const unsigned int secp160r1_n[6] = {
    0xCA752257U, 0xF927AED3U, 0x0001F4C8U, 0x00000000U, 0x00000000U, 0x00000001U,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r1_n_h[6] = {
    0xBD025FA7U, 0xDBB68B25U, 0xD1E8F73FU, 0xE9DD6F6CU, 0x39DE6382U, 0x00000000U,
};
#else
static const unsigned int secp160r1_n_h[6] = {
    0x6744F8A4U, 0x085E335FU, 0x3CDC3854U, 0x7A981E4BU, 0xA0E62683U, 0x00000000U,
};
static const unsigned int secp160r1_n_n0[1] = {
    0x306D1699U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r1_2_80_Gx[5] = {
    0x89665347U, 0xB21844A4U, 0xA2961A17U, 0xBE4AD4C7U, 0xF2E0A32FU,
};
static const unsigned int secp160r1_2_80_Gy[5] = {
    0x31B980CAU, 0x6519E3DEU, 0x92DEA640U, 0xCAA2F378U, 0x46B7032FU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp160r1[1] = {
    {
        160u,
        161u,
        secp160r1_p,
        secp160r1_p_h,
        secp160r1_a,
        secp160r1_b,
        secp160r1_Gx,
        secp160r1_Gy,
        secp160r1_n,
        secp160r1_n_h,
        secp160r1_2_80_Gx,
        secp160r1_2_80_Gy,
    },
};
#else
const eccp_curve_t secp160r1[1] = {
    {
        160u,
        161u,
        secp160r1_p,
        secp160r1_p_h,
        secp160r1_p_n0,
        secp160r1_a,
        secp160r1_b,
        secp160r1_Gx,
        secp160r1_Gy,
        secp160r1_n,
        secp160r1_n_h,
        secp160r1_n_n0,
    },
};
#endif
#endif

/**************************** secp160r2 ******************************/
#ifdef SUPPORT_SECP160R2
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r2_p[5] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r2_p_h[5] = {
    0x00000000U, 0x4DB32715U, 0x51CE3BE1U, 0x0000FAA7U, 0x00000001U,
};
#elif (defined(PKE_LP))
static const unsigned int secp160r2_p[6] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U,
};
static const unsigned int secp160r2_p_h[6] = {
    0x00000000U, 0x00000000U, 0x1B44BBA9U, 0x0000A71AU, 0x00000001U, 0x00000000U,
};
static const unsigned int secp160r2_p_n0[1] = {
    0xB4AB2745U,
};
#elif (defined(PKE_SECURE))
static const unsigned int secp160r2_p[5] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r2_p_h[5] = {
    0x1B44BBA9U, 0x0000A71AU, 0x00000001U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp160r2_p_n0[1] = {
    0xB4AB2745U,
};
#endif
static const unsigned int secp160r2_a[5] = {
    0xFFFFAC70U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160r2_b[5] = {
    0xF50388BAU, 0x04664D5AU, 0xAB572749U, 0xFB59EB8BU, 0xB4E134D3U,
};
static const unsigned int secp160r2_Gx[5] = {
    0x3144CE6DU, 0x30F7199DU, 0x1F4FF11BU, 0x293A117EU, 0x52DCB034U,
};
static const unsigned int secp160r2_Gy[5] = {
    0xA7D43F2EU, 0xF9982CFEU, 0xE071FA0DU, 0xE331F296U, 0xFEAFFEF2U,
};
static const unsigned int secp160r2_n[6] = {
    0xF3A1A16BU, 0xE786A818U, 0x0000351EU, 0x00000000U, 0x00000000U, 0x00000001U,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r2_n_h[6] = {
    0x214B0C57U, 0xD6937441U, 0x9EB754BCU, 0x39F295A5U, 0x11F4417DU, 0x00000000U,
};
#else
static const unsigned int secp160r2_n_h[6] = {
    0xD8C126C7U, 0x29AEB02AU, 0x8DD4D69EU, 0x4769EF9EU, 0x76E5A181U, 0x00000000U,
};
static const unsigned int secp160r2_n_n0[1] = {
    0xD9747CBDU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160r2_2_80_Gx[5] = {
    0x9FDBE7CCU, 0x2DD8DA5DU, 0x06A1961FU, 0x029E4D9BU, 0xAF6092BEU,
};
static const unsigned int secp160r2_2_80_Gy[5] = {
    0x31A7C90DU, 0x56D0EE47U, 0x952F7D35U, 0xDCD3FB29U, 0x3E9221CDU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp160r2[1] = {
    {
        160u,
        161u,
        secp160r2_p,
        secp160r2_p_h,
        secp160r2_a,
        secp160r2_b,
        secp160r2_Gx,
        secp160r2_Gy,
        secp160r2_n,
        secp160r2_n_h,
        secp160r2_2_80_Gx,
        secp160r2_2_80_Gy,
    },
};
#else
const eccp_curve_t secp160r2[1] = {
    {
        160u,
        161u,
        secp160r2_p,
        secp160r2_p_h,
        secp160r2_p_n0,
        secp160r2_a,
        secp160r2_b,
        secp160r2_Gx,
        secp160r2_Gy,
        secp160r2_n,
        secp160r2_n_h,
        secp160r2_n_n0,
    },
};
#endif
#endif

/**************************** secp192r1 ******************************/
#ifdef SUPPORT_SECP192R1
static const unsigned int secp192r1_p[6] = {
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192r1_p_h[6] = {
    0x00000002U, 0x00000000U, 0x00000003U, 0x00000000U, 0x00000002U, 0x00000000U,
};
#else
static const unsigned int secp192r1_p_h[6] = {
    0x00000001U, 0x00000000U, 0x00000002U, 0x00000000U, 0x00000001U, 0x00000000U,
};
static const unsigned int secp192r1_p_n0[1] = {
    1U,
};
#endif
static const unsigned int secp192r1_a[6] = {
    0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp192r1_b[6] = {
    0xC146B9B1U, 0xFEB8DEECU, 0x72243049U, 0x0FA7E9ABU, 0xE59C80E7U, 0x64210519U,
};
static const unsigned int secp192r1_Gx[6] = {
    0x82FF1012U, 0xF4FF0AFDU, 0x43A18800U, 0x7CBF20EBU, 0xB03090F6U, 0x188DA80EU,
};
static const unsigned int secp192r1_Gy[6] = {
    0x1E794811U, 0x73F977A1U, 0x6B24CDD5U, 0x631011EDU, 0xFFC8DA78U, 0x07192B95U,
};
static const unsigned int secp192r1_n[6] = {
    0xB4D22831U, 0x146BC9B1U, 0x99DEF836U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192r1_n_h[6] = {
    0x83134C27U, 0x01D1770AU, 0xCAAF687FU, 0xD69C6961U, 0xCEF5D8C5U, 0x126792C4U,
};
#else
static const unsigned int secp192r1_n_h[6] = {
    0xDEB35961U, 0xCE66BACCU, 0xBB3A6BEEU, 0x4696EA5BU, 0xEA0581A2U, 0x28BE5677U,
};
static const unsigned int secp192r1_n_n0[1] = {
    0x0DDBCF2FU,
};
#endif

//[2^96]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192r1_2_96_Gx[6] = {
    0xC0A1E340U, 0xB19963D8U, 0x80D1090BU, 0x4730D4F4U, 0x184AC737U, 0x51A581D9U,
};
static const unsigned int secp192r1_2_96_Gy[6] = {
    0xE69912A5U, 0xECC56731U, 0x2F683F16U, 0x7CDFCEA0U, 0xE0BB9F6EU, 0x5BD81EE2U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp192r1[1] = {
    {
        192u,
        192u,
        secp192r1_p,
        secp192r1_p_h,
        secp192r1_a,
        secp192r1_b,
        secp192r1_Gx,
        secp192r1_Gy,
        secp192r1_n,
        secp192r1_n_h,
        secp192r1_2_96_Gx,
        secp192r1_2_96_Gy,
    },
};
#else
const eccp_curve_t secp192r1[1] = {
    {
        192u,
        192u,
        secp192r1_p,
        secp192r1_p_h,
        secp192r1_p_n0,
        secp192r1_a,
        secp192r1_b,
        secp192r1_Gx,
        secp192r1_Gy,
        secp192r1_n,
        secp192r1_n_h,
        secp192r1_n_n0,
    },
};
#endif
#endif

/**************************** secp224r1 ******************************/
#ifdef SUPPORT_SECP224R1
static const unsigned int secp224r1_p[7] = {
    0x00000001U, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp224r1_p_h[7] = {
    0x00000001U, 0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFEU, 0xFFFFFFFFU,
};
#else
static const unsigned int secp224r1_p_h[7] = {
    0x00000001U, 0x00000000U, 0x00000000U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U,
};
static const unsigned int secp224r1_p_n0[1] = {
    0xFFFFFFFFU,
};
#endif
static const unsigned int secp224r1_a[7] = {
    0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp224r1_b[7] = {
    0x2355FFB4U, 0x270B3943U, 0xD7BFD8BAU, 0x5044B0B7U, 0xF5413256U, 0x0C04B3ABU, 0xB4050A85U,
};
static const unsigned int secp224r1_Gx[7] = {
    0x115C1D21U, 0x343280D6U, 0x56C21122U, 0x4A03C1D3U, 0x321390B9U, 0x6BB4BF7FU, 0xB70E0CBDU,
};
static const unsigned int secp224r1_Gy[7] = {
    0x85007E34U, 0x44D58199U, 0x5A074764U, 0xCD4375A0U, 0x4C22DFE6U, 0xB5F723FBU, 0xBD376388U,
};
static const unsigned int secp224r1_n[7] = {
    0x5C5C2A3DU, 0x13DD2945U, 0xE0B8F03EU, 0xFFFF16A2U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp224r1_n_h[7] = {
    0x5F517D15U, 0x29947A69U, 0x31D63F4BU, 0xABC8FF59U, 0xD9714856U, 0x6AD15F7CU, 0xB1E97961U,
};
#else
static const unsigned int secp224r1_n_h[7] = {
    0x3AD01289U, 0x6BDAAE6CU, 0x97A54552U, 0x6AD09D91U, 0xB1E97961U, 0x1822BC47U, 0xD4BAA4CFU,
};
static const unsigned int secp224r1_n_n0[1] = {
    0x6A1FC2EBU,
};
#endif

//[2^112]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp224r1_2_112_Gx[7] = {
    0x6CAB26E3U, 0xA0064196U, 0x2991FAB0U, 0x3A0B91FBU, 0xEC27A4E1U, 0x5F8EBEEFU, 0x0499AA8AU,
};
static const unsigned int secp224r1_2_112_Gy[7] = {
    0x7766AF5DU, 0x50751040U, 0x29610D54U, 0xF70684D9U, 0xD77AAE82U, 0x338C5B81U, 0x6916F6D4U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp224r1[1] = {
    {
        224u,
        224u,
        secp224r1_p,
        secp224r1_p_h,
        secp224r1_a,
        secp224r1_b,
        secp224r1_Gx,
        secp224r1_Gy,
        secp224r1_n,
        secp224r1_n_h,
        secp224r1_2_112_Gx,
        secp224r1_2_112_Gy,
    },
};
#else
const eccp_curve_t secp224r1[1] = {
    {
        224u,
        224u,
        secp224r1_p,
        secp224r1_p_h,
        secp224r1_p_n0,
        secp224r1_a,
        secp224r1_b,
        secp224r1_Gx,
        secp224r1_Gy,
        secp224r1_n,
        secp224r1_n_h,
        secp224r1_n_n0,
    },
};
#endif
#endif

/**************************** secp256r1 ******************************/
#ifdef SUPPORT_SECP256R1
static const unsigned int secp256r1_p[8] = {
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0xFFFFFFFFU,
};
static const unsigned int secp256r1_p_h[8] = {
    0x00000003U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFBU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFDU, 0x00000004U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int secp256r1_p_n0[1] = {
    1U,
};
#endif
static const unsigned int secp256r1_a[8] = {
    0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0xFFFFFFFFU,
};
static const unsigned int secp256r1_b[8] = {
    0x27D2604BU, 0x3BCE3C3EU, 0xCC53B0F6U, 0x651D06B0U, 0x769886BCU, 0xB3EBBD55U, 0xAA3A93E7U, 0x5AC635D8U,
};
static const unsigned int secp256r1_Gx[8] = {
    0xD898C296U, 0xF4A13945U, 0x2DEB33A0U, 0x77037D81U, 0x63A440F2U, 0xF8BCE6E5U, 0xE12C4247U, 0x6B17D1F2U,
};
static const unsigned int secp256r1_Gy[8] = {
    0x37BF51F5U, 0xCBB64068U, 0x6B315ECEU, 0x2BCE3357U, 0x7C0F9E16U, 0x8EE7EB4AU, 0xFE1A7F9BU, 0x4FE342E2U,
};
static const unsigned int secp256r1_n[8] = {
    0xFC632551U, 0xF3B9CAC2U, 0xA7179E84U, 0xBCE6FAADU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFFU,
};
static const unsigned int secp256r1_n_h[8] = {
    0xBE79EEA2U, 0x83244C95U, 0x49BD6FA6U, 0x4699799CU, 0x2B6BEC59U, 0x2845B239U, 0xF3D95620U, 0x66E12D94U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int secp256r1_n_n0[1] = {
    0xEE00BC4FU,
};
#endif

//[2^128]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp256r1_2_128_Gx[8] = {
    0xD789BD85U, 0x57C84FC9U, 0xC297EAC3U, 0xFC35FF7DU, 0x88C6766EU, 0xFB982FD5U, 0xEEDB5E67U, 0x447D739BU,
};
static const unsigned int secp256r1_2_128_Gy[8] = {
    0x72E25B32U, 0x0C7E33C9U, 0xA7FAE500U, 0x3D349B95U, 0x3A4AAFF7U, 0xE12E9D95U, 0x834131EEU, 0x2D4825ABU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp256r1[1] = {
    {
        256u,
        256u,
        secp256r1_p,
        secp256r1_p_h, // NULL, //
        secp256r1_a,
        secp256r1_b,
        secp256r1_Gx,
        secp256r1_Gy,
        secp256r1_n,
        secp256r1_n_h, // NULL, //
        secp256r1_2_128_Gx,
        secp256r1_2_128_Gy,
    },
};
#else
const eccp_curve_t secp256r1[1] = {
    {
        256u, 256u, secp256r1_p,
        secp256r1_p_h,  // NULL, //
        secp256r1_p_n0, // NULL, //
        secp256r1_a, secp256r1_b, secp256r1_Gx, secp256r1_Gy, secp256r1_n,
        secp256r1_n_h,  // NULL, //
        secp256r1_n_n0, // NULL, //
    },
};
#endif
#endif

/**************************** secp384r1 ******************************/
#ifdef SUPPORT_SECP384R1
static const unsigned int secp384r1_p[12] = {
    0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp384r1_p_h[12] = {
    0x00000000U, 0xFFFFFFFEU, 0x00000002U, 0x00000001U, 0xFFFFFFFDU, 0xFFFFFFFDU, 0x00000002U, 0x00000003U, 0x00000002U, 0xFFFFFFFEU, 0x00000000U, 0x00000002U,
};
#else
static const unsigned int secp384r1_p_h[12] = {
    0x00000001U, 0xFFFFFFFEU, 0x00000000U, 0x00000002U, 0x00000000U, 0xFFFFFFFEU, 0x00000000U, 0x00000002U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp384r1_p_n0[1] = {
    1U,
};
#endif
static const unsigned int secp384r1_a[12] = {
    0xFFFFFFFCU, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp384r1_b[12] = {
    0xD3EC2AEFU, 0x2A85C8EDU, 0x8A2ED19DU, 0xC656398DU, 0x5013875AU, 0x0314088FU, 0xFE814112U, 0x181D9C6EU, 0xE3F82D19U, 0x988E056BU, 0xE23EE7E4U, 0xB3312FA7U,
};
static const unsigned int secp384r1_Gx[12] = {
    0x72760AB7U, 0x3A545E38U, 0xBF55296CU, 0x5502F25DU, 0x82542A38U, 0x59F741E0U, 0x8BA79B98U, 0x6E1D3B62U, 0xF320AD74U, 0x8EB1C71EU, 0xBE8B0537U, 0xAA87CA22U,
};
static const unsigned int secp384r1_Gy[12] = {
    0x90EA0E5FU, 0x7A431D7CU, 0x1D7E819DU, 0x0A60B1CEU, 0xB5F0B8C0U, 0xE9DA3113U, 0x289A147CU, 0xF8F41DBDU, 0x9292DC29U, 0x5D9E98BFU, 0x96262C6FU, 0x3617DE4AU,
};
static const unsigned int secp384r1_n[12] = {
    0xCCC52973U, 0xECEC196AU, 0x48B0A77AU, 0x581A0DB2U, 0xF4372DDFU, 0xC7634D81U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp384r1_n_h[12] = {
    0xAE87B9E7U, 0x7FDB954AU, 0xB6F62810U, 0x8C23F7F3U, 0xAAAE6873U, 0xE99276EAU, 0xB33C33C5U, 0xD558BFBCU, 0xEED33213U, 0x3B277D79U, 0xF4A0E792U, 0xCC9601F9U,
};
#else
static const unsigned int secp384r1_n_h[12] = {
    0x19B409A9U, 0x2D319B24U, 0xDF1AA419U, 0xFF3D81E5U, 0xFCB82947U, 0xBC3E483AU, 0x4AAB1CC5U, 0xD40D4917U, 0x28266895U, 0x3FB05B7AU, 0x2B39BF21U, 0x0C84EE01U,
};
static const unsigned int secp384r1_n_n0[1] = {
    0xE88FDC45U,
};
#endif

//[2^192]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp384r1_2_192_Gx[12] = {
    0xAA03BD53U, 0xA628B09AU, 0xA4F52D78U, 0xBA065458U, 0x4D10DDEAU, 0xDB298789U, 0x8A3E297DU, 0xB42A31AFU, 0x06421279U, 0x40F7F9E7U, 0x800119C4U, 0xC19E0B4CU,
};
static const unsigned int secp384r1_2_192_Gy[12] = {
    0xE6C88C41U, 0x822D0FC5U, 0xE639D858U, 0xAF68AA6DU, 0x35F6EBF2U, 0xC1C7CAD1U, 0xE3567AF9U, 0x577A30EAU, 0x1F5B77F6U, 0xE5A0191DU, 0x0356B301U, 0x16F3FDBFU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp384r1[1] = {
    {
        384u,
        384u,
        secp384r1_p,
        secp384r1_p_h,
        secp384r1_a,
        secp384r1_b,
        secp384r1_Gx,
        secp384r1_Gy,
        secp384r1_n,
        secp384r1_n_h,
        secp384r1_2_192_Gx,
        secp384r1_2_192_Gy,
    },
};
#else
const eccp_curve_t secp384r1[1] = {
    {
        384u,
        384u,
        secp384r1_p,
        secp384r1_p_h,
        secp384r1_p_n0,
        secp384r1_a,
        secp384r1_b,
        secp384r1_Gx,
        secp384r1_Gy,
        secp384r1_n,
        secp384r1_n_h,
        secp384r1_n_n0,
    },
};
#endif
#endif

/**************************** secp521r1 ******************************/
#ifdef SUPPORT_SECP521R1
static const unsigned int secp521r1_p[17] = {
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp521r1_p_h[17] = {
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00004000U, 0x00000000U,
};
#else
static const unsigned int secp521r1_p_h[17] = {
    0x00000000U, 0x00004000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp521r1_p_n0[1] = {
    1U,
};
#endif
static const unsigned int secp521r1_a[17] = {
    0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU,
};
static const unsigned int secp521r1_b[17] = {
    0x6B503F00U, 0xEF451FD4U, 0x3D2C34F1U, 0x3573DF88U, 0x3BB1BF07U, 0x1652C0BDU, 0xEC7E937BU, 0x56193951U, 0x8EF109E1U,
    0xB8B48991U, 0x99B315F3U, 0xA2DA725BU, 0xB68540EEU, 0x929A21A0U, 0x8E1C9A1FU, 0x953EB961U, 0x00000051U,
};
static const unsigned int secp521r1_Gx[17] = {
    0xC2E5BD66U, 0xF97E7E31U, 0x856A429BU, 0x3348B3C1U, 0xA2FFA8DEU, 0xFE1DC127U, 0xEFE75928U, 0xA14B5E77U, 0x6B4D3DBAU,
    0xF828AF60U, 0x053FB521U, 0x9C648139U, 0x2395B442U, 0x9E3ECB66U, 0x0404E9CDU, 0x858E06B7U, 0x000000C6U,
};
static const unsigned int secp521r1_Gy[17] = {
    0x9FD16650U, 0x88BE9476U, 0xA272C240U, 0x353C7086U, 0x3FAD0761U, 0xC550B901U, 0x5EF42640U, 0x97EE7299U, 0x273E662CU,
    0x17AFBD17U, 0x579B4468U, 0x98F54449U, 0x2C7D1BD9U, 0x5C8A5FB4U, 0x9A3BC004U, 0x39296A78U, 0x00000118U,
};
static const unsigned int secp521r1_n[17] = {
    0x91386409U, 0xBB6FB71EU, 0x899C47AEU, 0x3BB5C9B8U, 0xF709A5D0U, 0x7FCC0148U, 0xBF2F966BU, 0x51868783U, 0xFFFFFFFAU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp521r1_n_h[17] = {
    0x60EC0915U, 0xABEE0E49U, 0xA4DEB96AU, 0x39B88F86U, 0x6440173AU, 0x36439BEAU, 0x53EBDD25U, 0xABED7C82U, 0x7A1814F8U,
    0xDB25B111U, 0xB2DD5C97U, 0xE290362FU, 0x158143E2U, 0xA9EA49FFU, 0x141CEAE0U, 0x07E35810U, 0x000001CDU,
};
#else
static const unsigned int secp521r1_n_h[17] = {
    0x61C64CA7U, 0x1163115AU, 0x4374A642U, 0x18354A56U, 0x0791D9DCU, 0x5D4DD6D3U, 0xD3402705U, 0x4FB35B72U, 0xB7756E3AU,
    0xCFF3D142U, 0xA8E567BCU, 0x5BCC6D61U, 0x492D0D45U, 0x2D8E03D1U, 0x8C44383DU, 0x5B5A3AFEU, 0x0000019AU,
};
static const unsigned int secp521r1_n_n0[1] = {
    0x79A995C7U,
};
#endif

//[2^260]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp521r1_2_260_Gx[17] = {
    0x9185544DU, 0x6D9B0C3CU, 0x8DF2765FU, 0xAD21890EU, 0xCBE030A2U, 0x47836EE3U, 0xF7651AEDU, 0x606B9133U, 0x71C00932U,
    0xB1A31586U, 0xCFE05F47U, 0x9806A369U, 0xF57F3700U, 0xC2EBC613U, 0xF065F07CU, 0x1022D6D2U, 0x00000109U,
};
static const unsigned int secp521r1_2_260_Gy[17] = {
    0x514C45EDU, 0xB292C583U, 0x947E68A1U, 0x89AC5BF2U, 0xAF507C14U, 0x633C4300U, 0x7DA4020AU, 0x943D7BA5U, 0xC0ED8274U,
    0xD1E90C7AU, 0xE59426E6U, 0x9634868CU, 0xC26BC9DEU, 0x24A6FFF2U, 0x152416CDU, 0x1A012168U, 0x0000000CU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp521r1[1] = {
    {
        521u,
        521u,
        secp521r1_p,
        secp521r1_p_h, // NULL,//
        secp521r1_a,
        secp521r1_b,
        secp521r1_Gx,
        secp521r1_Gy,
        secp521r1_n,
        secp521r1_n_h,
        secp521r1_2_260_Gx,
        secp521r1_2_260_Gy,
    },
};
#else
const eccp_curve_t secp521r1[1] = {
    {
        521u,
        521u,
        secp521r1_p,
        secp521r1_p_h,  // NULL,//
        secp521r1_p_n0, // NULL,//
        secp521r1_a,
        secp521r1_b,
        secp521r1_Gx,
        secp521r1_Gy,
        secp521r1_n,
        secp521r1_n_h,
        secp521r1_n_n0,
    },
};
#endif
#endif

/**************************** secp160k1 ******************************/
#ifdef SUPPORT_SECP160K1
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160k1_p[5] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160k1_p_h[5] = {
    0x00000000U, 0x4DB32715U, 0x51CE3BE1U, 0x0000FAA7U, 0x00000001U,
};
#elif (defined(PKE_LP))
static const unsigned int secp160k1_p[6] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U,
};
static const unsigned int secp160k1_p_h[6] = {
    0x00000000U, 0x00000000U, 0x1B44BBA9U, 0x0000A71AU, 0x00000001U, 0x00000000U,
};
static const unsigned int secp160k1_p_n0[1] = {
    0xB4AB2745U,
};
#elif (defined(PKE_SECURE))
static const unsigned int secp160k1_p[5] = {
    0xFFFFAC73U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp160k1_p_h[5] = {
    0x1B44BBA9U, 0x0000A71AU, 0x00000001U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp160k1_p_n0[1] = {
    0xB4AB2745U,
};
#endif
static const unsigned int secp160k1_a[5] = {
    0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp160k1_b[5] = {
    7U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp160k1_Gx[5] = {
    0xDD4D7EBBU, 0x3036F4F5U, 0xA4019E76U, 0xE37AA192U, 0x3B4C382CU,
};
static const unsigned int secp160k1_Gy[5] = {
    0xF03C4FEEU, 0x531733C3U, 0x6BC28286U, 0x318FDCEDU, 0x938CF935U,
};
static const unsigned int secp160k1_n[6] = {
    0xCA16B6B3U, 0x16DFAB9AU, 0x0001B8FAU, 0x00000000U, 0x00000000U, 0x00000001U,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160k1_n_h[6] = {
    0x705ACEF9U, 0xC89631C2U, 0x14AB01D8U, 0x2FD0CDC7U, 0xFEFCCD13U, 0x00000000U,
};
#else
static const unsigned int secp160k1_n_h[6] = {
    0x0E687AAFU, 0x0F849433U, 0x4D8A8AADU, 0xDFE35D2FU, 0xCDCF2BABU, 0x00000000U,
};
static const unsigned int secp160k1_n_n0[1] = {
    0x35931785U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp160k1_2_80_Gx[5] = {
    0xE398219CU, 0xCBFF95F1U, 0x47798032U, 0x6B450CC3U, 0x4B84DE5EU,
};
static const unsigned int secp160k1_2_80_Gy[5] = {
    0xA8E3C651U, 0x88FEB8C8U, 0x65923367U, 0xC00FA6FAU, 0x93F7C1DDU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp160k1[1] = {
    {
        160u,
        161u,
        secp160k1_p,
        secp160k1_p_h,
        secp160k1_a,
        secp160k1_b,
        secp160k1_Gx,
        secp160k1_Gy,
        secp160k1_n,
        secp160k1_n_h,
        secp160k1_2_80_Gx,
        secp160k1_2_80_Gy,
    },
};
#else
const eccp_curve_t secp160k1[1] = {
    {
        160u,
        161u,
        secp160k1_p,
        secp160k1_p_h,  // NULL,//
        secp160k1_p_n0, // NULL,//
        secp160k1_a,
        secp160k1_b,
        secp160k1_Gx,
        secp160k1_Gy,
        secp160k1_n,
        secp160k1_n_h,
        secp160k1_n_n0,
    },
};
#endif
#endif

/**************************** secp192k1 ******************************/
#ifdef SUPPORT_SECP192K1
static const unsigned int secp192k1_p[6] = {
    0xFFFFEE37U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192k1_p_h[6] = {
    0x000011C9U, 0x00000001U, 0x00000000U, 0x00000000U, 0x013C4FD1U, 0x00002392U,
};
#else
static const unsigned int secp192k1_p_h[6] = {
    0x013C4FD1U, 0x00002392U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp192k1_p_n0[1] = {
    0x7446D879U,
};
#endif
static const unsigned int secp192k1_a[6] = {
    0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp192k1_b[6] = {
    3U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp192k1_Gx[6] = {
    0xEAE06C7DU, 0x1DA5D1B1U, 0x80B7F434U, 0x26B07D02U, 0xC057E9AEU, 0xDB4FF10EU,
};
static const unsigned int secp192k1_Gy[6] = {
    0xD95E2F9DU, 0x4082AA88U, 0x15BE8634U, 0x844163D0U, 0x9C5628A7U, 0x9B2F2F6DU,
};
static const unsigned int secp192k1_n[6] = {
    0x74DEFD8DU, 0x0F69466AU, 0x26F2FC17U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192k1_n_h[6] = {
    0x87AE967CU, 0xBACF0434U, 0x150F5CF8U, 0x17BBAD83U, 0xBB194E8AU, 0x93FB81A6U,
};
#else
static const unsigned int secp192k1_n_h[6] = {
    0x250F0702U, 0x461C1989U, 0x195E97E2U, 0xF0F4F172U, 0x2EC4B2B1U, 0x6A21191CU,
};
static const unsigned int secp192k1_n_n0[1] = {
    0x560472BBU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp192k1_2_96_Gx[6] = {
    0x3C3803C1U, 0x4A0A43C8U, 0xA6FD6C4AU, 0x119C7C2BU, 0xF3B65531U, 0x79C8D6B2U,
};
static const unsigned int secp192k1_2_96_Gy[6] = {
    0xB7488B1EU, 0x3962A2ABU, 0x2EFDCE4CU, 0x71A2E4D2U, 0xE3A1306EU, 0xEB64375BU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp192k1[1] = {{
    192u,
    192u,
    secp192k1_p,
    secp192k1_p_h,
    secp192k1_a,
    secp192k1_b,
    secp192k1_Gx,
    secp192k1_Gy,
    secp192k1_n,
    secp192k1_n_h,
    secp192k1_2_96_Gx,
    secp192k1_2_96_Gy,
}};
#else
const eccp_curve_t secp192k1[1] = {
    {
        192u,
        192u,
        secp192k1_p,
        secp192k1_p_h,  // NULL,//
        secp192k1_p_n0, // NULL,//
        secp192k1_a,
        secp192k1_b,
        secp192k1_Gx,
        secp192k1_Gy,
        secp192k1_n,
        secp192k1_n_h,
        secp192k1_n_n0,
    },
};
#endif
#endif

/**************************** secp224k1 ******************************/
#ifdef SUPPORT_SECP224K1
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp224k1_p[7] = {
    0xFFFFE56DU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp224k1_p_h[7] = {
    0x00000000U, 0x00000000U, 0x02C23069U, 0x00003526U, 0x00000001U, 0x00000000U, 0x00000000U,
};
#elif (defined(PKE_LP))
static const unsigned int secp224k1_p[8] = {
    0xFFFFE56DU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U,
};
static const unsigned int secp224k1_p_h[8] = {
    0x00000000U, 0x00000000U, 0x02C23069U, 0x00003526U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp224k1_p_n0[1] = {0x198D139B};
#elif (defined(PKE_SECURE))
static const unsigned int secp224k1_p[7] = {
    0xFFFFE56DU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp224k1_p_h[7] = {
    0x02C23069U, 0x00003526U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
};
static const unsigned int secp224k1_p_n0[1] = {
    0x198D139BU,
};
#endif
static const unsigned int secp224k1_a[7] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp224k1_b[7] = {
    5U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp224k1_Gx[7] = {
    0xB6B7A45CU, 0x0F7E650EU, 0xE47075A9U, 0x69A467E9U, 0x30FC28A1U, 0x4DF099DFU, 0xA1455B33U,
};
static const unsigned int secp224k1_Gy[7] = {
    0x556D61A5U, 0xE2CA4BDBU, 0xC0B0BD59U, 0xF7E319F7U, 0x82CAFBD6U, 0x7FBA3442U, 0x7E089FEDU,
};
static const unsigned int secp224k1_n[8] = {
    0x769FB1F7U, 0xCAF0A971U, 0xD2EC6184U, 0x0001DCE8U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U,
};
static const unsigned int secp224k1_n_h[8] = {
    0xEC9FEAA0U, 0x34CE24FBU, 0x16F60AF5U, 0x8BE03208U, 0xBBFF32E4U, 0xB882BD88U, 0x993FF72BU, 0x00000000U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int secp224k1_n_n0[1] = {
    0x44C1A039U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp224k1_2_112_Gx[7] = {
    0x26A0F6BFU, 0x6958D26BU, 0x2EE2C11BU, 0x15E7E8DFU, 0xFCF05755U, 0x384E3EB5U, 0x6E88F7FFU,
};
static const unsigned int secp224k1_2_112_Gy[7] = {
    0xAE2CD1EFU, 0xBFF31064U, 0xBDA4B1A1U, 0x9638338CU, 0x67417F72U, 0xB7F8FA47U, 0x40AAFF87U,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp224k1[1] = {{
    224u,
    225u,
    secp224k1_p,
    secp224k1_p_h,
    secp224k1_a,
    secp224k1_b,
    secp224k1_Gx,
    secp224k1_Gy,
    secp224k1_n,
    secp224k1_n_h,
    secp224k1_2_112_Gx,
    secp224k1_2_112_Gy,
}};
#else
const eccp_curve_t secp224k1[1] = {
    {
        224u,
        225u,
        secp224k1_p,
        secp224k1_p_h,  // NULL,//
        secp224k1_p_n0, // NULL,//
        secp224k1_a,
        secp224k1_b,
        secp224k1_Gx,
        secp224k1_Gy,
        secp224k1_n,
        secp224k1_n_h,
        secp224k1_n_n0,
    },
};
#endif
#endif

/**************************** secp256k1 ******************************/
#ifdef SUPPORT_SECP256K1
static const unsigned int secp256k1_p[8] = {
    0xFFFFFC2FU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp256k1_p_h[8] = {
    0x000E90A1U, 0x000007A2U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int secp256k1_p_n0[1] = {
    0xD2253531U,
};
#endif
static const unsigned int secp256k1_a[8] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp256k1_b[8] = {
    7U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int secp256k1_Gx[8] = {
    0x16F81798U, 0x59F2815BU, 0x2DCE28D9U, 0x029BFCDBU, 0xCE870B07U, 0x55A06295U, 0xF9DCBBACU, 0x79BE667EU,
};
static const unsigned int secp256k1_Gy[8] = {
    0xFB10D4B8U, 0x9C47D08FU, 0xA6855419U, 0xFD17B448U, 0x0E1108A8U, 0x5DA4FBFCU, 0x26A3C465U, 0x483ADA77U,
};
static const unsigned int secp256k1_n[8] = {
    0xD0364141U, 0xBFD25E8CU, 0xAF48A03BU, 0xBAAEDCE6U, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
};
static const unsigned int secp256k1_n_h[8] = {
    0x67D7D140U, 0x896CF214U, 0x0E7CF878U, 0x741496C2U, 0x5BCD07C6U, 0xE697F5E4U, 0x81C69BC5U, 0x9D671CD5U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int secp256k1_n_n0[1] = {
    0x5588B13FU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int secp256k1_2_128_Gx[8] = {
    0x9EC4C0DAU, 0x1B7B444CU, 0x723EA335U, 0xE88C5678U, 0x981F162EU, 0x9239C1ADU, 0xF63B5F33U, 0x8F68B9D2U,
};
static const unsigned int secp256k1_2_128_Gy[8] = {
    0x501FFF82U, 0xF23CBF79U, 0x95510BFDU, 0xBBEA2CFEU, 0xB6BE215DU, 0xDE1D90C2U, 0xBA063986U, 0x662A9F2DU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t secp256k1[1] = {{
    256u,
    256u,
    secp256k1_p,
    secp256k1_p_h,
    secp256k1_a,
    secp256k1_b,
    secp256k1_Gx,
    secp256k1_Gy,
    secp256k1_n,
    secp256k1_n_h,
    secp256k1_2_128_Gx,
    secp256k1_2_128_Gy,
}};
#else
const eccp_curve_t secp256k1[1] = {
    {
        256u,
        256u,
        secp256k1_p,
        secp256k1_p_h,  // NULL,//
        secp256k1_p_n0, // NULL,//
        secp256k1_a,
        secp256k1_b,
        secp256k1_Gx,
        secp256k1_Gy,
        secp256k1_n,
        secp256k1_n_h,
        secp256k1_n_n0,
    },
};
#endif
#endif

/**************************** BN256 ******************************/
#ifdef SUPPORT_BN256
static const unsigned int bn256_p[8] = {
    0xAED33013U, 0xD3292DDBU, 0x12980A82U, 0x0CDC65FBU, 0xEE71A49FU, 0x46E5F25EU, 0xFFFCF0CDU, 0xFFFFFFFFU,
};
static const unsigned int bn256_p_h[8] = {
    0x1092B98FU, 0xFAC8C610U, 0xD7F91154U, 0xDB90D49CU, 0x32BF3141U, 0x4F325FC7U, 0x0E56A005U, 0x4DE578EAU,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int bn256_p_n0[1] = {
    0x0537E5E5U,
};
#endif
static const unsigned int bn256_a[8] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn256_b[8] = {
    3U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn256_Gx[8] = {
    1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn256_Gy[8] = {
    2U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn256_n[8] = {
    0xD10B500DU, 0xF62D536CU, 0x1299921AU, 0x0CDC65FBU, 0xEE71A49EU, 0x46E5F25EU, 0xFFFCF0CDU, 0xFFFFFFFFU,
};
static const unsigned int bn256_n_h[8] = {
    0x8F4C4808U, 0xAF948AA3U, 0x26123232U, 0xBD789EFDU, 0xEB526BE7U, 0x117FD17CU, 0xFB8F407AU, 0x2BFC4998U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int bn256_n_n0[1] = {
    0xC9C6813BU,
};
#endif

//[2^128]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int bn256_2_128_Gx[8] = {
    0x5E06FE34U, 0xECBD3164U, 0xCE4EAA4FU, 0x35B40F42U, 0x5E1339BAU, 0x2CB9923AU, 0xA165554DU, 0x3F80B083U,
};
static const unsigned int bn256_2_128_Gy[8] = {
    0x8B965F4AU, 0x6B5F5BF5U, 0x7334F693U, 0xD77BFA60U, 0x70406125U, 0xE49F4BD8U, 0x84F6A594U, 0x2268E7FBU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t bn256[1] = {
    {
        256u,
        256u,
        bn256_p,
        bn256_p_h, // NULL, //
        bn256_a,
        bn256_b,
        bn256_Gx,
        bn256_Gy,
        bn256_n,
        bn256_n_h, // NULL, //
        bn256_2_128_Gx,
        bn256_2_128_Gy,
    },
};
#else
const eccp_curve_t bn256[1] = {
    {
        256u, 256u, bn256_p,
        bn256_p_h,  // NULL, //
        bn256_p_n0, // NULL, //
        bn256_a, bn256_b, bn256_Gx, bn256_Gy, bn256_n,
        bn256_n_h,  // NULL, //
        bn256_n_n0, // NULL, //
    },
};
#endif
#endif

/**************************** BN638 ******************************/
#ifdef SUPPORT_BN638
static const unsigned int bn638_p[20] = {
    0x00000067U, 0x00000000U, 0xFFFFECE0U, 0xFFFFFFFFU, 0x80015ACDU, 0x0000004CU, 0xFFF4EB80U, 0xFFFFF51FU, 0x0021E55BU, 0xC0008652U,
    0x0008DE55U, 0xFFFDD0E0U, 0x0000D52FU, 0x3FFF9487U, 0xD000165EU, 0xFFFFF942U, 0x000001D3U, 0x7FFFFFB8U, 0xC000000DU, 0x23FFFFFDU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int bn638_p_h[20] = {
    0xDFD2A6C5U, 0x668E29B3U, 0x8FDFA6A7U, 0x2968BE60U, 0x041A17D7U, 0x13F3D015U, 0x0F669718U, 0xE1C52740U, 0xC8A102DCU, 0x046324EBU,
    0x40E86FEAU, 0x23F7E0BDU, 0x8AC0A2E4U, 0x6D5D613EU, 0xD0E98766U, 0xB853FF78U, 0x27596A54U, 0x1A09328CU, 0x6FEC09DDU, 0x040681E4U,
};
#else
static const unsigned int bn638_p_h[20] = {
    0x2B2F5227U, 0xA19430BFU, 0xB48B9102U, 0xF598D8A4U, 0x68EEEE4CU, 0x6004E38BU, 0x2C437647U, 0x3A2F454BU, 0xFDD8EA75U, 0x612285E3U,
    0x43B93508U, 0x697F87A2U, 0x9080FE0FU, 0xBC5C481BU, 0x958F4EB0U, 0x981685ECU, 0x9F98DA83U, 0x2437C11DU, 0x5B1DA7BEU, 0x0BD442FAU,
};
static const unsigned int bn638_p_n0[1] = {
    0x2CBCE4A9U,
};
#endif
static const unsigned int bn638_a[20] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn638_b[20] = {
    0x00000101U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn638_Gx[20] = {
    0x00000066U, 0x00000000U, 0xFFFFECE0U, 0xFFFFFFFFU, 0x80015ACDU, 0x0000004CU, 0xFFF4EB80U, 0xFFFFF51FU, 0x0021E55BU, 0xC0008652U,
    0x0008DE55U, 0xFFFDD0E0U, 0x0000D52FU, 0x3FFF9487U, 0xD000165EU, 0xFFFFF942U, 0x000001D3U, 0x7FFFFFB8U, 0xC000000DU, 0x23FFFFFDU,
};
static const unsigned int bn638_Gy[20] = {
    0x00000010U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};
static const unsigned int bn638_n[20] = {
    0x00000061U, 0x00000000U, 0xFFFFEDA0U, 0xFFFFFFFFU, 0x800154D9U, 0x00000049U, 0xFFF4EAC0U, 0xFFFFF54FU, 0x0021E555U, 0x60008655U,
    0x0008DE55U, 0xFFFDD0E0U, 0x0000D52FU, 0x3FFF9487U, 0xD000165EU, 0xFFFFF942U, 0x000001D3U, 0x7FFFFFB8U, 0xC000000DU, 0x23FFFFFDU,
};
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int bn638_n_h[20] = {
    0xE384A09FU, 0x808AA3ABU, 0x7EB1E973U, 0x6994B3DFU, 0x90FA7D82U, 0x98F5B679U, 0x115C5909U, 0x93452FE5U, 0x8D92072DU, 0x3B6FF122U,
    0x167C27CBU, 0x294483E9U, 0x488BAF5DU, 0xCF475845U, 0x4D35F74AU, 0xA82F3060U, 0x43E9F211U, 0xE6E1CAC8U, 0x4D06397AU, 0x11BFA298U,
};
#else
static const unsigned int bn638_n_h[20] = {
    0x54101DC4U, 0x3520F4DDU, 0x729F3A43U, 0x26CDB684U, 0x2080C8E0U, 0x146F0E77U, 0xD7DECA81U, 0xD8957773U, 0x3874C9BBU, 0x533C4EC2U,
    0x41C0ED0EU, 0x0A6F3E0AU, 0xE1EF5E67U, 0x0D257A80U, 0x9283EFA8U, 0x7D50C3E9U, 0xF49731EDU, 0x7CBE9A47U, 0xC86631A3U, 0x0CB898F5U,
};
static const unsigned int bn638_n_n0[1] = {
    0xA0FD5C5FU,
};
#endif

//[2^260]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int bn638_2_319_Gx[20] = {
    0xB5AF644EU, 0x0494F533U, 0x9D832CDDU, 0x3FBB675FU, 0x884BC489U, 0x067A81BCU, 0x620BD123U, 0x4DC263C8U, 0x86610AE9U, 0x0D49AE84U,
    0xC3353780U, 0x341CD7CBU, 0x7D0FE4F6U, 0x16475144U, 0x8AA5BFF4U, 0x1E1CC061U, 0x681BC09BU, 0xCEA15338U, 0x923212C6U, 0x0A378785U,
};
static const unsigned int bn638_2_319_Gy[20] = {
    0xA9ECB70BU, 0x43004DDAU, 0x7DEFAB17U, 0xAB2392A5U, 0x1E224FB2U, 0xE870977AU, 0x609FDB32U, 0xAAEE8A91U, 0xBFA96F6DU, 0x0137465BU,
    0x6E771BA8U, 0x03478023U, 0xD7CCC3C3U, 0xD76DEFE7U, 0xD55C8329U, 0xDE3DEEA7U, 0xCA50F0C8U, 0xB396C977U, 0x17FDF2F3U, 0x1B0C448CU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t bn638[1] = {
    {
        638u,
        638u,
        bn638_p,
        bn638_p_h, // NULL,//
        bn638_a,
        bn638_b,
        bn638_Gx,
        bn638_Gy,
        bn638_n,
        bn638_n_h,
        bn638_2_319_Gx,
        bn638_2_319_Gy,
    },
};
#else
const eccp_curve_t bn638[1] = {
    {
        638u,
        638u,
        bn638_p,
        bn638_p_h,  // NULL,//
        bn638_p_n0, // NULL,//
        bn638_a,
        bn638_b,
        bn638_Gx,
        bn638_Gy,
        bn638_n,
        bn638_n_h,
        bn638_n_n0,
    },
};
#endif
#endif

/**************************** ANDERS_1024_1 ******************************/
#ifdef SUPPORT_ANDERS_1024_1
static const unsigned int anders_1024_1_p[32] = {
    0x00000007U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0xfa000000U,
};
static const unsigned int anders_1024_1_p_h[32] = {
    0x78D4FE2DU, 0x083126E9U, 0x645A1CACU, 0x8D4FDF3BU, 0x83126E97U, 0x45A1CAC0U, 0xD4FDF3B6U, 0x3126E978U, 0x5A1CAC08U, 0x4FDF3B64U, 0x126E978DU,
    0xA1CAC083U, 0xFDF3B645U, 0x26E978D4U, 0x1CAC0831U, 0xDF3B645AU, 0x6E978D4FU, 0xCAC08312U, 0xF3B645A1U, 0xE978D4FDU, 0xAC083126U, 0x3B645A1CU,
    0x978D4FDFU, 0xC083126EU, 0xB645A1CAU, 0x78D4FDF3U, 0x083126E9U, 0x645A1CACU, 0x8D4FDF3BU, 0x83126E97U, 0x45A1CAC0U, 0xD2FDF3B6U,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int anders_1024_1_p_n0[1] = {
    0x49249249,
};
#endif
static const unsigned int anders_1024_1_a[32] = {
    0x00000004U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0xfa000000U,
};
static const unsigned int anders_1024_1_b[32] = {
    0x00000005U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x0A11CE00U, 0x00000000U, 0x00000000U, 0x00000000U, 0xDB5E2C40U, 0xD1E8C429U,
    0xF9ABD2DFU, 0xC4F0EDBEU, 0x4B016D9EU, 0x6EE8FC8EU, 0x7A807026U, 0x672C90D2U, 0x0000000DU, 0x0B000000U, 0x0000000BU, 0xFE000000U, 0x5FF25A5AU,
    0xF1EA5FF1U, 0xC001FFFFU, 0xA5FF15FFU, 0x4FFFFF1EU, 0xF15FFBE5U, 0xFFF1EA5FU, 0xFF900DFFU, 0xFAE5FF15U, 0xFDEADFFFU, 0xDE5FF15FU,
};
static const unsigned int anders_1024_1_Gx[32] = {
    0x00000000U, 0x00000000U, 0x00000000U, 0xA2D00B0BU, 0x0A11CE00U, 0xF00D0040U, 0xEEF15BADU, 0x000DEADBU, 0x5AFE0000U, 0xF15FF25AU, 0xFFF1EA5FU,
    0xFFC001FFU, 0x1EA5FF15U, 0xE54FFFFFU, 0x5FF15FFBU, 0xFFFFF1EAU, 0x15FF900DU, 0xFFFAE5FFU, 0x5FFDEADFU, 0x00DE5FF1U, 0xAFE00000U, 0x15FF25A5U,
    0xFF1EA5FFU, 0xFC001FFFU, 0xEA5FF15FU, 0x54FFFFF1U, 0xFF15FFBEU, 0xFFFF1EA5U, 0x5FF900DFU, 0xFFAE5FF1U, 0xFFDEADFFU, 0x0DE5FF15U,
};
static const unsigned int anders_1024_1_Gy[32] = {
    0x4BB53650U, 0xEE26A78BU, 0x618B85EDU, 0xA49A17BCU, 0x4D5BE050U, 0x82A87C58U, 0x12C3112AU, 0x3AA2F52CU, 0x00C2A9ADU, 0xF17AD7F9U, 0xF1622E4DU,
    0x614075FFU, 0x833352EFU, 0x90273C6AU, 0x64BA37FBU, 0x0E59FC88U, 0xF3C2894DU, 0xF454BC74U, 0x437DF946U, 0x8A569D2AU, 0xC60E05A1U, 0x92FB6C84U,
    0x6931D82FU, 0x2A5FF6A8U, 0x5BBC35D6U, 0x410B1797U, 0x724A73B2U, 0xD43ACAE9U, 0xD3C06B0AU, 0xBE9BD4E4U, 0x39E17B5EU, 0x19F05369U,
};
static const unsigned int anders_1024_1_n[32] = {
    0xA97F2707U, 0x07AC5CAAU, 0x79EA4BFAU, 0x527589B9U, 0xE569E1D3U, 0x2530F353U, 0x79921D61U, 0x8A6E3BB5U, 0xB9DE0DF5U, 0x8B50780FU, 0x38D5EBC5U,
    0xFC8E9F74U, 0x3D5C7FB9U, 0x2665E576U, 0x0C03CABDU, 0xC6962C2DU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0xFA000000U,
};
static const unsigned int anders_1024_1_n_h[32] = {
    0xF7BC108AU, 0x7D4D40C4U, 0x5FE418FBU, 0xA8134474U, 0xD93DAAADU, 0x8D8B907BU, 0x5D7D89B9U, 0x08077B5FU, 0x3B44032BU, 0x48F0BD36U, 0x540E9BDDU,
    0x921E1994U, 0x1CB0BFB2U, 0xA7F0FB61U, 0x9EC27B80U, 0x09DE53B1U, 0x586EEF75U, 0xF3DCAB7CU, 0x5D81062DU, 0x833ABB77U, 0x6F3C3428U, 0x302405A4U,
    0x2517E0ACU, 0x913E10EEU, 0x3F8E2BCCU, 0x729929A8U, 0x60000D2EU, 0xCB226631U, 0x0A6D792CU, 0x3D58CE4DU, 0x5179B581U, 0x6784387BU,
};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int anders_1024_1_n_n0[1] = {
    0x53646949,
};
#endif

//[2^512]G
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int anders_1024_1_2_512_Gx[32] = {
    0x9400C2E7U, 0x435CD89BU, 0x8D5A9A2EU, 0xD1E9DE98U, 0x4A11E433U, 0xB6EB6674U, 0xA4F7B02EU, 0x0EA107A0U, 0x5F907582U, 0x9B757345U, 0x00CF492CU,
    0x2B921CF0U, 0x7D3578C9U, 0x776FA500U, 0x1B4A4CE4U, 0xCF8746DEU, 0x2CA31E1CU, 0x18AB676DU, 0x51526524U, 0x7015BE82U, 0x4C727803U, 0x73FFE480U,
    0xE2BB2791U, 0x026A600AU, 0x1F55B3B3U, 0xEE5865E8U, 0xFACD6A6AU, 0x2527DDF2U, 0xB900FD08U, 0xFDADE5A3U, 0x579B6A46U, 0x56A76889U,
};
static const unsigned int anders_1024_1_2_512_Gy[32] = {
    0xAD0BF6F4U, 0x25049053U, 0x5FEF7098U, 0x27761912U, 0x3D58826DU, 0xC2D98116U, 0x1A6F9B57U, 0x0E800978U, 0xCB0EDF06U, 0x8E038C49U, 0x0D9FE57AU,
    0x1A331245U, 0x29CABA19U, 0xA148A8B5U, 0x297AD068U, 0x426B8751U, 0x4CACB65DU, 0x7C5FD75FU, 0x2BC6BE90U, 0x112A480AU, 0x46780139U, 0xB6EAC362U,
    0x662BF7CBU, 0x184AE477U, 0xA8CCB946U, 0xCDEEE45DU, 0xCD29D7F6U, 0xD34D6661U, 0xF4C14DE5U, 0x2F0A5321U, 0x11DBD27BU, 0x003F04FFU,
};
#endif

#if (defined(PKE_HP) || defined(PKE_UHP))
const eccp_curve_t anders_1024_1[1] = {
    {
        1024u,
        1024u,
        anders_1024_1_p,
        anders_1024_1_p_h, // NULL,//
        anders_1024_1_a,
        anders_1024_1_b,
        anders_1024_1_Gx,
        anders_1024_1_Gy,
        anders_1024_1_n,
        anders_1024_1_n_h,
        anders_1024_1_2_512_Gx,
        anders_1024_1_2_512_Gy,
    },
};
#else
const eccp_curve_t anders_1024_1[1] = {
    {
        1024u,
        1024u,
        anders_1024_1_p,
        anders_1024_1_p_h,  // NULL,//
        anders_1024_1_p_n0, // NULL,//
        anders_1024_1_a,
        anders_1024_1_b,
        anders_1024_1_Gx,
        anders_1024_1_Gy,
        anders_1024_1_n,
        anders_1024_1_n_h,
        anders_1024_1_n_n0,
    },
};
#endif
#endif

#ifdef SUPPORT_SM2
static const unsigned int sm2p256v1_p[8]   = {0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
static const unsigned int sm2p256v1_p_h[8] = {0x00000003u, 0x00000002u, 0xFFFFFFFFu, 0x00000002u, 0x00000001u, 0x00000001u, 0x00000002u, 0x00000004u};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int sm2p256v1_p_n0[1] = {
    1u,
};
#endif
static const unsigned int sm2p256v1_a[8]   = {0xFFFFFFFCu, 0xFFFFFFFFu, 0x00000000u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
static const unsigned int sm2p256v1_b[8]   = {0x4D940E93u, 0xDDBCBD41u, 0x15AB8F92u, 0xF39789F5u, 0xCF6509A7u, 0x4D5A9E4Bu, 0x9D9F5E34u, 0x28E9FA9Eu};
static const unsigned int sm2p256v1_Gx[8]  = {0x334C74C7u, 0x715A4589u, 0xF2660BE1u, 0x8FE30BBFu, 0x6A39C994u, 0x5F990446u, 0x1F198119u, 0x32C4AE2Cu};
static const unsigned int sm2p256v1_Gy[8]  = {0x2139F0A0u, 0x02DF32E5u, 0xC62A4740u, 0xD0A9877Cu, 0x6B692153u, 0x59BDCEE3u, 0xF4F6779Cu, 0xBC3736A2u};
static const unsigned int sm2p256v1_n[8]   = {0x39D54123u, 0x53BBF409u, 0x21C6052Bu, 0x7203DF6Bu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
static const unsigned int sm2p256v1_n_h[8] = {0x7C114F20u, 0x901192AFu, 0xDE6FA2FAu, 0x3464504Au, 0x3AFFE0D4u, 0x620FC84Cu, 0xA22B3D3Bu, 0x1EB5E412u};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int sm2p256v1_n_n0[1] = {
    0x72350975u,
};
#elif (defined(PKE_UHP_ECC))
static const unsigned int sm2p256v1_n_n0[8] = {0x72350975u, 0x327F9E88u, 0xFC8319A5u, 0xDF1E8D34u, 0xB08941D4u, 0x2B0068D3u, 0x82E4C7BCu, 0x6F39132Fu};
#endif

//[2^128]G, for [k]G of high speed
#if (defined(PKE_HP) || defined(PKE_UHP) || defined(PKE_UHP_ECC))
static const unsigned int sm2p256v1_2_128_G_x[8] = {0xD13A42EDu, 0xEAE3D9A9u, 0x484E1B38u, 0x2B2308F6u, 0x88C21F3Au, 0x3DB7B248u, 0x74D55DA9u, 0xB692E5B5u};
static const unsigned int sm2p256v1_2_128_G_y[8] = {0xE295E5ABu, 0xD186469Du, 0x73438E6Du, 0xDB61AC17u, 0x544926F9u, 0x5A924F85u, 0x0F3FB613u, 0xA175051Bu};
#endif

// SM2 para (n-1), for private key checking
const unsigned int g_sm2p256v1_n_minus_1[8] = {0x39D54122u, 0x53BBF409u, 0x21C6052Bu, 0x7203DF6Bu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};

const eccp_curve_t sm2_curve[1] = {{
    256u,
    256u,
    sm2p256v1_p,
    sm2p256v1_p_h,
#if (defined(PKE_LP) || defined(PKE_SECURE))
    sm2p256v1_p_n0,
#elif (defined(PKE_UHP_ECC))
    NULL, // actually for PKE_UHP_ECC this is useless
#endif
    sm2p256v1_a,
    sm2p256v1_b,
    sm2p256v1_Gx,
    sm2p256v1_Gy,
    sm2p256v1_n,
    sm2p256v1_n_h,
#if (defined(PKE_LP) || defined(PKE_SECURE) || defined(PKE_UHP_ECC))
    sm2p256v1_n_n0,
#endif
#if (defined(PKE_HP) || defined(PKE_UHP) || defined(PKE_UHP_ECC))
    sm2p256v1_2_128_G_x,
    sm2p256v1_2_128_G_y,
#endif
}};
#endif

#ifdef SUPPORT_SM9
static const unsigned int sm9p256v1_p[8]   = {0xE351457Du, 0xE56F9B27u, 0x1A7AEEDBu, 0x21F2934Bu, 0xF58EC745u, 0xD603AB4Fu, 0x02A3A6F1u, 0xB6400000u};
static const unsigned int sm9p256v1_p_h[8] = {0xB417E2D2u, 0x27DEA312u, 0xAE1A5D3Fu, 0x88F8105Fu, 0xD6706E7Bu, 0xE479B522u, 0x56F62FBDu, 0x2EA795A6u};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int sm9p256v1_p_n0[1] = {
    0x2F2EE42Bu,
};
#elif (defined(PKE_UHP_ECC))
static const unsigned int sm9p256v1_p_n0[8] = {0x2F2EE42Bu, 0x892BC42Cu, 0x13C8DBAFu, 0x181AE396u, 0x1522B137u, 0x966A4B29u, 0x558A13B3u, 0xAFD2BAC5u};
#endif
static const unsigned int sm9p256v1_a[8]   = {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u};
static const unsigned int sm9p256v1_b[8]   = {0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u};
static const unsigned int sm9p256v1_Gx[8]  = {0x7C66DDDDu, 0xE8C4E481u, 0x09DC3280u, 0xE1E40869u, 0x487D01D6u, 0xF5ED0704u, 0x62BF718Fu, 0x93DE051Du};
static const unsigned int sm9p256v1_Gy[8]  = {0x0A3EA616u, 0x0C464CD7u, 0xFA602435u, 0x1C1C00CBu, 0x5C395BBCu, 0x63106512u, 0x4F21E607u, 0x21FE8DDAu};
static const unsigned int sm9p256v1_n[8]   = {0xD69ECF25u, 0xE56EE19Cu, 0x18EA8BEEu, 0x49F2934Bu, 0xF58EC744u, 0xD603AB4Fu, 0x02A3A6F1u, 0xB6400000u};
static const unsigned int sm9p256v1_n_h[8] = {0xCD750C35u, 0x7598CD79u, 0xBB6DAEABu, 0xE4A08110u, 0x7D78A1F9u, 0xBFEE4BAEu, 0x63695D0Eu, 0x8894F5D1u};
#if (defined(PKE_LP) || defined(PKE_SECURE))
static const unsigned int sm9p256v1_n_n0[1] = {
    0x51974B53u,
};
#elif (defined(PKE_UHP_ECC))
static const unsigned int sm9p256v1_n_n0[8] = {0x51974B53u, 0x1D026623u, 0x939A510Du, 0xF590740Du, 0x48175059u, 0x205F4C4Bu, 0xCAA6FF31u, 0xE3582ED4u};
#endif

//[2^128]P1, for [k]P1 of high speed
#if (defined(PKE_HP) || defined(PKE_UHP))
static const unsigned int sm9p256v1_2_128_G_x[8] = {0x4FF01786u, 0xC677FCD6u, 0x4BC63E2Au, 0xB9FBCA7Cu, 0xDCDB5244u, 0xDBB96A1Cu, 0xE18508BCu, 0xAA480D0Cu};
static const unsigned int sm9p256v1_2_128_G_y[8] = {0x99177E0Bu, 0x9ADE138Bu, 0x647ADE95u, 0x4D452749u, 0x1E25BA7Bu, 0xBBE5D04Au, 0x4338FA7Fu, 0x765C0578u};
#endif

const eccp_curve_t sm9_curve[1] = {
    {
        256u,
        256u,
        sm9p256v1_p,
        sm9p256v1_p_h,
#if (defined(PKE_LP) || defined(PKE_SECURE) || defined(PKE_UHP_ECC))
        sm9p256v1_p_n0,
#endif
        sm9p256v1_a,
        sm9p256v1_b,
        sm9p256v1_Gx,
        sm9p256v1_Gy,
        sm9p256v1_n,
        sm9p256v1_n_h,
#if (defined(PKE_LP) || defined(PKE_SECURE) || defined(PKE_UHP_ECC))
        sm9p256v1_n_n0,
#endif
#if (defined(PKE_HP) || defined(PKE_UHP))
        sm9p256v1_2_128_G_x,
        sm9p256v1_2_128_G_y,
#elif (defined(PKE_UHP_ECC))
        NULL,
        NULL,
#endif
    },
};
#endif

#ifdef SUPPORT_C25519
/**************************** Curve25519 ******************************/
static const unsigned int curve25519_p[8] = {
    0xFFFFFFEDu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x7FFFFFFFu,
};
static const unsigned int curve25519_p_h[8] = {
    0x000005A4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
};
#if defined(PKE_LP)
static const unsigned int curve25519_p_n0[1] = {0x286BCA1Bu};
#endif
static const unsigned int curve25519_a24[8] = {
    0x0001DB41u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
};
#if 0
static const unsigned int curve25519_B[]         = {0x00000001u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,};
#endif
static const unsigned int curve25519_u[] = {
    0x00000009u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
};
static const unsigned int curve25519_v[] = {
    0x7ECED3D9u, 0x29E9C5A2u, 0x6D7C61B2u, 0x923D4D7Eu, 0x7748D14Cu, 0xE01EDD2Cu, 0xB8A086B4u, 0x20AE19A1u,
};
static const unsigned int curve25519_n[] = {
    0x5CF5D3EDu, 0x5812631Au, 0xA2F79CD6u, 0x14DEF9DEu, 0x00000000u, 0x00000000u, 0x00000000u, 0x10000000u,
};
static const unsigned int curve25519_n_h[8] = {
    0x449C0F01u, 0xA40611E3u, 0x68859347u, 0xD00E1BA7u, 0x17F5BE65u, 0xCEEC73D2u, 0x7C309A3Du, 0x0399411Bu,
};
#if defined(PKE_LP)
static const unsigned int curve25519_n_n0[1] = {0x12547E1Bu};
#endif
static const unsigned int curve25519_cofactor[1] = {8u};

const mont_curve_t c25519[1] = {
    {
        255u,
        253u,
        curve25519_p,
        curve25519_p_h,
#if defined(PKE_LP)
        curve25519_p_n0,
#endif
        curve25519_a24,
        curve25519_u,
        curve25519_v,
        curve25519_n,
        curve25519_n_h,
#if defined(PKE_LP)
        curve25519_n_n0,
#endif
        curve25519_cofactor,
    },
};

/**************************** ed25519 ******************************/
static const unsigned int ed25519_d[] = {
    0x135978A3u, 0x75EB4DCAu, 0x4141D8ABu, 0x00700A4Du, 0x7779E898u, 0x8CC74079u, 0x2B6FFE73u, 0x52036CEEu,
};
static const unsigned int ed25519_Gx[] = {
    0x8F25D51Au, 0xC9562D60u, 0x9525A7B2u, 0x692CC760u, 0xFDD6DC5Cu, 0xC0A4E231u, 0xCD6E53FEu, 0x216936D3u,
};
static const unsigned int ed25519_Gy[] = {
    0x66666658u, 0x66666666u, 0x66666666u, 0x66666666u, 0x66666666u, 0x66666666u, 0x66666666u, 0x66666666u,
};

const edward_curve_t ed25519[1] = {
    {
        255u,
        253u,
        curve25519_p,
        curve25519_p_h,
#if defined(PKE_LP)
        curve25519_p_n0,
#endif
        ed25519_d,
        ed25519_Gx,
        ed25519_Gy,
        curve25519_n,
        curve25519_n_h,
#if defined(PKE_LP)
        curve25519_n_n0,
#endif
        curve25519_cofactor,
    },
};
#endif
