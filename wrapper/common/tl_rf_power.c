/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include "tl_rf_power.h"
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
 #include <rf.h>
#elif CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
#include <rf_common.h>
#endif

#if CONFIG_SOC_RISCV_TELINK_B91
/* TX power B91 lookup table */
const uint8_t tl_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -25 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -24 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -23 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -22 */
	RF_POWER_N23p54dBm,     /* -23.5 dBm: -21 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -20 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -19 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -18 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -17 */
	RF_POWER_N17p83dBm,     /* -17.8 dBm: -16 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -15 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -14 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -13 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -12 */
	RF_POWER_N12p06dBm,     /* -12.0 dBm: -11 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm: -10 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -9 */
	RF_POWER_N8p78dBm,      /*  -8.7 dBm:  -8 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -7 */
	RF_POWER_N6p54dBm,      /*  -6.5 dBm:  -6 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -5 */
	RF_POWER_N4p77dBm,      /*  -4.7 dBm:  -4 */
	RF_POWER_N3p37dBm,      /*  -3.3 dBm:  -3 */
	RF_POWER_N2p01dBm,      /*  -2.0 dBm:  -2 */
	RF_POWER_N1p37dBm,      /*  -1.3 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P0p80dBm,      /*   0.8 dBm:   1 */
	RF_POWER_P2p32dBm,      /*   2.3 dBm:   2 */
	RF_POWER_P3p25dBm,      /*   3.2 dBm:   3 */
	RF_POWER_P4p35dBm,      /*   4.3 dBm:   4 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   5 */
	RF_POWER_P5p68dBm,      /*   5.6 dBm:   6 */
	RF_POWER_P6p98dBm,      /*   6.9 dBm:   7 */
	RF_POWER_P8p05dBm,      /*   8.0 dBm:   8 */
	RF_POWER_P9p11dBm,      /*   9.1 dBm:   9 */
};

#elif CONFIG_SOC_RISCV_TELINK_B92
/* TX power B92 lookup table */
const uint8_t tl_tx_pwr_lt[] = {
	RF_POWER_N30dBm,        /* -30.0 dBm: -30 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -29 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -28 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -27 */
	RF_POWER_N30dBm,        /* -30.0 dBm: -26 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -25 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -24 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -23 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -22 */
	RF_POWER_N22p53dBm,     /* -22.5 dBm: -21 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -20 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -19 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -18 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -17 */
	RF_POWER_N13p42dBm,     /* -13.5 dBm: -16 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -15 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -14 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -13 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -12 */
	RF_POWER_N9p03dBm,      /* -9.0 dBm: -11 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm: -10 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -9 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -8 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -7 */
	RF_POWER_N5p94dBm,      /*  -5.9 dBm:  -6 */
	RF_POWER_N3p95dBm,      /*  -4.0 dBm:  -5 */
	RF_POWER_N3p95dBm,      /*  -4.0 dBm:  -4 */
	RF_POWER_N2p51dBm,      /*  -2.5 dBm:  -3 */
	RF_POWER_N1p52dBm,      /*  -1.5 dBm:  -2 */
	RF_POWER_N0p43dBm,      /*  -0.5 dBm:  -1 */
	RF_POWER_P0p01dBm,      /*   0.0 dBm:   0 */
	RF_POWER_P1p03dBm,      /*   1.0 dBm:   1 */
	RF_POWER_P1p62dBm,      /*   1.6 dBm:   2 */
	RF_POWER_P2p51dBm,      /*   2.5 dBm:   3 */
	RF_POWER_P3p51dBm,      /*   3.5 dBm:   4 */
	RF_POWER_P4p61dBm,      /*   4.6 dBm:   5 */
	RF_POWER_P5p21dBm,      /*   5.2 dBm:   6 */
	RF_POWER_P7p00dBm,      /*   7.0 dBm:   7 */
	RF_POWER_P8p25dBm,      /*   8.2 dBm:   8 */
	RF_POWER_P9p90dBm,      /*   9.9 dBm:   9 */
};

