/********************************************************************************************************
 * @file    eccp_curve.c
 *
 * @brief   This is the source file for TL321X
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

#include "lib/include/crypto_common/eccp_curve.h"


/* CAUTION:
 * for PKE_LP and PKE_SECURE, secp160r1, secp160r2, secp160k1 and secp224k1 are so particular, their n 
 * are 1 bit longer than p, and n are 1 word longer than p, but the IP montgomery parameter width is 
 * 32 bits, so we have to set the curve parameter p width as n length(this affects the p_h value), 
 * to support point multiplication while the scalar may be longer than p.
 * but for PKE_HP, PKE_UHP, the IP montgomery parameter width is 256 bits, so there is no problem.
 */


/**************************** brainpoolp160r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP160R1
unsigned int brainpoolp160r1_p[5] = {0x9515620F, 0x95B3D813, 0x60DFC7AD, 0x737059DC, 0xE95E4A5F};
    #ifdef PKE_HP
unsigned int brainpoolp160r1_p_h[5] = {0x532B7BEB, 0xC57E4353, 0xB4BA7FB8, 0x4CC30F3B, 0xB3945136};
    #else
unsigned int brainpoolp160r1_p_h[5]  = {0x25BC14FF, 0xB333F8D6, 0xFED717E0, 0xC0CA7EF8, 0x6CF12F81};
unsigned int brainpoolp160r1_p_n0[1] = {0xADBCB311};
    #endif
unsigned int brainpoolp160r1_a[5]  = {0xE8F7C300, 0xDA745D97, 0xE2BE61BA, 0xA280EB74, 0x340E7BE2};
unsigned int brainpoolp160r1_b[5]  = {0xD8675E58, 0xBDEC95C8, 0x134FAA2D, 0x95423412, 0x1E589A85};
unsigned int brainpoolp160r1_Gx[5] = {0xBDBCDBC3, 0x31EB5AF7, 0x62938C46, 0xEA3F6A4F, 0xBED5AF16};
unsigned int brainpoolp160r1_Gy[5] = {0x16DA6321, 0x669C9763, 0x38F94741, 0x7A1A8EC3, 0x1667CB47};
unsigned int brainpoolp160r1_n[5]  = {0x9E60FC09, 0xD4502940, 0x60DF5991, 0x737059DC, 0xE95E4A5F};
    #ifdef PKE_HP
unsigned int brainpoolp160r1_n_h[5] = {0x9ADFB54B, 0xE00DFA53, 0x0E7C2B8D, 0x5C5494B1, 0x9B44D4F6};
    #else
unsigned int brainpoolp160r1_n_h[5]  = {0x1FDF90EA, 0xFC61D435, 0x9E31FE16, 0xFC9BE6F6, 0x2BC73851};
unsigned int brainpoolp160r1_n_n0[1] = {0x5C7AADC7};
    #endif

    //[2^80]G
    #ifdef PKE_HP
unsigned int brainpoolp160r1_2_80_Gx[5] = {0xB19BD5A1, 0xD3B8210C, 0xBB518725, 0x39FC8E94, 0x8E63BD39};
unsigned int brainpoolp160r1_2_80_Gy[5] = {0x68DE4448, 0xAF1015A3, 0x8E138836, 0x17A68390, 0x4D1E977B};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp160r1[1] = {
    {
     160u,
     160u,
     (unsigned int *)brainpoolp160r1_p,
     (unsigned int *)brainpoolp160r1_p_h, //NULL,//
        (unsigned int *)brainpoolp160r1_a,
     (unsigned int *)brainpoolp160r1_b,
     (unsigned int *)brainpoolp160r1_Gx,
     (unsigned int *)brainpoolp160r1_Gy,
     (unsigned int *)brainpoolp160r1_n,
     (unsigned int *)brainpoolp160r1_n_h, //NULL,//
        (unsigned int *)brainpoolp160r1_2_80_Gx,
     (unsigned int *)brainpoolp160r1_2_80_Gy,
     },
};
    #else
eccp_curve_t brainpoolp160r1[1] = {
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
unsigned int brainpoolp192r1_p[6] = {0xE1A86297, 0x8FCE476D, 0x93D18DB7, 0xA7A34630, 0x932A36CD, 0xC302F41D};
    #ifdef PKE_HP
unsigned int brainpoolp192r1_p_h[6] = {0xF1375215, 0x01E662B2, 0x62E2228F, 0x6AE9CFFA, 0x2760DC78, 0xB0AFEBA3};
    #else
unsigned int brainpoolp192r1_p_h[6]  = {0x72C7B21A, 0xE2474C69, 0x02C3FE69, 0x33BF4846, 0xEED34F10, 0xB6225126};
unsigned int brainpoolp192r1_p_n0[1] = {0x56A2C2D9};
    #endif
unsigned int brainpoolp192r1_a[6]  = {0xC69A28EF, 0xCAE040E5, 0xFE8685C1, 0x9C39C031, 0x76B1E0E1, 0x6A911740};
unsigned int brainpoolp192r1_b[6]  = {0x6FBF25C9, 0xCA7EF414, 0x4F4496BC, 0xDC721D04, 0x7C28CCA3, 0x469A28EF};
unsigned int brainpoolp192r1_Gx[6] = {0x53375FD6, 0x0A2F5C48, 0x6CB0F090, 0x53B033C5, 0xAAB6A487, 0xC0A0647E};
unsigned int brainpoolp192r1_Gy[6] = {0xFA299B8F, 0xE6773FA2, 0xC1490002, 0x8B5F4828, 0x6ABD5BB8, 0x14B69086};
unsigned int brainpoolp192r1_n[6]  = {0x9AC4ACC1, 0x5BE8F102, 0x9E9E916B, 0xA7A3462F, 0x932A36CD, 0xC302F41D};
    #ifdef PKE_HP
unsigned int brainpoolp192r1_n_h[6] = {0xE7FCB8CE, 0x6ECF9194, 0x9F23B907, 0xCFB52741, 0x85FD1D24, 0xB2C3A70B};
    #else
unsigned int brainpoolp192r1_n_h[6]  = {0xE407E8F8, 0xB4727C80, 0xBF53AFF0, 0xBF4AFD5D, 0xE772102B, 0x98769B9C};
unsigned int brainpoolp192r1_n_n0[1] = {0x75DE1CBF};
    #endif

    //[2^96]G
    #ifdef PKE_HP
unsigned int brainpoolp192r1_2_96_Gx[6] = {0x6E55384C, 0xB4006CD5, 0xB5B55527, 0x086B6F1D, 0x1E972E92, 0x773D0297};
unsigned int brainpoolp192r1_2_96_Gy[6] = {0x3014234C, 0x32828E3D, 0x6634C2CE, 0xA3D9346E, 0x01CE0488, 0x807F615C};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp192r1[1] = {
    {
     192u,
     192u,
     (unsigned int *)brainpoolp192r1_p,
     (unsigned int *)brainpoolp192r1_p_h, //NULL,//
        (unsigned int *)brainpoolp192r1_a,
     (unsigned int *)brainpoolp192r1_b,
     (unsigned int *)brainpoolp192r1_Gx,
     (unsigned int *)brainpoolp192r1_Gy,
     (unsigned int *)brainpoolp192r1_n,
     (unsigned int *)brainpoolp192r1_n_h, //NULL,//
        (unsigned int *)brainpoolp192r1_2_96_Gx,
     (unsigned int *)brainpoolp192r1_2_96_Gy,
     },
};
    #else
eccp_curve_t brainpoolp192r1[1] = {
    {
     192u,
     192u,
     brainpoolp192r1_p,
     brainpoolp192r1_p_h,
     brainpoolp192r1_p_n0,
     brainpoolp192r1_a,
     brainpoolp192r1_b,
     brainpoolp192r1_Gx,
     brainpoolp192r1_Gy,
     brainpoolp192r1_n,
     brainpoolp192r1_n_h,  //NULL,//
        brainpoolp192r1_n_n0, //NULL,//
    },
};
    #endif
#endif


/**************************** brainpoolp224r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP224R1
unsigned int brainpoolp224r1_p[7] = {0x7EC8C0FF, 0x97DA89F5, 0xB09F0757, 0x75D1D787, 0x2A183025, 0x26436686, 0xD7C134AA};
    #ifdef PKE_HP
unsigned int brainpoolp224r1_p_h[7] = {0x6B3D58FF, 0x3BFBC9BF, 0xF691D105, 0x76F9EE46, 0x77E3D7E4, 0x7EA5577C, 0x2B3D40DD};
    #else
unsigned int brainpoolp224r1_p_h[7]  = {0x64DCD04F, 0x7867CA80, 0x43C20E72, 0x96AF774C, 0x3FE8A2AA, 0x2E6A6CE4, 0x0578FD59};
unsigned int brainpoolp224r1_p_n0[1] = {0xE149C101};
    #endif
unsigned int brainpoolp224r1_a[7]  = {0xCAD29F43, 0xB0042A59, 0x4E182AD8, 0xC1530B51, 0x299803A6, 0xA9CE6C1C, 0x68A5E62C};
unsigned int brainpoolp224r1_b[7]  = {0x386C400B, 0x66DBB372, 0x3E2135D2, 0xA92369E3, 0x870713B1, 0xCFE44138, 0x2580F63C};
unsigned int brainpoolp224r1_Gx[7] = {0xEE12C07D, 0x4C1E6EFD, 0x9E4CE317, 0xA87DC68C, 0x340823B2, 0x2C7E5CF4, 0x0D9029AD};
unsigned int brainpoolp224r1_Gy[7] = {0x761402CD, 0xCAA3F6D3, 0x354B9E99, 0x4ECDAC24, 0x24C6B89E, 0x72C0726F, 0x58AA56F7};
unsigned int brainpoolp224r1_n[7]  = {0xA5A7939F, 0x6DDEBCA3, 0xD116BC4B, 0x75D0FB98, 0x2A183025, 0x26436686, 0xD7C134AA};
    #ifdef PKE_HP
unsigned int brainpoolp224r1_n_h[7] = {0xE0D86B49, 0xF3D67605, 0xBAB96B21, 0xF0F40A07, 0xA35371E1, 0xB4581327, 0x5234FE17};
    #else
unsigned int brainpoolp224r1_n_h[7]  = {0x486CA401, 0xADDAF8AA, 0x9399652C, 0x9F24919B, 0x1E9CAE24, 0x3211A561, 0x4A73A656};
unsigned int brainpoolp224r1_n_n0[1] = {0x6CFB37A1};
    #endif

    //[2^112]G
    #ifdef PKE_HP
unsigned int brainpoolp224r1_2_112_Gx[7] = {0x18671CAC, 0x794ADD48, 0x619F1D35, 0xEE591079, 0xD7C84B0E, 0x0699B4A5, 0xB3CDA5BC};
unsigned int brainpoolp224r1_2_112_Gy[7] = {0xBB4A5447, 0x6E6080C1, 0x4D88B767, 0x3B07A68C, 0xD6D750A1, 0xFEF32E40, 0xBC2DC034};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp224r1[1] = {
    {
     224u,
     224u,
     (unsigned int *)brainpoolp224r1_p,
     (unsigned int *)brainpoolp224r1_p_h,
     (unsigned int *)brainpoolp224r1_a,
     (unsigned int *)brainpoolp224r1_b,
     (unsigned int *)brainpoolp224r1_Gx,
     (unsigned int *)brainpoolp224r1_Gy,
     (unsigned int *)brainpoolp224r1_n,
     (unsigned int *)brainpoolp224r1_n_h,
     (unsigned int *)brainpoolp224r1_2_112_Gx,
     (unsigned int *)brainpoolp224r1_2_112_Gy,
     },
};
    #else
eccp_curve_t brainpoolp224r1[1] = {
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
unsigned int brainpoolp256r1_p[8]   = {0x1F6E5377, 0x2013481D, 0xD5262028, 0x6E3BF623, 0x9D838D72, 0x3E660A90, 0xA1EEA9BC, 0xA9FB57DB};
unsigned int brainpoolp256r1_p_h[8] = {0xA6465B6C, 0x8CFEDF7B, 0x614D4F4D, 0x5CCE4C26, 0x6B1AC807, 0xA1ECDACD, 0xE5957FA8, 0x4717AA21};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int brainpoolp256r1_p_n0[1] = {0xCEFD89B9};
    #endif
unsigned int brainpoolp256r1_a[8]   = {0xF330B5D9, 0xE94A4B44, 0x26DC5C6C, 0xFB8055C1, 0x417AFFE7, 0xEEF67530, 0xFC2C3057, 0x7D5A0975};
unsigned int brainpoolp256r1_b[8]   = {0xFF8C07B6, 0x6BCCDC18, 0x5CF7E1CE, 0x95841629, 0xBBD77CBF, 0xF330B5D9, 0xE94A4B44, 0x26DC5C6C};
unsigned int brainpoolp256r1_Gx[8]  = {0x9ACE3262, 0x3A4453BD, 0xE3BD23C2, 0xB9DE27E1, 0xFC81B7AF, 0x2C4B482F, 0xCB7E57CB, 0x8BD2AEB9};
unsigned int brainpoolp256r1_Gy[8]  = {0x2F046997, 0x5C1D54C7, 0x2DED8E54, 0xC2774513, 0x14611DC9, 0x97F8461A, 0xC3DAC4FD, 0x547EF835};
unsigned int brainpoolp256r1_n[8]   = {0x974856A7, 0x901E0E82, 0xB561A6F7, 0x8C397AA3, 0x9D838D71, 0x3E660A90, 0xA1EEA9BC, 0xA9FB57DB};
unsigned int brainpoolp256r1_n_h[8] = {0x3312FCA6, 0xE1D8D8DE, 0x1134E4A0, 0xF35D176A, 0x6C815CB0, 0x9B7F25E7, 0xC3236762, 0x0B25F1B9};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int brainpoolp256r1_n_n0[1] = {0xCBB40EE9};
    #endif

    //[2^128]G
    #ifdef PKE_HP
unsigned int brainpoolp256r1_2_128_Gx[8] = {0xF58472C9, 0xEB6B651C, 0x11006590, 0x1200CA9B, 0x7F87ED9D, 0xB4438511, 0x3B856C94, 0x4A14C030};
unsigned int brainpoolp256r1_2_128_Gy[8] = {0x28F852D1, 0x529C5CD6, 0xCD732117, 0xD544A068, 0x8B47CC5E, 0xE6387349, 0xDAE2D5EF, 0x7B81E470};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp256r1[1] = {
    {
     256u,
     256u,
     (unsigned int *)brainpoolp256r1_p,
     (unsigned int *)brainpoolp256r1_p_h,
     (unsigned int *)brainpoolp256r1_a,
     (unsigned int *)brainpoolp256r1_b,
     (unsigned int *)brainpoolp256r1_Gx,
     (unsigned int *)brainpoolp256r1_Gy,
     (unsigned int *)brainpoolp256r1_n,
     (unsigned int *)brainpoolp256r1_n_h, //NULL,//
        (unsigned int *)brainpoolp256r1_2_128_Gx,
     (unsigned int *)brainpoolp256r1_2_128_Gy,
     },
};
    #else
eccp_curve_t brainpoolp256r1[1] = {
    {
     256u,
     256u,
     brainpoolp256r1_p,
     brainpoolp256r1_p_h,
     brainpoolp256r1_p_n0,
     brainpoolp256r1_a,
     brainpoolp256r1_b,
     brainpoolp256r1_Gx,
     brainpoolp256r1_Gy,
     brainpoolp256r1_n,
     brainpoolp256r1_n_h,  //NULL,//
        brainpoolp256r1_n_n0, //NULL,//
    },
};
    #endif
#endif


/**************************** brainpoolp320r1 ******************************/
#ifdef SUPPORT_BRAINPOOLP320R1
unsigned int brainpoolp320r1_p[10] = {0xF1B32E27, 0xFCD412B1, 0x7893EC28, 0x4F92B9EC, 0xF6F40DEF, 0xF98FCFA6, 0xD201E065, 0xE13C785E, 0x36BC4FB7, 0xD35E4720};
    #ifdef PKE_HP