#elif CONFIG_SOC_RISCV_TELINK_TL321X
/* TX power TL321X lookup table */
const uint8_t tl_tx_pwr_lt[] = {
	/*VANT*/
	RF_POWER_N20p01dBm,   /**< -20.0 dbm: -20 */
	RF_POWER_N20p01dBm,   /**< -20.0 dbm: -19 */
	RF_POWER_N20p01dBm,   /**< -20.0 dbm: -18 */
	RF_POWER_N20p01dBm,   /**< -20.0 dbm: -17 */
	RF_POWER_N14p63dBm,   /**< -14.6 dbm: -16 */
	RF_POWER_N14p63dBm,   /**< -14.6 dbm: -15 */
	RF_POWER_N14p63dBm,   /**< -14.6 dbm: -14 */
	RF_POWER_N12p66dBm,   /**< -12.6 dbm: -13 */
	RF_POWER_N12p66dBm,   /**< -12.6 dbm: -12 */
	RF_POWER_N11p28dBm,   /**< -11.3 dbm: -11 */
	RF_POWER_N9p92dBm,    /**<  -9.9 dbm: -10 */
	RF_POWER_N8p88dBm,    /**<  -8.8 dbm: -9  */
	RF_POWER_N7p86dBm,    /**<  -7.8 dbm: -8  */
	RF_POWER_N7p86dBm,    /**<  -7.8 dbm: -7  */
	RF_POWER_N6p25dBm,    /**<  -6.2 dbm: -6  */
	RF_POWER_N5p58dBm,    /**<  -5.5 dbm: -5  */
	RF_POWER_N4p36dBm,    /**<  -4.3 dbm: -4  */
	RF_POWER_N3p37dBm,    /**<  -3.3 dbm: -3  */
	RF_POWER_N2p00dBm,    /**<  -2.0 dbm: -2  */
	RF_POWER_N1p22dBm,    /**<  -1.2 dbm: -1  */
	RF_POWER_P0p08dBm,    /**<   0.0 dbm:  0  */
	RF_POWER_P1p19dBm,    /**<   1.2 dbm:  1  */
	RF_POWER_P2p03dBm,    /**<   2.0 dbm:  2  */
	RF_POWER_P3p03dBm,    /**<   3.0 dbm:  3  */
	RF_POWER_P4p13dBm,    /**<   4.1 dbm:  4  */
	RF_POWER_P5p00dBm,    /**<   5.0 dbm:  5  */
	/*VBAT*/
	RF_POWER_P6p05dBm,    /**<   6.0 dbm:  6  */
	RF_POWER_P6p97dBm,    /**<   7.0 dbm:  7  */
	RF_POWER_P8p03dBm,    /**<   8.0 dbm:  8  */
	RF_POWER_P9p10dBm,    /**<   9.1 dbm:  9  */
	RF_POWER_P10p00dBm,    /**<  10.0 dbm:  10 */
	RF_POWER_P11p16dBm,   /**<  11.1 dbm:  11 */
};