unsigned int brainpoolp320r1_p_h[10] = {0x173FD2B9, 0xB487F1A2, 0x6246117A, 0x639B0116, 0xEA02CBC0, 0x44C86B0D, 0x9289E4AB, 0xD6D9D773, 0xD1FEA1C3, 0xD2E2BD5A};
    #else
unsigned int brainpoolp320r1_p_h[10] = {0x743B52F9, 0x994EE88A, 0x906978EF, 0xC2478A8D, 0x30C5B676, 0x1F4C881F, 0xE614D6D2, 0x5455A964, 0x6C2D9252, 0xA259BA4A};
unsigned int brainpoolp320r1_p_n0[1] = {0x2A8A9E69};
    #endif
unsigned int brainpoolp320r1_a[10]  = {0x7D860EB4, 0x92F375A9, 0x85FFA9F4, 0x66190EB0, 0xF5EB79DA, 0xA2A73513, 0x6D3F3BB8, 0x83CCEBD4, 0x8FBAB0F8, 0x3EE30B56};
unsigned int brainpoolp320r1_b[10]  = {0x8FB1F1A6, 0x6F5EB4AC, 0x88453981, 0xCC31DCCD, 0x9554B49A, 0xE13F4134, 0x40688A6F, 0xD3AD1986, 0x9DFDBC42, 0x52088394};
unsigned int brainpoolp320r1_Gx[10] = {0x39E20611, 0x10AF8D0D, 0x10A599C7, 0xE7871E2A, 0x0A087EB6, 0xF20137D1, 0x8EE5BFE6, 0x5289BCC4, 0xFB53D8B8, 0x43BD7E9A};
unsigned int brainpoolp320r1_Gy[10] = {0x692E8EE1, 0xD35245D1, 0xAAAC6AC7, 0xA9C77877, 0x117182EA, 0x0743FFED, 0x7F77275E, 0xAB409324, 0x45EC1CC8, 0x14FDD055};
unsigned int brainpoolp320r1_n[10]  = {0x44C59311, 0x8691555B, 0xEE8658E9, 0x2D482EC7, 0xB68F12A3, 0xF98FCFA5, 0xD201E065, 0xE13C785E, 0x36BC4FB7, 0xD35E4720};
    #ifdef PKE_HP
unsigned int brainpoolp320r1_n_h[10] = {0x69C0E896, 0x7C11F348, 0xE18227E6, 0xC61AF312, 0x60B04341, 0x6666D6A0, 0xB83FEB01, 0x66ED2F8B, 0x017FEA5B, 0x2606C905};
    #else
unsigned int brainpoolp320r1_n_h[10] = {0x2513E4CD, 0x679D29DF, 0xE0E16805, 0x91C3001B, 0xAF86C409, 0x86B330BC, 0x4E6390FE, 0xE30D3524, 0x3200B14F, 0x31EC87C7};
unsigned int brainpoolp320r1_n_n0[1] = {0xFC62420F};
    #endif

    //[2^160]G
    #ifdef PKE_HP
unsigned int brainpoolp320r1_2_160_Gx[10] = {0x7C841CBC, 0x7723D2D1, 0x13C4A1FA, 0x20619B2A, 0xB7ED9FD8, 0x41C14B62, 0xD324B0FF, 0x37C47C97, 0x675062D3, 0x3BC19CAE};
unsigned int brainpoolp320r1_2_160_Gy[10] = {0x45BEE1AA, 0xEB2238EE, 0x4F8EA777, 0x68A3020A, 0x2422C609, 0xA45BC518, 0x6B9BA3E5, 0x704D6FB7, 0xE7B2DC94, 0x8C1DE9E0};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp320r1[1] = {
    {
     320u,
     320u,
     (unsigned int *)brainpoolp320r1_p,
     (unsigned int *)brainpoolp320r1_p_h,
     (unsigned int *)brainpoolp320r1_a,
     (unsigned int *)brainpoolp320r1_b,
     (unsigned int *)brainpoolp320r1_Gx,
     (unsigned int *)brainpoolp320r1_Gy,
     (unsigned int *)brainpoolp320r1_n,
     (unsigned int *)brainpoolp320r1_n_h,
     (unsigned int *)brainpoolp320r1_2_160_Gx,
     (unsigned int *)brainpoolp320r1_2_160_Gy,
     },
};
    #else
eccp_curve_t brainpoolp320r1[1] = {
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
unsigned int brainpoolp384r1_p[12] = {0x3107EC53, 0x87470013, 0x901D1A71, 0xACD3A729, 0x7FB71123, 0x12B1DA19, 0xED5456B4, 0x152F7109, 0x50E641DF, 0x0F5D6F7E, 0xA3386D28, 0x8CB91E82};
    #ifdef PKE_HP
unsigned int brainpoolp384r1_p_h[12] = {0xF2E00133, 0x41FB3B28, 0x62CAC6F4, 0xDC038BB7, 0xD1773502, 0x07B40011, 0x37CF40F3, 0x2DEB1A30, 0xEB9AA2CA, 0xCCE5246A, 0x96900F16, 0x273B1C6C};
    #else
unsigned int brainpoolp384r1_p_h[12] = {0x40B64BDE, 0x087CEFFF, 0x3D7FD965, 0x53528334, 0xC9940899, 0x8E28F99C, 0x9918D5AF, 0x62140191, 0xA57E052C, 0xD5C6EF3B, 0x178DF842, 0x36BF6883};
unsigned int brainpoolp384r1_p_n0[1] = {0xEA9EC825};
    #endif
unsigned int brainpoolp384r1_a[12]  = {0x22CE2826, 0x04A8C7DD, 0x503AD4EB, 0x8AA5814A, 0xBA91F90F, 0x139165EF, 0x4FB22787, 0xC2BEA28E, 0xCE05AFA0, 0x3C72080A, 0x3D8C150C, 0x7BC382C6};
unsigned int brainpoolp384r1_b[12]  = {0xFA504C11, 0x3AB78696, 0x95DBC994, 0x7CB43902, 0x3EEB62D5, 0x2E880EA5, 0x07DCD2A6, 0x2FB77DE1, 0x16F0447C, 0x8B39B554, 0x22CE2826, 0x04A8C7DD};
unsigned int brainpoolp384r1_Gx[12] = {0x47D4AF1E, 0xEF87B2E2, 0x36D646AA, 0xE826E034, 0x0CBD10E8, 0xDB7FCAFE, 0x7EF14FE3, 0x8847A3E7, 0xB7C13F6B, 0xA2A63A81, 0x68CF45FF, 0x1D1C64F0};
unsigned int brainpoolp384r1_Gy[12] = {0x263C5315, 0x42820341, 0x77918111, 0x0E464621, 0xF9912928, 0xE19C054F, 0xFEEC5864, 0x62B70B29, 0x95CFD552, 0x5CB1EB8E, 0x20F9C2A4, 0x8ABE1D75};
unsigned int brainpoolp384r1_n[12]  = {0xE9046565, 0x3B883202, 0x6B7FC310, 0xCF3AB6AF, 0xAC0425A7, 0x1F166E6C, 0xED5456B3, 0x152F7109, 0x50E641DF, 0x0F5D6F7E, 0xA3386D28, 0x8CB91E82};
    #ifdef PKE_HP
unsigned int brainpoolp384r1_n_h[12] = {0x4894533B, 0x2FC7D3C1, 0x16DB4EF9, 0x18D9AF26, 0x02483F80, 0x6A0C5704, 0x8D80258B, 0xB8A756A8, 0x3F1936AF, 0xFDA24123, 0x143E3D5C, 0x354DFF02};
    #else
unsigned int brainpoolp384r1_n_h[12] = {0xDE771C8E, 0xAC4ED3A2, 0x2F2B6B6E, 0x37264E20, 0x9802688A, 0x2A927E3B, 0x52D748FF, 0x574A74CB, 0x65165FDB, 0x8F886DC9, 0x614E97C2, 0x0CE8941A};
unsigned int brainpoolp384r1_n_n0[1] = {0x5CB5BB93};
    #endif

    //[2^192]G
    #ifdef PKE_HP
unsigned int brainpoolp384r1_2_192_Gx[12] = {0x4D994B04, 0x6BF550B0, 0x5345A946, 0x02A353BC, 0x02C7F727, 0xE18A5D2D, 0xBB1FDC04, 0xDF4DDACC, 0x8E81AF98, 0x8974B568, 0x397C99F1, 0x2369DBB6};
unsigned int brainpoolp384r1_2_192_Gy[12] = {0x14504C85, 0x0EE188BB, 0x500CB2C1, 0x43F143E6, 0x3334CE66, 0xCD41CAE9, 0x0742FA8F, 0x127368F1, 0x925FF0A8, 0x93B08775, 0xAA3B124F, 0x6F47B11D};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp384r1[1] = {
    {
     384u,
     384u,
     (unsigned int *)brainpoolp384r1_p,
     (unsigned int *)brainpoolp384r1_p_h,
     (unsigned int *)brainpoolp384r1_a,
     (unsigned int *)brainpoolp384r1_b,
     (unsigned int *)brainpoolp384r1_Gx,
     (unsigned int *)brainpoolp384r1_Gy,
     (unsigned int *)brainpoolp384r1_n,
     (unsigned int *)brainpoolp384r1_n_h,
     (unsigned int *)brainpoolp384r1_2_192_Gx,
     (unsigned int *)brainpoolp384r1_2_192_Gy,
     },
};
    #else
eccp_curve_t brainpoolp384r1[1] = {
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
unsigned int brainpoolp512r1_p[16]   = {0x583A48F3, 0x28AA6056, 0x2D82C685, 0x2881FF2F, 0xE6A380E6, 0xAECDA12A, 0x9BC66842, 0x7D4D9B00, 0x70330871, 0xD6639CCA, 0xB3C9D20E, 0xCB308DB3, 0x33C9FC07, 0x3FD4E6AE, 0xDBE9C48B, 0xAADD9DB8};
unsigned int brainpoolp512r1_p_h[16] = {0x6158F205, 0x49AD144A, 0x27157905, 0x793FB130, 0x905AFFD3, 0x53B7F9BC, 0x83514A25, 0xE0C19A77, 0xD5898057, 0x19486FD8, 0xD42BFF83, 0xA16DAA5F, 0x2056EECC, 0x202E1940, 0xA9FF6450, 0x3C4C9D05};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int brainpoolp512r1_p_n0[1] = {0x7D89EFC5};
    #endif
unsigned int brainpoolp512r1_a[16]   = {0x77FC94CA, 0xE7C1AC4D, 0x2BF2C7B9, 0x7F1117A7, 0x8B9AC8B5, 0x0A2EF1C9, 0xA8253AA1, 0x2DED5D5A, 0xEA9863BC, 0xA83441CA, 0x3DF91610, 0x94CBDD8D, 0xAC234CC5, 0xE2327145, 0x8B603B89, 0x7830A331};
unsigned int brainpoolp512r1_b[16]   = {0x8016F723, 0x2809BD63, 0x5EBAE5DD, 0x984050B7, 0xDC083E67, 0x77FC94CA, 0xE7C1AC4D, 0x2BF2C7B9, 0x7F1117A7, 0x8B9AC8B5, 0x0A2EF1C9, 0xA8253AA1, 0x2DED5D5A, 0xEA9863BC, 0xA83441CA, 0x3DF91610};
unsigned int brainpoolp512r1_Gx[16]  = {0xBCB9F822, 0x8B352209, 0x406A5E68, 0x7C6D5047, 0x93B97D5F, 0x50D1687B, 0xE2D0D48D, 0xFF3B1F78, 0xF4D0098E, 0xB43B62EE, 0xB5D916C1, 0x85ED9F70, 0x9C4C6A93, 0x5A21322E, 0xD82ED964, 0x81AEE4BD};
unsigned int brainpoolp512r1_Gy[16]  = {0x3AD80892, 0x78CD1E0F, 0xA8F05406, 0xD1CA2B2F, 0x8A2763AE, 0x5BCA4BD8, 0x4A5F485E, 0xB2DCDE49, 0x881F8111, 0xA000C55B, 0x24A57B1A, 0xF209F700, 0xCF7822FD, 0xC0EABFA9, 0x566332EC, 0x7DDE385D};
unsigned int brainpoolp512r1_n[16]   = {0x9CA90069, 0xB5879682, 0x085DDADD, 0x1DB1D381, 0x7FAC1047, 0x41866119, 0x4CA92619, 0x553E5C41, 0x70330870, 0xD6639CCA, 0xB3C9D20E, 0xCB308DB3, 0x33C9FC07, 0x3FD4E6AE, 0xDBE9C48B, 0xAADD9DB8};
unsigned int brainpoolp512r1_n_h[16] = {0xCDA81671, 0xD2A3681E, 0x95283DDD, 0x0886B758, 0x33B7627F, 0x3EC64BD0, 0x2F0207E8, 0xA6F230C7, 0x3B790DE3, 0xD7F9CC26, 0x2F16BBDF, 0x723C37A2, 0x194B2E56, 0x95DF1B4C, 0x718407B0, 0xA794586A};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int brainpoolp512r1_n_n0[1] = {0x0F1B7027};
    #endif

    //[2^256]G
    #ifdef PKE_HP
unsigned int brainpoolp512r1_2_256_Gx[16] = {0xEF66EFB5, 0xCA448127, 0x08AC7DA5, 0x9D189C54, 0x730A3721, 0x97502F33, 0xACA3B94E, 0x1BAE0F47, 0x945E8460, 0x79A83D9B, 0x8F284870, 0xC056FD65, 0xB8F1B2AF, 0xD5262CE1, 0x66C4ED95, 0x7B5913F7};
unsigned int brainpoolp512r1_2_256_Gy[16] = {0xC2F45872, 0xD610453C, 0xDA0092C9, 0x3E9470F8, 0x4EB496C3, 0x24EC6F84, 0xBC578877, 0xFB4EDB0D, 0x5FC450A8, 0xE46656A7, 0x61306AF0, 0xC21FCA91, 0xEA20B708, 0x3727EA1E, 0x4FFC593A, 0x3B4D8E45};
    #endif

    #ifdef PKE_HP
eccp_curve_t brainpoolp512r1[1] = {
    {
     512u,
     512u,
     (unsigned int *)brainpoolp512r1_p,
     (unsigned int *)brainpoolp512r1_p_h,
     (unsigned int *)brainpoolp512r1_a,
     (unsigned int *)brainpoolp512r1_b,
     (unsigned int *)brainpoolp512r1_Gx,
     (unsigned int *)brainpoolp512r1_Gy,
     (unsigned int *)brainpoolp512r1_n,
     (unsigned int *)brainpoolp512r1_n_h, //NULL,//
        (unsigned int *)brainpoolp512r1_2_256_Gx,
     (unsigned int *)brainpoolp512r1_2_256_Gy,
     },
};
    #else
eccp_curve_t brainpoolp512r1[1] = {
    {
     512u,
     512u,
     brainpoolp512r1_p,
     brainpoolp512r1_p_h,
     brainpoolp512r1_p_n0,
     brainpoolp512r1_a,
     brainpoolp512r1_b,
     brainpoolp512r1_Gx,
     brainpoolp512r1_Gy,
     brainpoolp512r1_n,
     brainpoolp512r1_n_h,  //NULL,//
        brainpoolp512r1_n_n0, //NULL,//
    },
};
    #endif
#endif


/**************************** secp160r1 ******************************/
#ifdef SUPPORT_SECP160R1
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160r1_p[5]   = {0x7FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r1_p_h[5] = {0x00000000, 0x80000001, 0xC0000001, 0x20000000, 0x00000000};
    #elif (defined(PKE_LP))
unsigned int secp160r1_p[6]    = {0x7FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
unsigned int secp160r1_p_h[6]  = {0x00000000, 0x00000000, 0x00000001, 0x40000001, 0x00000000, 0x00000000};
unsigned int secp160r1_p_n0[1] = {0x80000001};
    #elif (defined(PKE_SECURE))
unsigned int secp160r1_p[5]    = {0x7FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r1_p_h[5]  = {0x00000001, 0x40000001, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp160r1_p_n0[1] = {0x80000001};
    #endif
unsigned int secp160r1_a[5]  = {0x7FFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r1_b[5]  = {0xC565FA45, 0x81D4D4AD, 0x65ACF89F, 0x54BD7A8B, 0x1C97BEFC};
unsigned int secp160r1_Gx[5] = {0x13CBFC82, 0x68C38BB9, 0x46646989, 0x8EF57328, 0x4A96B568};
unsigned int secp160r1_Gy[5] = {0x7AC5FB32, 0x04235137, 0x59DCC912, 0x3168947D, 0x23A62855};
unsigned int secp160r1_n[6]  = {0xCA752257, 0xF927AED3, 0x0001F4C8, 0x00000000, 0x00000000, 0x00000001};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160r1_n_h[6] = {0xBD025FA7, 0xDBB68B25, 0xD1E8F73F, 0xE9DD6F6C, 0x39DE6382, 0x00000000};
    #else
unsigned int secp160r1_n_h[6]  = {0x6744F8A4, 0x085E335F, 0x3CDC3854, 0x7A981E4B, 0xA0E62683, 0x00000000};
unsigned int secp160r1_n_n0[1] = {0x306D1699};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160r1_2_80_Gx[5] = {0x89665347, 0xB21844A4, 0xA2961A17, 0xBE4AD4C7, 0xF2E0A32F};
unsigned int secp160r1_2_80_Gy[5] = {0x31B980CA, 0x6519E3DE, 0x92DEA640, 0xCAA2F378, 0x46B7032F};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp160r1[1] = {
    {
     160u,
     161u,
     (unsigned int *)secp160r1_p,
     (unsigned int *)secp160r1_p_h,
     (unsigned int *)secp160r1_a,
     (unsigned int *)secp160r1_b,
     (unsigned int *)secp160r1_Gx,
     (unsigned int *)secp160r1_Gy,
     (unsigned int *)secp160r1_n,
     (unsigned int *)secp160r1_n_h,
     (unsigned int *)secp160r1_2_80_Gx,
     (unsigned int *)secp160r1_2_80_Gy,
     },
};
    #else
eccp_curve_t secp160r1[1] = {
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
unsigned int secp160r2_p[5]   = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r2_p_h[5] = {0x00000000, 0x4DB32715, 0x51CE3BE1, 0x0000FAA7, 0x00000001};
    #elif (defined(PKE_LP))
unsigned int secp160r2_p[6]    = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
unsigned int secp160r2_p_h[6]  = {0x00000000, 0x00000000, 0x1B44BBA9, 0x0000A71A, 0x00000001, 0x00000000};
unsigned int secp160r2_p_n0[1] = {0xB4AB2745};
    #elif (defined(PKE_SECURE))
unsigned int secp160r2_p[5]    = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r2_p_h[5]  = {0x1B44BBA9, 0x0000A71A, 0x00000001, 0x00000000, 0x00000000};
unsigned int secp160r2_p_n0[1] = {0xB4AB2745};
    #endif
unsigned int secp160r2_a[5]  = {0xFFFFAC70, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160r2_b[5]  = {0xF50388BA, 0x04664D5A, 0xAB572749, 0xFB59EB8B, 0xB4E134D3};
unsigned int secp160r2_Gx[5] = {0x3144CE6D, 0x30F7199D, 0x1F4FF11B, 0x293A117E, 0x52DCB034};
unsigned int secp160r2_Gy[5] = {0xA7D43F2E, 0xF9982CFE, 0xE071FA0D, 0xE331F296, 0xFEAFFEF2};
unsigned int secp160r2_n[6]  = {0xF3A1A16B, 0xE786A818, 0x0000351E, 0x00000000, 0x00000000, 0x00000001};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160r2_n_h[6] = {0x214B0C57, 0xD6937441, 0x9EB754BC, 0x39F295A5, 0x11F4417D, 0x00000000};
    #else
unsigned int secp160r2_n_h[6]  = {0xD8C126C7, 0x29AEB02A, 0x8DD4D69E, 0x4769EF9E, 0x76E5A181, 0x00000000};
unsigned int secp160r2_n_n0[1] = {0xD9747CBD};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160r2_2_80_Gx[5] = {0x9FDBE7CC, 0x2DD8DA5D, 0x06A1961F, 0x029E4D9B, 0xAF6092BE};
unsigned int secp160r2_2_80_Gy[5] = {0x31A7C90D, 0x56D0EE47, 0x952F7D35, 0xDCD3FB29, 0x3E9221CD};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp160r2[1] = {
    {
     160u,
     161u,
     (unsigned int *)secp160r2_p,
     (unsigned int *)secp160r2_p_h,
     (unsigned int *)secp160r2_a,
     (unsigned int *)secp160r2_b,
     (unsigned int *)secp160r2_Gx,
     (unsigned int *)secp160r2_Gy,
     (unsigned int *)secp160r2_n,
     (unsigned int *)secp160r2_n_h,
     (unsigned int *)secp160r2_2_80_Gx,
     (unsigned int *)secp160r2_2_80_Gy,
     },
};
    #else
eccp_curve_t secp160r2[1] = {
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
unsigned int secp192r1_p[6] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192r1_p_h[6] = {0x00000002, 0x00000000, 0x00000003, 0x00000000, 0x00000002, 0x00000000};
    #else
unsigned int secp192r1_p_h[6]  = {0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000001, 0x00000000};
unsigned int secp192r1_p_n0[1] = {1};
    #endif
unsigned int secp192r1_a[6]  = {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp192r1_b[6]  = {0xC146B9B1, 0xFEB8DEEC, 0x72243049, 0x0FA7E9AB, 0xE59C80E7, 0x64210519};
unsigned int secp192r1_Gx[6] = {0x82FF1012, 0xF4FF0AFD, 0x43A18800, 0x7CBF20EB, 0xB03090F6, 0x188DA80E};
unsigned int secp192r1_Gy[6] = {0x1E794811, 0x73F977A1, 0x6B24CDD5, 0x631011ED, 0xFFC8DA78, 0x07192B95};
unsigned int secp192r1_n[6]  = {0xB4D22831, 0x146BC9B1, 0x99DEF836, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192r1_n_h[6] = {0x83134C27, 0x01D1770A, 0xCAAF687F, 0xD69C6961, 0xCEF5D8C5, 0x126792C4};
    #else
unsigned int secp192r1_n_h[6]  = {0xDEB35961, 0xCE66BACC, 0xBB3A6BEE, 0x4696EA5B, 0xEA0581A2, 0x28BE5677};
unsigned int secp192r1_n_n0[1] = {0x0DDBCF2F};
    #endif

    //[2^96]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192r1_2_96_Gx[6] = {0xC0A1E340, 0xB19963D8, 0x80D1090B, 0x4730D4F4, 0x184AC737, 0x51A581D9};
unsigned int secp192r1_2_96_Gy[6] = {0xE69912A5, 0xECC56731, 0x2F683F16, 0x7CDFCEA0, 0xE0BB9F6E, 0x5BD81EE2};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp192r1[1] = {
    {
     192u,
     192u,
     (unsigned int *)secp192r1_p,
     (unsigned int *)secp192r1_p_h,
     (unsigned int *)secp192r1_a,
     (unsigned int *)secp192r1_b,
     (unsigned int *)secp192r1_Gx,
     (unsigned int *)secp192r1_Gy,
     (unsigned int *)secp192r1_n,
     (unsigned int *)secp192r1_n_h,
     (unsigned int *)secp192r1_2_96_Gx,
     (unsigned int *)secp192r1_2_96_Gy,
     },
};
    #else
eccp_curve_t secp192r1[1] = {
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
unsigned int secp224r1_p[7] = {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp224r1_p_h[7] = {0x00000001, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFE, 0xFFFFFFFF};
    #else
unsigned int secp224r1_p_h[7]  = {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
unsigned int secp224r1_p_n0[1] = {0xFFFFFFFF};
    #endif
unsigned int secp224r1_a[7]  = {0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp224r1_b[7]  = {0x2355FFB4, 0x270B3943, 0xD7BFD8BA, 0x5044B0B7, 0xF5413256, 0x0C04B3AB, 0xB4050A85};
unsigned int secp224r1_Gx[7] = {0x115C1D21, 0x343280D6, 0x56C21122, 0x4A03C1D3, 0x321390B9, 0x6BB4BF7F, 0xB70E0CBD};
unsigned int secp224r1_Gy[7] = {0x85007E34, 0x44D58199, 0x5A074764, 0xCD4375A0, 0x4C22DFE6, 0xB5F723FB, 0xBD376388};
unsigned int secp224r1_n[7]  = {0x5C5C2A3D, 0x13DD2945, 0xE0B8F03E, 0xFFFF16A2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp224r1_n_h[7] = {0x5F517D15, 0x29947A69, 0x31D63F4B, 0xABC8FF59, 0xD9714856, 0x6AD15F7C, 0xB1E97961};
    #else
unsigned int secp224r1_n_h[7]  = {0x3AD01289, 0x6BDAAE6C, 0x97A54552, 0x6AD09D91, 0xB1E97961, 0x1822BC47, 0xD4BAA4CF};
unsigned int secp224r1_n_n0[1] = {0x6A1FC2EB};
    #endif

    //[2^112]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp224r1_2_112_Gx[7] = {0x6CAB26E3, 0xA0064196, 0x2991FAB0, 0x3A0B91FB, 0xEC27A4E1, 0x5F8EBEEF, 0x0499AA8A};
unsigned int secp224r1_2_112_Gy[7] = {0x7766AF5D, 0x50751040, 0x29610D54, 0xF70684D9, 0xD77AAE82, 0x338C5B81, 0x6916F6D4};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp224r1[1] = {
    {
     224u,
     224u,
     (unsigned int *)secp224r1_p,
     (unsigned int *)secp224r1_p_h,
     (unsigned int *)secp224r1_a,
     (unsigned int *)secp224r1_b,
     (unsigned int *)secp224r1_Gx,
     (unsigned int *)secp224r1_Gy,
     (unsigned int *)secp224r1_n,
     (unsigned int *)secp224r1_n_h,
     (unsigned int *)secp224r1_2_112_Gx,
     (unsigned int *)secp224r1_2_112_Gy,
     },
};
    #else
eccp_curve_t secp224r1[1] = {
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
unsigned int secp256r1_p[8]   = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF};
unsigned int secp256r1_p_h[8] = {0x00000003, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFB, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFD, 0x00000004};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int secp256r1_p_n0[1] = {1};
    #endif
unsigned int secp256r1_a[8]   = {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF};
unsigned int secp256r1_b[8]   = {0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0, 0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8};
unsigned int secp256r1_Gx[8]  = {0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81, 0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2};
unsigned int secp256r1_Gy[8]  = {0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357, 0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2};
unsigned int secp256r1_n[8]   = {0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF};
unsigned int secp256r1_n_h[8] = {0xBE79EEA2, 0x83244C95, 0x49BD6FA6, 0x4699799C, 0x2B6BEC59, 0x2845B239, 0xF3D95620, 0x66E12D94};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int secp256r1_n_n0[1] = {0xEE00BC4F};
    #endif

    //[2^128]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp256r1_2_128_Gx[8] = {0xD789BD85, 0x57C84FC9, 0xC297EAC3, 0xFC35FF7D, 0x88C6766E, 0xFB982FD5, 0xEEDB5E67, 0x447D739B};
unsigned int secp256r1_2_128_Gy[8] = {0x72E25B32, 0x0C7E33C9, 0xA7FAE500, 0x3D349B95, 0x3A4AAFF7, 0xE12E9D95, 0x834131EE, 0x2D4825AB};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp256r1[1] = {
    {
     256u,
     256u,
     (unsigned int *)secp256r1_p,
     (unsigned int *)secp256r1_p_h, //NULL, //
        (unsigned int *)secp256r1_a,
     (unsigned int *)secp256r1_b,
     (unsigned int *)secp256r1_Gx,
     (unsigned int *)secp256r1_Gy,
     (unsigned int *)secp256r1_n,
     (unsigned int *)secp256r1_n_h, //NULL, //
        (unsigned int *)secp256r1_2_128_Gx,
     (unsigned int *)secp256r1_2_128_Gy,
     },
};
    #else
eccp_curve_t secp256r1[1] = {
    {
     256u,
     256u,
     secp256r1_p,
     secp256r1_p_h,  //NULL, //
        secp256r1_p_n0, //NULL, //
        secp256r1_a,
     secp256r1_b,
     secp256r1_Gx,
     secp256r1_Gy,
     secp256r1_n,
     secp256r1_n_h,  //NULL, //
        secp256r1_n_n0, //NULL, //
    },
};
    #endif
#endif


/**************************** secp384r1 ******************************/
#ifdef SUPPORT_SECP384R1
unsigned int secp384r1_p[12] = {0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp384r1_p_h[12] = {0x00000000, 0xFFFFFFFE, 0x00000002, 0x00000001, 0xFFFFFFFD, 0xFFFFFFFD, 0x00000002, 0x00000003, 0x00000002, 0xFFFFFFFE, 0x00000000, 0x00000002};
    #else
unsigned int secp384r1_p_h[12] = {0x00000001, 0xFFFFFFFE, 0x00000000, 0x00000002, 0x00000000, 0xFFFFFFFE, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp384r1_p_n0[1] = {1};
    #endif
unsigned int secp384r1_a[12]  = {0xFFFFFFFC, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp384r1_b[12]  = {0xD3EC2AEF, 0x2A85C8ED, 0x8A2ED19D, 0xC656398D, 0x5013875A, 0x0314088F, 0xFE814112, 0x181D9C6E, 0xE3F82D19, 0x988E056B, 0xE23EE7E4, 0xB3312FA7};
unsigned int secp384r1_Gx[12] = {0x72760AB7, 0x3A545E38, 0xBF55296C, 0x5502F25D, 0x82542A38, 0x59F741E0, 0x8BA79B98, 0x6E1D3B62, 0xF320AD74, 0x8EB1C71E, 0xBE8B0537, 0xAA87CA22};
unsigned int secp384r1_Gy[12] = {0x90EA0E5F, 0x7A431D7C, 0x1D7E819D, 0x0A60B1CE, 0xB5F0B8C0, 0xE9DA3113, 0x289A147C, 0xF8F41DBD, 0x9292DC29, 0x5D9E98BF, 0x96262C6F, 0x3617DE4A};
unsigned int secp384r1_n[12]  = {0xCCC52973, 0xECEC196A, 0x48B0A77A, 0x581A0DB2, 0xF4372DDF, 0xC7634D81, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp384r1_n_h[12] = {0xAE87B9E7, 0x7FDB954A, 0xB6F62810, 0x8C23F7F3, 0xAAAE6873, 0xE99276EA, 0xB33C33C5, 0xD558BFBC, 0xEED33213, 0x3B277D79, 0xF4A0E792, 0xCC9601F9};
    #else
unsigned int secp384r1_n_h[12] = {0x19B409A9, 0x2D319B24, 0xDF1AA419, 0xFF3D81E5, 0xFCB82947, 0xBC3E483A, 0x4AAB1CC5, 0xD40D4917, 0x28266895, 0x3FB05B7A, 0x2B39BF21, 0x0C84EE01};
unsigned int secp384r1_n_n0[1] = {0xE88FDC45};
    #endif

    //[2^192]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp384r1_2_192_Gx[12] = {0xAA03BD53, 0xA628B09A, 0xA4F52D78, 0xBA065458, 0x4D10DDEA, 0xDB298789, 0x8A3E297D, 0xB42A31AF, 0x06421279, 0x40F7F9E7, 0x800119C4, 0xC19E0B4C};
unsigned int secp384r1_2_192_Gy[12] = {0xE6C88C41, 0x822D0FC5, 0xE639D858, 0xAF68AA6D, 0x35F6EBF2, 0xC1C7CAD1, 0xE3567AF9, 0x577A30EA, 0x1F5B77F6, 0xE5A0191D, 0x0356B301, 0x16F3FDBF};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp384r1[1] = {
    {
     384u,
     384u,
     (unsigned int *)secp384r1_p,
     (unsigned int *)secp384r1_p_h,
     (unsigned int *)secp384r1_a,
     (unsigned int *)secp384r1_b,
     (unsigned int *)secp384r1_Gx,
     (unsigned int *)secp384r1_Gy,
     (unsigned int *)secp384r1_n,
     (unsigned int *)secp384r1_n_h,
     (unsigned int *)secp384r1_2_192_Gx,
     (unsigned int *)secp384r1_2_192_Gy,
     },
};
    #else
eccp_curve_t secp384r1[1] = {
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
unsigned int secp521r1_p[17] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp521r1_p_h[17] = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00004000, 0x00000000};
    #else
unsigned int secp521r1_p_h[17] = {0x00000000, 0x00004000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp521r1_p_n0[1] = {1};
    #endif
unsigned int secp521r1_a[17]  = {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF};
unsigned int secp521r1_b[17]  = {0x6B503F00, 0xEF451FD4, 0x3D2C34F1, 0x3573DF88, 0x3BB1BF07, 0x1652C0BD, 0xEC7E937B, 0x56193951, 0x8EF109E1, 0xB8B48991, 0x99B315F3, 0xA2DA725B, 0xB68540EE, 0x929A21A0, 0x8E1C9A1F, 0x953EB961, 0x00000051};
unsigned int secp521r1_Gx[17] = {0xC2E5BD66, 0xF97E7E31, 0x856A429B, 0x3348B3C1, 0xA2FFA8DE, 0xFE1DC127, 0xEFE75928, 0xA14B5E77, 0x6B4D3DBA, 0xF828AF60, 0x053FB521, 0x9C648139, 0x2395B442, 0x9E3ECB66, 0x0404E9CD, 0x858E06B7, 0x000000C6};
unsigned int secp521r1_Gy[17] = {0x9FD16650, 0x88BE9476, 0xA272C240, 0x353C7086, 0x3FAD0761, 0xC550B901, 0x5EF42640, 0x97EE7299, 0x273E662C, 0x17AFBD17, 0x579B4468, 0x98F54449, 0x2C7D1BD9, 0x5C8A5FB4, 0x9A3BC004, 0x39296A78, 0x00000118};
unsigned int secp521r1_n[17]  = {0x91386409, 0xBB6FB71E, 0x899C47AE, 0x3BB5C9B8, 0xF709A5D0, 0x7FCC0148, 0xBF2F966B, 0x51868783, 0xFFFFFFFA, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp521r1_n_h[17] = {0x60EC0915, 0xABEE0E49, 0xA4DEB96A, 0x39B88F86, 0x6440173A, 0x36439BEA, 0x53EBDD25, 0xABED7C82, 0x7A1814F8, 0xDB25B111, 0xB2DD5C97, 0xE290362F, 0x158143E2, 0xA9EA49FF, 0x141CEAE0, 0x07E35810, 0x000001CD};
    #else
unsigned int secp521r1_n_h[17] = {0x61C64CA7, 0x1163115A, 0x4374A642, 0x18354A56, 0x0791D9DC, 0x5D4DD6D3, 0xD3402705, 0x4FB35B72, 0xB7756E3A, 0xCFF3D142, 0xA8E567BC, 0x5BCC6D61, 0x492D0D45, 0x2D8E03D1, 0x8C44383D, 0x5B5A3AFE, 0x0000019A};
unsigned int secp521r1_n_n0[1] = {0x79A995C7};
    #endif

    //[2^260]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp521r1_2_260_Gx[17] = {0x9185544D, 0x6D9B0C3C, 0x8DF2765F, 0xAD21890E, 0xCBE030A2, 0x47836EE3, 0xF7651AED, 0x606B9133, 0x71C00932, 0xB1A31586, 0xCFE05F47, 0x9806A369, 0xF57F3700, 0xC2EBC613, 0xF065F07C, 0x1022D6D2, 0x00000109};
unsigned int secp521r1_2_260_Gy[17] = {0x514C45ED, 0xB292C583, 0x947E68A1, 0x89AC5BF2, 0xAF507C14, 0x633C4300, 0x7DA4020A, 0x943D7BA5, 0xC0ED8274, 0xD1E90C7A, 0xE59426E6, 0x9634868C, 0xC26BC9DE, 0x24A6FFF2, 0x152416CD, 0x1A012168, 0x0000000C};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp521r1[1] = {
    {
     521u,
     521u,
     (unsigned int *)secp521r1_p,
     (unsigned int *)secp521r1_p_h, //NULL,//
        (unsigned int *)secp521r1_a,
     (unsigned int *)secp521r1_b,
     (unsigned int *)secp521r1_Gx,
     (unsigned int *)secp521r1_Gy,
     (unsigned int *)secp521r1_n,
     (unsigned int *)secp521r1_n_h,
     (unsigned int *)secp521r1_2_260_Gx,
     (unsigned int *)secp521r1_2_260_Gy,
     },
};
    #else
eccp_curve_t secp521r1[1] = {
    {
     521u,
     521u,
     secp521r1_p,
     secp521r1_p_h,  //NULL,//
        secp521r1_p_n0, //NULL,//
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
unsigned int secp160k1_p[5]   = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160k1_p_h[5] = {0x00000000, 0x4DB32715, 0x51CE3BE1, 0x0000FAA7, 0x00000001};
    #elif (defined(PKE_LP))
unsigned int secp160k1_p[6]    = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
unsigned int secp160k1_p_h[6]  = {0x00000000, 0x00000000, 0x1B44BBA9, 0x0000A71A, 0x00000001, 0x00000000};
unsigned int secp160k1_p_n0[1] = {0xB4AB2745};
    #elif (defined(PKE_SECURE))
unsigned int secp160k1_p[5]    = {0xFFFFAC73, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp160k1_p_h[5]  = {0x1B44BBA9, 0x0000A71A, 0x00000001, 0x00000000, 0x00000000};
unsigned int secp160k1_p_n0[1] = {0xB4AB2745};
    #endif
unsigned int secp160k1_a[5]  = {0, 0, 0, 0, 0};
unsigned int secp160k1_b[5]  = {7, 0, 0, 0, 0};
unsigned int secp160k1_Gx[5] = {0xDD4D7EBB, 0x3036F4F5, 0xA4019E76, 0xE37AA192, 0x3B4C382C};
unsigned int secp160k1_Gy[5] = {0xF03C4FEE, 0x531733C3, 0x6BC28286, 0x318FDCED, 0x938CF935};
unsigned int secp160k1_n[6]  = {0xCA16B6B3, 0x16DFAB9A, 0x0001B8FA, 0x00000000, 0x00000000, 0x00000001};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160k1_n_h[6] = {0x705ACEF9, 0xC89631C2, 0x14AB01D8, 0x2FD0CDC7, 0xFEFCCD13, 0x00000000};
    #else
unsigned int secp160k1_n_h[6]  = {0x0E687AAF, 0x0F849433, 0x4D8A8AAD, 0xDFE35D2F, 0xCDCF2BAB, 0x00000000};
unsigned int secp160k1_n_n0[1] = {0x35931785};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp160k1_2_80_Gx[5] = {0xE398219C, 0xCBFF95F1, 0x47798032, 0x6B450CC3, 0x4B84DE5E};
unsigned int secp160k1_2_80_Gy[5] = {0xA8E3C651, 0x88FEB8C8, 0x65923367, 0xC00FA6FA, 0x93F7C1DD};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp160k1[1] = {
    {
     160u,
     161u,
     (unsigned int *)secp160k1_p,
     (unsigned int *)secp160k1_p_h,
     (unsigned int *)secp160k1_a,
     (unsigned int *)secp160k1_b,
     (unsigned int *)secp160k1_Gx,
     (unsigned int *)secp160k1_Gy,
     (unsigned int *)secp160k1_n,
     (unsigned int *)secp160k1_n_h,
     (unsigned int *)secp160k1_2_80_Gx,
     (unsigned int *)secp160k1_2_80_Gy,
     },
};
    #else
eccp_curve_t secp160k1[1] = {
    {
     160u,
     161u,
     secp160k1_p,
     secp160k1_p_h,  //NULL,//
        secp160k1_p_n0, //NULL,//
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
unsigned int secp192k1_p[6] = {0xFFFFEE37, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192k1_p_h[6] = {0x000011C9, 0x00000001, 0x00000000, 0x00000000, 0x013C4FD1, 0x00002392};
    #else
unsigned int secp192k1_p_h[6]  = {0x013C4FD1, 0x00002392, 0x00000001, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp192k1_p_n0[1] = {0x7446D879};
    #endif
unsigned int secp192k1_a[6]  = {0, 0, 0, 0, 0, 0};
unsigned int secp192k1_b[6]  = {3, 0, 0, 0, 0, 0};
unsigned int secp192k1_Gx[6] = {0xEAE06C7D, 0x1DA5D1B1, 0x80B7F434, 0x26B07D02, 0xC057E9AE, 0xDB4FF10E};
unsigned int secp192k1_Gy[6] = {0xD95E2F9D, 0x4082AA88, 0x15BE8634, 0x844163D0, 0x9C5628A7, 0x9B2F2F6D};
unsigned int secp192k1_n[6]  = {0x74DEFD8D, 0x0F69466A, 0x26F2FC17, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192k1_n_h[6] = {0x87AE967C, 0xBACF0434, 0x150F5CF8, 0x17BBAD83, 0xBB194E8A, 0x93FB81A6};
    #else
unsigned int secp192k1_n_h[6]  = {0x250F0702, 0x461C1989, 0x195E97E2, 0xF0F4F172, 0x2EC4B2B1, 0x6A21191C};
unsigned int secp192k1_n_n0[1] = {0x560472BB};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp192k1_2_96_Gx[6] = {0x3C3803C1, 0x4A0A43C8, 0xA6FD6C4A, 0x119C7C2B, 0xF3B65531, 0x79C8D6B2};
unsigned int secp192k1_2_96_Gy[6] = {0xB7488B1E, 0x3962A2AB, 0x2EFDCE4C, 0x71A2E4D2, 0xE3A1306E, 0xEB64375B};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp192k1[1] =
    {
        {
         192u,
         192u,
         (unsigned int *)secp192k1_p,
         (unsigned int *)secp192k1_p_h,
         (unsigned int *)secp192k1_a,
         (unsigned int *)secp192k1_b,
         (unsigned int *)secp192k1_Gx,
         (unsigned int *)secp192k1_Gy,
         (unsigned int *)secp192k1_n,
         (unsigned int *)secp192k1_n_h,
         (unsigned int *)secp192k1_2_96_Gx,
         (unsigned int *)secp192k1_2_96_Gy,
         }
};
    #else
eccp_curve_t secp192k1[1] = {
    {
     192u,
     192u,
     secp192k1_p,
     secp192k1_p_h,  //NULL,//
        secp192k1_p_n0, //NULL,//
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
unsigned int secp224k1_p[7]   = {0xFFFFE56D, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp224k1_p_h[7] = {0x00000000, 0x00000000, 0x02C23069, 0x00003526, 0x00000001, 0x00000000, 0x00000000};
    #elif (defined(PKE_LP))
unsigned int secp224k1_p[8]    = {0xFFFFE56D, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};
unsigned int secp224k1_p_h[8]  = {0x00000000, 0x00000000, 0x02C23069, 0x00003526, 0x00000001, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp224k1_p_n0[1] = {0x198D139B};
    #elif (defined(PKE_SECURE))
unsigned int secp224k1_p[7]    = {0xFFFFE56D, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp224k1_p_h[7]  = {0x02C23069, 0x00003526, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
unsigned int secp224k1_p_n0[1] = {0x198D139B};
    #endif
unsigned int secp224k1_a[7]   = {0, 0, 0, 0, 0, 0, 0};
unsigned int secp224k1_b[7]   = {5, 0, 0, 0, 0, 0, 0};
unsigned int secp224k1_Gx[7]  = {0xB6B7A45C, 0x0F7E650E, 0xE47075A9, 0x69A467E9, 0x30FC28A1, 0x4DF099DF, 0xA1455B33};
unsigned int secp224k1_Gy[7]  = {0x556D61A5, 0xE2CA4BDB, 0xC0B0BD59, 0xF7E319F7, 0x82CAFBD6, 0x7FBA3442, 0x7E089FED};
unsigned int secp224k1_n[8]   = {0x769FB1F7, 0xCAF0A971, 0xD2EC6184, 0x0001DCE8, 0x00000000, 0x00000000, 0x00000000, 0x00000001};
unsigned int secp224k1_n_h[8] = {0xEC9FEAA0, 0x34CE24FB, 0x16F60AF5, 0x8BE03208, 0xBBFF32E4, 0xB882BD88, 0x993FF72B, 0x00000000};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int secp224k1_n_n0[1] = {0x44C1A039};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp224k1_2_112_Gx[7] = {0x26A0F6BF, 0x6958D26B, 0x2EE2C11B, 0x15E7E8DF, 0xFCF05755, 0x384E3EB5, 0x6E88F7FF};
unsigned int secp224k1_2_112_Gy[7] = {0xAE2CD1EF, 0xBFF31064, 0xBDA4B1A1, 0x9638338C, 0x67417F72, 0xB7F8FA47, 0x40AAFF87};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp224k1[1] =
    {
        {
         224u,
         225u,
         (unsigned int *)secp224k1_p,
         (unsigned int *)secp224k1_p_h,
         (unsigned int *)secp224k1_a,
         (unsigned int *)secp224k1_b,
         (unsigned int *)secp224k1_Gx,
         (unsigned int *)secp224k1_Gy,
         (unsigned int *)secp224k1_n,
         (unsigned int *)secp224k1_n_h,
         (unsigned int *)secp224k1_2_112_Gx,
         (unsigned int *)secp224k1_2_112_Gy,
         }
};
    #else
eccp_curve_t secp224k1[1] = {
    {
     224u,
     225u,
     secp224k1_p,
     secp224k1_p_h,  //NULL,//
        secp224k1_p_n0, //NULL,//
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
unsigned int secp256k1_p[8]   = {0xFFFFFC2F, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp256k1_p_h[8] = {0x000E90A1, 0x000007A2, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int secp256k1_p_n0[1] = {0xD2253531};
    #endif
unsigned int secp256k1_a[8]   = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int secp256k1_b[8]   = {7, 0, 0, 0, 0, 0, 0, 0};
unsigned int secp256k1_Gx[8]  = {0x16F81798, 0x59F2815B, 0x2DCE28D9, 0x029BFCDB, 0xCE870B07, 0x55A06295, 0xF9DCBBAC, 0x79BE667E};
unsigned int secp256k1_Gy[8]  = {0xFB10D4B8, 0x9C47D08F, 0xA6855419, 0xFD17B448, 0x0E1108A8, 0x5DA4FBFC, 0x26A3C465, 0x483ADA77};
unsigned int secp256k1_n[8]   = {0xD0364141, 0xBFD25E8C, 0xAF48A03B, 0xBAAEDCE6, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
unsigned int secp256k1_n_h[8] = {0x67D7D140, 0x896CF214, 0x0E7CF878, 0x741496C2, 0x5BCD07C6, 0xE697F5E4, 0x81C69BC5, 0x9D671CD5};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int secp256k1_n_n0[1] = {0x5588B13F};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int secp256k1_2_128_Gx[8] = {0x9EC4C0DA, 0x1B7B444C, 0x723EA335, 0xE88C5678, 0x981F162E, 0x9239C1AD, 0xF63B5F33, 0x8F68B9D2};
unsigned int secp256k1_2_128_Gy[8] = {0x501FFF82, 0xF23CBF79, 0x95510BFD, 0xBBEA2CFE, 0xB6BE215D, 0xDE1D90C2, 0xBA063986, 0x662A9F2D};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t secp256k1[1] =
    {
        {
         256u,
         256u,
         (unsigned int *)secp256k1_p,
         (unsigned int *)secp256k1_p_h,
         (unsigned int *)secp256k1_a,
         (unsigned int *)secp256k1_b,
         (unsigned int *)secp256k1_Gx,
         (unsigned int *)secp256k1_Gy,
         (unsigned int *)secp256k1_n,
         (unsigned int *)secp256k1_n_h,
         (unsigned int *)secp256k1_2_128_Gx,
         (unsigned int *)secp256k1_2_128_Gy,
         }
};
    #else
eccp_curve_t secp256k1[1] = {
    {
     256u,
     256u,
     secp256k1_p,
     secp256k1_p_h,  //NULL,//
        secp256k1_p_n0, //NULL,//
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
unsigned int bn256_p[8]   = {0xAED33013, 0xD3292DDB, 0x12980A82, 0x0CDC65FB, 0xEE71A49F, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
unsigned int bn256_p_h[8] = {0x1092B98F, 0xFAC8C610, 0xD7F91154, 0xDB90D49C, 0x32BF3141, 0x4F325FC7, 0x0E56A005, 0x4DE578EA};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int bn256_p_n0[1] = {0x0537E5E5};
    #endif
unsigned int bn256_a[8]   = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int bn256_b[8]   = {3, 0, 0, 0, 0, 0, 0, 0};
unsigned int bn256_Gx[8]  = {1, 0, 0, 0, 0, 0, 0, 0};
unsigned int bn256_Gy[8]  = {2, 0, 0, 0, 0, 0, 0, 0};
unsigned int bn256_n[8]   = {0xD10B500D, 0xF62D536C, 0x1299921A, 0x0CDC65FB, 0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD, 0xFFFFFFFF};
unsigned int bn256_n_h[8] = {0x8F4C4808, 0xAF948AA3, 0x26123232, 0xBD789EFD, 0xEB526BE7, 0x117FD17C, 0xFB8F407A, 0x2BFC4998};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int bn256_n_n0[1] = {0xC9C6813B};
    #endif

    //[2^128]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int bn256_2_128_Gx[8] = {0x5E06FE34, 0xECBD3164, 0xCE4EAA4F, 0x35B40F42, 0x5E1339BA, 0x2CB9923A, 0xA165554D, 0x3F80B083};
unsigned int bn256_2_128_Gy[8] = {0x8B965F4A, 0x6B5F5BF5, 0x7334F693, 0xD77BFA60, 0x70406125, 0xE49F4BD8, 0x84F6A594, 0x2268E7FB};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t bn256[1] = {
    {
     256u,
     256u,
     (unsigned int *)bn256_p,
     (unsigned int *)bn256_p_h, //NULL, //
        (unsigned int *)bn256_a,
     (unsigned int *)bn256_b,
     (unsigned int *)bn256_Gx,
     (unsigned int *)bn256_Gy,
     (unsigned int *)bn256_n,
     (unsigned int *)bn256_n_h, //NULL, //
        (unsigned int *)bn256_2_128_Gx,
     (unsigned int *)bn256_2_128_Gy,
     },
};
    #else
eccp_curve_t bn256[1] = {
    {
     256u,
     256u,
     bn256_p,
     bn256_p_h,  //NULL, //
        bn256_p_n0, //NULL, //
        bn256_a,
     bn256_b,
     bn256_Gx,
     bn256_Gy,
     bn256_n,
     bn256_n_h,  //NULL, //
        bn256_n_n0, //NULL, //
    },
};
    #endif
#endif


/**************************** BN638 ******************************/
#ifdef SUPPORT_BN638
unsigned int bn638_p[20] = {
    0x00000067,
    0x00000000,
    0xFFFFECE0,
    0xFFFFFFFF,
    0x80015ACD,
    0x0000004C,
    0xFFF4EB80,
    0xFFFFF51F,
    0x0021E55B,
    0xC0008652,
    0x0008DE55,
    0xFFFDD0E0,
    0x0000D52F,
    0x3FFF9487,
    0xD000165E,
    0xFFFFF942,
    0x000001D3,
    0x7FFFFFB8,
    0xC000000D,
    0x23FFFFFD,
};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int bn638_p_h[20] = {0xDFD2A6C5, 0x668E29B3, 0x8FDFA6A7, 0x2968BE60, 0x041A17D7, 0x13F3D015, 0x0F669718, 0xE1C52740, 0xC8A102DC, 0x046324EB, 0x40E86FEA, 0x23F7E0BD, 0x8AC0A2E4, 0x6D5D613E, 0xD0E98766, 0xB853FF78, 0x27596A54, 0x1A09328C, 0x6FEC09DD, 0x040681E4};
    #else
unsigned int bn638_p_h[20] = {0x2B2F5227, 0xA19430BF, 0xB48B9102, 0xF598D8A4, 0x68EEEE4C, 0x6004E38B, 0x2C437647, 0x3A2F454B, 0xFDD8EA75, 0x612285E3, 0x43B93508, 0x697F87A2, 0x9080FE0F, 0xBC5C481B, 0x958F4EB0, 0x981685EC, 0x9F98DA83, 0x2437C11D, 0x5B1DA7BE, 0x0BD442FA};
unsigned int bn638_p_n0[1] = {0x2CBCE4A9};
    #endif
unsigned int bn638_a[20] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
unsigned int bn638_b[20] = {
    0x00000101U,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
unsigned int bn638_Gx[20] = {
    0x00000066,
    0x00000000,
    0xFFFFECE0,
    0xFFFFFFFF,
    0x80015ACD,
    0x0000004C,
    0xFFF4EB80,
    0xFFFFF51F,
    0x0021E55B,
    0xC0008652,
    0x0008DE55,
    0xFFFDD0E0,
    0x0000D52F,
    0x3FFF9487,
    0xD000165E,
    0xFFFFF942,
    0x000001D3,
    0x7FFFFFB8,
    0xC000000D,
    0x23FFFFFD,
};
unsigned int bn638_Gy[20] = {
    0x00000010U,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
unsigned int bn638_n[20] = {
    0x00000061,
    0x00000000,
    0xFFFFEDA0,
    0xFFFFFFFF,
    0x800154D9,
    0x00000049,
    0xFFF4EAC0,
    0xFFFFF54F,
    0x0021E555,
    0x60008655,
    0x0008DE55,
    0xFFFDD0E0,
    0x0000D52F,
    0x3FFF9487,
    0xD000165E,
    0xFFFFF942,
    0x000001D3,
    0x7FFFFFB8,
    0xC000000D,
    0x23FFFFFD,
};
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int bn638_n_h[20] = {0xE384A09F, 0x808AA3AB, 0x7EB1E973, 0x6994B3DF, 0x90FA7D82, 0x98F5B679, 0x115C5909, 0x93452FE5, 0x8D92072D, 0x3B6FF122, 0x167C27CB, 0x294483E9, 0x488BAF5D, 0xCF475845, 0x4D35F74A, 0xA82F3060, 0x43E9F211, 0xE6E1CAC8, 0x4D06397A, 0x11BFA298};
    #else
unsigned int bn638_n_h[20] = {0x54101DC4, 0x3520F4DD, 0x729F3A43, 0x26CDB684, 0x2080C8E0, 0x146F0E77, 0xD7DECA81, 0xD8957773, 0x3874C9BB, 0x533C4EC2, 0x41C0ED0E, 0x0A6F3E0A, 0xE1EF5E67, 0x0D257A80, 0x9283EFA8, 0x7D50C3E9, 0xF49731ED, 0x7CBE9A47, 0xC86631A3, 0x0CB898F5};
unsigned int bn638_n_n0[1] = {0xA0FD5C5F};
    #endif

    //[2^260]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int bn638_2_319_Gx[20] = {0xB5AF644E, 0x0494F533, 0x9D832CDD, 0x3FBB675F, 0x884BC489, 0x067A81BC, 0x620BD123, 0x4DC263C8, 0x86610AE9, 0x0D49AE84, 0xC3353780, 0x341CD7CB, 0x7D0FE4F6, 0x16475144, 0x8AA5BFF4, 0x1E1CC061, 0x681BC09B, 0xCEA15338, 0x923212C6, 0x0A378785};
unsigned int bn638_2_319_Gy[20] = {0xA9ECB70B, 0x43004DDA, 0x7DEFAB17, 0xAB2392A5, 0x1E224FB2, 0xE870977A, 0x609FDB32, 0xAAEE8A91, 0xBFA96F6D, 0x0137465B, 0x6E771BA8, 0x03478023, 0xD7CCC3C3, 0xD76DEFE7, 0xD55C8329, 0xDE3DEEA7, 0xCA50F0C8, 0xB396C977, 0x17FDF2F3, 0x1B0C448C};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t bn638[1] = {
    {
     638u,
     638u,
     (unsigned int *)bn638_p,
     (unsigned int *)bn638_p_h, //NULL,//
        (unsigned int *)bn638_a,
     (unsigned int *)bn638_b,
     (unsigned int *)bn638_Gx,
     (unsigned int *)bn638_Gy,
     (unsigned int *)bn638_n,
     (unsigned int *)bn638_n_h,
     (unsigned int *)bn638_2_319_Gx,
     (unsigned int *)bn638_2_319_Gy,
     },
};
    #else
eccp_curve_t bn638[1] = {
    {
     638u,
     638u,
     (unsigned int *)bn638_p,
     (unsigned int *)bn638_p_h,  //NULL,//
        (unsigned int *)bn638_p_n0, //NULL,//
        (unsigned int *)bn638_a,
     (unsigned int *)bn638_b,
     (unsigned int *)bn638_Gx,
     (unsigned int *)bn638_Gy,
     (unsigned int *)bn638_n,
     (unsigned int *)bn638_n_h,
     (unsigned int *)bn638_n_n0,
     },
};
    #endif
#endif


/**************************** ANDERS_1024_1 ******************************/
#ifdef SUPPORT_ANDERS_1024_1
unsigned int anders_1024_1_p[32] = {
    0x00000007,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0xfa000000,
};
unsigned int anders_1024_1_p_h[32] = {
    0x78D4FE2D,
    0x083126E9,
    0x645A1CAC,
    0x8D4FDF3B,
    0x83126E97,
    0x45A1CAC0,
    0xD4FDF3B6,
    0x3126E978,
    0x5A1CAC08,
    0x4FDF3B64,
    0x126E978D,
    0xA1CAC083,
    0xFDF3B645,
    0x26E978D4,
    0x1CAC0831,
    0xDF3B645A,
    0x6E978D4F,
    0xCAC08312,
    0xF3B645A1,
    0xE978D4FD,
    0xAC083126,
    0x3B645A1C,
    0x978D4FDF,
    0xC083126E,
    0xB645A1CA,
    0x78D4FDF3,
    0x083126E9,
    0x645A1CAC,
    0x8D4FDF3B,
    0x83126E97,
    0x45A1CAC0,
    0xD2FDF3B6,
};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int anders_1024_1_p_n0[1] = {
    0x49249249,
};
    #endif
unsigned int anders_1024_1_a[32] = {
    0x00000004,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0xfa000000,
};
unsigned int anders_1024_1_b[32] = {
    0x00000005,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x0A11CE00,
    0x00000000,
    0x00000000,
    0x00000000,
    0xDB5E2C40,
    0xD1E8C429,
    0xF9ABD2DF,
    0xC4F0EDBE,
    0x4B016D9E,
    0x6EE8FC8E,
    0x7A807026,
    0x672C90D2,
    0x0000000D,
    0x0B000000,
    0x0000000B,
    0xFE000000,
    0x5FF25A5A,
    0xF1EA5FF1,
    0xC001FFFF,
    0xA5FF15FF,
    0x4FFFFF1E,
    0xF15FFBE5,
    0xFFF1EA5F,
    0xFF900DFF,
    0xFAE5FF15,
    0xFDEADFFF,
    0xDE5FF15F,
};
unsigned int anders_1024_1_Gx[32] = {
    0x00000000,
    0x00000000,
    0x00000000,
    0xA2D00B0B,
    0x0A11CE00,
    0xF00D0040,
    0xEEF15BAD,
    0x000DEADB,
    0x5AFE0000,
    0xF15FF25A,
    0xFFF1EA5F,
    0xFFC001FF,
    0x1EA5FF15,
    0xE54FFFFF,
    0x5FF15FFB,
    0xFFFFF1EA,
    0x15FF900D,
    0xFFFAE5FF,
    0x5FFDEADF,
    0x00DE5FF1,
    0xAFE00000,
    0x15FF25A5,
    0xFF1EA5FF,
    0xFC001FFF,
    0xEA5FF15F,
    0x54FFFFF1,
    0xFF15FFBE,
    0xFFFF1EA5,
    0x5FF900DF,
    0xFFAE5FF1,
    0xFFDEADFF,
    0x0DE5FF15,
};
unsigned int anders_1024_1_Gy[32] = {
    0x4BB53650,
    0xEE26A78B,
    0x618B85ED,
    0xA49A17BC,
    0x4D5BE050,
    0x82A87C58,
    0x12C3112A,
    0x3AA2F52C,
    0x00C2A9AD,
    0xF17AD7F9,
    0xF1622E4D,
    0x614075FF,
    0x833352EF,
    0x90273C6A,
    0x64BA37FB,
    0x0E59FC88,
    0xF3C2894D,
    0xF454BC74,
    0x437DF946,
    0x8A569D2A,
    0xC60E05A1,
    0x92FB6C84,
    0x6931D82F,
    0x2A5FF6A8,
    0x5BBC35D6,
    0x410B1797,
    0x724A73B2,
    0xD43ACAE9,
    0xD3C06B0A,
    0xBE9BD4E4,
    0x39E17B5E,
    0x19F05369,
};
unsigned int anders_1024_1_n[32] = {
    0xA97F2707,
    0x07AC5CAA,
    0x79EA4BFA,
    0x527589B9,
    0xE569E1D3,
    0x2530F353,
    0x79921D61,
    0x8A6E3BB5,
    0xB9DE0DF5,
    0x8B50780F,
    0x38D5EBC5,
    0xFC8E9F74,
    0x3D5C7FB9,
    0x2665E576,
    0x0C03CABD,
    0xC6962C2D,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0xFA000000,
};
unsigned int anders_1024_1_n_h[32] = {
    0xF7BC108A,
    0x7D4D40C4,
    0x5FE418FB,
    0xA8134474,
    0xD93DAAAD,
    0x8D8B907B,
    0x5D7D89B9,
    0x08077B5F,
    0x3B44032B,
    0x48F0BD36,
    0x540E9BDD,
    0x921E1994,
    0x1CB0BFB2,
    0xA7F0FB61,
    0x9EC27B80,
    0x09DE53B1,
    0x586EEF75,
    0xF3DCAB7C,
    0x5D81062D,
    0x833ABB77,
    0x6F3C3428,
    0x302405A4,
    0x2517E0AC,
    0x913E10EE,
    0x3F8E2BCC,
    0x729929A8,
    0x60000D2E,
    0xCB226631,
    0x0A6D792C,
    0x3D58CE4D,
    0x5179B581,
    0x6784387B,
};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int anders_1024_1_n_n0[1] = {
    0x53646949,
};
    #endif

    //[2^512]G
    #if (defined(PKE_HP) || defined(PKE_UHP))
unsigned int anders_1024_1_2_512_Gx[32] = {
    0x9400C2E7,
    0x435CD89B,
    0x8D5A9A2E,
    0xD1E9DE98,
    0x4A11E433,
    0xB6EB6674,
    0xA4F7B02E,
    0x0EA107A0,
    0x5F907582,
    0x9B757345,
    0x00CF492C,
    0x2B921CF0,
    0x7D3578C9,
    0x776FA500,
    0x1B4A4CE4,
    0xCF8746DE,
    0x2CA31E1C,
    0x18AB676D,
    0x51526524,
    0x7015BE82,
    0x4C727803,
    0x73FFE480,
    0xE2BB2791,
    0x026A600A,
    0x1F55B3B3,
    0xEE5865E8,
    0xFACD6A6A,
    0x2527DDF2,
    0xB900FD08,
    0xFDADE5A3,
    0x579B6A46,
    0x56A76889,
};
unsigned int anders_1024_1_2_512_Gy[32] = {
    0xAD0BF6F4,
    0x25049053,
    0x5FEF7098,
    0x27761912,
    0x3D58826D,
    0xC2D98116,
    0x1A6F9B57,
    0x0E800978,
    0xCB0EDF06,
    0x8E038C49,
    0x0D9FE57A,
    0x1A331245,
    0x29CABA19,
    0xA148A8B5,
    0x297AD068,
    0x426B8751,
    0x4CACB65D,
    0x7C5FD75F,
    0x2BC6BE90,
    0x112A480A,
    0x46780139,
    0xB6EAC362,
    0x662BF7CB,
    0x184AE477,
    0xA8CCB946,
    0xCDEEE45D,
    0xCD29D7F6,
    0xD34D6661,
    0xF4C14DE5,
    0x2F0A5321,
    0x11DBD27B,
    0x003F04FF,
};
    #endif

    #if (defined(PKE_HP) || defined(PKE_UHP))
eccp_curve_t anders_1024_1[1] = {
    {
     1024u,
     1024u,
     (unsigned int *)anders_1024_1_p,
     (unsigned int *)anders_1024_1_p_h, //NULL,//
        (unsigned int *)anders_1024_1_a,
     (unsigned int *)anders_1024_1_b,
     (unsigned int *)anders_1024_1_Gx,
     (unsigned int *)anders_1024_1_Gy,
     (unsigned int *)anders_1024_1_n,
     (unsigned int *)anders_1024_1_n_h,
     (unsigned int *)anders_1024_1_2_512_Gx,
     (unsigned int *)anders_1024_1_2_512_Gy,
     },
};
    #else
eccp_curve_t anders_1024_1[1] = {
    {
     1024u,
     1024u,
     (unsigned int *)anders_1024_1_p,
     (unsigned int *)anders_1024_1_p_h,  //NULL,//
        (unsigned int *)anders_1024_1_p_n0, //NULL,//
        (unsigned int *)anders_1024_1_a,
     (unsigned int *)anders_1024_1_b,
     (unsigned int *)anders_1024_1_Gx,
     (unsigned int *)anders_1024_1_Gy,
     (unsigned int *)anders_1024_1_n,
     (unsigned int *)anders_1024_1_n_h,
     (unsigned int *)anders_1024_1_n_n0,
     },
};
    #endif
#endif