#elif CONFIG_SOC_RISCV_TELINK_TL721X
/* TX power TL721X lookup table */
const uint8_t tl_tx_pwr_lt[] = {
	/*VANT*/
	RF_POWER_N20p22dBm,   /**< -20.2 dbm: -20 */
	RF_POWER_N20p22dBm,   /**< -20.2 dbm: -19 */
	RF_POWER_N20p22dBm,   /**< -20.2 dbm: -18 */
	RF_POWER_N16p82dBm,   /**< -16.8 dbm: -17 */
	RF_POWER_N16p82dBm,   /**< -16.8 dbm: -16 */
	RF_POWER_N14p64dBm,   /**< -14.6 dbm: -15 */
	RF_POWER_N14p64dBm,   /**< -14.6 dbm: -14 */
	RF_POWER_N12p72dBm,   /**< -12.7 dbm: -13 */
	RF_POWER_N12p72dBm,   /**< -12.7 dbm: -12 */
	RF_POWER_N11p42dBm,   /**< -11.4 dbm: -11 */
	RF_POWER_N10p09dBm,   /**< -10.1 dbm: -10 */
	RF_POWER_N9p14dBm,    /**<  -9.1 dbm: -9  */
	RF_POWER_N8p14dBm,    /**<  -8.1 dbm: -8  */
	RF_POWER_N7p39dBm,    /**<  -7.4 dbm: -7  */
	RF_POWER_N6p03dBm,    /**<  -6.0 dbm: -6  */
	RF_POWER_N5p38dBm,    /**<  -5.4 dbm: -5  */
	RF_POWER_N3p89dBm,    /**<  -3.8 dbm: -4  */
	RF_POWER_N3p08dBm,    /**<  -3.0 dbm: -3  */
	RF_POWER_N2p30dBm,    /**<  -2.3 dbm: -2  */
	RF_POWER_N1p08dBm,    /**<  -1.0 dbm  -1  */
	RF_POWER_P0p03dBm,    /**<   0.0 dbm:   0 */
	RF_POWER_P0p98dBm,    /**<   1.0 dbm:   1 */
	/*VBAT*/
	RF_POWER_P2p00dBm,    /**<   2.0 dbm:   2 */
	RF_POWER_P3p04dBm,    /**<   3.0 dbm:   3 */
	RF_POWER_P4p09dBm,    /**<   4.1 dbm:   4 */
	RF_POWER_P5p53dBm,    /**<   5.5 dbm:   5 */
	RF_POWER_P6p08dBm,    /**<   6.0 dbm:   6 */
	RF_POWER_P6p71dBm,    /**<   6.7 dbm:   7 */
	RF_POWER_P8p06dBm,    /**<   8.0 dbm:   8 */
	RF_POWER_P9p10dBm,    /**<   9.1 dbm:   9 */
	RF_POWER_P10p00dBm,   /**<  10.0 dbm:  10 */
};


#elif CONFIG_SOC_RISCV_TELINK_TL322X
/* TX power TL721X lookup table */
const uint8_t tl_tx_pwr_lt[] = {
	/*VANT*/
	RF_POWER_N19p40dBm,   /**< -19.4 dbm: -20 */
	RF_POWER_N19p40dBm,   /**< -19.4 dbm: -19 */
	RF_POWER_N19p40dBm,   /**< -19.4 dbm: -18 */
	RF_POWER_N19p40dBm,   /**< -19.4 dbm: -17 */
	RF_POWER_N16p30dBm,   /**< -16.3 dbm: -16 */
	RF_POWER_N16p30dBm,   /**< -16.3 dbm: -15 */
	RF_POWER_N16p30dBm,   /**< -16.3 dbm: -14 */
	RF_POWER_N13p60dBm,   /**< -13.6 dbm: -13 */
	RF_POWER_N12p00dBm,   /**< -12.0 dbm: -12 */
	RF_POWER_N12p00dBm,   /**< -12.0 dbm: -11 */
	RF_POWER_N10p30dBm,   /**< -10.3 dbm: -10 */
	RF_POWER_N9p00dBm,    /**<  -9.0 dbm: -9  */
	RF_POWER_N8p00dBm,    /**<  -8.0 dbm: -8  */
	RF_POWER_N7p00dBm,    /**<  -7.0 dbm: -7  */
	RF_POWER_N6p20dBm,    /**<  -6.2 dbm: -6  */
	RF_POWER_N5p50dBm,    /**<  -5.5 dbm: -5  */
	RF_POWER_N4p00dBm,    /**<  -4.0 dbm: -4  */
	RF_POWER_N3p00dBm,    /**<  -3.0 dbm: -3  */
	RF_POWER_N2p00dBm,    /**<  -2.0 dbm: -2  */
	RF_POWER_N0p80dBm,    /**<  -0.8 dbm  -1  */
	RF_POWER_P0p00dBm,    /**<   0.0 dbm:   0 */
	RF_POWER_P1p00dBm,    /**<   1.0 dbm:   1 */
	RF_POWER_P2p00dBm,    /**<   2.0 dbm:   2 */
	RF_POWER_P3p00dBm,    /**<   3.0 dbm:   3 */
	RF_POWER_P4p00dBm,    /**<   4.0 dbm:   4 */
	/*VBAT*/
	RF_POWER_P5p00dBm,    /**<   5.0 dbm:   5 */
	RF_POWER_P6p00dBm,    /**<   6.0 dbm:   6 */
	RF_POWER_P7p00dBm,    /**<   7.0 dbm:   7 */
	RF_POWER_P8p00dBm,    /**<   8.0 dbm:   8 */
	RF_POWER_P9p00dBm,    /**<   9.0 dbm:   9 */
	RF_POWER_P10p00dBm,   /**<  10.0 dbm:  10 */
};


#elif CONFIG_SOC_RISCV_TELINK_TL323X
/* Copy from TX power TL321X lookup table, needs to update */
const uint8_t tl_tx_pwr_lt[] = {
	/*VANT*/
	RF_POWER_N20p25dBm,   /**< -20.0 dbm: -20 */
	RF_POWER_N20p25dBm,   /**< -20.0 dbm: -19 */
	RF_POWER_N20p25dBm,   /**< -20.0 dbm: -18 */
	RF_POWER_N16p82dBm,   /**< -20.0 dbm: -17 */
	RF_POWER_N14p37dBm,   /**< -14.6 dbm: -16 */
	RF_POWER_N14p37dBm,   /**< -14.6 dbm: -15 */
	RF_POWER_N14p37dBm,   /**< -14.6 dbm: -14 */
	RF_POWER_N14p37dBm,   /**< -12.6 dbm: -13 */
	RF_POWER_N11p67dBm,   /**< -12.6 dbm: -12 */
	RF_POWER_N11p67dBm,   /**< -11.3 dbm: -11 */
	RF_POWER_N10p28dBm,    /**<  -9.9 dbm: -10 */
	RF_POWER_N8p52dBm,    /**<  -8.8 dbm: -9  */
	RF_POWER_N8p52dBm,    /**<  -7.8 dbm: -8  */
	RF_POWER_N7p57dBm,    /**<  -7.8 dbm: -7  */
	RF_POWER_N6p72dBm,    /**<  -6.2 dbm: -6  */
	RF_POWER_N5p59dBm,    /**<  -5.5 dbm: -5  */
	RF_POWER_N4p06dBm,    /**<  -4.3 dbm: -4  */
	RF_POWER_N3p02dBm,    /**<  -3.3 dbm: -3  */
	RF_POWER_N2p12dBm,    /**<  -2.0 dbm: -2  */
	RF_POWER_N0p96dBm,    /**<  -1.2 dbm: -1  */
	RF_POWER_P0p03dBm,    /**<   0.0 dbm:  0  */
	RF_POWER_P1p00dBm,    /**<   1.2 dbm:  1  */
	RF_POWER_P1p90dBm,    /**<   2.0 dbm:  2  */

	/*VBAT*/
	RF_POWER_P3p21dBm,    /**<   3.0 dbm:  3  */
	RF_POWER_P4p47dBm,    /**<   4.1 dbm:  4  */
	RF_POWER_P5p02dBm,    /**<   5.0 dbm:  5  */
	RF_POWER_P6p03dBm,    /**<   6.0 dbm:  6  */
	RF_POWER_P7p11dBm,    /**<   7.0 dbm:  7  */
	RF_POWER_P8p03dBm,    /**<   8.0 dbm:  8  */
	RF_POWER_P8p99dBm,    /**<   9.1 dbm:  9  */
	RF_POWER_P10p04dBm,    /**<  10.0 dbm:  10 */
	RF_POWER_P10p73dBm,   /**<  11.1 dbm:  11 */

	
};


#endif